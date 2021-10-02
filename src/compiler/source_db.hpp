#ifndef TIRO_COMPILER_SOURCE_DB_HPP
#define TIRO_COMPILER_SOURCE_DB_HPP

#include "common/entities/entity_id.hpp"
#include "common/entities/entity_storage.hpp"
#include "common/format.hpp"
#include "compiler/source_map.hpp"
#include "compiler/source_range.hpp"

#include "absl/container/flat_hash_set.h"

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

    /// Returns a range over the available source file ids in this db.
    auto ids() const { return files_.keys(); }

    /// Returns the number of files in this db.
    size_t size() const { return files_.size(); }

    /// Returns true if there is already a file with the given name.
    bool contains(std::string_view filename) const;

    /// Inserts a new source file with the given name and content.
    /// Returns the file's unique id, or an invalid id if a file with such a name already exists.
    SourceId insert_new(std::string filename, std::string content);

    /// Returns the filename of the given file.
    /// Note: the string view is stable in memory.
    /// \pre `id` must be valid.
    std::string_view filename(SourceId id) const;

    /// Returns the content of the given file.
    /// Note: the string view is stable in memory.
    /// \pre `id` must be valid.
    std::string_view content(SourceId id) const;

    /// Returns the substring of referenced by the range.
    /// Note: the string view is stable in memory.
    /// \pre `range` must be valid.
    std::string_view substring(const AbsoluteSourceRange& range) const;

    /// Returns the source line mappings for the given file.
    /// Note: the reference is stable in memory.
    /// \pre `id` must be valid.
    const SourceMap& source_lines(SourceId id) const;

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

    // keys point into file_ entries.
    absl::flat_hash_set<std::string_view> seen_;
};

} // namespace tiro

TIRO_ENABLE_MEMBER_FORMAT(tiro::AbsoluteSourceRange);

#endif // TIRO_COMPILER_SOURCE_DB_HPP
