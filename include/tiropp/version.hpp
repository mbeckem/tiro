#ifndef TIROPP_VERSION_HPP_INCLUDED
#define TIROPP_VERSION_HPP_INCLUDED

#include "tiropp/def.hpp"

#include "tiro/version.h"

namespace tiro {

/// Represents a library version.
struct version {
    /// The full version number (see TIRO_MAKE_VERSION).
    uint32_t version_number = 0;

    /// The major version extracted from the version number.
    uint32_t major = 0;

    /// The minor version extracted from the version number.
    uint32_t minor = 0;

    /// The patch version extracted from the version number.
    uint32_t patch = 0;

    /// The library's version as a string.
    /// Points into static storage if this object was returned by this libray.
    const char* version = "";

    /// The library's source id (build system identifier).
    /// Points into static storage if this object was returned by this libray.
    const char* source_id = "";

    /// The library's full version as a string.
    /// Points into static storage if this object was returned by this libray.
    const char* full_version = "";
};

/// Returns the compile time version of the tiro library, i.e. the version
/// the application was built against.
inline constexpr version compile_time_version() {
    version v;
    v.version_number = TIRO_VERSION_NUMBER;
    v.major = v.version_number / 1000000;
    v.minor = (v.version_number % 1000000) / 1000;
    v.patch = (v.version_number % 1000);
    v.version = TIRO_VERSION;
    v.source_id = TIRO_SOURCE_ID;
    v.full_version = TIRO_FULL_VERSION;
    return v;
}

/// Returns the runtime version of the tiro library, i.e. the version the application
/// is currently running against.
inline version runtime_version() {
    version v;
    v.version_number = tiro_version_number();
    v.major = v.version_number / 1000000;
    v.minor = (v.version_number % 1000000) / 1000;
    v.patch = (v.version_number % 1000);
    v.version = tiro_version();
    v.source_id = tiro_source_id();
    v.full_version = tiro_full_version();
    return v;
}

} // namespace tiro

#endif // TIROPP_VERSION_HPP_INCLUDED
