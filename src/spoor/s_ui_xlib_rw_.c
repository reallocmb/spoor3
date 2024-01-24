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
    if (strncmp(XlibHandleGlobal.key_sym_str, "Control_L", 9) == 0)
    {
        if (XlibHandleGlobal.key_control_down)
            XlibHandleGlobal.key_control_down = false;
        else
            XlibHandleGlobal.key_control_down = true;
    }
    printf("key_control_down %s\n", (XlibHandleGlobal.key_control_down) ?"TRUE" :"FALSE");
}

void xlib_render(void);
void ui_calendar_input(void)
{
    printf("key_input_buffer[0]: %c\n", XlibHandleGlobal.key_input_buffer[0]);
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
}

void xlib_line_horizontal_draw(uint32_t px, uint32_t py, uint32_t width, uint32_t color)
{
    uint32_t width_max = px + width;
    if (width_max > XlibHandleGlobal.window_width)
        width_max = XlibHandleGlobal.window_width;

    uint32_t x;
    for (x = px; x < width_max; x++)
        XlibHandleGlobal.bits[py * XlibHandleGlobal.window_width + x] = color;
}

void xlib_line_vertical_draw(uint32_t px, uint32_t py, uint32_t height, uint32_t color)
{
    uint32_t height_max = py + height;
    if (height_max > XlibHandleGlobal.window_height)
        height_max = XlibHandleGlobal.window_height;

    uint32_t y;
    for (y = py; y < height_max; y++)
        XlibHandleGlobal.bits[y * XlibHandleGlobal.window_width + px] = color;
}

void xlib_rectangle_draw(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color)
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
            XlibHandleGlobal.bits[i * XlibHandleGlobal.window_width + j] = color;
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

    y += UIFontGlobal.font_size;

    uint32_t k;
    for (k = 0; k < buffer_size; k++)
    {
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

        if (y + bitmap.rows > XlibHandleGlobal.window_height)
            return;

        for (size_t i = 0; i < bitmap.rows; i++) {
            for (size_t j = 0; j < bitmap.width; j++) {
                int alpha =
                    bitmap.buffer[i * bitmap.pitch + j];
                alpha = 255 - alpha;

                uint32_t position = (y + i - UIFontGlobal.face->glyph->bitmap_top) * XlibHandleGlobal.window_width + (x + j);
                uint32_t old_color = XlibHandleGlobal.bits[(y + i) * XlibHandleGlobal.window_width + (x + j)]; 
                XlibHandleGlobal.bits[position] = alphaBlend(color, old_color, alpha);
            }
        }
        x += bitmap.pitch;
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

        if (i + UICalendarGlobal.today_offset == 0)
            xlib_rectangle_draw(x + 2, y + 2, width - 4, 56, 0x5454bb);

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
#if 0
        printf("spoor_objects_count: %d\n", spoor_objects_count);
        uint32_t j;
        uint32_t fs = 10;
        ui_font_size_set(fs);
        for (j = 0; j < spoor_objects_count; j++)
        {
            if (spoor_objects[j].schedule.start.year != -1)
            {
                if (spoor_time_compare_day(&spoor_objects[j].schedule.start, (SpoorTime *)time_today_tm))
                {
                    int minute_start = spoor_objects[j].schedule.start.hour * 60 + spoor_objects[j].schedule.start.min + 60 - UICalendarGlobal.hour_offset * 60;
                    int minute_end = spoor_objects[j].schedule.end.hour * 60 + spoor_objects[j].schedule.end.min + 60 - UICalendarGlobal.hour_offset * 60;
                    if (minute_start >= 60)
                    {
                        /*
                        uint32_t color = schedule_item_color_get(spoor_objects[j].type);
                        */
                        uint32_t color = 0x48abdd;

                        xlib_rectangle_draw(x + 45, minute_start, width - 50, minute_end - minute_start, color);
                        xlib_text_draw(spoor_objects[j].title, x + 50, minute_start + 2, 0x000000);

                        char time_format_deadline[50] = { 0 };
                        time_format_parse_deadline(&spoor_objects[j].deadline, time_format_deadline);
                        xlib_text_draw(time_format_deadline, x + 50, minute_start + 2 + fs + 2, 0x000000);

                        /*
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
                        */
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
                        0x828282);
}

void xlib_render(void)
{
    /* clear */
    uint32_t i;
    for (i = 0; i < XlibHandleGlobal.window_width * XlibHandleGlobal.window_height; i++)
        XlibHandleGlobal.bits[i] = XLIB_BACKGROUND_COLOR;

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
                printf("expose\n");
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
                printf("key press\n");
                XLookupString(&XlibHandleGlobal.event.xkey,
                              XlibHandleGlobal.key_input_buffer,
                              128,
                              &XlibHandleGlobal.key_sym,
                              NULL);
                XlibHandleGlobal.key_sym_str = XKeysymToString(XlibHandleGlobal.key_sym);
                printf("key_sym_str: %s\n", XlibHandleGlobal.key_sym_str);
                printf("key_input_buff: %s\n", XlibHandleGlobal.key_input_buffer);
                if (XlibHandleGlobal.key_input_buffer[0] == 0)
                    input_special_keys();
                else
                    ui_calendar_input();
            } break;
            case KeyRelease:
            {
                printf("key release\n");
                XLookupString(&XlibHandleGlobal.event.xkey,
                              XlibHandleGlobal.key_input_buffer,
                              128,
                              &XlibHandleGlobal.key_sym,
                              NULL);
                XlibHandleGlobal.key_sym_str = XKeysymToString(XlibHandleGlobal.key_sym);
                printf("key_sym_str: %s\n", XlibHandleGlobal.key_sym_str);
                printf("key_input_buff: %s\n", XlibHandleGlobal.key_input_buffer);
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


void spoor_ui_xlib_show_rw_(void)
{
    spoor_objects_count = spoor_object_storage_load(NULL);

    xlib_window_create();

    while (XlibHandleGlobal.running)
    {
        xlib_events();
    }
}
