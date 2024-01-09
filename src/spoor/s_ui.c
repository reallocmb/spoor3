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

uint32_t buffer_command_count_get(char *buffer_command, uint32_t buffer_command_length)
{
    uint32_t i;
    uint32_t offset = 0;

    for (i = 0; i < buffer_command_length; i++)
    {
        offset *= 10;
        offset += buffer_command[i] - 0x30;
    }

    return offset;
}

void index_current_check(int32_t *index_current, uint32_t spoor_objects_count)
{
    if (*index_current < 0)
        *index_current = 0;
    if (*index_current >= (int32_t)spoor_objects_count)
        *index_current = spoor_objects_count - 1;
}

void spoor_ui_object_show(void)
{
    int32_t index_current = 0;
#ifdef __unix__
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

    uint32_t window_rows = 0;
    ui_window_rows_get(&window_rows);

    uint32_t offset = 0;
    uint32_t modus_num = 1;

    cursor_hide();

    char buffer_command[200];
    uint8_t buffer_command_length = 0;

    bool command_mode = false;
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
        uint32_t _i;
        for (i = 0; i < spoor_objects_count && i < window_rows - 5; i++)
        {
            title_format_parse(spoor_objects[i + offset].title, title_format);
            time_format_parse_deadline(&spoor_objects[i + offset].deadline, time_format_deadline);
            time_format_parse_schedule(&spoor_objects[i + offset].schedule, time_format_schedule);
            cursor_move(0, 3 + i);

            /* relativ numbers and shit */
            if ((int32_t)i == index_current)
                printf("\e[2;30;46m");
            if (modus_num == 1)
            {
                if ((int32_t)i == index_current)
                    _i = i;
                else if ((int32_t)i > index_current)
                    _i = i - index_current;
                else
                    _i = index_current - i;
            }
            else
                _i = i;

            fprintf(stdout,
                    "%-5d%-30s%-18s%-24s%-21s%-13s%s",
                    _i,
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
        cursor_move(0, window_rows);
        fprintf(stdout,
                "-- Elements: [%d - %d](%d) %.*s",
                offset, offset + window_rows - 5,
                spoor_objects_count,
                buffer_command_length, 
                buffer_command);

        /* key input */
        uint32_t input = getchar();

        buffer_command[buffer_command_length++] = input;
        buffer_command[buffer_command_length] = 0;
        if (command_mode)
        {
            if (input == 0x7f)
            {
                buffer_command_length -= 2;
                buffer_command[buffer_command_length] = 0;
            }
            if (input == '\n')
            {
                buffer_command[buffer_command_length - 1] = 0;
                if (strncmp(buffer_command + 1, "q", 1) == 0)
                {
                    cursor_move(0, 0);
                    screen_clear();

#if 0
                    spoor_storage_clean_up();
#endif
                    break;
                }
                else if (strncmp(buffer_command + 1, "c", 1) == 0)
                {
                    SpoorObject *spoor_object = spoor_object_create(buffer_command + 2);
                    spoor_storage_save(spoor_object);

                    free(spoor_object);
                    spoor_objects_count = spoor_object_storage_load(&spoor_filter);
                }
                else if (buffer_command[1] == 'l')
                {
                    screen_clear();
                    cursor_move(0, 0);
                    spoor_debug_links();
                    getchar();
                }
                else if (buffer_command[1] == 'h')
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
                    if (!(buffer_command[1] >= 0x30 && buffer_command[1] <= 0x39))
                        index = index_current;
                    else
                    {
                        while (buffer_command[1 + p] >= 0x30 && buffer_command[1 + p] <= 0x39)
                        {
                            index *= 10;
                            index += buffer_command[1 + p] - 0x30;
                            p++;
                        }
                    }
                    SpoorObject *spoor_object = &spoor_objects[index + offset];
                    if (buffer_command[p + 1] == 'e')
                    {
                        SpoorObject spoor_object_old = *spoor_object;
                        spoor_object_edit(spoor_object, buffer_command + p + 2);

                        spoor_storage_change(&spoor_object_old, spoor_object);
                        spoor_objects_count = spoor_object_storage_load(&spoor_filter);
                    }
                    else if (buffer_command[p + 1] == 'd')
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

                buffer_command_length = 0;
                buffer_command[0] = 0;
                command_mode = false;
            }
        }
        else
        {
            if (input == '\n')
            {
                command_mode = true;
                buffer_command[0] = ':';
                buffer_command[1] = 0;
                buffer_command_length = 1;
            }
            if (input == 'q')
            {
                cursor_move(0, 0);
                screen_clear();

                break;
            }
            switch (input)
            {
                case ':':
                {
                    command_mode = true;
                    buffer_command[0] = ':';
                    buffer_command[1] = 0;
                    buffer_command_length = 1;
                    break;
                }
                case 'c':
                {
                    command_mode = true;
                    buffer_command[0] = ':';
                    buffer_command[1] = 'c';
                    buffer_command[2] = 0;
                    buffer_command_length = 2;
                    break;
                }
                case 'e':
                {
                    command_mode = true;
                    buffer_command[0] = ':';
                    buffer_command[1] = 'e';
                    buffer_command[2] = 0;
                    buffer_command_length = 2;
                    break;
                }
                case 'd':
                {
                    command_mode = true;
                    buffer_command[0] = ':';
                    buffer_command[1] = 'd';
                    buffer_command[2] = 0;
                    buffer_command_length = 2;
                    break;
                }
                case 'l':
                {
                    command_mode = true;
                    buffer_command[0] = ':';
                    buffer_command[1] = 'l';
                    buffer_command[2] = 0;
                    buffer_command_length = 2;
                    break;
                }
                case 'n':
                {
                    uint32_t offset = buffer_command_count_get(buffer_command, buffer_command_length - 1);
                    if (offset)
                        index_current += offset;
                    else
                        index_current++;
                    buffer_command_length = 0;
                    buffer_command[0] = 0;
                    index_current_check(&index_current, spoor_objects_count);
                    break;
                }
                case 'r':
                {
                    uint32_t offset = buffer_command_count_get(buffer_command, buffer_command_length - 1);
                    if (offset)
                        index_current -= offset;
                    else
                        index_current--;
                    buffer_command_length = 0;
                    buffer_command[0] = 0;
                    index_current_check(&index_current, spoor_objects_count);
                    break;
                }
            }
        }
    }
    cursor_show();

#if defined(__unix__)
    printf("\e[m");
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
#endif
}
