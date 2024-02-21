#include"spoor_internal.h"

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdbool.h>
#include<string.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<unistd.h>

#define XLIB_BACKGROUND_COLOR 0xd3b083
#define XLIB_WINDOW_WIDTH 801
#define XLIB_WINDOW_HEIGHT 600
#define UI_STATUS_BAR_HEIGHT 20

#define UI_AREA_FLAG_HORIZONTAL 0b1
#define UI_AREA_FLAG_VERTICAL 0b10

typedef struct UIArea {
    u32 id;
    i32 flags;
    u32 x, y, width, height;
    struct UIArea *parent;
    struct UIArea *childs;
    u32 childs_count;
    struct UIArea *prev;
    struct UIArea *next;
    void (*drawing_func)(struct UIArea *ui_area);
    void (*key_input_func)(struct UIArea *ui_area);
} UIArea;

void ui_area_blank_draw_func(UIArea *ui_area);

void ui_area_key_input_default_func(UIArea *ui_area);
void xlib_ui_calendar_draw(UIArea *ui_area);
void ui_calendar_input(UIArea *ui_area);
void ui_list_draw_func(UIArea *ui_area);
void ui_list_key_input_func(UIArea *ui_area);

enum {
    MODE_DEFAULT,
    MODE_NORMAL,
    MODE_INSERT
};

const char *modes_str[10] = {
    "| DEFAULT |",
    "| NORMAL |",
    "| INSERT |",
};

/* xlib globals */
struct XlibHandle {
    Display *display;
    Window window;
    bool running;
    XEvent event;
    char key_input_buffer[256 + 1];
    KeySym key_sym;
    char *key_sym_str;
    u32 key_code;
    u32 window_width;
    u32 window_height;
    XImage *image;
    u32 *bits;
    bool key_control_down;
    bool key_return_pressed;
    UIArea *ui_area_head;
    UIArea *ui_area_feet;
    bool ui_area_mode_insert;
    u32 mode;
    u32 ui_area_counts;
    char buffer_command[250];
    u32 buffer_command_size;
    bool mode_command;
} XlibHandleGlobal = {
    .running = true,
    .window_width = XLIB_WINDOW_WIDTH,
    .window_height = XLIB_WINDOW_HEIGHT,
    .key_control_down = false,
    .key_return_pressed = false,
    .ui_area_head = &(UIArea){
        .id = 0,
        .flags = 0,
        .x = 0, .y = 0,
        .width = XLIB_WINDOW_WIDTH,
        .height = XLIB_WINDOW_HEIGHT - UI_STATUS_BAR_HEIGHT,
        .parent = NULL,
        .childs = NULL,
        .childs_count = 0,
        .prev = NULL,
        .next = NULL,
        .drawing_func = ui_area_blank_draw_func,
        .key_input_func = ui_area_key_input_default_func,
    },
    .ui_area_mode_insert = false,
    .mode = MODE_DEFAULT,
    .ui_area_counts = 1,
    .buffer_command[0] = 0,
    .buffer_command_size = 0,
    .mode_command = false,
};

struct UIList {
    u32 index_current;
} UIListGlobal = {
    .index_current = 0,
};

struct UICalendar {
    uint8_t days_count;
    i32 today_offset;
    u32 hour_offset;
    u32 spoor_objects_index;
    bool spoor_objects_grab;
} UICalendarGlobal = {
    .days_count = 5,
    .today_offset = 0,
    .hour_offset = 7,
    .spoor_objects_grab = false,
};

const char ui_calendar_week_day_names[7][10] = {
    "SUNDAY",
    "MONDAY",
    "TUESDAY",
    "WEDNESDAY",
    "THURSDAY",
    "FRIDAY",
    "SATURDAY",
};

#define UI_CALENDAR_DAYS_COUNT_LIMIT 255
#define UI_CALENDAR_FONT_COLOR 0x050510
#define DAY_SEC 86400


i32 alphaBlend(i32 colorA, i32 colorB, uint8_t alpha);

void xlib_window_create(void)
{
    XlibHandleGlobal.display = XOpenDisplay(NULL);
    if (XlibHandleGlobal.display == NULL)
    {
        fprintf(stderr,
                "failed to connect to X-Server\n");
        exit(EXIT_FAILURE);
    }

    XSetWindowAttributes attributes = {};
    XlibHandleGlobal.window = XCreateWindow(XlibHandleGlobal.display,
                           DefaultRootWindow(XlibHandleGlobal.display),
                           0, 0,
                           XlibHandleGlobal.window_width, XlibHandleGlobal.window_height,
                           0, 0,
                           InputOutput,
                           XDefaultVisual(XlibHandleGlobal.display, 0),
                           CWOverrideRedirect,
                           &attributes);
    XStoreName(XlibHandleGlobal.display, XlibHandleGlobal.window, "Test");
    XSelectInput(XlibHandleGlobal.display, XlibHandleGlobal.window,
                 KeyPressMask | KeyReleaseMask |
                 StructureNotifyMask | ExposureMask
                 );
    XMapRaised(XlibHandleGlobal.display, XlibHandleGlobal.window);

    XlibHandleGlobal.image = XCreateImage(XlibHandleGlobal.display,
                                          XDefaultVisual(XlibHandleGlobal.display, 0),
                                          24,
                                          ZPixmap,
                                          0, NULL, 0, 0, 32, 0);

    XlibHandleGlobal.bits = realloc(XlibHandleGlobal.bits,
                                    XlibHandleGlobal.window_width * XlibHandleGlobal.window_height * 4);
    XlibHandleGlobal.image->width = XlibHandleGlobal.window_width;
    XlibHandleGlobal.image->height = XlibHandleGlobal.window_height;
    XlibHandleGlobal.image->bytes_per_line = XlibHandleGlobal.window_width * 4;
    XlibHandleGlobal.image->data = (char *)XlibHandleGlobal.bits;

}

void input_special_keys(void)
{
    if (strncmp(XlibHandleGlobal.key_sym_str, "Escape", 6) == 0)
        XlibHandleGlobal.key_input_buffer[0] = 0x1b;

    if (strncmp(XlibHandleGlobal.key_sym_str, "Return", 6) == 0)
        XlibHandleGlobal.key_input_buffer[0] = '\n';

    if (strncmp(XlibHandleGlobal.key_sym_str, "Control_L", 9) == 0)
    {
        if (XlibHandleGlobal.key_control_down)
            XlibHandleGlobal.key_control_down = false;
        else
            XlibHandleGlobal.key_control_down = true;
    }
}

bool spoor_object_index_inside_week(SpoorObject *spoor_object)
{
    time_t time_today_sec_start = time(NULL) + UICalendarGlobal.today_offset * DAY_SEC;
    time_t time_today_sec_end = time_today_sec_start + UICalendarGlobal.days_count * DAY_SEC;
    SpoorTime spoor_time_start = *(SpoorTime *)localtime(&time_today_sec_start);
    SpoorTime spoor_time_end = *(SpoorTime *)localtime(&time_today_sec_end);

    if (spoor_time_compare_day(&spoor_object->schedule.start, &spoor_time_start) < 0 ||
        spoor_time_compare_day(&spoor_object->schedule.start, &spoor_time_end) > 0)
        return false;

    return true;
}

u32 buffer_command_counter_detect_rw(u32 counter_default)
{
    u32 counter = 0;
    u32 i;

    for (i = 0; i < XlibHandleGlobal.buffer_command_size; i++)
    {
        if (XlibHandleGlobal.buffer_command[i] >= 0x30 && XlibHandleGlobal.buffer_command[i] <= 0x39)
        {
            counter *= 10;
            counter += XlibHandleGlobal.buffer_command[i] - 0x30;
        }
    }

    if (counter == 0)
        counter = counter_default;

    /* clear the XlibHandleGlobal.buffer_command */
    XlibHandleGlobal.buffer_command[0] = 0;
    XlibHandleGlobal.buffer_command_size = 0;

    return counter;
}

void ui_calendar_spoor_object_index_next(void)
{
    if (spoor_object_index_inside_week(&spoor_objects[UICalendarGlobal.spoor_objects_index + 1]))
        UICalendarGlobal.spoor_objects_index++;
}

void ui_calendar_spoor_object_index_prev(void)
{
    if (UICalendarGlobal.spoor_objects_index == 0)
        return;
    if (spoor_object_index_inside_week(&spoor_objects[UICalendarGlobal.spoor_objects_index - 1]))
        UICalendarGlobal.spoor_objects_index--;
}

void ui_calendar_spoor_object_index_left(void)
{
    u32 i;
    for (i = UICalendarGlobal.spoor_objects_index - 1; i < spoor_objects_count; i--)
    {
        if (spoor_time_compare_day(&spoor_objects[i].schedule.start,
                                   &spoor_objects[UICalendarGlobal.spoor_objects_index].schedule.start) != 0)
        {
            if (spoor_object_index_inside_week(&spoor_objects[i]))
            {
                for (--i; i < spoor_objects_count; i--)
                {
                    int diff0 = spoor_objects[UICalendarGlobal.spoor_objects_index].schedule.start.hour * 60 + spoor_objects[UICalendarGlobal.spoor_objects_index].schedule.start.min -
                        spoor_objects[i + 1].schedule.start.hour * 60 + spoor_objects[i + 1].schedule.start.min;
                    int diff1 = spoor_objects[UICalendarGlobal.spoor_objects_index].schedule.start.hour * 60 + spoor_objects[UICalendarGlobal.spoor_objects_index].schedule.start.min -
                        spoor_objects[i].schedule.start.hour * 60 + spoor_objects[i].schedule.start.min;

                    if (diff0 + diff1 > 0 ||
                        spoor_time_compare_day(&spoor_objects[i + 1].schedule.start, &spoor_objects[i].schedule.start) != 0)
                    {
                        if (spoor_object_index_inside_week(&spoor_objects[i + 1]))
                            UICalendarGlobal.spoor_objects_index = i + 1;
                        return;
                    }
                }
                UICalendarGlobal.spoor_objects_index = i + 1;
            }
            else
                return;
        }
    }
}

void ui_calendar_spoor_object_index_right(void)
{
    u32 i;
    for (i = UICalendarGlobal.spoor_objects_index + 1; i < spoor_objects_count; i++)
    {
        if (spoor_time_compare_day(&spoor_objects[i].schedule.start,
                                   &spoor_objects[UICalendarGlobal.spoor_objects_index].schedule.start) != 0)
        {
            if (spoor_object_index_inside_week(&spoor_objects[i]))
            {
                for (++i; i < spoor_objects_count; i++)
                {

                    int diff0 = spoor_objects[i - 1].schedule.start.hour * 60 + spoor_objects[i - 1].schedule.start.min -
                        spoor_objects[UICalendarGlobal.spoor_objects_index].schedule.start.hour * 60 + spoor_objects[UICalendarGlobal.spoor_objects_index].schedule.start.min;
                    int diff1 = spoor_objects[i].schedule.start.hour * 60 + spoor_objects[i].schedule.start.min -
                        spoor_objects[UICalendarGlobal.spoor_objects_index].schedule.start.hour * 60 + spoor_objects[UICalendarGlobal.spoor_objects_index].schedule.start.min;

                    if (diff0 + diff1 >= 0 ||
                        spoor_time_compare_day(&spoor_objects[i - 1].schedule.start, &spoor_objects[i].schedule.start) != 0)
                    {
                        if (spoor_object_index_inside_week(&spoor_objects[i - 1]))
                            UICalendarGlobal.spoor_objects_index = i - 1;
                        return;
                    }
                }
                UICalendarGlobal.spoor_objects_index = i - 1;
            }
            else
                return;
        }
    }
}

void ui_calendar_spoor_object_move_left(void)
{
    u32 counter = buffer_command_counter_detect_rw(1);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_days_add(&spoor_object->schedule.start, ~(i32)counter + 1);
    spoor_time_days_add(&spoor_object->schedule.end, ~(i32)counter + 1);
    spoor_storage_change(spoor_object, spoor_object);

    UICalendarGlobal.spoor_objects_index = spoor_sort_objects_reposition_up(UICalendarGlobal.spoor_objects_index);
}

void ui_calendar_spoor_object_move_right(void)
{
    u32 counter = buffer_command_counter_detect_rw(1);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_days_add(&spoor_object->schedule.start, counter);
    spoor_time_days_add(&spoor_object->schedule.end, counter);
    spoor_storage_change(spoor_object, spoor_object);

    UICalendarGlobal.spoor_objects_index = spoor_sort_objects_reposition_down(UICalendarGlobal.spoor_objects_index);
}

void ui_calendar_spoor_object_move_down(void)
{
    u32 counter = buffer_command_counter_detect_rw(10);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_minutes_add(&spoor_object->schedule.start, counter);
    spoor_time_minutes_add(&spoor_object->schedule.end, counter);
    spoor_storage_change(spoor_object, spoor_object);

    UICalendarGlobal.spoor_objects_index = spoor_sort_objects_reposition_down(UICalendarGlobal.spoor_objects_index);
}

void ui_calendar_spoor_object_move_up(void)
{
    u32 counter = buffer_command_counter_detect_rw(10);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_minutes_add(&spoor_object->schedule.start, ~(i32)counter + 1);
    spoor_time_minutes_add(&spoor_object->schedule.end, ~(i32)counter + 1);
    spoor_storage_change(spoor_object, spoor_object);

    UICalendarGlobal.spoor_objects_index = spoor_sort_objects_reposition_up(UICalendarGlobal.spoor_objects_index);
}

void ui_calendar_spoor_object_move_grow(void)
{
    u32 counter = buffer_command_counter_detect_rw(10);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_minutes_add(&spoor_object->schedule.end, counter);
    spoor_storage_change(spoor_object, spoor_object);
}

void ui_calendar_spoor_object_move_shrink(void)
{
    u32 counter = buffer_command_counter_detect_rw(10);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_minutes_add(&spoor_object->schedule.end, ~(i32)counter + 1);
    spoor_storage_change(spoor_object, spoor_object);
}

void mode_command_process(u32 spoor_object_index)
{
    if (XlibHandleGlobal.buffer_command[1] == 'c')
    {
        SpoorObject *spoor_object = spoor_object_create(XlibHandleGlobal.buffer_command + 2);
        spoor_storage_save(spoor_object);

        spoor_sort_objects_append(spoor_object);

        free(spoor_object);
    }
    else if (XlibHandleGlobal.buffer_command[1] == 'e')
    {
        SpoorObject spoor_object = spoor_objects[spoor_object_index];
        SpoorObject spoor_object_old = spoor_object;
        spoor_object_edit(&spoor_object, XlibHandleGlobal.buffer_command + 2);

        spoor_storage_change(&spoor_object_old, &spoor_object);
        spoor_sort_objects_remove(spoor_object_index);
        spoor_sort_objects_append(&spoor_object);
    }
    else if (XlibHandleGlobal.buffer_command[1] == 'q')
    {
        exit(0);
    }
}

void xlib_render(void);
void ui_calendar_input(UIArea *ui_area)
{
    if (XlibHandleGlobal.key_input_buffer[0] == 0x1b)
    {
        XlibHandleGlobal.mode = MODE_DEFAULT;
        xlib_render();
        return;
    }
    if (XlibHandleGlobal.key_control_down)
    {
        switch (XlibHandleGlobal.key_sym_str[0])
        {
            case 's': UICalendarGlobal.today_offset--; break;
            case 'n': UICalendarGlobal.hour_offset++; break;
            case 'r': if (UICalendarGlobal.hour_offset != 0) UICalendarGlobal.hour_offset--; break;
            case 't': UICalendarGlobal.today_offset++; break;
        }
        xlib_render();
    }
    else
    {
        XlibHandleGlobal.buffer_command[XlibHandleGlobal.buffer_command_size++] = XlibHandleGlobal.key_input_buffer[0];
        XlibHandleGlobal.buffer_command[XlibHandleGlobal.buffer_command_size] = 0;
        printf("BUFFER COMMAND: %s\n", XlibHandleGlobal.buffer_command);


        if (XlibHandleGlobal.mode_command)
        {
            if (strncmp(XlibHandleGlobal.key_sym_str, "BackSpace", 9) == 0)
            {
                if (XlibHandleGlobal.buffer_command_size != 1)
                {
                    XlibHandleGlobal.buffer_command_size -= 2;
                    XlibHandleGlobal.buffer_command[XlibHandleGlobal.buffer_command_size] = 0;
                }
            }
            else if (strncmp(XlibHandleGlobal.key_sym_str, "Return", 6) == 0)
            {
                mode_command_process(UICalendarGlobal.spoor_objects_index);
                XlibHandleGlobal.mode_command = false;
                XlibHandleGlobal.buffer_command[0] = 0;
                XlibHandleGlobal.buffer_command_size = 0;
            }

            xlib_render();
            return;
        }

        bool buffer_command_clear = true;

        if (strncmp(XlibHandleGlobal.buffer_command + XlibHandleGlobal.buffer_command_size - 2, "dd", 2) == 0)
        {
            spoor_storage_delete(&spoor_objects[UICalendarGlobal.spoor_objects_index]);
            spoor_sort_objects_remove(UICalendarGlobal.spoor_objects_index);
        }
        if (strncmp(XlibHandleGlobal.buffer_command + XlibHandleGlobal.buffer_command_size - 1, ":", 1) == 0)
        {
            XlibHandleGlobal.mode_command = true;
            XlibHandleGlobal.buffer_command[0] = ':';
            XlibHandleGlobal.buffer_command[1] = 0;
            XlibHandleGlobal.buffer_command_size = 1;

            buffer_command_clear = false;
        }

        if (strncmp(XlibHandleGlobal.buffer_command + XlibHandleGlobal.buffer_command_size - 1, "c", 1) == 0)
        {
            XlibHandleGlobal.mode_command = true;
            XlibHandleGlobal.buffer_command[0] = ':';
            XlibHandleGlobal.buffer_command[1] = 'c';
            XlibHandleGlobal.buffer_command[2] = 0;
            XlibHandleGlobal.buffer_command_size = 2;

            buffer_command_clear = false;
        }
        else if (strncmp(XlibHandleGlobal.buffer_command + XlibHandleGlobal.buffer_command_size - 1, "e", 1) == 0)
        {
            XlibHandleGlobal.mode_command = true;
            XlibHandleGlobal.buffer_command[0] = ':';
            XlibHandleGlobal.buffer_command[1] = 'e';
            XlibHandleGlobal.buffer_command[2] = 0;
            XlibHandleGlobal.buffer_command_size = 2;

            buffer_command_clear = false;
        }
        else if (UICalendarGlobal.spoor_objects_grab)
        {
            switch (XlibHandleGlobal.key_sym_str[0])
            {
                case 'v': UICalendarGlobal.spoor_objects_grab = false; break;
                case 's': ui_calendar_spoor_object_move_left(); break;
                case 'n': ui_calendar_spoor_object_move_down(); break;
                case 'r': ui_calendar_spoor_object_move_up(); break;
                case 't': ui_calendar_spoor_object_move_right(); break;
                case 'N': ui_calendar_spoor_object_move_grow(); break;
                case 'R': ui_calendar_spoor_object_move_shrink(); break;
                default: buffer_command_clear = false; break;

            }
        }
        else
        {
            switch (XlibHandleGlobal.key_sym_str[0])
            {
                case 's': ui_calendar_spoor_object_index_left(); break;
                case 'n': ui_calendar_spoor_object_index_next(); break;
                case 'r': ui_calendar_spoor_object_index_prev(); break;
                case 't': ui_calendar_spoor_object_index_right(); break;
                case 'v': UICalendarGlobal.spoor_objects_grab = true; break;
                case 'N': UICalendarGlobal.days_count--; break;
                case 'R': UICalendarGlobal.days_count++; break;
                default: buffer_command_clear = false; break;
            }
        }
        if (buffer_command_clear)
        {
            XlibHandleGlobal.buffer_command[0] = 0;
            XlibHandleGlobal.buffer_command_size = 0;
        }

        xlib_render();
    }
}

void xlib_line_horizontal_draw(u32 px, u32 py, u32 width, u32 color)
{
    if (px >= XlibHandleGlobal.window_width)
        return;
    if (py >= XlibHandleGlobal.window_height)
        return;

    u32 width_max = px + width;
    if (width_max >= XlibHandleGlobal.window_width)
        width_max = XlibHandleGlobal.window_width;

    u32 x;
    for (x = px; x < width_max; x++)
        XlibHandleGlobal.bits[py * XlibHandleGlobal.window_width + x] = color;
}

void xlib_line_vertical_draw(u32 px, u32 py, u32 height, u32 color)
{
    if (px >= XlibHandleGlobal.window_width)
        return;
    if (py >= XlibHandleGlobal.window_height)
        return;

    u32 height_max = py + height;
    if (height_max >= XlibHandleGlobal.window_height)
        height_max = XlibHandleGlobal.window_height;

    u32 y;
    for (y = py; y < height_max; y++)
        XlibHandleGlobal.bits[y * XlibHandleGlobal.window_width + px] = color;
}

void xlib_rectangle_lines_draw(u32 x, u32 y, u32 width, u32 height, u32 color)
{
    xlib_line_horizontal_draw(x, y, width, color);
    xlib_line_horizontal_draw(x, y + height - 1, width, color);
    xlib_line_vertical_draw(x, y, height, color);
    xlib_line_vertical_draw(x + width - 1, y, height, color);
}

void xlib_rectangle_draw(u32 x, u32 y, u32 width, u32 height, u32 color, u32 alpha)
{
    u32 width_max = x + width;
    u32 height_max = y + height;
    if (width_max > XlibHandleGlobal.window_width)
        width_max = XlibHandleGlobal.window_width;
    if (height_max > XlibHandleGlobal.window_height)
        height_max = XlibHandleGlobal.window_height;

    u32 i, j;
    for (i = y; i < height_max; i++)
        for (j = x; j < width_max; j++)
            XlibHandleGlobal.bits[i * XlibHandleGlobal.window_width + j] = alphaBlend(color, XlibHandleGlobal.bits[i * XlibHandleGlobal.window_width + j], alpha);
}

i32 alphaBlend(i32 colorA, i32 colorB, uint8_t alpha) {
    uint8_t invAlpha = 255 - alpha;

    uint8_t rA = (colorA >> 16) & 0xFF;
    uint8_t gA = (colorA >> 8) & 0xFF;
    uint8_t bA = colorA & 0xFF;

    uint8_t rB = (colorB >> 16) & 0xFF;
    uint8_t gB = (colorB >> 8) & 0xFF;
    uint8_t bB = colorB & 0xFF;

    uint8_t rC = (invAlpha * rA + alpha * rB) / 255;
    uint8_t gC = (invAlpha * gA + alpha * gB) / 255;
    uint8_t bC = (invAlpha * bA + alpha * bB) / 255;

    return (rC << 16) | (gC << 8) | bC;
}


void xlib_text_draw(const char *buffer, u32 x, u32 y, u32 color)
{
    u32 buffer_size = strlen(buffer);

    u32 metrics_height = UIFontGlobal.face->size->metrics.height / 64;
    /*
    printf("y: %d metri_hei: %d\n", y, metrics_height / 4 * 3);
    */
    y += metrics_height / 4 * 3;
    /*
    printf("y: %d metri_hei: %d\n", y, metrics_height / 4 * 3);
    */
    if (y + metrics_height / 4 > XlibHandleGlobal.window_height)
    {
        return;
    }

    u32 k;
    for (k = 0; k < buffer_size; k++)
    {
        if (buffer[k] == ' ')
        {
            x += 3;
            continue;
        }
        FT_UInt glyph_index = FT_Get_Char_Index(UIFontGlobal.face, buffer[k]);

        FT_Int32 load_flags = FT_LOAD_DEFAULT;
        UIFontGlobal.err = FT_Load_Glyph(UIFontGlobal.face, glyph_index, load_flags);
        if (UIFontGlobal.err != 0) {
            printf("Failed to load glyph\n");
            exit(EXIT_FAILURE);
        }

        FT_Int32 render_flags = FT_RENDER_MODE_NORMAL;
        UIFontGlobal.err = FT_Render_Glyph(UIFontGlobal.face->glyph, render_flags);
        if (UIFontGlobal.err != 0) {
            printf("Failed to render the glyph\n");
            exit(EXIT_FAILURE);
        }

        FT_Bitmap bitmap = UIFontGlobal.face->glyph->bitmap;


        for (size_t i = 0; i < bitmap.rows; i++) {
            for (size_t j = 0; j < bitmap.width; j++) {
                int alpha =
                    bitmap.buffer[i * bitmap.pitch + j];
                alpha = 255 - alpha;

                u32 position = (y + i - UIFontGlobal.face->glyph->bitmap_top) * XlibHandleGlobal.window_width + (x + j);
                u32 old_color = XlibHandleGlobal.bits[position]; 
                XlibHandleGlobal.bits[position] = alphaBlend(color, old_color, alpha);
                // XlibHandleGlobal.bits[position] = 0x0000ff;
            }
        }
        x += bitmap.pitch;
    }
}

u32 ui_calendar_schedule_item_color(SpoorType type)
{
    switch (type)
    {
        case TYPE_TASK: return 0xb3b083; break;
        case TYPE_PROJECT: return 0xd39083; break;
        case TYPE_EVENT: return 0xd3b063; break;
        case TYPE_APPOINTMENT: return 0xd3a073; break;
        case TYPE_GOAL: return 0xc3b073; break;
        case TYPE_HABIT: return 0xc3a083; break;
    }

    return 0x000000;
}

/* !!! spoor_objects need to be sorted
 * Functions sets the current pointer index of UICalendarGlobal to
 * the first spoor_objects in the calendar */
void ui_calendar_spoor_objects_pointer_init(void)
{
    time_t time_today_sec = time(NULL) + UICalendarGlobal.today_offset * DAY_SEC;
    struct tm *time_today_tm = localtime(&time_today_sec);
    SpoorTime spoor_time = *(SpoorTime *)time_today_tm;

    u32 i;
    for (i = 0; i < spoor_objects_count; i++)
    {
        if (spoor_time_compare_day(&spoor_objects[i].schedule.start, &spoor_time) >= 0)
        {
            UICalendarGlobal.spoor_objects_index = i;
            return;
        }
    }
}

#if 1
void xlib_ui_calendar_draw(UIArea *ui_area)
{
    time_t time_today_sec  = time(NULL) + UICalendarGlobal.today_offset * DAY_SEC; 
    char today_date_format[36];

    uint8_t i;
    u32 k;
    for (i = 0; i < UICalendarGlobal.days_count; i++, time_today_sec += DAY_SEC)
    {
        u32 width = ui_area->width / UICalendarGlobal.days_count + 1;
        u32 height = ui_area->height;
        u32 x = i * width + ui_area->x;
        u32 y = 0 + ui_area->y;

        struct tm *time_today_tm = localtime(&time_today_sec);

        /*
        if (i + UICalendarGlobal.today_offset == 0)
            xlib_rectangle_draw(x + 2, y + 2, width - 4, 56, 0x5454bb);
            */
        if (i + UICalendarGlobal.today_offset == 0)
            xlib_rectangle_draw(x, y, width, height, 0x552588, 150);

        xlib_text_draw(ui_calendar_week_day_names[time_today_tm->tm_wday],
                    x + 6, y + 6, 0x000000);
        /* draw date text */
        sprintf(today_date_format,
                "%d.%d.%d",
                time_today_tm->tm_mday,
                time_today_tm->tm_mon + 1,
                time_today_tm->tm_year + 1900);
        xlib_text_draw(today_date_format,
                    x + 6, y + 6 + 14 + 6, 0x000000);

        /* separation line between days */
        if (UICalendarGlobal.days_count - i != 1)
            xlib_line_vertical_draw(x + width, y, height - y, 0x000000);

        /* offset y axis */
        y += 60;
        height -= 60;

        /* draw calendar lines */
        u32 hours;
        char hour_format[6];
        for (k = 0; k < height; k += 5)
        {
            if (k % 60 == 0)
            {
                hours = k / 60 + UICalendarGlobal.hour_offset;
                if (hours > 23)
                    break;
                sprintf(hour_format,
                        "%s%u:00",
                        (hours < 10) ?"0" :"", hours);
                xlib_text_draw(hour_format,
                               x + 4, y + k,
                               0x000000);
                xlib_line_horizontal_draw(x, y + k,
                                          width,
                                          0x000000);
            }
            else if (k % 30 == 0)
            {
                xlib_line_horizontal_draw(x + 40, y + k,
                                          width - 40, 0x000000);
            }
            else
            {
                xlib_line_horizontal_draw(x + 40, y + k,
                                          width - 40, 0x643c64);
            }
        }
        /* draw spoor objects scheudle rectangles */
#if 1
        u32 j;
        u32 fs = 10;
        ui_font_size_set(fs);
        for (j = 0; j < spoor_objects_count; j++)
        {
            if (spoor_objects[j].schedule.start.year != -1)
            {
                if (spoor_time_compare_day(&spoor_objects[j].schedule.start, (SpoorTime *)time_today_tm) == 0)
                {
                    int minute_start = spoor_objects[j].schedule.start.hour * 60 + spoor_objects[j].schedule.start.min - UICalendarGlobal.hour_offset * 60;
                    int minute_end = spoor_objects[j].schedule.end.hour * 60 + spoor_objects[j].schedule.end.min - UICalendarGlobal.hour_offset * 60;
                    printf("(%d, %d)\n", minute_start, minute_end);
                    u32 color_outline = 0x000000;

                    if (j == UICalendarGlobal.spoor_objects_index)
                    {
                        if (UICalendarGlobal.spoor_objects_grab)
                            color_outline = 0x0000aa;
                        else
                            color_outline = 0xffffff;
                    }

                    u32 schedule_item_color = ui_calendar_schedule_item_color(spoor_objects[j].type);
                    if (minute_end < 0 || minute_start >= (i32)height)
                        continue;
                    else if (minute_start < 0)
                    {
                        xlib_rectangle_draw(x + 45, y, width - 50, minute_end - minute_start + minute_start, schedule_item_color, 50);
                        xlib_line_horizontal_draw(x + 45, y + minute_end - minute_start + minute_start, width - 50, color_outline);
                        xlib_line_vertical_draw(x + 45, y, minute_end - minute_start + minute_start, color_outline);
                        xlib_line_vertical_draw(x + 45 + width - 50 - 1, y, minute_end - minute_start + minute_start, color_outline);
                        xlib_text_draw(spoor_objects[j].title, x + 50, y + 2, 0x000000);
                    }
                    else
                    {
                        xlib_rectangle_draw(x + 45, y + minute_start, width - 50, minute_end - minute_start, schedule_item_color, 50);
                        xlib_rectangle_lines_draw(x + 45, y + minute_start, width - 50, minute_end - minute_start, color_outline);
                        xlib_text_draw(spoor_objects[j].title, x + 50, y + minute_start + 2, 0x000000);
                    }

#if 0
                    if (minute_start >= 60)
                    {
                        u32 color = ui_calendar_schedule_item_color(spoor_objects[j].type);

                        xlib_rectangle_draw(x + 45, y + minute_start, width - 50, minute_end - minute_start, color, 50);
                        xlib_text_draw(spoor_objects[j].title, x + 50, y + minute_start + 2, 0x000000);

                        char time_format_deadline[50] = { 0 };
                        time_format_parse_deadline(&spoor_objects[j].deadline, time_format_deadline);
                        xlib_text_draw(time_format_deadline, x + 50, y + minute_start + 2 + fs + 2, 0x000000);

                        u32 color_outline = 0x000000;

                        if (j == UICalendarGlobal.spoor_objects_index)
                        {
                            if (UICalendarGlobal.spoor_objects_grab)
                                color_outline = 0x0000aa;
                            else
                                color_outline = 0xffffff;
                        }
                        xlib_rectangle_lines_draw(x + 45, y + minute_start, width - 50, minute_end - minute_start, color_outline);
                    }
#endif
                }
            }
        }
        ui_font_size_set(14);
#endif
    }
}
#endif

void xlib_ui_status_bar_draw(void)
{
    xlib_rectangle_draw(0, XlibHandleGlobal.window_height - UI_STATUS_BAR_HEIGHT,
                        XlibHandleGlobal.window_width, UI_STATUS_BAR_HEIGHT,
                        0x552588, 50);

    printf("BUFFER COMMAND: %s\n", XlibHandleGlobal.buffer_command);
    ui_font_size_set(16);
    if (XlibHandleGlobal.mode_command)
    {
        xlib_text_draw(XlibHandleGlobal.buffer_command, 6, XlibHandleGlobal.window_height - 20,  0x000000);
    }
    else
    {
        xlib_text_draw(modes_str[XlibHandleGlobal.mode], 6, XlibHandleGlobal.window_height - UI_STATUS_BAR_HEIGHT, 0x000000);
        xlib_text_draw(XlibHandleGlobal.buffer_command, XlibHandleGlobal.window_width - 110, XlibHandleGlobal.window_height - 20,  0x000000);
    }
    ui_font_size_set(14);
}

void ui_area_append(void *drawing_func, i32 flags)
{
    if (XlibHandleGlobal.ui_area_feet->parent != NULL && flags == XlibHandleGlobal.ui_area_feet->parent->flags)
    {
        UIArea *ui_area0 = malloc(sizeof(*ui_area0));
        UIArea *ptr = XlibHandleGlobal.ui_area_feet;

        /*
        while (ptr->next != NULL)
            ptr = ptr->next;
            */

        *ui_area0 = *ptr;
        ui_area0->id = XlibHandleGlobal.ui_area_counts++;
        ui_area0->next = ptr->next;
        if (ptr->next)
            ptr->next->prev = ui_area0;
        ptr->next = ui_area0;
        ui_area0->prev = ptr;
        ui_area0->parent = ptr->parent;
        ui_area0->drawing_func = drawing_func;

        ui_area0->parent->childs_count++;

        XlibHandleGlobal.ui_area_feet = ui_area0;

        return;
    }
    UIArea *parent = XlibHandleGlobal.ui_area_feet;
    UIArea *ui_area0 = malloc(sizeof(*ui_area0));
    UIArea *ui_area1 = malloc(sizeof(*ui_area1));

    *ui_area0 = *parent;
    *ui_area1 = *ui_area0;

    parent->id = XlibHandleGlobal.ui_area_counts++;
    ui_area1->id = XlibHandleGlobal.ui_area_counts++;

    ui_area1->drawing_func = drawing_func;

    /* parent */
    parent->flags = flags;
    parent->drawing_func = NULL;
    parent->childs = ui_area0;
    parent->childs_count = 2;

    ui_area0->next = ui_area1;
    ui_area0->prev = NULL;
    ui_area1->prev = ui_area0;
    ui_area1->next = NULL;

    ui_area0->parent = parent;
    ui_area1->parent = parent;

    XlibHandleGlobal.ui_area_feet = ui_area1;
}

void ui_area_close(void)
{
    UIArea *current = XlibHandleGlobal.ui_area_feet;
    if (current == XlibHandleGlobal.ui_area_head)
        return;

    if (current->parent->childs_count == 2)
    {
        UIArea *ptr;
        if (current->prev == NULL)
            ptr = current->next;
        else
            ptr = current->prev;

        if (current->parent == XlibHandleGlobal.ui_area_head)
        {
            ptr->width = XlibHandleGlobal.window_width;
            ptr->height = XlibHandleGlobal.window_height - UI_STATUS_BAR_HEIGHT;
            ptr->x = 0;
            ptr->y = 0;
            ptr->next = NULL;
            ptr->prev = NULL;
            ptr->childs = NULL;
            ptr->parent = NULL;
            ptr->childs_count = 0;
            ptr->flags = 0;
            *XlibHandleGlobal.ui_area_head = *ptr;
            XlibHandleGlobal.ui_area_feet = XlibHandleGlobal.ui_area_head;
            free(ptr);
            free(current);
        }
        else if (current->parent->prev == NULL)
        {
            ptr->parent = ptr->parent->parent;
            ptr->next = current->parent->parent->childs->next;
            current->parent->parent->childs = ptr;
            ptr->next->prev = ptr;
            XlibHandleGlobal.ui_area_feet = ptr;
            free(current->parent);
            free(current);
        }
        else if (current->parent->next == NULL)
        {
            ptr->parent = ptr->parent->parent;
            current->parent->prev->next = ptr;
            ptr->prev = current->parent->prev;
            ptr->next = NULL;
            XlibHandleGlobal.ui_area_feet = ptr;
            free(current->parent);
            free(current);
        }
        else
        {
            ptr->parent = ptr->parent->parent;
            ptr->next = current->parent->next;
            current->parent->prev = ptr;
            current->parent->prev->next = ptr;
            ptr->prev = current->parent->prev;
            XlibHandleGlobal.ui_area_feet = ptr;
            free(current->parent);
            free(current);
        }
        while (XlibHandleGlobal.ui_area_feet->childs != NULL)
            XlibHandleGlobal.ui_area_feet = XlibHandleGlobal.ui_area_feet->childs;
    }
    else
    {
        /* first child */
        if (current->prev == NULL)
        {
            current->parent->childs = current->next;
            current->next->prev = NULL;
            current->parent->childs_count--;

            XlibHandleGlobal.ui_area_feet = current->parent->childs;

            free(current);
        }
        /* last child */
        else if (current->next == NULL)
        {
            current->prev->next = NULL;
            current->parent->childs_count--;

            XlibHandleGlobal.ui_area_feet = current->prev;

            free(current);
        }
        /* middle child */
        else
        {
            current->prev->next = current->next;
            current->next->prev = current->prev;
            current->parent->childs_count--;

            XlibHandleGlobal.ui_area_feet = current->prev;

            free(current);
        }


    }
    xlib_render();
}

void ui_area_move_feet_left(void)
{
    UIArea *current = XlibHandleGlobal.ui_area_feet;
    UIArea *parent = current->parent;
    if (parent == NULL)
        return;

    while (!(current->prev != NULL && parent->flags & UI_AREA_FLAG_HORIZONTAL))
    {
        current = parent;
        if (current == NULL)
            return;
        parent = current->parent;
    }
    current = current->prev;
    if (current == NULL)
        return;
    while (current->flags != 0)
        current = current->childs;

    XlibHandleGlobal.ui_area_feet = current;
}

void ui_area_move_feet_right(void)
{
    UIArea *current = XlibHandleGlobal.ui_area_feet;
    UIArea *parent = current->parent;
    if (parent == NULL)
        return;

    while (!(current->next != NULL && parent->flags & UI_AREA_FLAG_HORIZONTAL))
    {
        current = parent;
        if (current == NULL)
            return;
        parent = current->parent;
    }
    current = current->next;
    if (current == NULL)
        return;
    while (current->flags != 0)
        current = current->childs;

    XlibHandleGlobal.ui_area_feet = current;
}

void ui_area_move_feet_up(void)
{
    UIArea *current = XlibHandleGlobal.ui_area_feet;
    UIArea *parent = current->parent;
    if (parent == NULL)
        return;

    while (!(current->prev != NULL && parent->flags & UI_AREA_FLAG_VERTICAL))
    {
        current = parent;
        if (current == NULL)
            return;
        parent = current->parent;
    }
    current = current->prev;
    if (current == NULL)
        return;
    while (current->flags != 0)
        current = current->childs;

    XlibHandleGlobal.ui_area_feet = current;
}

void ui_area_move_feet_down(void)
{
    UIArea *current = XlibHandleGlobal.ui_area_feet;
    UIArea *parent = current->parent;
    if (parent == NULL)
        return;

    while (!(current->next != NULL && parent->flags & UI_AREA_FLAG_VERTICAL))
    {
        current = parent;
        if (current == NULL)
            return;
        parent = current->parent;
    }
    current = current->next;
    if (current == NULL)
        return;
    while (current->flags != 0)
        current = current->childs;

    XlibHandleGlobal.ui_area_feet = current;
}

void ui_area_blank_draw_func(UIArea *ui_area)
{
    if (XlibHandleGlobal.ui_area_feet != ui_area)
        xlib_rectangle_draw(ui_area->x + 2, ui_area->y + 2,
                            ui_area->width - 4, ui_area->height - 4,
                            0x402c50, 0);
    else
        xlib_rectangle_draw(ui_area->x + 2, ui_area->y + 2,
                            ui_area->width - 4, ui_area->height - 4,
                            0x204c20, 0);

    xlib_rectangle_lines_draw(ui_area->x + 2, ui_area->y + 2,
                              ui_area->width - 4, ui_area->height - 4,
                              0x000000);

    char id_str[20];
    sprintf(id_str, "%d", ui_area->id);
    xlib_text_draw(id_str, ui_area->x + 10, ui_area->y + 10, 0xffffff);
}

void ui_area_key_input_default_func(UIArea *ui_area)
{
}

void _padding(u32 padding)
{
    while (padding--)
        putchar(' ');
}

void ui_area_debug(UIArea *head)
{
    static u32 indent = 0;
    _padding(indent);
    printf("-----\n");
    _padding(indent);
    printf("ID: %d\n", head->id);

    _padding(indent);
    if (head->flags == 0)
        printf("FLAGS: (%d) %s\n",
               head->flags,
               "EMPTY");
    else

        printf("FLAGS: (%d) %s\n",
               head->flags,
               (head->flags & UI_AREA_FLAG_HORIZONTAL) ?"HORIZONTAL" :"VERTICAL");
    _padding(indent);
    printf("DIMENIONS: (%d, %d) (%d, %d)\n",
           head->x, head->y,
           head->width, head->height);
    _padding(indent);
    printf("CHILD_COUNT: %u\n", head->childs_count);
    _padding(indent);
    printf("PARENT: %i\n", (head->parent) ?(i32)head->parent->id :-1);
    _padding(indent);
    printf("CHILDS: %i\n", (head->childs) ?(i32)head->childs->id :-1);
    _padding(indent);
    printf("PREV: %i\n", (head->prev) ?(i32)head->prev->id :-1);
    _padding(indent);
    printf("NEXT: %i\n", (head->next) ?(i32)head->next->id :-1);
    _padding(indent);
    printf("DRAWING_FUNC: %s\n", (head->drawing_func) ?"0X..." :"NULL");
    _padding(indent);
    printf("KEY_INPUT_FUNC: %s\n", (head->key_input_func) ?"0X..." :"NULL");

    if (head->childs != NULL)
    {
        UIArea *parent = head;
        head = parent->childs;

        _padding(indent);
        printf("{\n");
        indent += 5;

        while (head)
        {
            ui_area_debug(head);
            head = head->next;
        }
        indent -= 5;
        _padding(indent);
        printf("}\n");
    }
}

void ui_area_draw(UIArea *head)
{
    if (head->drawing_func != NULL)
    {
        head->drawing_func(head);
        return;
    }
    else if (head->childs != NULL)
    {
        UIArea *parent = head;
        head = parent->childs;

        u32 x = 0;
        u32 diff = parent->width % parent->childs_count;
        while (head)
        {
            if (parent->flags & UI_AREA_FLAG_HORIZONTAL)
            {
                head->width = parent->width / parent->childs_count;
                head->height = parent->height;
                head->x = x * head->width + parent->x;
                head->y = parent->y;

                if (head->next == NULL)
                    head->width += diff;
                ui_area_draw(head);
                head = head->next;
            }
            else
            {
                head->width = parent->width;
                head->height = parent->height / parent->childs_count;
                head->x = parent->x;
                head->y = x * head->height + parent->y;

                if (head->next == NULL)
                    head->height += diff;
                ui_area_draw(head);
                head = head->next;
            }

            x++;
        }
    }
}

void xlib_render(void)
{
    /* clear */
    u32 i;
    for (i = 0; i < XlibHandleGlobal.window_width * XlibHandleGlobal.window_height; i++)
        XlibHandleGlobal.bits[i] = XLIB_BACKGROUND_COLOR;

    /* draw ui areas */
    ui_area_draw(XlibHandleGlobal.ui_area_head);
    xlib_ui_status_bar_draw();

    /* draw current area */
    UIArea *current = XlibHandleGlobal.ui_area_feet;
    xlib_rectangle_lines_draw(current->x, current->y, current->width, current->height, 0xaa0000);

    XPutImage(XlibHandleGlobal.display,
              XlibHandleGlobal.window,
              XDefaultGC(XlibHandleGlobal.display, 0),
              XlibHandleGlobal.image,
              0, 0, 0, 0,
              XlibHandleGlobal.window_width,
              XlibHandleGlobal.window_height);
}

void default_input(void)
{
    XlibHandleGlobal.buffer_command[XlibHandleGlobal.buffer_command_size++] = XlibHandleGlobal.key_input_buffer[0];
    XlibHandleGlobal.buffer_command[XlibHandleGlobal.buffer_command_size] = 0;
    printf("buffer: %s\n", XlibHandleGlobal.buffer_command);

    if (strncmp(XlibHandleGlobal.buffer_command + XlibHandleGlobal.buffer_command_size - 2, "ll", 2) == 0)
    {
        printf("ll - command\n");
        XlibHandleGlobal.ui_area_feet->drawing_func = ui_list_draw_func;
        XlibHandleGlobal.ui_area_feet->key_input_func = ui_list_key_input_func;
        XlibHandleGlobal.buffer_command[0] = 0;
        XlibHandleGlobal.buffer_command_size = 0;
        xlib_render();
        return;
    }

    if (strncmp(XlibHandleGlobal.buffer_command + XlibHandleGlobal.buffer_command_size - 2, "lc", 2) == 0)
    {
        printf("lc - command\n");
        XlibHandleGlobal.ui_area_feet->drawing_func = xlib_ui_calendar_draw;
        XlibHandleGlobal.ui_area_feet->key_input_func = ui_calendar_input;
        XlibHandleGlobal.buffer_command[0] = 0;
        XlibHandleGlobal.buffer_command_size = 0;
        xlib_render();
        return;
    }

    bool buffer_command_clear = true;

    switch (XlibHandleGlobal.buffer_command[XlibHandleGlobal.buffer_command_size - 1])
    {
        case 'o':
        {
            XlibHandleGlobal.ui_area_mode_insert = true;
            XlibHandleGlobal.mode = MODE_NORMAL;
        } break;
        case 'a':
        {
            printf("a test\n");
            ui_area_append(ui_area_blank_draw_func, UI_AREA_FLAG_HORIZONTAL);
        } break;
        case 'i':
        {
            printf("i test\n");
            ui_area_append(ui_area_blank_draw_func, UI_AREA_FLAG_VERTICAL);
        } break;
        case 's':
        {
            ui_area_move_feet_left();
        } break;
        case 'n':
        {
            ui_area_move_feet_down();
        } break;
        case 'r':
        {
            ui_area_move_feet_up();
        } break;
        case 't':
        {
            ui_area_move_feet_right();
        } break;
        case 'c':
        {
            ui_area_close();
        } break;
        case 'd':
        {
            system("clear");
            ui_area_debug(XlibHandleGlobal.ui_area_head);
        } break;
        default:
        {
            buffer_command_clear = false;
        } break;
    }

    if (buffer_command_clear)
    {
        XlibHandleGlobal.buffer_command[0] = 0;
        XlibHandleGlobal.buffer_command_size = 0;
    }

    xlib_render();

}

const char UI_LIST_STATUS[3][12] = {
    "NOT STARTED",
    "IN PROGRESS",
    "COMPLETED",
};

const u32 UI_LIST_STATUS_COLOR[3] = {
    0x222222,
    0x604000,
    0x118011,
};

const char UI_LIST_TYPES[][17] = {
    "TASK",
    "PROJECT",
    "EVENT",
    "APPOINTMENT",
    "GOAL",
    "HABIT",
};

void ui_list_draw_func(UIArea *ui_area)
{
    const u32 ui_list_font_color = 0x000000;
    const u32 line_height = 14;
    ui_font_size_set(12);
    printf("ui_list dim: %d %d\n", ui_area->width, ui_area->height);
    /*
    xlib_rectangle_draw(ui_area->x, ui_area->y, ui_area->width, ui_area->height, 0x1a3b2c, 0);
    */

    /* table title's */
    xlib_text_draw("I", ui_area->x + 10, ui_area->y + 10, ui_list_font_color);
    xlib_text_draw("TITLE", ui_area->x + 30, ui_area->y + 10, ui_list_font_color);
    xlib_text_draw("DEADLINE", ui_area->x + 230, ui_area->y + 10, ui_list_font_color);
    xlib_text_draw("SCHEDULE", ui_area->x + 350, ui_area->y + 10, ui_list_font_color);
    xlib_text_draw("STATUS", ui_area->x + 500, ui_area->y + 10, ui_list_font_color);
    xlib_text_draw("TYPE", ui_area->x + 590, ui_area->y + 10, ui_list_font_color);
    xlib_text_draw("PROJECT", ui_area->x + 680, ui_area->y + 10, ui_list_font_color);

    /* current spoor object selected */
    xlib_rectangle_draw(ui_area->x, ui_area->y + 24 + UIListGlobal.index_current * line_height, ui_area->width, line_height, 0x110524, 190);

    xlib_line_horizontal_draw(ui_area->x, ui_area->y + 22, ui_area->width, 0x000000);

    char time_format_deadline[50] = { 0 };
    char time_format_schedule[50] = { 0 };
    i32 index = 0;
    char index_buf[50] = { 0 };

    u32 i;
    for (i = 0; i < spoor_objects_count; i++)
    {
        index = UIListGlobal.index_current - i;
        if (index == 0)
            index = i;
        else if (index < 1)
            index = ~index + 1;

        sprintf(index_buf, "%d", index);
        xlib_text_draw(index_buf, ui_area->x + 10, ui_area->y + 24 + i * line_height + 2, ui_list_font_color);

        xlib_text_draw(spoor_objects[i].title, ui_area->x + 30, ui_area->y + 24 + i * line_height + 2, ui_list_font_color);

        time_format_parse_deadline(&spoor_objects[i].deadline, time_format_deadline);
        xlib_text_draw(time_format_deadline, ui_area->x + 230, ui_area->y + 24 + i * line_height + 2, ui_list_font_color);

        time_format_parse_schedule(&spoor_objects[i].schedule, time_format_schedule);
        xlib_text_draw(time_format_schedule, ui_area->x + 350, ui_area->y + 24 + i * line_height + 2, ui_list_font_color);

        xlib_text_draw(UI_LIST_STATUS[spoor_objects[i].status], ui_area->x + 500, ui_area->y + 24 + i * line_height + 2, UI_LIST_STATUS_COLOR[spoor_objects[i].status]);

        xlib_text_draw(UI_LIST_TYPES[spoor_objects[i].type], ui_area->x + 590, ui_area->y + 24 + i * line_height + 2, ui_list_font_color);

        xlib_text_draw(spoor_objects[i].parent_title, ui_area->x + 680, ui_area->y + 24 + i * line_height + 2, ui_list_font_color);
    }
    ui_font_size_set(14);
}

void ui_list_key_input_func(UIArea *ui_area)
{
    if (XlibHandleGlobal.key_input_buffer[0] == 0x1b)
    {
        XlibHandleGlobal.mode = MODE_DEFAULT;
        xlib_render();
        return;
    }

    XlibHandleGlobal.buffer_command[XlibHandleGlobal.buffer_command_size++] = XlibHandleGlobal.key_input_buffer[0];
    XlibHandleGlobal.buffer_command[XlibHandleGlobal.buffer_command_size] = 0;

    if (XlibHandleGlobal.mode_command)
    {
        if (strncmp(XlibHandleGlobal.key_sym_str, "BackSpace", 9) == 0)
        {
            if (XlibHandleGlobal.buffer_command_size != 1)
            {
                XlibHandleGlobal.buffer_command_size -= 2;
                XlibHandleGlobal.buffer_command[XlibHandleGlobal.buffer_command_size] = 0;
            }
        }
        else if (strncmp(XlibHandleGlobal.key_sym_str, "Return", 6) == 0)
        {
            XlibHandleGlobal.buffer_command[XlibHandleGlobal.buffer_command_size - 1] = 0;
            mode_command_process(UIListGlobal.index_current);
            XlibHandleGlobal.mode_command = false;
            XlibHandleGlobal.buffer_command[0] = 0;
            XlibHandleGlobal.buffer_command_size = 0;
        }
        xlib_render();
        return;
    }

    bool buffer_command_clear = true;

    if (strncmp(XlibHandleGlobal.buffer_command + XlibHandleGlobal.buffer_command_size - 2, "dd", 2) == 0)
    {
        spoor_storage_delete(&spoor_objects[UIListGlobal.index_current]);
        spoor_sort_objects_remove(UIListGlobal.index_current);
        XlibHandleGlobal.buffer_command[0] = 0;
        XlibHandleGlobal.buffer_command_size = 0;
        xlib_render();
        return;
    }

    switch (XlibHandleGlobal.buffer_command[XlibHandleGlobal.buffer_command_size - 1])
    {
        case ':':
        {
            XlibHandleGlobal.mode_command = true;
            XlibHandleGlobal.buffer_command[0] = ':';
            XlibHandleGlobal.buffer_command[1] = 0;
            XlibHandleGlobal.buffer_command_size = 1;

            buffer_command_clear = false;
        } break;
        case 'c':
        {
            XlibHandleGlobal.mode_command = true;
            XlibHandleGlobal.buffer_command[0] = ':';
            XlibHandleGlobal.buffer_command[1] = 'c';
            XlibHandleGlobal.buffer_command[2] = 0;
            XlibHandleGlobal.buffer_command_size = 2;

            buffer_command_clear = false;
        } break;
        case 'e':
        {
            XlibHandleGlobal.mode_command = true;
            XlibHandleGlobal.buffer_command[0] = ':';
            XlibHandleGlobal.buffer_command[1] = 'e';
            XlibHandleGlobal.buffer_command[2] = 0;
            XlibHandleGlobal.buffer_command_size = 2;

            buffer_command_clear = false;
        } break;
        case 'n':
        {
            u32 counter = buffer_command_counter_detect_rw(1);
            UIListGlobal.index_current += counter;
        } break;
        case 'r':
        {
            u32 counter = buffer_command_counter_detect_rw(1);
            UIListGlobal.index_current -= counter;
        } break;
        default:
        {
            buffer_command_clear = false;
        } break;
    }

    if (buffer_command_clear)
    {
        XlibHandleGlobal.buffer_command[0] = 0;
        XlibHandleGlobal.buffer_command_size = 0;
    }

    xlib_render();
}

void xlib_events(void)
{
    while (XPending(XlibHandleGlobal.display) > 0)
    {
        XNextEvent(XlibHandleGlobal.display, &XlibHandleGlobal.event);
        switch (XlibHandleGlobal.event.type)
        {
            case Expose:
            {
                xlib_render();
            } break;
            case ConfigureNotify:
            {
                XlibHandleGlobal.window_width = XlibHandleGlobal.event.xconfigure.width;
                XlibHandleGlobal.window_height = XlibHandleGlobal.event.xconfigure.height;
                XlibHandleGlobal.ui_area_head->width = XlibHandleGlobal.window_width;
                XlibHandleGlobal.ui_area_head->height = XlibHandleGlobal.window_height - UI_STATUS_BAR_HEIGHT;
                XlibHandleGlobal.bits = realloc(XlibHandleGlobal.bits,
                                                XlibHandleGlobal.window_width * XlibHandleGlobal.window_height * 4);
                XlibHandleGlobal.image->width = XlibHandleGlobal.window_width;
                XlibHandleGlobal.image->height = XlibHandleGlobal.window_height;
                XlibHandleGlobal.image->bytes_per_line = XlibHandleGlobal.window_width * 4;
                XlibHandleGlobal.image->data = (char *)XlibHandleGlobal.bits;
            } break;
            case KeyPress:
            {
                XLookupString(&XlibHandleGlobal.event.xkey,
                              XlibHandleGlobal.key_input_buffer,
                              128,
                              &XlibHandleGlobal.key_sym,
                              NULL);
                XlibHandleGlobal.key_sym_str = XKeysymToString(XlibHandleGlobal.key_sym);
                printf("key_sym_str: %s\n", XlibHandleGlobal.key_sym_str);

                if (XlibHandleGlobal.mode == MODE_NORMAL)
                {
                    if (XlibHandleGlobal.key_input_buffer[0] == 0)
                        input_special_keys();
                    else
                        XlibHandleGlobal.ui_area_feet->key_input_func(XlibHandleGlobal.ui_area_feet);
                }
                else
                    default_input();
#if 0
                if (XlibHandleGlobal.key_input_buffer[0] == 0)
                    input_special_keys();
                else
                    ui_calendar_input();
#endif
            } break;
            case KeyRelease:
            {
                XLookupString(&XlibHandleGlobal.event.xkey,
                              XlibHandleGlobal.key_input_buffer,
                              128,
                              &XlibHandleGlobal.key_sym,
                              NULL);
                XlibHandleGlobal.key_sym_str = XKeysymToString(XlibHandleGlobal.key_sym);
                input_special_keys();
            } break;
            default:
            {
                printf("event: %d\n", XlibHandleGlobal.event.type);
            } break;
        }
    }
}

void spoor_ui_xlib_show_rw_(void)
{
    spoor_objects_count = spoor_object_storage_load(NULL);
    spoor_sort_objects_by_deadline();
    ui_calendar_spoor_objects_pointer_init();

    XlibHandleGlobal.ui_area_feet = XlibHandleGlobal.ui_area_head;

    xlib_window_create();

    while (XlibHandleGlobal.running)
        xlib_events();
}
