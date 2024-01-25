#ifndef EENHEID_H
#define EENHEID_H

#include"eenheid_internal.h"

#include<stdlib.h>
#include<stdbool.h>

/* ASSERT Functions */
void eenheid_assert(EenheidTest *eenheid_test, bool condition, char *file, uint32_t line);
void eenheid_assert_char(EenheidTest *eenheid_test, char received, char expected, char *file, uint32_t line);
void eenheid_assert_str(EenheidTest *eenheid_test, char *received, char *expected, char *file, uint32_t line);
void eenheid_assert_str_size(EenheidTest *eenheid_test, char *received, char *expected, uint32_t size, char *file, uint32_t line);
void eenheid_assert_int8(EenheidTest *eenheid_test, int8_t received, int8_t expected, char *file, uint32_t line);
void eenheid_assert_int32(EenheidTest *eenheid_test, int32_t received, int32_t expected, char *file, uint32_t line);
void eenheid_assert_int64(EenheidTest *eenheid_test, int64_t received, int64_t expected, char *file, uint32_t line);
void ennheid_assert_uint32(EenheidTest *eenheid_test, uint32_t received, uint32_t expected, char *file, uint32_t line);

#define eenheid_assert(condition) eenheid_assert(&eenheid_test, condition, __FILE__, __LINE__)
#define eenheid_assert_char(received, expected) eenheid_assert_char(&eenheid_test, received, expected, __FILE__, __LINE__)
#define eenheid_assert_str(received, expected) eenheid_assert_str(&eenheid_test, received, expected, __FILE__, __LINE__)
#define eenheid_assert_str_size(received, expected, size) eenheid_assert_str_size(&eenheid_test, received, expected, size, __FILE__, __LINE__)
#define eenheid_assert_int8(received, expected) eenheid_assert_int8(&eenheid_test, received, expected, __FILE__, __LINE__)
#define eenheid_assert_int32(received, expected) eenheid_assert_int32(&eenheid_test, received, expected, __FILE__, __LINE__)
#define eenheid_assert_int64(received, expected) eenheid_assert_int64(&eenheid_test, received, expected, __FILE__, __LINE__)
#define eenheid_assert_uint32(received, expected) ennheid_assert_uint32(&eenheid_test, received, expected, __FILE__, __LINE__)

void eenheid_title_print(void);
void eenheid_test_create(EenheidTest *eenheid_test, char *suite, char *test);
void eenheid_test_clean(EenheidTest *eenheid_test);
void eenheid_stats_print(EenheidTest *eenheid_test);
void eenheid_suite_create(EenheidTest *eenheid_test, char *suite);
void eenheid_suite_clean(EenheidTest *eenheid_test);

#define __EENHEID_INIT__ \
    EenheidTest eenheid_test = { \
        .test = "", \
        .suite = "", \
        .test_old = "", \
        .suite_old = "", \
        .status_sign = "", \
        .message_error_offset = 0, \
    }; \
int main(void) \
{ \
    eenheid_test.message_error = malloc(sizeof(*eenheid_test.message_error)); \
    eenheid_test.message_error[0] = 0; \
    eenheid_title_print(); \

#define SUITE(suite) \
    eenheid_suite_create(&eenheid_test, #suite); \
    eenheid_suite_clean(&eenheid_test); \

#define SUITE_END \
    eenheid_suite_clean(&eenheid_test); \

#define TEST(test) \
    eenheid_test_create(&eenheid_test, "", #test); \
    eenheid_test.pending_count++; \

#define TEST_END \
    eenheid_test_clean(&eenheid_test); \

#define __EENHEID_END__ \
    eenheid_stats_print(&eenheid_test); \
    return 0; \
} \

#endif
