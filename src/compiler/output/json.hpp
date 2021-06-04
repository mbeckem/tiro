#ifndef TIRO_COMPILER_OUTPUT_JSON_HPP
#define TIRO_COMPILER_OUTPUT_JSON_HPP

#include <nlohmann/json.hpp>

#include "compiler/source_map.hpp"
#include "compiler/source_range.hpp"

namespace tiro {

using ordered_json = nlohmann::ordered_json;

inline ordered_json to_json(const CursorPosition& pos) {
    auto jv = ordered_json::object();
    jv.emplace("line", pos.line());
    jv.emplace("column", pos.column());
    return jv;
}

inline std::pair<ordered_json, ordered_json>
to_json(const SourceRange& range, const SourceMap& map) {
    const auto& [start, end] = map.cursor_pos(range);
    return std::pair(to_json(start), to_json(end));
}

} // namespace tiro

#endif // TIRO_COMPILER_OUTPUT_JSON_HPP
