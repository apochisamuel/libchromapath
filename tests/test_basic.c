#include "chromapath.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

int main(void) {
    uint8_t profile[132] = {0};
    put_u32(profile + 0, sizeof(profile));
    put_u32(profile + 12, cp_signature("mntr"));
    put_u32(profile + 16, cp_signature("RGB "));
    put_u32(profile + 20, cp_signature("XYZ "));
    put_u32(profile + 36, cp_signature("acsp"));
    put_u32(profile + 128, 0);
    cp_profile *parsed = NULL;
    assert(cp_profile_open_memory(profile, sizeof(profile), NULL, &parsed) == CP_OK);
    assert(cp_profile_tag_count(parsed) == 0);
    cp_header_info header;
    assert(cp_profile_header(parsed, &header) == CP_OK);
    assert(header.device_class == cp_signature("mntr"));
    cp_transform *transform = NULL;
    assert(cp_transform_create(parsed, 0, &transform) == CP_OK);
    double input[3] = {0.25, 0.5, 0.75};
    double output[3] = {0};
    assert(cp_transform_apply_rgb(transform, input, output) == CP_OK);
    assert(output[0] == input[0]);
    assert(output[1] == input[1]);
    assert(output[2] == input[2]);
    cp_transform_destroy(transform);
    cp_profile_close(parsed);
    puts("ok");
    return 0;
}
