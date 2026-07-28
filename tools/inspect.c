#include "chromapath.h"

#include <stdio.h>
#include <stdlib.h>

static unsigned char *read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long length = ftell(f);
    if (length < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    unsigned char *data = (unsigned char *)malloc((size_t)length);
    if (!data) { fclose(f); return NULL; }
    if (fread(data, 1, (size_t)length, f) != (size_t)length) {
        free(data); fclose(f); return NULL;
    }
    fclose(f);
    *size = (size_t)length;
    return data;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s profile.icc\n", argv[0]);
        return 2;
    }
    size_t size = 0;
    unsigned char *data = read_file(argv[1], &size);
    if (!data) {
        fprintf(stderr, "could not read %s\n", argv[1]);
        return 1;
    }
    cp_profile *profile = NULL;
    cp_status st = cp_profile_open_memory(data, size, NULL, &profile);
    free(data);
    if (st != CP_OK) {
        fprintf(stderr, "parse failed: %s\n", cp_status_string(st));
        return 1;
    }
    char report[32768];
    size_t written = 0;
    st = cp_profile_format_report(profile, report, sizeof(report), &written);
    if (st == CP_OK) fwrite(report, 1, written, stdout);
    cp_profile_close(profile);
    return st == CP_OK ? 0 : 1;
}
