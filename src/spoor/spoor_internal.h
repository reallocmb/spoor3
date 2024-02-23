#ifndef SPOOR_INTERNAL_H
#define SPOOR_INTERNAL_H

#include<time.h>
#include<stdint.h>
#include<stdbool.h>

#include<ft2build.h>
#include FT_FREETYPE_H

#define SPOOR_APPLICATION_NAME "SPOOR ~ by reallocmb"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

extern const u32 STATUS_BITS[3];
#define FILTER_STATUS_NOT_STARTED 0b1
#define FILTER_STATUS_IN_PROGRESS 0b10
#define FILTER_STATUS_COMPLETED 0b100
#define FILTER_STATUS_ALL 0b111

typedef enum {
    STATUS_NOT_STARTED,
    STATUS_IN_PROGRESS,
    STATUS_COMPLETED,
} SpoorStatus;

extern const u32 TYPE_BITS[6];
#define FILTER_TYPE_TASK 0b1
#define FILTER_TYPE_PROJECT 0b10
#define FILTER_TYPE_EVENT 0b100
#define FILTER_TYPE_APPOINTMENT 0b1000
#define FILTER_TYPE_GOAL 0b10000
#define FILTER_TYPE_HABIT 0b100000
#define FILTER_TYPE_ALL 0b111111

typedef enum {
    TYPE_TASK,
    TYPE_PROJECT,
    TYPE_EVENT,
    TYPE_APPOINTMENT,
    TYPE_GOAL,
    TYPE_HABIT,
} SpoorType;

typedef struct {
    int32_t sec;
    int32_t min;
    int32_t hour;
    int32_t day;
    int32_t mon;
    int32_t year;
} SpoorTime;

typedef struct {
    SpoorTime start;
    SpoorTime end;
} SpoorTimeSpan;

typedef struct {
    char parent_id[100];
    char id[100];
} SpoorChild;

#define SPOOR_OBJECT_DELETED_ID 0xffffffff
#define SPOOR_OBJECT_NO_LINK 0xffffffff
#define SPOOR_OBJECT_NEW_LINK 0xfffffffe

typedef struct SpoorObject {
    uint32_t id;
    char title[250];
    char parent_title[250];
    SpoorTimeSpan deadline;
    SpoorTimeSpan schedule;
    SpoorTimeSpan tracked;
    SpoorTimeSpan complete;
    SpoorTimeSpan created;
    uint32_t id_link;
    SpoorStatus status;
    SpoorType type;
} SpoorObject;

extern SpoorObject spoor_objects[500];
extern uint32_t spoor_objects_count;

typedef struct {
    uint32_t id;
    char location_parent[11];
    uint32_t id_parent;
    char location_child[11];
    uint32_t id_child;
    uint32_t id_prev;
    uint32_t id_next;
} SpoorLink;

extern SpoorLink link_global;

typedef struct {
    SpoorTimeSpan spoor_time;
    u32 types;
    u32 status;
} SpoorFilter;

SpoorObject *spoor_object_create(char *arguments);
void spoor_object_edit(SpoorObject *spoor_object, char *arguments);

void mdb_func_error_check(int error, char *func_name, int line, char *file);
void spoor_debug_spoor_object_print(SpoorObject *spoor_object);

uint32_t spoor_object_storage_load(SpoorFilter *spoor_filter);

void spoor_object_progress_change(SpoorObject *spoor_object, SpoorStatus status);
void spoor_storage_save(SpoorObject *spoor_object);

void spoor_storage_change(SpoorObject *spoor_object_old, SpoorObject *spoor_object);
void spoor_storage_delete(SpoorObject *spoor_object);
void spoor_object_schedule_set(SpoorObject *spoor_object, char *command);
void spoor_object_deadline_set(SpoorObject *spoor_object, char *command);

void spoor_storage_clean_up(void);
uint32_t spoor_object_storage_load_filter_time_span(SpoorTimeSpan *time_span);

void spoor_time_span_create(SpoorTimeSpan *spoor_time_span, char *command);

/* sort */
int64_t spoor_time_compare(SpoorTime *time1, SpoorTime *time2);
void spoor_sort_objects_by_title(void);
void spoor_sort_objects_by_deadline(void);
void spoor_sort_objects_append(SpoorObject *spoor_object);
void spoor_sort_objects(void);
uint32_t spoor_sort_objects_reposition_up(uint32_t index);
uint32_t spoor_sort_objects_reposition_down(uint32_t index);
void spoor_sort_objects_remove(uint32_t index);

void spoor_time_deadline_create(char *argument, uint32_t argument_length, SpoorTimeSpan *spoor_time);

void storage_db_path_clean(SpoorObject *spoor_object, char *db_path_clean);
void spoor_object_children_append(SpoorObject *spoor_object_head, SpoorObject *spoor_object);
void spoor_object_children_append_edit(SpoorObject *spoor_object_head, SpoorObject *spoor_object, SpoorObject *old);
void spoor_storage_object_append(SpoorObject *spoor_object_parent, SpoorObject *spoor_object);
void spoor_storage_object_remove(SpoorObject *spoor_object);
void title_format_parse(char *title, char *title_format);
void time_format_parse_deadline(SpoorTimeSpan *spoor_time, char *time_format);
void time_format_parse_schedule(SpoorTimeSpan *spoor_time, char *time_format);
SpoorTime spoor_time_struct_tm_convert(time_t *time);
struct tm spoor_time_convert_to_struct_tm(SpoorTime *spoor_time);
void spoor_time_minutes_add(SpoorTime *spoor_time, int32_t minutes);
void spoor_time_days_add(SpoorTime *spoor_time, int32_t days);

/* for unit tests */
#ifdef EENHEID_UNIT_TESTS
void test_spoor_time_today_set(SpoorTime *spoor_time);
#endif

void spoor_link_load(uint32_t id);
void spoor_debug_links(void);


typedef struct UIFont {
    FT_Face face;
    FT_Library ft;
    FT_Int major, minor, patch;
    FT_Error err;
    uint32_t font_size;
} UIFont;

extern struct UIFont UIFontGlobal;

void ui_font_size_set(uint32_t font_size);
uint32_t arguments_next(char **arguments, uint32_t arguments_last_length);

int32_t spoor_time_compare_day(SpoorTime *spoor_time0, SpoorTime *spoor_time1); /* Returns 0 if (spoor_time0) and (spoor_time1) are equal. Returns a positiv number if (spoor_time0) is bigger than (spoor_time1). Returns a negativ number if (spoor_time1 is bigger than (spoor_time0) */
u32 spoor_filter_use(u32 *spoor_objects_indexes_orginal, SpoorFilter *spoor_filter);
void spoor_filter_change(SpoorFilter *spoor_filter, char *arguments);

#endif
