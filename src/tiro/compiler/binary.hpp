#ifndef TIRO_COMPILER_BINARY_HPP
#define TIRO_COMPILER_BINARY_HPP

#include "tiro/core/byte_order.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/span.hpp"

#include <cstring>

namespace tiro {

class CheckedBinaryReader final {
public:
    explicit CheckedBinaryReader(Span<const byte> code)
        : code_(code) {
        TIRO_CHECK(code_.size() <= std::numeric_limits<u32>::max(),
            "Invalid code: cannot have more than 2**32 bytes.");
    }

    size_t pos() const { return pos_; }
    size_t size() const { return code_.size(); }
    size_t remaining() const { return code_.size() - pos_; }

    u8 read_u8() { return read_raw<u8>(); }
    u16 read_u16() { return read_raw<u16>(); }
    u32 read_u32() { return read_raw<u32>(); }
    u64 read_u64() { return read_raw<u64>(); }
    i8 read_i8() { return cast<i8>(read_u8()); }
    i16 read_i16() { return cast<i16>(read_u16()); }
    i32 read_i32() { return cast<i32>(read_u32()); }
    i64 read_i64() { return cast<i64>(read_u64()); }

    f64 read_f64() { return cast<f64>(read_u64()); }

private:
    template<typename T>
    T read_raw() {
        TIRO_CHECK(remaining() >= sizeof(T), "Invalid code: out of bounds read.");
        T value;
        std::memcpy(&value, code_.data() + pos_, sizeof(T));
        pos_ += sizeof(T);
        return be_to_host(value);
    }

    template<typename To, typename From>
    To cast(From from) {
        // FIXME f64 format is not really well defined, not portable!
        // Ideally we would have a static assertion here that verifies the host
        // uses IEEE 754 encoding.
        static_assert(sizeof(To) == sizeof(From), "Size mismatch.");
        To value;
        std::memcpy(&value, &from, sizeof(From));
        return value;
    }

private:
    Span<const byte> code_;
    u32 pos_ = 0;
};

class BinaryWriter final {
public:
    explicit BinaryWriter(std::vector<byte>& out)
        : out_(&out) {}

    // Current byte offset, where the next write will take place.
    size_t pos() const { return out_->size(); }

    // Explicit function names to guard against accidental implicit conversions.
    void emit_u8(u8 v) { emit_raw(v); }
    void emit_u16(u16 v) { emit_raw(v); }
    void emit_u32(u32 v) { emit_raw(v); }
    void emit_u64(u64 v) { emit_raw(v); }
    void emit_i8(i8 v) { emit_raw(u8(v)); }
    void emit_i16(i16 v) { emit_raw(u16(v)); }
    void emit_i32(i32 v) { emit_raw(u32(v)); }
    void emit_i64(i64 v) { emit_raw(u64(v)); }
    void emit_f64(f64 v) { emit_raw(cast_f64(v)); }

    void overwrite_u8(size_t pos, u8 v) { overwrite_raw(pos, v); }
    void overwrite_u16(size_t pos, u16 v) { overwrite_raw(pos, v); }
    void overwrite_u32(size_t pos, u32 v) { overwrite_raw(pos, v); }
    void overwrite_u64(size_t pos, u64 v) { overwrite_raw(pos, v); }
    void overwrite_i8(size_t pos, i8 v) { overwrite_raw(pos, u8(v)); }
    void overwrite_i16(size_t pos, i16 v) { overwrite_raw(pos, u16(v)); }
    void overwrite_i32(size_t pos, i32 v) { overwrite_raw(pos, u32(v)); }
    void overwrite_i64(size_t pos, i64 v) { overwrite_raw(pos, u64(v)); }
    void overwrite_f64(size_t pos, f64 v) { overwrite_raw(pos, cast_f64(v)); }

private:
    template<typename T>
    void emit_raw(T v) {
        v = host_to_be(v);
        const byte* addr = reinterpret_cast<const byte*>(std::addressof(v));
        out_->insert(out_->end(), addr, addr + sizeof(v));
    }

    template<typename T>
    void overwrite_raw(size_t pos, T v) {
        v = host_to_be(v);

        const byte* addr = reinterpret_cast<const byte*>(std::addressof(v));
        TIRO_DEBUG_ASSERT(
            pos < out_->size() && (sizeof(v) <= out_->size() - pos), "Overwrite out of bounds.");
        std::memcpy(out_->data() + pos, addr, sizeof(T));
    }

    u64 cast_f64(f64 v) {
        // FIXME f64 format is not really well defined, not portable!
        // Ideally we would have a static assertion here that verifies the host
        // uses IEEE 754 encoding.
        u64 as_u64;
        std::memcpy(&as_u64, &v, sizeof(v));
        return as_u64;
    }

private:
    std::vector<byte>* out_;
};

} // namespace tiro

#endif // TIRO_COMPILER_BINARY_HPP
