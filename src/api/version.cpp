#include "tiro/api.h"

uint32_t tiro_version_number() {
    return TIRO_VERSION_NUMBER;
}

const char* tiro_version() {
    return TIRO_VERSION;
}

const char* tiro_build_date() {
    return TIRO_BUILD_DATE;
}

const char* tiro_source_id() {
    return TIRO_SOURCE_ID;
}

const char* tiro_full_version() {
    return TIRO_FULL_VERSION;
}
