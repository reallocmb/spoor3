#ifndef EENHEID_INTERNAL_H
#define EENHEID_INTERNAL_H

#define EENHEID_VERSION_MAJOR 0
#define EENHEID_VERSION_MINOR 0
#define EENHEID_VERSION_PATCH 0

#include<stdint.h>

#define EENHEID_TESTS_ALLOC 2

enum { PASSED, FAILED, PENDING };

typedef struct {
    char *suite_old;
    char *test_old;
    char *suite;
    char *test;
    uint8_t status;
    char *status_sign;
    uint32_t passed_count;
    uint32_t failed_count;
    uint32_t pending_count;
    char *message_error;
    uint32_t message_error_offset;
} EenheidTest;

#endif
