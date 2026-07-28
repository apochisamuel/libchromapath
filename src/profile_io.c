#include "internal.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

cp_status cp_profile_open_file(const char *path, const cp_parse_options *options,
                               cp_profile **out_profile) {
    if (!path || !out_profile) return CP_ERR_ARGUMENT;
    *out_profile = NULL;
    FILE *file = fopen(path, "rb");
    if (!file) return CP_ERR_IO;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return CP_ERR_IO;
    }
    long end = ftell(file);
    if (end < 0) {
        fclose(file);
        return CP_ERR_IO;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return CP_ERR_IO;
    }
    size_t size = (size_t)end;
    if ((long)size != end || size < 132) {
        fclose(file);
        return CP_ERR_FORMAT;
    }
    uint8_t *data = (uint8_t *)malloc(size);
    if (!data) {
        fclose(file);
        return CP_ERR_NOMEM;
    }
    size_t read_size = fread(data, 1, size, file);
    int close_result = fclose(file);
    if (read_size != size || close_result != 0) {
        free(data);
        return CP_ERR_IO;
    }
    cp_status status = cp_profile_open_memory(data, size, options, out_profile);
    free(data);
    return status;
}

cp_status cp_profile_tag_data(const cp_profile *profile, uint32_t signature,
                              const uint8_t **data, size_t *size) {
    if (!profile || !data || !size) return CP_ERR_ARGUMENT;
    const cp_tag_record *record = cp_find_record(profile, signature);
    if (!record) return CP_ERR_NOT_FOUND;
    const uint8_t *slice;
    if (!cp_slice(profile->storage, profile->storage_size, record->offset,
                  record->size, &slice)) return CP_ERR_TRUNCATED;
    *data = slice;
    *size = record->size;
    return CP_OK;
}

cp_status cp_profile_compute_id(const cp_profile *profile, uint8_t digest[16]) {
    if (!profile || !digest) return CP_ERR_ARGUMENT;
    if (profile->storage_size < 100) return CP_ERR_TRUNCATED;
    uint8_t *copy = (uint8_t *)malloc(profile->storage_size);
    if (!copy) return CP_ERR_NOMEM;
    memcpy(copy, profile->storage, profile->storage_size);
    memset(copy + 44, 0, 4);
    memset(copy + 64, 0, 4);
    memset(copy + 84, 0, 16);
    cp_md5(copy, profile->storage_size, digest);
    free(copy);
    return CP_OK;
}
