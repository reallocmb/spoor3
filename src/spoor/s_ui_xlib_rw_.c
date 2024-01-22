#include"spoor_internal.h"

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdbool.h>
#include<string.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<X11/extensions/Xdbe.h>
#include<unistd.h>

#define FPS 60

#define XLIB_BACKGROUND_COLOR 0xd3b083
#define XLIB_WINDOW_WIDTH 800
#define XLIB_WINDOW_HEIGHT 600

/* xlib globals */
struct XlibHandle {
    Display *display;
    Window window;
    GC gc;
    bool running;
    XdbeBackBuffer back_buffer;
    XEvent event;
    char key_input_buffer[256 + 1];
    KeySym key_sym;
    char *key_sym_str;
    uint32_t key_code;
    uint32_t window_width;
    uint32_t window_height;
    bool key_control_down;
} XlibHandleGlobal = {
    .running = true,
    .window_width = XLIB_WINDOW_WIDTH,
    .window_height = XLIB_WINDOW_HEIGHT,
    .key_control_down = false,
};

struct UICalendar {
    uint8_t days_count;
    int32_t today_offset;
    uint32_t hour_offset;
} UICalendarGlobal = {
    .days_count = 7,
    .today_offset = 0,
    .hour_offset = 7,
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


void xlib_window_create(void)
{
    XlibHandleGlobal.display = XOpenDisplay(NULL);
    if (XlibHandleGlobal.display == NULL)
    {
        fprintf(stderr,
                "failed to connect to X-Server\n");
        exit(EXIT_FAILURE);
    }

    XlibHandleGlobal.window = XCreateSimpleWindow(XlibHandleGlobal.display,
                                                  XDefaultRootWindow(XlibHandleGlobal.display),
                                                  0, 0,
                                                  XLIB_WINDOW_WIDTH, XLIB_WINDOW_HEIGHT,
                                                  0,
                                                  0,
                                                  XLIB_BACKGROUND_COLOR);

    XStoreName(XlibHandleGlobal.display, XlibHandleGlobal.window, SPOOR_APPLICATION_NAME);
    XMapWindow(XlibHandleGlobal.display, XlibHandleGlobal.window);

    XSync(XlibHandleGlobal.display, False);

    XlibHandleGlobal.gc = XCreateGC(XlibHandleGlobal.display,
                                    XlibHandleGlobal.window,
                                    0,
                                    NULL);

    XSelectInput(XlibHandleGlobal.display,
                 XlibHandleGlobal.window,
                 KeyPressMask | KeyReleaseMask
                 | StructureNotifyMask
                 | ExposureMask
                 /*
                 | ResizeRedirectMask
                 */
                 );

    /* back buffer init */
    XlibHandleGlobal.back_buffer = XdbeAllocateBackBufferName(XlibHandleGlobal.display,
                                                              XlibHandleGlobal.window,
                                                              0);
}

void input_special_keys(void)
{
    if (strncmp(XlibHandleGlobal.key_sym_str, "Control_L", 9) == 0)
    {
        if (XlibHandleGlobal.key_control_down)
            XlibHandleGlobal.key_control_down = false;
        else
            XlibHandleGlobal.key_control_down = true;
    }
    printf("key_control_down %s\n", (XlibHandleGlobal.key_control_down) ?"TRUE" :"FALSE");
}

void ui_calendar_input(void)
{
    printf("key_input_buffer[0]: %c\n", XlibHandleGlobal.key_input_buffer[0]);
    if (XlibHandleGlobal.key_control_down)
    {
        switch (XlibHandleGlobal.key_sym_str[0])
        {
            case 's': UICalendarGlobal.today_offset--; break;
            case 'n': UICalendarGlobal.hour_offset++; break;
            case 'r': UICalendarGlobal.hour_offset--; break;
            case 't': UICalendarGlobal.today_offset++; break;
        }
    }
}

void xlib_render(void);

void xlib_events(void)
{
    while (XPending(XlibHandleGlobal.display) > 0)
    {
        XNextEvent(XlibHandleGlobal.display, &XlibHandleGlobal.event);
        switch (XlibHandleGlobal.event.type)
        {
            case Expose:
            {
                printf("expose\n");
                xlib_render();
            } break;
            case ConfigureNotify:
            {
                XlibHandleGlobal.window_width = XlibHandleGlobal.event.xconfigure.width;
                XlibHandleGlobal.window_height = XlibHandleGlobal.event.xconfigure.height;
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
                if (XlibHandleGlobal.key_input_buffer[0] == 0)
                    input_special_keys();
            } break;
            default:
            {
                printf("event: %d\n", XlibHandleGlobal.event.type);
            } break;
        }
    }
}

void xlib_ui_calendar_draw(void)
{
    /* set draw color to black */
    XSetForeground(XlibHandleGlobal.display,
                   XlibHandleGlobal.gc,
                   UI_CALENDAR_FONT_COLOR);

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

        /* draw week-day text */
        XDrawString(XlibHandleGlobal.display,
                    XlibHandleGlobal.back_buffer,
                    XlibHandleGlobal.gc,
                    x + 6, y + 14,
                    ui_calendar_week_day_names[time_today_tm->tm_wday],
                    strlen(ui_calendar_week_day_names[time_today_tm->tm_wday]));

        /* draw date text */
        sprintf(today_date_format,
                "%d.%d.%d",
                time_today_tm->tm_mday,
                time_today_tm->tm_mon + 1,
                time_today_tm->tm_year + 1900);
        XDrawString(XlibHandleGlobal.display,
                    XlibHandleGlobal.back_buffer,
                    XlibHandleGlobal.gc,
                    x + 6, y + 2 * 14,
                    today_date_format,
                    strlen(today_date_format));

        /* separation line between days */
        XDrawLine(XlibHandleGlobal.display,
                  XlibHandleGlobal.back_buffer,
                  XlibHandleGlobal.gc,
                  x + width, y,
                  x + width, y + height);

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
                XDrawString(XlibHandleGlobal.display,
                            XlibHandleGlobal.back_buffer,
                            XlibHandleGlobal.gc,
                            x + 4, y + k + 14,
                            hour_format,
                            strlen(hour_format));
                XDrawLine(XlibHandleGlobal.display,
                          XlibHandleGlobal.back_buffer,
                          XlibHandleGlobal.gc,
                          x, y + k,
                          x + width, y + k);
            }
            else if (k % 30 == 0)
            {
                XDrawLine(XlibHandleGlobal.display,
                          XlibHandleGlobal.back_buffer,
                          XlibHandleGlobal.gc,
                          x + 40, y + k,
                          x + width, y + k);
            }
            else
            {
                XSetForeground(XlibHandleGlobal.display,
                               XlibHandleGlobal.gc,
                               0x643c64);
                XDrawLine(XlibHandleGlobal.display,
                          XlibHandleGlobal.back_buffer,
                          XlibHandleGlobal.gc,
                          x + 40, y + k,
                          x + width - 1, y + k);
                XSetForeground(XlibHandleGlobal.display,
                               XlibHandleGlobal.gc,
                               0x000000);
            }
        }
    }
}

void xlib_render(void)
{
    /* clear window */
    XSetForeground(XlibHandleGlobal.display,
                   XlibHandleGlobal.gc,
                   XLIB_BACKGROUND_COLOR);
    XFillRectangle(XlibHandleGlobal.display,
                   XlibHandleGlobal.back_buffer,
                   XlibHandleGlobal.gc,
                   0, 0,
                   XlibHandleGlobal.window_width,
                   XlibHandleGlobal.window_height);

#if 0
    xlib_ui_calendar_draw();
#endif

    /* swap buffers */
    XdbeSwapInfo swap_info;
    swap_info.swap_window = XlibHandleGlobal.window;
    swap_info.swap_action = 0;
    XdbeSwapBuffers(XlibHandleGlobal.display, &swap_info, 1);

    usleep(1000 * 1000 / FPS);
}

void spoor_ui_xlib_show_rw_(void)
{
    xlib_window_create();

    xlib_render();
    while (XlibHandleGlobal.running)
    {
        xlib_events();
    }
}
