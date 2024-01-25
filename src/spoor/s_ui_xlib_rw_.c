#include"spoor_internal.h"

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdbool.h>
#include<string.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<unistd.h>

#define FPS 60

#define XLIB_BACKGROUND_COLOR 0xd3b083
#define XLIB_WINDOW_WIDTH 800
#define XLIB_WINDOW_HEIGHT 600

/* xlib globals */
struct XlibHandle {
    Display *display;
    Window window;
    bool running;
    XEvent event;
    char key_input_buffer[256 + 1];
    KeySym key_sym;
    char *key_sym_str;
    uint32_t key_code;
    uint32_t window_width;
    uint32_t window_height;
    XImage *image;
    uint32_t *bits;
    bool key_control_down;
    bool key_return_pressed;
} XlibHandleGlobal = {
    .running = true,
    .window_width = XLIB_WINDOW_WIDTH,
    .window_height = XLIB_WINDOW_HEIGHT,
    .key_control_down = false,
    .key_return_pressed = false,
};

struct UICalendar {
    uint8_t days_count;
    int32_t today_offset;
    uint32_t hour_offset;
    uint32_t spoor_objects_index;
    bool spoor_objects_grab;
    char buffer_command[250];
    uint32_t buffer_command_size;
    bool mode_command;
} UICalendarGlobal = {
    .days_count = 5,
    .today_offset = 0,
    .hour_offset = 7,
    .spoor_objects_grab = false,
    .buffer_command[0] = 0,
    .buffer_command_size = 0,
    .mode_command = false,
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

#define UI_STATUS_BAR_HEIGHT 20

int32_t alphaBlend(int32_t colorA, int32_t colorB, uint8_t alpha);

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

uint32_t buffer_command_counter_detect_rw(uint32_t counter_default)
{
    uint32_t counter = 0;
    uint32_t i;

    for (i = 0; i < UICalendarGlobal.buffer_command_size; i++)
    {
        if (UICalendarGlobal.buffer_command[i] >= 0x30 && UICalendarGlobal.buffer_command[i] <= 0x39)
        {
            counter *= 10;
            counter += UICalendarGlobal.buffer_command[i] - 0x30;
        }
    }

    if (counter == 0)
        counter = counter_default;

    /* clear the UICalendarGlobal.buffer_command */
    UICalendarGlobal.buffer_command[0] = 0;
    UICalendarGlobal.buffer_command_size = 0;

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
    uint32_t i;
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
    uint32_t i;
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
    uint32_t counter = buffer_command_counter_detect_rw(1);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_days_add(&spoor_object->schedule.start, ~(int32_t)counter + 1);
    spoor_time_days_add(&spoor_object->schedule.end, ~(int32_t)counter + 1);
    spoor_storage_change(spoor_object, spoor_object);

    UICalendarGlobal.spoor_objects_index = spoor_sort_objects_reposition_up(UICalendarGlobal.spoor_objects_index);
}

void ui_calendar_spoor_object_move_right(void)
{
    uint32_t counter = buffer_command_counter_detect_rw(1);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_days_add(&spoor_object->schedule.start, counter);
    spoor_time_days_add(&spoor_object->schedule.end, counter);
    spoor_storage_change(spoor_object, spoor_object);

    UICalendarGlobal.spoor_objects_index = spoor_sort_objects_reposition_down(UICalendarGlobal.spoor_objects_index);
}

void ui_calendar_spoor_object_move_down(void)
{
    uint32_t counter = buffer_command_counter_detect_rw(10);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_minutes_add(&spoor_object->schedule.start, counter);
    spoor_time_minutes_add(&spoor_object->schedule.end, counter);
    spoor_storage_change(spoor_object, spoor_object);

    UICalendarGlobal.spoor_objects_index = spoor_sort_objects_reposition_down(UICalendarGlobal.spoor_objects_index);
}

void ui_calendar_spoor_object_move_up(void)
{
    uint32_t counter = buffer_command_counter_detect_rw(10);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_minutes_add(&spoor_object->schedule.start, ~(int32_t)counter + 1);
    spoor_time_minutes_add(&spoor_object->schedule.end, ~(int32_t)counter + 1);
    spoor_storage_change(spoor_object, spoor_object);

    UICalendarGlobal.spoor_objects_index = spoor_sort_objects_reposition_up(UICalendarGlobal.spoor_objects_index);
}

void ui_calendar_spoor_object_move_grow(void)
{
    uint32_t counter = buffer_command_counter_detect_rw(10);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_minutes_add(&spoor_object->schedule.end, counter);
    spoor_storage_change(spoor_object, spoor_object);
}

void ui_calendar_spoor_object_move_shrink(void)
{
    uint32_t counter = buffer_command_counter_detect_rw(10);

    SpoorObject *spoor_object = &spoor_objects[UICalendarGlobal.spoor_objects_index];
    spoor_time_minutes_add(&spoor_object->schedule.end, ~(int32_t)counter + 1);
    spoor_storage_change(spoor_object, spoor_object);
}

void mode_command_process(void)
{
    if (UICalendarGlobal.buffer_command[1] == 'c')
    {
        SpoorObject *spoor_object = spoor_object_create(UICalendarGlobal.buffer_command + 2);
        spoor_storage_save(spoor_object);

        spoor_sort_objects_append(spoor_object);

        free(spoor_object);
    }
    else if (UICalendarGlobal.buffer_command[1] == 'e')
    {
        SpoorObject spoor_object = spoor_objects[UICalendarGlobal.spoor_objects_index];
        SpoorObject spoor_object_old = spoor_object;
        spoor_object_edit(&spoor_object, UICalendarGlobal.buffer_command + 2);

        spoor_storage_change(&spoor_object_old, &spoor_object);
        spoor_sort_objects_remove(UICalendarGlobal.spoor_objects_index);
        spoor_sort_objects_append(&spoor_object);
    }
}

void xlib_render(void);
void ui_calendar_input(void)
{
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
        UICalendarGlobal.buffer_command[UICalendarGlobal.buffer_command_size++] = XlibHandleGlobal.key_input_buffer[0];
        UICalendarGlobal.buffer_command[UICalendarGlobal.buffer_command_size] = 0;
        printf("BUFFER COMMAND: %s\n", UICalendarGlobal.buffer_command);


        if (UICalendarGlobal.mode_command)
        {
            if (strncmp(XlibHandleGlobal.key_sym_str, "BackSpace", 9) == 0)
            {
                UICalendarGlobal.buffer_command_size -= 2;
                UICalendarGlobal.buffer_command[UICalendarGlobal.buffer_command_size] = 0;
            }
            if (strncmp(XlibHandleGlobal.key_sym_str, "Return", 6) == 0)
            {
                mode_command_process();
                UICalendarGlobal.mode_command = false;
                UICalendarGlobal.buffer_command[0] = 0;
                UICalendarGlobal.buffer_command_size = 0;
            }

            xlib_render();
            return;
        }

        bool buffer_command_clear = true;

        if (strncmp(UICalendarGlobal.buffer_command + UICalendarGlobal.buffer_command_size - 2, "dd", 2) == 0)
        {
            spoor_storage_delete(&spoor_objects[UICalendarGlobal.spoor_objects_index]);
            spoor_sort_objects_remove(UICalendarGlobal.spoor_objects_index);
        }
        if (strncmp(UICalendarGlobal.buffer_command + UICalendarGlobal.buffer_command_size - 1, ":", 1) == 0)
        {
            UICalendarGlobal.mode_command = true;
            UICalendarGlobal.buffer_command[0] = ':';
            UICalendarGlobal.buffer_command[1] = 0;
            UICalendarGlobal.buffer_command_size = 1;

            buffer_command_clear = false;
        }

        if (strncmp(UICalendarGlobal.buffer_command + UICalendarGlobal.buffer_command_size - 1, "c", 1) == 0)
        {
            UICalendarGlobal.mode_command = true;
            UICalendarGlobal.buffer_command[0] = ':';
            UICalendarGlobal.buffer_command[1] = 'c';
            UICalendarGlobal.buffer_command[2] = 0;
            UICalendarGlobal.buffer_command_size = 2;

            buffer_command_clear = false;
        }
        else if (strncmp(UICalendarGlobal.buffer_command + UICalendarGlobal.buffer_command_size - 1, "e", 1) == 0)
        {
            UICalendarGlobal.mode_command = true;
            UICalendarGlobal.buffer_command[0] = ':';
            UICalendarGlobal.buffer_command[1] = 'c';
            UICalendarGlobal.buffer_command[2] = 0;
            UICalendarGlobal.buffer_command_size = 2;

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
                default: buffer_command_clear = false; break;
            }
        }
        if (buffer_command_clear)
        {
            UICalendarGlobal.buffer_command[0] = 0;
            UICalendarGlobal.buffer_command_size = 0;
        }

        xlib_render();
    }
}

void xlib_line_horizontal_draw(uint32_t px, uint32_t py, uint32_t width, uint32_t color)
{
    if (px >= XlibHandleGlobal.window_width)
        return;
    if (py >= XlibHandleGlobal.window_height)
        return;

    uint32_t width_max = px + width;
    if (width_max >= XlibHandleGlobal.window_width)
        width_max = XlibHandleGlobal.window_width;

    uint32_t x;
    for (x = px; x < width_max; x++)
        XlibHandleGlobal.bits[py * XlibHandleGlobal.window_width + x] = color;
}

void xlib_line_vertical_draw(uint32_t px, uint32_t py, uint32_t height, uint32_t color)
{
    if (px >= XlibHandleGlobal.window_width)
        return;
    if (py >= XlibHandleGlobal.window_height)
        return;

    uint32_t height_max = py + height;
    if (height_max >= XlibHandleGlobal.window_height)
        height_max = XlibHandleGlobal.window_height;

    uint32_t y;
    for (y = py; y < height_max; y++)
        XlibHandleGlobal.bits[y * XlibHandleGlobal.window_width + px] = color;
}

void xlib_rectangle_lines_draw(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color)
{
    xlib_line_horizontal_draw(x, y, width, color);
    xlib_line_horizontal_draw(x, y + height, width, color);
    xlib_line_vertical_draw(x, y, height, color);
    xlib_line_vertical_draw(x + width, y, height, color);
}

void xlib_rectangle_draw(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color, uint32_t alpha)
{
    uint32_t width_max = x + width;
    uint32_t height_max = y + height;
    if (width_max > XlibHandleGlobal.window_width)
        width_max = XlibHandleGlobal.window_width;
    if (height_max > XlibHandleGlobal.window_height)
        height_max = XlibHandleGlobal.window_height;

    uint32_t i, j;
    for (i = y; i < height_max; i++)
        for (j = x; j < width_max; j++)
            XlibHandleGlobal.bits[i * XlibHandleGlobal.window_width + j] = alphaBlend(color, XlibHandleGlobal.bits[i * XlibHandleGlobal.window_width + j], alpha);
}

int32_t alphaBlend(int32_t colorA, int32_t colorB, uint8_t alpha) {
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


void xlib_text_draw(const char *buffer, uint32_t x, uint32_t y, uint32_t color)
{
    uint32_t buffer_size = strlen(buffer);

    uint32_t metrics_height = UIFontGlobal.face->size->metrics.height / 64;
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

    uint32_t k;
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

                uint32_t position = (y + i - UIFontGlobal.face->glyph->bitmap_top) * XlibHandleGlobal.window_width + (x + j);
                uint32_t old_color = XlibHandleGlobal.bits[position]; 
                XlibHandleGlobal.bits[position] = alphaBlend(color, old_color, alpha);
                // XlibHandleGlobal.bits[position] = 0x0000ff;
            }
        }
        x += bitmap.pitch;
    }
}

uint32_t ui_calendar_schedule_item_color(SpoorType type)
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

    uint32_t i;
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
void xlib_ui_calendar_draw(void)
{
    time_t time_today_sec  = time(NULL) + UICalendarGlobal.today_offset * DAY_SEC; 
    char today_date_format[36];

    uint8_t i;
    uint32_t k;
    for (i = 0; i < UICalendarGlobal.days_count; i++, time_today_sec += DAY_SEC)
    {
        uint32_t width = XlibHandleGlobal.window_width / UICalendarGlobal.days_count + 1;
        uint32_t height = XlibHandleGlobal.window_height - UI_STATUS_BAR_HEIGHT;
        uint32_t x = i * width;
        uint32_t y = 0;

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
        uint32_t hours;
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
        uint32_t j;
        uint32_t fs = 10;
        ui_font_size_set(fs);
        for (j = 0; j < spoor_objects_count; j++)
        {
            if (spoor_objects[j].schedule.start.year != -1)
            {
                if (spoor_time_compare_day(&spoor_objects[j].schedule.start, (SpoorTime *)time_today_tm) == 0)
                {
                    int minute_start = spoor_objects[j].schedule.start.hour * 60 + spoor_objects[j].schedule.start.min + 60 - UICalendarGlobal.hour_offset * 60;
                    int minute_end = spoor_objects[j].schedule.end.hour * 60 + spoor_objects[j].schedule.end.min + 60 - UICalendarGlobal.hour_offset * 60;
                    if (minute_start >= 60)
                    {
                        uint32_t color = ui_calendar_schedule_item_color(spoor_objects[j].type);

                        xlib_rectangle_draw(x + 45, minute_start, width - 50, minute_end - minute_start, color, 50);
                        xlib_text_draw(spoor_objects[j].title, x + 50, minute_start + 2, 0x000000);

                        char time_format_deadline[50] = { 0 };
                        time_format_parse_deadline(&spoor_objects[j].deadline, time_format_deadline);
                        xlib_text_draw(time_format_deadline, x + 50, minute_start + 2 + fs + 2, 0x000000);

                        uint32_t color_outline = 0x000000;

                        if (j == UICalendarGlobal.spoor_objects_index)
                        {
                            if (UICalendarGlobal.spoor_objects_grab)
                                color_outline = 0x0000aa;
                            else
                                color_outline = 0xffffff;
                        }
                        xlib_rectangle_lines_draw(x + 45, minute_start, width - 50, minute_end - minute_start, color_outline);
                    }
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

    printf("BUFFER COMMAND: %s\n", UICalendarGlobal.buffer_command);
    ui_font_size_set(16);
    if (UICalendarGlobal.mode_command)
    {
        xlib_text_draw(UICalendarGlobal.buffer_command, 6, XlibHandleGlobal.window_height - 20,  0x000000);
    }
    else
    {
        xlib_text_draw(UICalendarGlobal.buffer_command, XlibHandleGlobal.window_width - 110, XlibHandleGlobal.window_height - 20,  0x000000);
    }
    ui_font_size_set(14);
}

void xlib_render(void)
{
    /* clear */
    uint32_t i;
    for (i = 0; i < XlibHandleGlobal.window_width * XlibHandleGlobal.window_height; i++)
        XlibHandleGlobal.bits[i] = XLIB_BACKGROUND_COLOR;

    /*
    ui_font_size_set(14);
    xlib_rectangle_draw(0, 0, 100, 10, 0xaaaaaa);
    xlib_rectangle_draw(0, 0, 10, 100, 0xaaaaaa);
    xlib_text_draw("vN evtg", 10, 10, 0x000000);
    */
    xlib_ui_calendar_draw();
    xlib_ui_status_bar_draw();

    XPutImage(XlibHandleGlobal.display,
              XlibHandleGlobal.window,
              XDefaultGC(XlibHandleGlobal.display, 0),
              XlibHandleGlobal.image,
              0, 0, 0, 0,
              XlibHandleGlobal.window_width,
              XlibHandleGlobal.window_height);
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
                if (XlibHandleGlobal.key_input_buffer[0] == 0)
                    input_special_keys();
                else
                    ui_calendar_input();
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

    xlib_window_create();

    while (XlibHandleGlobal.running)
        xlib_events();
}
