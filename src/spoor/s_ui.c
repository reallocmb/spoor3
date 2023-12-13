#include"spoor_internal.h"

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<unistd.h>
#if defined(_WIN32)
#include<windows.h>
#endif
#ifdef __unix__
#include<termios.h>
#include<sys/ioctl.h>
#endif

const char UI_TYPES[][17] = {
    "TASK",
    "PROJECT",
    "EVENT",
    "APPOINTMENT",
    "GOAL",
    "HABIT",
};

const char UI_STATUS[][50] = {
    "\x1B[31mNOT STARTED\x1B[m",
    "\x1B[33mIN PROGRESS\x1B[m",
    "\x1B[32mCOMPLETED\x1B[m",
};

void screen_clear(void)
{
    fprintf(stdout, "\033[2J");
}

void cursor_show(void)
{
    fprintf(stdout, "\033[?25h");
}

void cursor_hide(void)
{
    fprintf(stdout, "\033[?25l");
}

void cursor_move(int x, int y)
{
    fprintf(stdout, "\033[%d;%dH", y, x);
}

void time_format_parse_deadline(SpoorTimeSpan *spoor_time, char *time_format)
{
    if (spoor_time->end.year == -1)
    {
        sprintf(time_format, "--.--.---- --:--");
        return;
    }

    char time_format_start[6];

    if (spoor_time->end.hour == -1 || spoor_time->end.min == -1)
        sprintf(time_format_start, "--:--");
    else
        sprintf(time_format_start, "%s%d:%s%d",
                (spoor_time->end.hour < 10) ?"0" :"", spoor_time->end.hour,
                (spoor_time->end.min < 10) ?"0" : "", spoor_time->end.min);

    sprintf(time_format, "%s%d.%s%d.%d %s",
            (spoor_time->end.day < 10) ?"0" :"",
            spoor_time->end.day,
            (spoor_time->end.mon + 1 < 10) ?"0" :"",
            spoor_time->end.mon + 1,
            spoor_time->end.year + 1900,
            time_format_start);
}

void title_format_parse(char *title, char *title_format)
{
    if (strlen(title) >= 30)
    {
        strncpy(title_format, title, 25);
        strcpy(title_format + 25, "...");
    }
    else
        strcpy(title_format, title);
}

void time_format_parse_schedule(SpoorTimeSpan *spoor_time, char *time_format)
{
    if (spoor_time->end.year == -1)
    {
        sprintf(time_format, "--.--.---- --:-- --:--");
        return;
    }

    char time_format_start[6] = { 0 };
    char time_format_end[6] = { 0 };

    if (spoor_time->start.hour == -1 || spoor_time->start.min == -1)
        sprintf(time_format_start, "--:--");
    else
        sprintf(time_format_start, "%s%d:%s%d",
                (spoor_time->start.hour < 10) ?"0" :"", spoor_time->start.hour,
                (spoor_time->start.min < 10) ?"0" : "", spoor_time->start.min);

    if (spoor_time->end.hour == -1 || spoor_time->end.min == -1)
        sprintf(time_format_end, "--:--");
    else
        sprintf(time_format_end, "%s%d:%s%d",
                (spoor_time->end.hour < 10) ?"0" :"", spoor_time->end.hour,
                (spoor_time->end.min < 10) ?"0" : "", spoor_time->end.min);

    if (spoor_time->start.year == -1)
    {
        sprintf(time_format, "%s%d.%s%d.%d %s %s",
                (spoor_time->end.day < 10) ?"0" :"",
                spoor_time->end.day,
                (spoor_time->end.mon + 1 < 10) ?"0" :"",
                spoor_time->end.mon + 1,
                spoor_time->end.year + 1900,
                time_format_end,
                time_format_end);
    }
    else
    {
        sprintf(time_format, "%s%d.%s%d.%d %s %s",
                (spoor_time->start.day < 10) ?"0" :"",
                spoor_time->start.day,
                (spoor_time->start.mon + 1 < 10) ?"0" :"",
                spoor_time->start.mon + 1,
                spoor_time->start.year + 1900,
                time_format_start,
                time_format_end);
    }
}

void ui_window_rows_get(uint32_t *window_rows)
{
#ifdef __unix__
    static struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *window_rows = w.ws_row;
#elif defined(_WIN32)
    HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(h_console, &csbi);

    *window_rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#endif
}

void spoor_ui_object_show(void)
{
    int32_t index_current = 0;
#if 0
    SpoorFilter spoor_filter;
    SpoorObject spoor_objects[500];
    uint32_t spoor_objects_count = 0;
    spoor_objects_count = spoor_object_storage_load(spoor_objects, &spoor_filter);

    uint32_t i;
    for (i = 0; i < spoor_objects_count; i++)
    {
        spoor_debug_spoor_object_print(&spoor_objects[i]);
    }

#else
#ifndef _WIN32
    struct termios old;
    struct termios new;

    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new);
#endif

    SpoorFilter spoor_filter;
    spoor_filter.spoor_time.start.year = 123;
    spoor_filter.spoor_time.start.mon = 9;
    spoor_filter.spoor_time.start.day = 22;
    spoor_filter.spoor_time.start.hour = -1;
    spoor_filter.spoor_time.start.min = -1;

    spoor_filter.spoor_time.end = spoor_filter.spoor_time.start;
    spoor_filter.spoor_time.end.hour = -1;
    spoor_filter.spoor_time.end.min = -1;
    spoor_objects_count = spoor_object_storage_load(&spoor_filter);
#if 0
    SpoorTimeSpan spoor_span;
    spoor_span.start.year = 123;
    spoor_span.start.mon = 9;
    spoor_span.start.day = 20;

    spoor_span.end = spoor_span.start;
    spoor_objects_count = spoor_object_storage_load_filter_time_span(spoor_objects, &spoor_span);
#endif

    uint32_t window_rows = 0;
    ui_window_rows_get(&window_rows);


    uint32_t offset = 0;

    cursor_hide();
    bool command_mode = false;
    char arguments[200] = { 0 };
    uint8_t arguments_pos = 0;
    while (1)
    {
        /* sorting */
        spoor_sort_objects_by_deadline();

        /* window size update */
        ui_window_rows_get(&window_rows);

        /* print title bar */
        cursor_move(0, 0);
        screen_clear();
        fprintf(stdout, "%-5s%-30s%-18s%-24s%-13s%-13s%-12s", "I", "TITLE", "DEADLINE", "SCHEDULE", "STATUS", "TYPE", "PROJECT");
        cursor_move(0, 2);
        fprintf(stdout, "--------------------------------------------------------------------------------------------------------------");

        /* print spoor_objects */
        char title_format[30];
        char time_format_deadline[50] = { 0 };
        char time_format_schedule[50] = { 0 };

        uint32_t i;
        for (i = 0; i < spoor_objects_count && i < window_rows - 5; i++)
        {
            title_format_parse(spoor_objects[i + offset].title, title_format);
            time_format_parse_deadline(&spoor_objects[i + offset].deadline, time_format_deadline);
            time_format_parse_schedule(&spoor_objects[i + offset].schedule, time_format_schedule);
            cursor_move(0, 3 + i);
            if (i == (uint32_t)index_current)
                printf("\e[1;47m");
            fprintf(stdout,
                    "%-5d%-30s%-18s%-24s%-21s%-13s%s",
                    i,
                    title_format,
                    time_format_deadline,
                    time_format_schedule,
                    UI_STATUS[spoor_objects[i + offset].status],
                    UI_TYPES[spoor_objects[i + offset].type],
                    spoor_objects[i + offset].parent_title);
            printf("\e[m");
        }

        /* print status bar */
        cursor_move(0, window_rows - 2);
        fprintf(stdout, "---------------------------------------------------------------------------------------------------------------");
        cursor_move(0, window_rows -1);
        fprintf(stdout, "%s", arguments);
        cursor_move(0, window_rows);
        fprintf(stdout, "-- Elements: [%d - %d](%d)", offset, offset + window_rows - 5, spoor_objects_count);

        /* key input */
        if (command_mode)
        {
            uint32_t c = getchar();
            if (c == 0x7f)
            {
                arguments[--arguments_pos] = 0;
            }
            else if (c == '\n')
            {
                if (strncmp(arguments + 1, "q", 1) == 0)
                {
                    cursor_move(0, 0);
                    screen_clear();

#if 0
                    spoor_storage_clean_up();
#endif
                    break;
                }
                else if (strncmp(arguments + 1, "c", 1) == 0)
                {
                    SpoorObject *spoor_object = spoor_object_create(arguments + 2);
                    spoor_storage_save(spoor_object);

                    free(spoor_object);
                    spoor_objects_count = spoor_object_storage_load(&spoor_filter);
                }
                else if (arguments[1] == 'l')
                {
                    screen_clear();
                    cursor_move(0, 0);
                    spoor_debug_links();
                    getchar();
                }
                else if (arguments[1] == 'h')
                {
                    screen_clear();
                    cursor_move(0, 0);
                    printf("--- HELP PAGE ---\n");
                    printf("commands:\n"
                           ":c[title],[deadline] [schedule] [status || type]\t\tcreate object\n"
                           "link: l[parent_index]\n"
                           "type: t = TASK\n"
                           "type: e = EVENT\n"
                           "type: a = APPOINTMENT\n"
                           "type: g = GOAL\n"
                           "type: h = HABIT\n"
                           "status: c = COMPLETED\n"
                           "status: ip = IN PROGRESS\n"
                           "status: ns = NOT STARTED\n"
                           ":[index]e [title],[deadline] [schedule] [status || type]\t\tedit object by index");
                    getchar();
                }
                else
                {
                    /* edit spoor object */

                    /* index */
                    uint32_t index = 0;
                    uint32_t p = 0;
                    if (!(arguments[1] >= 0x30 && arguments[1] <= 0x39))
                        index = index_current;
                    else
                    {
                        while (arguments[1 + p] >= 0x30 && arguments[1 + p] <= 0x39)
                        {
                            index *= 10;
                            index += arguments[1 + p] - 0x30;
                            p++;
                        }
                    }
                    SpoorObject *spoor_object = &spoor_objects[index + offset];
                    if (arguments[p + 1] == 'e')
                    {
                        SpoorObject spoor_object_old = *spoor_object;
                        spoor_object_edit(spoor_object, arguments + p + 2);
                        spoor_storage_change(&spoor_object_old, spoor_object);

                        spoor_objects_count = spoor_object_storage_load(&spoor_filter);
                    }
                    else if (arguments[p + 1] == 'd')
                    {
                        spoor_storage_delete(spoor_object);
                        spoor_objects_count = spoor_object_storage_load(&spoor_filter);
                    }
                    else
                    {
                        screen_clear();
                        cursor_move(0, 0);
                        spoor_debug_spoor_object_print(spoor_object);
                        getchar();
                    }
                }
                memset(arguments, 0, 200);
                command_mode = false;
                arguments_pos = 0;
            }
            else
                arguments[arguments_pos++] = c;
        }
        else
        {
            uint32_t c = getchar();
            if (c == ':')
            {
                command_mode = true;
                arguments[0] = ':';
                arguments_pos++;
            }
            if (c == 'c')
            {
                command_mode = true;
                arguments[0] = ':';
                arguments[1] = 'c';
                arguments_pos += 2;
            }
            if (c == 'e')
            {
                command_mode = true;
                arguments[0] = ':';
                arguments[1] = 'e';
                arguments_pos += 2;
            }
            if (c == 'd')
            {
                command_mode = true;
                arguments[0] = ':';
                arguments[1] = 'e';
                arguments_pos += 2;
            }
            if (c == 'n')
                index_current++;
            if (c == 'r')
                index_current--;
            if (index_current < 0)
                index_current = 0;
            if (c == 'f')
                offset += (window_rows - 4) / 2;
            if (c == 'b')
            {
                if (((int64_t)offset - (window_rows - 4) / 2) >= 0)
                    offset -= (window_rows - 4) / 2;
            }

            if (c == 'q')
            {
                cursor_move(0, 0);
                screen_clear();

#if 0
                spoor_storage_clean_up();
#endif

                break;
            }
        }
    }
    cursor_show();

#if defined(__unix__)
    printf("\e[m");
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
#endif
#endif
}
