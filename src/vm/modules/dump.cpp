#include "vm/modules/dump.hpp"

#include "common/format.hpp"
#include "common/text/code_point_range.hpp"
#include "vm/context.hpp"
#include "vm/handles/handle.hpp"
#include "vm/objects/all.hpp"

#include <absl/container/flat_hash_set.h>

namespace tiro::vm {

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

        dump_value(value);

        // Repeated occurrences in neighbor fields are fine, we just don't want
        // to recurse endlessly.
        mark_unseen(value);
    }

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
    StringFormatStream stream_;

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

    // Probably horribly inefficient, but its for debug repr..
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

        if (is_printable(cp)) {
            // TODO: Ouch! Need utf8 append
            stream.format("{}", to_string_utf8(cp));
            return;
        }

        stream.format("\\u{{{:X}}}", cp);
    }
};

class [[nodiscard]] DumpStruct {
public:
    explicit DumpStruct(std::string_view name, DumpHelper & parent)
        : parent_(parent)
        , stream_(parent.stream()) {
        stream_.format("{}{{", name);
    }

    template<typename T>
    DumpStruct& field(std::string_view name, const T& value) {
        start_field(name);
        parent_.dump(value);
        return *this;
    }

    void finish() { stream_.format("}}"); }

private:
    void start_field(std::string_view name) {
        if (has_fields_)
            stream_.format(", "); // TODO: indent

        stream_.format("{}: ", name);
        has_fields_ = true;
    }

private:
    DumpHelper& parent_;
    FormatStream& stream_;
    bool has_fields_ = false;
};

class [[nodiscard]] DumpList {
public:
    explicit DumpList(std::string_view open, std::string_view close, DumpHelper & parent)
        : parent_(parent)
        , stream_(parent.stream())
        , close_(close) {
        stream_.format("{}", open);
    }

    template<typename T>
    DumpList& item(const T& value) {
        start_item();
        parent_.dump(value);
        return *this;
    }

    void finish() { stream_.format("{}", close_); }

private:
    void start_item() {
        if (has_fields_)
            stream_.format(", ");

        has_fields_ = true;
    }

private:
    DumpHelper& parent_;
    FormatStream& stream_;
    std::string_view close_;
    bool has_fields_ = false;
};

class [[nodiscard]] DumpTuple {
public:
    explicit DumpTuple(DumpHelper & parent)
        : parent_(parent)
        , stream_(parent.stream()) {
        stream_.format("(");
    }

    template<typename T>
    DumpTuple& field(const T& value) {
        start_field();
        parent_.dump(value);
        return *this;
    }

    void finish() {
        if (field_count_ == 1)
            stream_.format(",");

        stream_.format(")");
    }

private:
    void start_field() {
        if (field_count_ > 0) {
            // TODO: indent
            stream_.format(", ");
        }

        ++field_count_;
    }

private:
    DumpHelper& parent_;
    FormatStream& stream_;
    std::string_view close_;
    size_t field_count_ = 0;
};

class [[nodiscard]] DumpRecord {
public:
    explicit DumpRecord(DumpHelper & parent)
        : parent_(parent)
        , stream_(parent.stream()) {
        stream_.format("(");
    }

    template<typename T>
    DumpRecord& field(std::string_view name, const T& value) {
        start_field(name);
        parent_.dump(value);
        return *this;
    }

    void finish() {
        if (!has_fields_)
            stream_.format(":");

        stream_.format(")");
    }

private:
    void start_field(std::string_view name) {
        if (has_fields_) {
            // TODO: indent
            stream_.format(", ");
        }

        stream_.format("{}: ", name);
        has_fields_ = true;
    }

private:
    DumpHelper& parent_;
    FormatStream& stream_;
    std::string_view close_;
    bool has_fields_ = false;
};

class [[nodiscard]] DumpMap {
public:
    explicit DumpMap(std::string_view open, std::string_view close, DumpHelper & parent)
        : parent_(parent)
        , stream_(parent.stream())
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

    void finish() { stream_.format("{}", close_); }

private:
    void start_item() {
        if (has_fields_)
            stream_.format(", ");

        has_fields_ = true;
    }

private:
    DumpHelper& parent_;
    FormatStream& stream_;
    std::string_view close_;
    bool has_fields_ = false;
};

} // namespace

String dump(Context& ctx, Handle<Value> value) {
    std::string dumped;
    {
        DumpHelper dh;
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
    case ValueType::Exception:
        dump_struct(type_name).field("message", value.must_cast<Exception>().message()).finish();
        break;
    case ValueType::Result: {
        auto result = value.must_cast<Result>();
        dump_struct(type_name)
            .field("type", EscapedString{result.is_success() ? "success"sv : "failure"sv})
            .field("value", result.is_success() ? result.value() : Null())
            .field("reason", result.is_failure() ? result.reason() : Null())
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

} // namespace tiro::vm

TIRO_ENABLE_MEMBER_FORMAT(tiro::vm::EscapedString)
