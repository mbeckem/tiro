#ifndef TIRO_COMPILER_SOURCE_DB_HPP
#define TIRO_COMPILER_SOURCE_DB_HPP

#include "common/entities/entity_id.hpp"
#include "common/entities/entity_storage.hpp"
#include "common/format.hpp"
#include "compiler/source_map.hpp"
#include "compiler/source_range.hpp"

namespace tiro {

TIRO_DEFINE_ENTITY_ID(SourceId, u32)

/// Combines a source range with a file id to provide an unambiguous range
/// in the compiler's current context.
class AbsoluteSourceRange final {
public:
    /// Constructs an invalid instance that does not belong to a file.
    AbsoluteSourceRange();

    /// Constructs a range that belongs to the given file id.
    AbsoluteSourceRange(SourceId id, const SourceRange& range);

    /// Returns true if this range belongs to a file.
    explicit operator bool() const { return valid(); }

    /// Returns true if this range belongs to a file.
    bool valid() const;

    /// Returns the source file id. May be invalid (see valid()).
    SourceId id() const;

    /// Returns the source range in the associated file.
    const SourceRange& range() const;

    void format(FormatStream& stream) const;

private:
    SourceId id_;
    SourceRange range_;
};

inline bool operator==(const AbsoluteSourceRange& lhs, const AbsoluteSourceRange& rhs) {
    return lhs.id() == rhs.id() && lhs.range() == rhs.range();
}

inline bool operator!=(const AbsoluteSourceRange& lhs, const AbsoluteSourceRange& rhs) {
    return !(lhs == rhs);
}

/// Manages source file contents for the compiler during the compilation of a single module.
class SourceDb final {
public:
    SourceDb();
    ~SourceDb();

    SourceDb(const SourceDb&) = delete;
    SourceDb& operator=(const SourceDb&) = delete;

    /// Inserts a new source file with the given name and content.
    /// Returns the file's unique id.
    SourceId insert(std::string filename, std::string content);

    /// Returns the filename of the given file.
    /// Note: the string view is stable in memory.
    /// \pre `id` must be valid.
    std::string_view filename(SourceId id) const;

    /// Returns the content of the given file.
    /// Note: the string view is stable in memory.
    /// \pre `id` must be valid.
    std::string_view content(SourceId id) const;

    /// Returns the cursor position for the given offset in the file with the provided id.
    /// \pre `id` must be valid.
    /// \pre `offset` must not be out of bounds for the file's content.
    CursorPosition cursor_pos(SourceId id, u32 offset) const;

    /// Returns the cursor positions for the given source range.
    /// \pre `range` must be associated with a valid file.
    /// \pre `range` must not be out of bounds for that file's content.
    std::pair<CursorPosition, CursorPosition> cursor_pos(const AbsoluteSourceRange& range) const;

private:
    struct SourceFile {
        std::string filename;
        std::string content;
        SourceMap map;

        SourceFile(std::string filename, std::string content);
    };

    // unique ptr -> stable string views
    EntityStorage<std::unique_ptr<SourceFile>, SourceId> files_;
};

} // namespace tiro

TIRO_ENABLE_MEMBER_FORMAT(tiro::AbsoluteSourceRange);

#endif // TIRO_COMPILER_SOURCE_DB_HPP
