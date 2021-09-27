#include "compiler/source_db.hpp"

namespace tiro {

AbsoluteSourceRange::AbsoluteSourceRange() {}

AbsoluteSourceRange::AbsoluteSourceRange(SourceId id, const SourceRange& range)
    : id_(id)
    , range_(range) {}

bool AbsoluteSourceRange::valid() const {
    return id_.valid();
}

SourceId AbsoluteSourceRange::id() const {
    return id_;
}

const SourceRange& AbsoluteSourceRange::range() const {
    return range_;
}

void AbsoluteSourceRange::format(FormatStream& stream) const {
    if (!valid()) {
        stream.format("<invalid>");
        return;
    }

    stream.format("{}:{}", id_, range_);
}

SourceDb::SourceDb() {}

SourceDb::~SourceDb() {}

SourceId SourceDb::insert(std::string filename, std::string content) {
    auto entry = std::make_unique<SourceFile>(std::move(filename), std::move(content));
    return files_.push_back(std::move(entry));
}

std::string_view SourceDb::filename(SourceId id) const {
    return files_[id]->filename;
}

std::string_view SourceDb::content(SourceId id) const {
    return files_[id]->content;
}

const SourceMap& SourceDb::source_lines(SourceId id) const {
    return files_[id]->map;
}

CursorPosition SourceDb::cursor_pos(SourceId id, u32 offset) const {
    return files_[id]->map.cursor_pos(offset);
}

std::pair<CursorPosition, CursorPosition>
SourceDb::cursor_pos(const AbsoluteSourceRange& range) const {
    TIRO_DEBUG_ASSERT(range.valid(), "invalid source file id");
    return files_[range.id()]->map.cursor_pos(range.range());
}

SourceDb::SourceFile::SourceFile(std::string filename_, std::string content_)
    : filename(std::move(filename_))
    , content(std::move(content_))
    , map(content) // Note: takes reference to content {}
{}

} // namespace tiro
