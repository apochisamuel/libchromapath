#include "internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void cp_parse_options_init(cp_parse_options *options) {
    if (!options) return;
    options->max_profile_size = 64u * 1024u * 1024u;
    options->max_tag_count = 4096u;
    options->max_text_bytes = 1024u * 1024u;
    options->max_lut_cells = 16u * 1024u * 1024u;
    options->strict = 0;
}

void cp_set_error(cp_profile *profile, const char *format, ...) {
    if (!profile || !format) return;
    va_list args;
    va_start(args, format);
    vsnprintf(profile->error, sizeof(profile->error), format, args);
    va_end(args);
}

const char *cp_profile_error(const cp_profile *profile) {
    if (!profile) return "invalid profile";
    return profile->error[0] ? profile->error : "no error";
}

const char *cp_status_string(cp_status status) {
    switch (status) {
        case CP_OK: return "ok";
        case CP_ERR_ARGUMENT: return "invalid argument";
        case CP_ERR_NOMEM: return "out of memory";
        case CP_ERR_TRUNCATED: return "truncated input";
        case CP_ERR_FORMAT: return "invalid format";
        case CP_ERR_UNSUPPORTED: return "unsupported feature";
        case CP_ERR_NOT_FOUND: return "not found";
        case CP_ERR_STATE: return "invalid state";
        case CP_ERR_OVERFLOW: return "numeric or output overflow";
        case CP_ERR_IO: return "input or output error";
        default: return "unknown error";
    }
}

uint32_t cp_signature(const char text[4]) {
    if (!text) return 0;
    return ((uint32_t)(uint8_t)text[0] << 24) |
           ((uint32_t)(uint8_t)text[1] << 16) |
           ((uint32_t)(uint8_t)text[2] << 8) |
           (uint32_t)(uint8_t)text[3];
}

void cp_signature_text(uint32_t signature, char text[5]) {
    if (!text) return;
    text[0] = (char)((signature >> 24) & 0xffu);
    text[1] = (char)((signature >> 16) & 0xffu);
    text[2] = (char)((signature >> 8) & 0xffu);
    text[3] = (char)(signature & 0xffu);
    text[4] = '\0';
    for (size_t i = 0; i < 4; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c < 32 || c > 126) text[i] = '.';
    }
}
