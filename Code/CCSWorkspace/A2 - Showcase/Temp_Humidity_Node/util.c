#include <stdint.h>
#include <stdlib.h>
#include "util.h"
#include <limits.h>

// int range is [-32768,32767] for MSP430FR5994

uint8_t write_int_to_string(int i, char *str) {
    char *loc;
    int b;
    uint8_t count, flag;

    loc = str;
    b = i;
    flag = 0;
    if (b < 0) {
        *loc = '-';
        loc++;
        b = -b;
    }
    if (b >= 10000) {
        count = 0;
        while (b >= 10000) {
            count++;
            b -= 10000;
        }
        *loc = '0' + count;
        loc++;
        flag = 1;
    }
    if (flag || b >= 1000) {
        count = 0;
        while (b >= 1000) {
            count++;
            b -= 1000;
        }
        *loc = '0' + count;
        loc++;
        flag = 1;
    }
    if (flag || b >= 100) {
        count = 0;
        while (b >= 100) {
            count++;
            b -= 100;
        }
        *loc = '0' + count;
        loc++;
        flag = 1;
    }
    if (flag || b >= 10) {
        count = 0;
        while (b >= 10) {
            count++;
            b -= 10;
        }
        *loc = '0' + count;
        loc++;
        flag = 1;
    }
    *loc = '0' + b;
    return loc - str + 1;
}

char *convert_int_to_str(int i) {
    char *res;
    uint8_t count;

    res = malloc(6);
    count = write_int_to_string(i, res);
    return realloc(res, count);
}
