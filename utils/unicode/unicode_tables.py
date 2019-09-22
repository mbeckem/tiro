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
import re


def data_lines(filename):
    """Iterate over data lines in a unicode database file."""
    with open(filename, mode="rt", encoding="utf-8") as file:
        for line in file:
            line = line.partition("#")[0].strip()
            if not line:
                continue

            entries = [entry.strip() for entry in line.split(";")]
            yield entries


def codepoint_from_hex(hex):
    return int(hex, 16)


IS_CODEPOINT_RANGE = re.compile(r"(\w*)\.\.(\w*)", flags=re.ASCII)


def parse_codepoint_range(codepoints):
    match_range = re.fullmatch(IS_CODEPOINT_RANGE, codepoints)
    if match_range:
        return (codepoint_from_hex(match_range[1]), codepoint_from_hex(match_range[2]))

    cp = codepoint_from_hex(codepoints)
    return (cp, cp)


# Path to the unicode character database
UCD_DIR = os.path.join(os.path.abspath(os.path.dirname(__file__)), "12.1.0")


def generate_table_file(path):
    HEADER = (f"// THIS FILE has been auto generated. Do not edit.\n"
              f"// Generated at {datetime.datetime.now().isoformat()}\n\n"
              f"#include \"hammer/core/defs.hpp\"\n"
              f"#include \"hammer/core/unicode.hpp\"\n\n"
              f"namespace hammer::unicode_data {{\n\n")

    FOOTER = "\n} // namespace hammer::unicode_data\n"

    with open(path, mode="x+", encoding="utf-8") as cpp:
        cpp.write(HEADER)
        generate_category_table(cpp)
        cpp.write(FOOTER)


def generate_category_table(cpp):
    CATEGORY_FILE = os.path.join(
        UCD_DIR, "extracted/DerivedGeneralCategory.txt")
    PUBLIC_NAME = "cps_to_cat"
    PRIVATE_NAME = PUBLIC_NAME + "_"

    # list of ((lowcp, highcp), category)
    ranges = [(parse_codepoint_range(line[0]), line[1])
              for line in data_lines(CATEGORY_FILE)]
    ranges = sorted(ranges, key=lambda entry: entry[0])

    cpp.write(
        f"static constexpr MapEntry<CodePoint, GeneralCategory> "
        f"{PRIVATE_NAME}[] = {{\n")

    last_max = None
    for (codepoints, category) in ranges:
        (min_cp, max_cp) = codepoints
        if last_max:
            if min_cp != last_max + 1:
                raise RuntimeError("Values are not contiguous")
        else:
            if min_cp != 0:
                raise RuntimeError("Values must start at 0")

        cpp.write(f"    {{{min_cp}, GeneralCategory::{category}}},\n")
        last_max = max_cp
    cpp.write(f"    {{{last_max + 1}, GeneralCategory::Invalid}},\n")

    cpp.write("};\n\n")
    cpp.write(
        f"constexpr Span<const MapEntry<CodePoint, GeneralCategory>> "
        f"{PUBLIC_NAME}({PRIVATE_NAME});\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate the unicode database file.")
    parser.add_argument(
        "dest", metavar="PATH", type=str, help="The destination path (must not exist).")

    args = parser.parse_args()
    generate_table_file(args.dest)
