#include"spoor_internal.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<stdint.h>
#include<unistd.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<X11/extensions/Xdbe.h>

XdbeBackBuffer back_buffer;
#define FPS 60

#define WINDOW_NAME "SPOOR ~ BY ~ REALLOCMB"

#define UI_STATUS_BAR_HEIGHT 20

#define BACKGROUND_COLOR 0xd3b083
#define SCHEDULE_ITEM_COLOR 0x556644

GC gc;
Display *display;
Window window;

uint32_t schedule_item_color_get(SpoorType type)
{
    switch (type)
    {
        case TYPE_TASK: return 0x6d6d6d; break;
        case TYPE_PROJECT: return 0x8551ff; break;
        case TYPE_EVENT: return 0xfff851; break;
        case TYPE_APPOINTMENT: return 0x5151ff; break;
        case TYPE_GOAL: return 0xff51fc; break;
        case TYPE_HABIT: return 0xff5151; break;
    }

    return 0x00000000;
}

void window_create(void)
{
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "connection to X-Server failed!!!\n");
        exit(1);
    }

    window = XCreateSimpleWindow(display, 
                                 XDefaultRootWindow(display), 
                                 0, 0, 
                                 800, 450, 
                                 0, 
                                 0x0000000, 
                                 BACKGROUND_COLOR);
    XStoreName(display, window, WINDOW_NAME);
    XMapWindow(display, window);

    XSync(display, False);
}

void window_destroy(void)
{
    XCloseDisplay(display);
}

uint32_t window_width;
uint32_t window_height;

void window_size_current(void)
{
    XWindowAttributes x_window_attributes;
    XGetWindowAttributes(display, window, &x_window_attributes);
    window_width = x_window_attributes.width;
    window_height = x_window_attributes.height;
}

char ui_week_day_names[7][10] = {
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
};

time_t ui_week_today_time;
int32_t ui_week_today_offset = 0;
uint32_t ui_week_time_start = 7;
uint32_t ui_week_spoor_object_index_current;
bool ui_week_spoor_object_index_grap = false;

void ui_week_today_time_update(void)
{
    ui_week_today_time = time(NULL);
}

uint32_t ui_week_days_count = 5;
void ui_week_draw(void)
{
    /*
    XDrawString(display, back_buffer, gc, 6, 14, "Datum - Datum", 13);
    XDrawLine(display, back_buffer, gc, 0, 20, window_width, 20);
    */

    uint32_t i;
    uint32_t k;
    uint32_t j;
    for (i = 0; i < ui_week_days_count; i++)
    {
        uint32_t width = window_width / ui_week_days_count + 1;
        uint32_t height = window_height - UI_STATUS_BAR_HEIGHT;
        uint32_t x = i * width;
        uint32_t y = 0;

        int32_t day_offset = ((int32_t)i + ui_week_today_offset);
        time_t week_day_time = ui_week_today_time + 60 * 60 * 24 * day_offset;

        struct tm *tm_time = localtime(&week_day_time);
        if (day_offset == 0)
            XSetForeground(display, gc, 0xaa0099);

        XDrawString(display, back_buffer, gc, x + 6, y + 14, ui_week_day_names[tm_time->tm_wday], strlen(ui_week_day_names[tm_time->tm_wday]));

        char date[36];
        sprintf(date, "%d.%d.%d",
                tm_time->tm_mday,
                tm_time->tm_mon + 1,
                tm_time->tm_year + 1900);

        XDrawString(display, back_buffer, gc, x + 6, y + 2 * 14, date, strlen(date));

        XSetForeground(display, gc, 0x000000);

        /* separition line between week_days */
        XDrawLine(display, back_buffer, gc, x + width, y, x + width, y + height);

        /* offset */
        y += 60;
        height -= 60;

        /* draw the calender lines */
        for (k = 0; k < height; k += 5)
        {

            if (k % 60 == 0)
            {
                uint32_t hours = k / 60 + ui_week_time_start;
                if (hours > 23)
                    break;
                char time[6];
                sprintf(time,
                        "%s%u:00",
                        (hours < 10) ?"0" :"", hours);
                XDrawString(display, back_buffer, gc, x + 4, y + k + 14, time, strlen(time));

                XDrawLine(display, back_buffer, gc, x, y + k, x + width, y + k);
            }
            else if (k % 30 == 0)
            {
                XDrawLine(display, back_buffer, gc, x + 40, y + k, x + width, y + k);
            }
            else
            {
                XSetForeground(display, gc, 0x643c64);
                XDrawLine(display, back_buffer, gc, x + 40, y + k, x + width - 1, y + k);
                XSetForeground(display, gc, 0x000000);
            }
        }

        /* draw schedule spoor_objects */
        printf("spoor_objects_count: %d\n", spoor_objects_count);
#if 1
        for (j = 0; j < spoor_objects_count; j++)
        {
            if (spoor_objects[j].schedule.start.year != -1) 
            {
                if (spoor_time_compare_day(&spoor_objects[j].schedule.start, (SpoorTime*)tm_time) == 0)
                {
                    int minute_start = spoor_objects[j].schedule.start.hour * 60 + spoor_objects[j].schedule.start.min + 60 - ui_week_time_start * 60;
                    int minute_end = spoor_objects[j].schedule.end.hour * 60 + spoor_objects[j].schedule.end.min + 60 - ui_week_time_start * 60;
                    printf("mitue start - end: %d - %d\n", minute_start, minute_end);
#if 1
                    if (minute_start >= 60)
                    {
                        uint32_t color = schedule_item_color_get(spoor_objects[j].type);
                        XSetForeground(display, gc, color);
                        XFillRectangle(display, back_buffer, gc, x + 45, minute_start, width - 50, minute_end - minute_start);
                        XSetForeground(display, gc, 0x000000);
                        XDrawString(display, back_buffer, gc, x + 50, minute_start + 14, spoor_objects[j].title, strlen(spoor_objects[j].title));

                        char time_format_deadline[50] = { 0 };
                        time_format_parse_deadline(&spoor_objects[j].deadline, time_format_deadline);
                        XDrawString(display, back_buffer, gc, x + 50, minute_start + 2 * 14, time_format_deadline, strlen(time_format_deadline));

                        if (j == ui_week_spoor_object_index_current)
                        {
                            if (ui_week_spoor_object_index_grap)
                            {
                                XSetForeground(display, gc, 0x0000ff);
                                XDrawRectangle(display, back_buffer, gc, x + 45, minute_start, width - 50, minute_end - minute_start);
                                XSetForeground(display, gc, 0x000000);
                            }
                            else 
                            {
                                XSetForeground(display, gc, 0xffffff);
                                XDrawRectangle(display, back_buffer, gc, x + 45, minute_start, width - 50, minute_end - minute_start);
                                XSetForeground(display, gc, 0x000000);
                            }
                        }
                    }
#endif
                }
            }
            
        }
#endif
    }
}

char buffer_command[200] = { 0 };
uint32_t buffer_command_count = 0;

uint32_t buffer_command_counter_detect(uint32_t counter_default)
{
    uint32_t counter = 0;
    uint32_t i;

    for (i = 0; i < buffer_command_count; i++)
    {
        if (buffer_command[i] >= 0x30 && buffer_command[i] <= 0x39)
        {
            counter *= 10;
            counter += buffer_command[i] - 0x30;
        }
    }

    if (counter == 0)
        counter = counter_default;

    /* clear the buffer_command */
    buffer_command[0] = 0;
    buffer_command_count = 0;

    return counter;
}

bool mode_command = false;
void ui_status_bar_draw(void)
{
    XSetForeground(display, gc, 0x828282);
    XFillRectangle(display, back_buffer, gc, 0, window_height - UI_STATUS_BAR_HEIGHT, window_width, UI_STATUS_BAR_HEIGHT);
    XSetForeground(display, gc, 0x000000);


    if (mode_command)
    {
        uint32_t buffer_command_offset = 0;
        if (buffer_command_count > 200)
            buffer_command_offset = buffer_command_count - 200;

        XDrawString(display, back_buffer, gc, 6, window_height - 6, buffer_command + buffer_command_offset, buffer_command_count);
    }
    else
    {
        XDrawString(display, back_buffer, gc, 6, window_height - 6, "NORMAL", 6);

        uint32_t buffer_command_offset = 0;
        if (buffer_command_count > 14)
            buffer_command_offset = buffer_command_count - 14;

        XDrawString(display, back_buffer, gc, window_width - 120, window_height - 6, buffer_command + buffer_command_offset, buffer_command_count);
    }
} 

void draw()
{
    /*
    XClearWindow(display, window);
    */
    window_size_current();
    ui_week_draw();
    ui_status_bar_draw();
}

uint32_t ui_week_schedule_select_update(void)
{
    /* todo */
    return 0;
}

uint32_t ui_week_schedule_first(void)
{
    time_t time_sec = ui_week_today_time + ui_week_today_offset * 60 * 60 * 24;
    SpoorTime spoor_time = spoor_time_struct_tm_convert(&time_sec);

    uint32_t i;
    for (i = 0; i < spoor_objects_count; i++)
    {
        if (spoor_objects[i].schedule.start.day >= spoor_time.day &&
            spoor_objects[i].schedule.start.mon >= spoor_time.mon &&
            spoor_objects[i].schedule.start.year >= spoor_time.year)
        {
            return i;
        }
    }

    return -1;
}

bool ui_week_schedule_check(SpoorObject *spoor_object)
{
    time_t week_day_time_start = ui_week_today_time + 60 * 60 * 24 * ui_week_today_offset;
    time_t week_day_time_end = week_day_time_start + ui_week_days_count * 60 * 60 * 24;
    SpoorTime spoor_time_start = spoor_time_struct_tm_convert(&week_day_time_start);
    SpoorTime spoor_time_end = spoor_time_struct_tm_convert(&week_day_time_end);
    
    if (spoor_object->schedule.start.day < spoor_time_start.day ||
        spoor_object->schedule.start.mon < spoor_time_start.mon ||
        spoor_object->schedule.start.year < spoor_time_start.year)
        return false;

    if (spoor_object->schedule.start.day > spoor_time_end.day ||
        spoor_object->schedule.start.mon > spoor_time_end.mon ||
        spoor_object->schedule.start.year > spoor_time_end.year)
        return false;
    
    return true;
}

void ui_week_schedule_next(void)
{
    uint32_t counter = buffer_command_counter_detect(1);

    if (ui_week_schedule_check(&spoor_objects[ui_week_spoor_object_index_current + counter]))
        ui_week_spoor_object_index_current += counter;
}

void ui_week_schedule_prev(void)
{
    uint32_t counter = buffer_command_counter_detect(1);

    if (ui_week_spoor_object_index_current == 0)
        return;
    if (ui_week_schedule_check(&spoor_objects[ui_week_spoor_object_index_current - counter]))
        ui_week_spoor_object_index_current -= counter;
}

void ui_week_schedule_index_right(void)
{
    uint32_t i;
    for (i = ui_week_spoor_object_index_current + 1; i < spoor_objects_count; i++)
    {
        if (spoor_objects[i].schedule.start.day != spoor_objects[ui_week_spoor_object_index_current].schedule.start.day)
        {
            ui_week_spoor_object_index_current = i;
            return;
        }
    }
}

void ui_week_schedule_index_left(void)
{
    uint32_t i;
    printf("index_current %d\n", ui_week_spoor_object_index_current);
    for (i = ui_week_spoor_object_index_current - 1; i >= 0; i--)
    {
        if (spoor_objects[i].schedule.start.day != spoor_objects[ui_week_spoor_object_index_current].schedule.start.day)
        {
            ui_week_spoor_object_index_current = i;
            printf("index_current %d\n", ui_week_spoor_object_index_current);
            return;
        }
    }
}

void ui_week_schedule_move_down(void)
{
    uint32_t counter = buffer_command_counter_detect(10);

    SpoorObject *spoor_object = &spoor_objects[ui_week_spoor_object_index_current];
    spoor_time_minutes_add(&spoor_object->schedule.start, counter);
    spoor_time_minutes_add(&spoor_object->schedule.end, counter);
    spoor_storage_change(spoor_object, spoor_object);

    ui_week_spoor_object_index_current = spoor_sort_objects_reposition_down(ui_week_spoor_object_index_current);
}

void ui_week_schedule_move_up(void)
{
    uint32_t counter = buffer_command_counter_detect(10);

    SpoorObject *spoor_object = &spoor_objects[ui_week_spoor_object_index_current];
    spoor_time_minutes_add(&spoor_object->schedule.start, ~(int32_t)counter + 1);
    spoor_time_minutes_add(&spoor_object->schedule.end, ~(int32_t)counter + 1);
    spoor_storage_change(spoor_object, spoor_object);

    ui_week_spoor_object_index_current = spoor_sort_objects_reposition_up(ui_week_spoor_object_index_current);
}

void ui_week_schedule_move_left(void)
{
    uint32_t counter = buffer_command_counter_detect(1);

    SpoorObject *spoor_object = &spoor_objects[ui_week_spoor_object_index_current];
    spoor_time_days_add(&spoor_object->schedule.start, ~(int32_t)counter + 1);
    spoor_time_days_add(&spoor_object->schedule.end, ~(int32_t)counter + 1);
    spoor_storage_change(spoor_object, spoor_object);

    ui_week_spoor_object_index_current = spoor_sort_objects_reposition_up(ui_week_spoor_object_index_current);
}

void ui_week_schedule_move_right(void)
{
    uint32_t counter = buffer_command_counter_detect(1);

    SpoorObject *spoor_object = &spoor_objects[ui_week_spoor_object_index_current];
    spoor_time_days_add(&spoor_object->schedule.start, counter);
    spoor_time_days_add(&spoor_object->schedule.end, counter);
    spoor_storage_change(spoor_object, spoor_object);

    ui_week_spoor_object_index_current = spoor_sort_objects_reposition_down(ui_week_spoor_object_index_current);
}

void ui_week_schedule_grow(void)
{
    uint32_t counter = buffer_command_counter_detect(10);

    SpoorObject *spoor_object = &spoor_objects[ui_week_spoor_object_index_current];
    spoor_time_minutes_add(&spoor_object->schedule.end, counter);
    spoor_storage_change(spoor_object, spoor_object);
}

void ui_week_schedule_shrink(void)
{
    uint32_t counter = buffer_command_counter_detect(10);

    SpoorObject *spoor_object = &spoor_objects[ui_week_spoor_object_index_current];
    spoor_time_minutes_add(&spoor_object->schedule.end, ~(int32_t)counter + 1);
    spoor_storage_change(spoor_object, spoor_object);
}

void spoor_ui_xlib_show(void)
{
    window_create();

    unsigned long valuemask = 0;
    XGCValues values;
    gc = XCreateGC(display, window, valuemask, &values);

    XSelectInput(display, window, KeyPressMask | KeyReleaseMask | ExposureMask);

    ui_week_today_time_update();

    spoor_objects_count = spoor_object_storage_load(NULL);
    spoor_sort_objects_by_deadline();
    ui_week_spoor_object_index_current = ui_week_schedule_first();

    /* swap buffers */
    int major_version_return, minor_version_return;
    if(XdbeQueryExtension(display, &major_version_return, &minor_version_return)) {
        printf("XDBE version %d.%d\n", major_version_return, minor_version_return);
    } else {
        fprintf(stderr, "XDBE is not supported!!!1\n");
        exit(1);
    }

    back_buffer = XdbeAllocateBackBufferName(display, window, 0);

    char buf[256 + 1] = { 0 };
    int bufsz = 128;
    XEvent event;
    int quit = 0;
    bool control_down = false;


    while (!quit)
    {
        XNextEvent(display, &event);
        switch (event.type)
        {
            case KeyPress:
            {
                int key_code;
                KeySym key_sym;
                XLookupString(&event.xkey, buf, bufsz, &key_sym, NULL);
                if (strlen (buf) != 0 && buf[0] != '\n' && buf[0] != '\r')
                {
                    key_code = XKeysymToKeycode(display, key_sym);
                }
                else
                {
                    key_code = XKeysymToKeycode(display, key_sym);
                }

                char *key_sym_string = XKeysymToString(key_sym);
                if (strcmp(key_sym_string, "q") == 0)
                    quit = 1;
                if (strcmp(key_sym_string, "Control_L") == 0)
                    control_down = true;

                if (control_down)
                {
                    switch (key_sym_string[0])
                    {
                        case 's':
                        {
                            ui_week_today_offset--;
                        } break;

                        case 't':
                        {
                            ui_week_today_offset++;
                        } break;

                        case 'n':
                        {
                            ui_week_time_start++;
                        } break;
                        case 'r':
                        {
                            if (!(ui_week_time_start == 0))
                                ui_week_time_start--;
                        }
                    }
                }
                else
                {
                    if (buf[0] > 0x7f || buf[0] == 0)
                        break;
                    buffer_command[buffer_command_count++] = buf[0];
                    buffer_command[buffer_command_count] = 0;

                    if (mode_command)
                    {
                        if (strcmp(key_sym_string, "BackSpace") == 0)
                        {
                            printf("test backspace\n");
                            buffer_command_count -= 2;
                            buffer_command[buffer_command_count] = 0;
                        }
                        if (strcmp(key_sym_string, "Return") == 0)
                        {
                            buffer_command[buffer_command_count - 1] = 0;
                            if (strncmp(buffer_command + 1, "q", 1) == 0)
                                quit = true;

                            if (buffer_command[1] == 'c')
                            {
                                SpoorObject *spoor_object = spoor_object_create(buffer_command + 2);
                                spoor_storage_save(spoor_object);

                                spoor_sort_objects_append(spoor_object);

                                free(spoor_object);
                            }
                            else if (buffer_command[1] == 'e')
                            {
                                SpoorObject spoor_object = spoor_objects[ui_week_spoor_object_index_current];
                                SpoorObject spoor_object_old = spoor_object;
                                spoor_object_edit(&spoor_object, buffer_command + 2);

                                spoor_storage_change(&spoor_object_old, &spoor_object);
                                spoor_sort_objects_remove(ui_week_spoor_object_index_current);
                                spoor_sort_objects_append(&spoor_object);
                            }

                            printf("escape -----\n");
                            mode_command = false;
                            buffer_command[0] = 0;
                            buffer_command_count = 0;
                        }
                    }
                    else
                    {
                        bool command_buffer_clear = true;
                        switch (buffer_command[buffer_command_count - 1])
                        {
                            case ':':
                            {
                                mode_command = true;
                                command_buffer_clear = false;
                                buffer_command[0] = ':';
                                buffer_command[1] = 0;
                                buffer_command_count = 1;
                            } break;
                            default:
                            {
                                command_buffer_clear = false;
                            } break;
                        }
                        if (ui_week_spoor_object_index_grap)
                        {
                            switch (buffer_command[buffer_command_count - 1])
                            {
                                case 'n':
                                {
                                    ui_week_schedule_move_down();
                                } break;
                                case 'r':
                                {
                                    ui_week_schedule_move_up();
                                } break;
                                case 's':
                                {
                                    ui_week_schedule_move_left();
                                } break;
                                case 't':
                                {
                                    ui_week_schedule_move_right();
                                } break;
                                case 'v':
                                {
                                    ui_week_spoor_object_index_grap = false;
                                } break;
                                case 'R':
                                {
                                    ui_week_schedule_shrink();
                                } break;
                                case 'N':
                                {
                                    ui_week_schedule_grow();
                                } break;
                                default:
                                {
                                    command_buffer_clear = false;
                                } break;
                            }
                        }
                        else
                        {
                            if (strncmp(buffer_command, "dd", 2) == 0)
                            {
                                spoor_storage_delete(&spoor_objects[ui_week_spoor_object_index_current]);
                                spoor_sort_objects_remove(ui_week_spoor_object_index_current);
                                buffer_command[0] = 0;
                                buffer_command_count = 0;
                                break;
                            }

                            switch (buffer_command[buffer_command_count - 1])
                            {
                                case 'n':
                                {
                                    ui_week_schedule_next();
                                } break;
                                case 'r':
                                {
                                    ui_week_schedule_prev();
                                } break;
                                case 's':
                                {
                                    ui_week_schedule_index_left();
                                } break;
                                case 't':
                                {
                                    ui_week_schedule_index_right();
                                } break;
                                case 'v':
                                {
                                    ui_week_spoor_object_index_grap = true;
                                } break;
                                default:
                                {
                                    command_buffer_clear = false;
                                } break;
                            }
                        }
                        if (command_buffer_clear)
                        {
                            buffer_command[0] = 0;
                            buffer_command_count = 0;
                        }
                    }

                }

                printf("key down\n");
                printf("keycode %d\n", key_code);
                printf("string  %s\n", key_sym_string);
                printf("buffer %s\n", buf);
            } break;

            case KeyRelease:
            {
                printf("key release\n");
                KeySym key_sym = XLookupKeysym(&event.xkey, 0);
                XLookupString(&event.xkey, buf, bufsz, &key_sym, NULL);
                int key_code = XKeysymToKeycode(display, key_sym);

                char *test = XKeysymToString(key_sym);
                if (strcmp(test, "Control_L") == 0)
                    control_down = false;

                printf("key release\n");
                printf("keycode %d\n", key_code);
                printf("string  %s\n", test);
            } break;
        }
        /* clear */
        XSetForeground(display, gc, BACKGROUND_COLOR);
        XFillRectangle(display, back_buffer, gc, 0, 0, window_width, window_height);

        draw();

        /* swap buffers */
        XdbeSwapInfo swap_info;
        swap_info.swap_window = window;
        swap_info.swap_action = 0;
        XdbeSwapBuffers(display, &swap_info, 1);

        usleep(1000 * 1000 / FPS);
    }
}
