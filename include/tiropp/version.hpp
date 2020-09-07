#ifndef TIROPP_VERSION_HPP_INCLUDED
#define TIROPP_VERSION_HPP_INCLUDED

#include "tiropp/def.hpp"

#include "tiro/version.h"

namespace tiro {

/// Represents a library version.
struct version {
    uint32_t version_number = 0;
    uint32_t major = 0;
    uint32_t minor = 0;
    uint32_t patch = 0;
    const char* version_string = "";
    const char* source_id = "";
    const char* full_version_string = "";
};

/// Returns the compile time version of the tiro library, i.e. the version
/// the application was built against.
inline constexpr version compile_time_version() {
    version v;
    v.version_number = TIRO_VERSION_NUMBER;
    v.major = v.version_number / 1000000;
    v.minor = (v.version_number % 1000000) / 1000;
    v.patch = (v.version_number % 1000);
    v.version_string = TIRO_VERSION;
    v.source_id = TIRO_SOURCE_ID;
    v.full_version_string = TIRO_FULL_VERSION;
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
    v.version_string = tiro_version();
    v.source_id = tiro_source_id();
    v.full_version_string = tiro_full_version();
    return v;
}

} // namespace tiro

#endif // TIROPP_VERSION_HPP_INCLUDED
