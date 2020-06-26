/**
 * macfand - hipuranyhou - 21.06.2020
 * 
 * Daemon for controlling fans on Linux systems using
 * applesmc and coretemp.
 * 
 * https://github.com/Hipuranyhou/macfand
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "helper.h"


char* concatenate_format(const char* format, ...) {
    va_list ap;
    int cnt = 0;
    char *string = NULL;

    // Get length of string
    va_start(ap, format);
    cnt = vsnprintf(NULL, 0, format, ap);
    va_end(ap);
    if (cnt < 0)
        return NULL;

    string = (char*)malloc(cnt + 1);
    if (!string)
        return NULL;

    // Concatenate
    va_start(ap, format);
    vsnprintf(string, cnt + 1, format, ap);
    va_end(ap);

    return string;
}


int convert_valid_int(char *string, int *destination) {
    char *end = NULL;
    long int tmp = 0;

    errno = 0;
    tmp = strtol(string, &end, 10);

    if (string == end || errno == ERANGE || tmp < INT_MIN || tmp > INT_MAX)
        return 0;

    *destination = (int)tmp;
    return 1;
}


int max(const int a, const int b) {
    return (a > b) ? a : b;
}


int min(const int a, const int b) {
    return (a < b) ? a : b;
}