#include "vm/builtins/dump.hpp"

#include "common/format.hpp"
#include "common/text/code_point_range.hpp"
#include "vm/context.hpp"
#include "vm/handles/handle.hpp"
#include "vm/objects/all.hpp"

#include <absl/container/flat_hash_set.h>

namespace tiro::vm {

static void indent(size_t depth, FormatStream& stream);

namespace {

struct EscapedString;

class DumpStruct;
class DumpList;
class DumpTuple;
class DumpRecord;
class DumpMap;

// TODO: This is a prime candidate to port to coroutine functions
// once native functions can call into tiro code.
//
// TODO: Can overflow the native stack since naive recursion is used (TODO 1 should solve this :))
class DumpHelper {
public:
    explicit DumpHelper(bool pretty)
        : pretty_(pretty) {}

    template<typename T>
    void dump(const T& value) {
        if constexpr (std::is_base_of_v<Value, remove_cvref_t<T>>) {
            visit(value);
        } else {
            stream_.format("{}", value);
        }
    }

    void visit(Value value) {
        // Avoid infinite recursion because of cycles.
        const bool inserted = mark_seen(value);
        if (!inserted) {
            stream_.format("{{...}}");
            return;
        }

        depth_ += 1;
        dump_value(value);
        depth_ -= 1;

        // Repeated occurrences in neighbor fields are fine, we just don't want
        // to recurse endlessly.
        mark_unseen(value);
    }

    bool pretty() const { return pretty_; }

    size_t depth() const { return depth_; }

    FormatStream& stream() { return stream_; }

    std::string take() { return stream_.take_str(); }

private:
    void dump_value(Value value);
    DumpStruct dump_struct(std::string_view type_name);
    DumpList dump_list(std::string_view open, std::string_view close);
    DumpRecord dump_record();
    DumpTuple dump_tuple();
    DumpMap dump_map(std::string_view open, std::string_view close);

    bool mark_seen(Value v) {
        auto [it, inserted] = seen_.emplace(/* XXX */ v.raw());
        (void) it;
        return inserted;
    }

    void mark_unseen(Value v) { seen_.erase(/* XXX */ v.raw()); }

private:
    bool pretty_;
    StringFormatStream stream_;

    /// Recursion depth, incremented for every visit(Value) call.
    size_t depth_ = 0;

    /// XXX: Currently relies on the values not moving, i.e. no garbage collection can be done!
    absl::flat_hash_set<uintptr_t> seen_;
};

struct EscapedString {
    std::string_view str;

    void format(FormatStream& stream) const {
        stream.format("\"");
        for (auto cp : CodePointRange(str)) {
            format_escaped(cp, stream);
        }
        stream.format("\"");
    }

    // Probably horribly inefficient, but it's for debug repr..
    static void format_escaped(CodePoint cp, FormatStream& stream) {
        switch (cp) {
        case '\n':
            stream.format("\\n");
            return;
        case '\r':
            stream.format("\\r");
            return;
        case '\t':
            stream.format("\\t");
            return;
        case '$':
            stream.format("\\$");
            return;
        case '\'':
            stream.format("\\'");
            return;
        case '"':
            stream.format("\\\"");
            return;
        case '\\':
            stream.format("\\\\");
            return;
        }

        // Printable ascii chars (deciding which unicode characters are printable is difficult
        // and probably requires a better unicode character database on our side).
        if (cp >= 0x20 && cp <= 0x7E) {
            // TODO: Ouch! Need utf8 append
            stream.format("{}", to_string_utf8(cp));
            return;
        }

        if (cp <= 0xFF) {
            stream.format("\\x{:02X}", static_cast<u8>(cp));
        } else {
            stream.format("\\u{{{:X}}}", cp);
        }
    }
};

class DumpBase {
protected:
    DumpBase(DumpHelper& parent)
        : parent_(parent)
        , stream_(parent.stream())
        , depth_(parent.depth()) {}

    bool pretty() const { return parent_.pretty(); }

    void indent_self() { indent(depth_, stream_); }

    void indent_child() { indent(depth_ + 1, stream_); }

protected:
    DumpHelper& parent_;
    FormatStream& stream_;
    size_t depth_;
};

class [[nodiscard]] DumpStruct : DumpBase {
public:
    explicit DumpStruct(std::string_view name, DumpHelper& parent)
        : DumpBase(parent) {
        stream_.format("{}{{", name);
    }

    template<typename T>
    DumpStruct& field(std::string_view name, const T& value) {
        start_field(name);
        parent_.dump(value);
        return *this;
    }

    void finish() {
        if (pretty() && has_fields_) {
            stream_.format("\n");
            indent_self();
        }
        stream_.format("}}");
    }

private:
    void start_field(std::string_view name) {
        if (pretty()) {
            if (has_fields_)
                stream_.format(",");
            stream_.format("\n");
            indent_child();
        } else {
            if (has_fields_)
                stream_.format(", ");
        }

        stream_.format("{}: ", name);
        has_fields_ = true;
    }

private:
    bool has_fields_ = false;
};

class [[nodiscard]] DumpList : DumpBase {
public:
    explicit DumpList(std::string_view open, std::string_view close, DumpHelper& parent)
        : DumpBase(parent)
        , close_(close) {
        stream_.format("{}", open);
    }

    template<typename T>
    DumpList& item(const T& value) {
        start_item();
        parent_.dump(value);
        return *this;
    }

    void finish() {
        if (pretty() && has_fields_) {
            stream_.format("\n");
            indent_self();
        }
        stream_.format("{}", close_);
    }

private:
    void start_item() {
        if (pretty()) {
            if (has_fields_)
                stream_.format(",");
            stream_.format("\n");
            indent_child();
        } else {
            if (has_fields_)
                stream_.format(", ");
        }

        has_fields_ = true;
    }

private:
    std::string_view close_;
    bool has_fields_ = false;
};

class [[nodiscard]] DumpTuple : DumpBase {
public:
    explicit DumpTuple(DumpHelper& parent)
        : DumpBase(parent) {
        stream_.format("(");
    }

    template<typename T>
    DumpTuple& field(const T& value) {
        start_field();
        parent_.dump(value);
        return *this;
    }

    void finish() {
        if (pretty()) {
            if (field_count_ == 1)
                stream_.format(",");

            if (field_count_ > 0) {
                stream_.format("\n");
                indent_self();
            }
        } else {
            if (field_count_ == 1)
                stream_.format(",");
        }

        stream_.format(")");
    }

private:
    void start_field() {
        if (pretty()) {
            if (field_count_ > 0)
                stream_.format(",");
            stream_.format("\n");
            indent_child();
        } else {
            if (field_count_ > 0)
                stream_.format(", ");
        }

        ++field_count_;
    }

private:
    std::string_view close_;
    size_t field_count_ = 0;
};

class [[nodiscard]] DumpRecord : DumpBase {
public:
    explicit DumpRecord(DumpHelper& parent)
        : DumpBase(parent) {
        stream_.format("(");
    }

    template<typename T>
    DumpRecord& field(std::string_view name, const T& value) {
        start_field(name);
        parent_.dump(value);
        return *this;
    }

    void finish() {
        if (pretty() && has_fields_) {
            stream_.format("\n");
            indent_self();
        }

        if (!has_fields_)
            stream_.format(":");

        stream_.format(")");
    }

private:
    void start_field(std::string_view name) {
        if (pretty()) {
            if (has_fields_)
                stream_.format(",");
            stream_.format("\n");
            indent_child();
        } else {
            if (has_fields_)
                stream_.format(", ");
        }

        stream_.format("{}: ", name);
        has_fields_ = true;
    }

private:
    bool has_fields_ = false;
};

class [[nodiscard]] DumpMap : DumpBase {
public:
    explicit DumpMap(std::string_view open, std::string_view close, DumpHelper& parent)
        : DumpBase(parent)
        , close_(close) {
        stream_.format("{}", open);
    }

    template<typename T>
    DumpMap& item(const T& key, const T& value) {
        start_item();
        parent_.dump(key);
        stream_.format(": ");
        parent_.dump(value);
        return *this;
    }

    void finish() {
        if (pretty() && has_fields_) {
            stream_.format("\n");
            indent_self();
        }

        stream_.format("{}", close_);
    }

private:
    void start_item() {
        if (pretty()) {
            if (has_fields_)
                stream_.format(",");
            stream_.format("\n");
            indent_child();
        } else {
            if (has_fields_)
                stream_.format(", ");
        }

        has_fields_ = true;
    }

private:
    std::string_view close_;
    bool has_fields_ = false;
};

} // namespace

String dump(Context& ctx, Handle<Value> value, bool pretty) {
    std::string dumped;
    {
        DumpHelper dh(pretty);
        dh.visit(*value);
        dumped = dh.take();
    }
    return String::make(ctx, dumped);
}

void DumpHelper::dump_value(Value value) {
    auto native_type = value.type();
    auto public_type = to_public_type(native_type);
    if (!public_type) {
        stream_.format("<<<internal>>>");
        return;
    }

    auto type_name = to_string(*public_type);
    switch (native_type) {
        // Primitive types
    case ValueType::Undefined:
        stream_.format("undefined");
        break;
    case ValueType::Null:
        stream_.format("null");
        break;
    case ValueType::Boolean:
        stream_.format("{}", value.must_cast<Boolean>().value());
        break;
    case ValueType::SmallInteger:
        stream_.format("{}", value.must_cast<SmallInteger>().value());
        break;
    case ValueType::HeapInteger:
        stream_.format("{}", value.must_cast<HeapInteger>().value());
        break;
    case ValueType::Float:
        stream_.format("{:#}", value.must_cast<Float>().value());
        break;
    case ValueType::String:
        stream_.format("{}", EscapedString{value.must_cast<String>().view()});
        break;
    case ValueType::Symbol: {
        stream_.format("#{}", value.must_cast<Symbol>().name().view());
        break;

        // Structures
    case ValueType::Coroutine:
        dump_struct(type_name).field("name", value.must_cast<Coroutine>().name()).finish();
        break;
    case ValueType::Exception: {
        auto ex = value.must_cast<Exception>();
        dump_struct(type_name).field("message", ex.message()).field("trace", ex.trace()).finish();
        break;
    }
    case ValueType::Result: {
        auto result = value.must_cast<Result>();
        dump_struct(type_name)
            .field("type", EscapedString{result.is_success() ? "success"sv : "error"sv})
            .field("value", result.is_success() ? result.unchecked_value() : Null())
            .field("error", result.is_error() ? result.unchecked_error() : Null())
            .finish();
        break;
    }
    case ValueType::StringSlice:
        dump_struct(type_name)
            .field("value", EscapedString{value.must_cast<StringSlice>().view()})
            .finish();
        break;
    case ValueType::Type:
        dump_struct(type_name).field("name", value.must_cast<Type>().name()).finish();
        break;
    }

        // Containers
    case ValueType::Tuple: {
        auto tuple = value.must_cast<Tuple>();
        auto dump = dump_tuple();
        for (const auto item : tuple.values()) {
            dump.field(item);
        }
        dump.finish();
        break;
    }
    case ValueType::Record: {
        auto record = value.must_cast<Record>();
        auto dump = dump_record();
        record.for_each_unsafe([&](Symbol k, Value v) { dump.field(k.name().view(), v); });
        dump.finish();
        break;
    }
    case ValueType::Array: {
        auto array = value.must_cast<Array>();
        auto dump = dump_list("[", "]");
        for (const auto item : array.values()) {
            dump.item(item);
        }
        dump.finish();
        break;
    }
    case ValueType::HashTable: {
        auto map = value.must_cast<HashTable>();
        auto dump = dump_map("map{", "}");
        map.for_each_unsafe([&](Value k, Value v) { dump.item(k, v); });
        dump.finish();
        break;
    }
    case ValueType::Set: {
        auto set = value.must_cast<Set>();
        auto dump = dump_list("set{", "}");
        set.for_each_unsafe([&](Value item) { dump.item(item); });
        dump.finish();
        break;
    }

    // All other types are opaque
    default:
        stream_.format("{}", type_name);
        break;
    }
}

DumpStruct DumpHelper::dump_struct(std::string_view type_name) {
    return DumpStruct(type_name, *this);
}

DumpList DumpHelper::dump_list(std::string_view open, std::string_view close) {
    return DumpList(open, close, *this);
}

DumpTuple DumpHelper::dump_tuple() {
    return DumpTuple(*this);
}

DumpRecord DumpHelper::dump_record() {
    return DumpRecord(*this);
}

DumpMap DumpHelper::dump_map(std::string_view open, std::string_view close) {
    return DumpMap(open, close, *this);
}

static void indent(size_t depth, FormatStream& stream) {
    TIRO_DEBUG_ASSERT(depth >= 1, "invalid depth");
    size_t indent_chars = (depth == 0 ? 0 : (depth - 1)) * 4;
    stream.format("{}", repeat(' ', indent_chars));
}

} // namespace tiro::vm

TIRO_ENABLE_MEMBER_FORMAT(tiro::vm::EscapedString)
