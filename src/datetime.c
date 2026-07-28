#include "chromapath.h"

#include <stdio.h>

static int cp_leap_year(unsigned year) {
    return (year % 4u == 0u && year % 100u != 0u) || year % 400u == 0u;
}

static unsigned cp_days_in_month(unsigned year, unsigned month) {
    static const unsigned days[] = {0, 31, 28, 31, 30, 31, 30,
                                     31, 31, 30, 31, 30, 31};
    if (month == 0 || month > 12) return 0;
    if (month == 2 && cp_leap_year(year)) return 29;
    return days[month];
}

int cp_datetime_valid(const cp_datetime *value) {
    if (!value) return 0;
    if (value->year == 0 || value->month == 0 || value->month > 12) return 0;
    if (value->day == 0 || value->day > cp_days_in_month(value->year, value->month)) return 0;
    if (value->hour > 23 || value->minute > 59 || value->second > 60) return 0;
    return 1;
}

int cp_datetime_compare(const cp_datetime *left, const cp_datetime *right) {
    if (!left || !right) return 0;
    const uint16_t *a = &left->year;
    const uint16_t *b = &right->year;
    for (size_t i = 0; i < 6; ++i) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    return 0;
}

cp_status cp_datetime_format_iso8601(const cp_datetime *value, char *buffer,
                                     size_t capacity) {
    if (!value || !buffer || capacity == 0) return CP_ERR_ARGUMENT;
    if (!cp_datetime_valid(value)) return CP_ERR_FORMAT;
    int result = snprintf(buffer, capacity, "%04u-%02u-%02uT%02u:%02u:%02uZ",
                          (unsigned)value->year, (unsigned)value->month,
                          (unsigned)value->day, (unsigned)value->hour,
                          (unsigned)value->minute, (unsigned)value->second);
    if (result < 0 || (size_t)result >= capacity) return CP_ERR_OVERFLOW;
    return CP_OK;
}

cp_status cp_datetime_parse_icc(const uint8_t data[12], cp_datetime *out) {
    if (!data || !out) return CP_ERR_ARGUMENT;
    out->year = (uint16_t)((uint16_t)data[0] << 8 | data[1]);
    out->month = (uint16_t)((uint16_t)data[2] << 8 | data[3]);
    out->day = (uint16_t)((uint16_t)data[4] << 8 | data[5]);
    out->hour = (uint16_t)((uint16_t)data[6] << 8 | data[7]);
    out->minute = (uint16_t)((uint16_t)data[8] << 8 | data[9]);
    out->second = (uint16_t)((uint16_t)data[10] << 8 | data[11]);
    return cp_datetime_valid(out) ? CP_OK : CP_ERR_FORMAT;
}
