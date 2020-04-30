#!/usr/bin/env python3
#
# Parses the DerivedGeneralCategory file from the unicode character database
# and generates a c++ header file that contains lookup tables for
# code_point -> category lookups.
#
# The data files from the official unicode standard can be downloaded at
#   https://www.unicode.org/Public/12.1.0/
# for the 12.1.0 version.
#
# Michael Beckemeyer <m.beckemeyer@gmx.net>

import argparse
import csv
import datetime
import os


def data_lines(filename):
    """Iterate over data lines in a unicode database file."""
    with open(filename, mode="rt", encoding="utf-8") as file:
        for line in file:
            line = line.partition("#")[0].strip()
            if not line:
                continue

            entries = [entry.strip() for entry in line.split(";")]
            yield entries


def parse_codepoint_range(codepoints):
    """Parses a string that specifies a range of code points or a single code point.
    Returns a range of (numeric) code points."""

    begin, sep, end = codepoints.partition("..")
    if not sep:
        return [int(begin, 16)]
    return range(int(begin, 16), int(end, 16) + 1)


# Path to the unicode character database
UCD_DIR = os.path.join(os.path.abspath(os.path.dirname(__file__)), "12.1.0")
CATEGORY_FILE = os.path.join(
    UCD_DIR, "extracted/DerivedGeneralCategory.txt")
PROPERTIES_FILE = os.path.join(UCD_DIR, "PropList.txt")


def generate_table_file(path):
    HEADER = (f"// THIS FILE has been auto generated. Do not edit.\n"
              f"// Generated at {datetime.datetime.now().isoformat()}\n\n"
              f"#include \"tiro/core/defs.hpp\"\n"
              f"#include \"tiro/core/unicode.hpp\"\n\n"
              f"namespace tiro::unicode_data {{\n\n")

    FOOTER = "} // namespace tiro::unicode_data\n"

    with open(path, mode="x+", encoding="utf-8") as cpp:
        cpp.write(HEADER)
        generate_category_table(cpp)
        generate_property_tables(cpp)
        cpp.write(FOOTER)


def generate_category_table(cpp):
    ranges = [(parse_codepoint_range(line[0]), line[1])
              for line in data_lines(CATEGORY_FILE)]
    table = {
        code_point: cat for (code_points, cat) in ranges for code_point in code_points
    }
    generate_code_point_map(cpp, "cps_to_cat", "GeneralCategory", table)


def generate_property_tables(cpp):
    # maps property name to list of ranges
    properties = dict()
    for line in data_lines(PROPERTIES_FILE):
        code_points = parse_codepoint_range(line[0])
        prop = line[1]

        if prop not in properties:
            properties[prop] = set()
        properties[prop].update(code_points)

    generate_code_point_set(cpp, "is_whitespace", properties["White_Space"])


def generate_code_point_map(cpp, public_name, enum_name, table):
    """Generates a sparse map from code point (range) to enum value.
    The first value in each array entry starts a new range of code points (with the given value)."""

    private_name = public_name + "_"

    cpp.write(
        f"static constexpr MapEntry<CodePoint, {enum_name}> "
        f"{private_name}[] = {{\n")

    # Generate one array entry at the start of the mapping.
    # Only emit a new entry if the current value changes.
    last_value = None
    max_cp = max(table)
    for cp in range(0, max_cp + 2):
        # Improvement: Could make the invalid value more generic in the future
        value = table.get(cp, "Invalid")
        if value != last_value:
            cpp.write(f"    {{{hex(cp)}, {enum_name}::{value}}},\n")
            last_value = value

    cpp.write("};\n\n")
    cpp.write(
        f"constexpr Span<const MapEntry<CodePoint, {enum_name}>> "
        f"{public_name}({private_name});\n\n")


def generate_code_point_set(cpp, public_name, set):
    """Generates a sparse set of code points. Every entry in the array encodes a range
    of code points that are included in that set. Both ends are inclusive."""

    private_name = public_name + "_"

    ranges = []
    current = None
    expected = None
    for cp in sorted(set):
        if expected is None or cp != expected:
            current = [cp, cp]
            ranges.append(current)
        else:
            current[1] = cp
        expected = cp + 1

    cpp.write(
        f"static constexpr Interval<CodePoint> "
        f"{private_name}[] = {{\n"
    )

    for (min_cp, max_cp) in ranges:
        cpp.write(f"    {{{hex(min_cp)}, {hex(max_cp)}}},\n")

    cpp.write("};\n\n")
    cpp.write(
        f"constexpr Span<const Interval<CodePoint>> "
        f"{public_name}({private_name});\n\n"
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate the unicode database file.")
    parser.add_argument(
        "dest", metavar="PATH", type=str, help="The destination path (must not exist).")

    args = parser.parse_args()
    generate_table_file(args.dest)
