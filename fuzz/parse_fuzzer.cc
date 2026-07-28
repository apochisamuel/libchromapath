#include "chromapath.h"
#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 64 || size > 16*1024*1024) return 0;
    cp_parse_options options;
    cp_parse_options_init(&options);
    options.strict = 0;
    cp_profile *profile = nullptr;
    if (cp_profile_open_memory(data, size, &options, &profile) != CP_OK) return 0;
    char report[8192];
    size_t written = 0;
    cp_profile_format_report(profile, report, sizeof(report), &written);
    cp_profile_close(profile);
    return 0;
}
