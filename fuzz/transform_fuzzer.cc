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
    cp_transform *transform = nullptr;
    if (cp_transform_create(profile, 0, &transform) == CP_OK) {
        double input[3] = {0.2, 0.4, 0.6};
        double output[3];
        cp_transform_apply_rgb(transform, input, output);
        cp_transform_destroy(transform);
    }
    cp_profile_close(profile);
    return 0;
}
