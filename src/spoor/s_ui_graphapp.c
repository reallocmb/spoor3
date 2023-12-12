#if 0
#include"spoor_internal.h"

#include<stdio.h>
#include<string.h>
#include<graphapp.h>

SpoorObject spoor_objects[500];
uint32_t spoor_objects_count = 0;

int offset = 0;
void day_draw(Control *c, Graphics *g, char *name, uint32_t width, uint32_t day_index)
{
    uint32_t x = day_index * width;
    int padding_y = 60;
    set_colour(g, BLACK);
    app_draw_text(g, rect(5 + x, 5, width, 20), 1, name, strlen(name));
    app_draw_text(g, rect(5 + x, 25, width, 20), 1, "Datum", 5);

    set_colour(g, LIGHT_GRAY);

    char time[6];
    int i;
    for (i = padding_y; i < c->area.height; i += 5)
    {
        if ((i - padding_y + offset) % 60 == 0)
        {
            int hours = (i - padding_y + offset) / 60;
            sprintf(time, "%s%d:00", (hours < 10) ?"0" :"", hours);

            set_colour(g, BLACK);
            app_draw_text(g, rect(2 + x, i + 2, 50, 20), 1, time, 5);
            draw_line(g, pt(0 + x, i), pt(width + x, i));
            set_colour(g, LIGHT_GRAY);
        }
        else if ((i - padding_y + offset) % 30 == 0)
        {
            set_colour(g, BLACK);
            draw_line(g, pt(50 + x, i), pt(width + x, i));
            set_colour(g, LIGHT_GRAY);
        }
        else
            draw_line(g, pt(50 + x, i), pt(width + x, i));
    }
}

char ui_day_names_g[7][10] = {
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
    "Sunday",
};

uint32_t ui_day_names_count_g = 7;

void ui_day_names_draw(Control *c, Graphics *g)
{
    int day_width = c->area.width / ui_day_names_count_g;
    int i;
    for (i = 0; i < (int)ui_day_names_count_g; i++)
        day_draw(c, g, ui_day_names_g[i], day_width, i);
}

void window_redraw_func(Window *w, Graphics *g)
{
    /* status line */
    set_colour(g, GRAY);
    fill_rect(g, rect(0, w->area.height - 20, w->area.width, 20));
    set_colour(g, WHITE);
    draw_line(g, pt(0, w->area.height - 20), pt(w->area.width, w->area.height - 20));
    app_draw_text(g, rect(5, w->area.height - 20 + 5, 500, 20), 1, "Status", 6);
}

void week_control_redraw_func(Control *c, Graphics *g)
{
    set_colour(g, GRAY);
    app_size_control(c, rect(0, 0, c->win->area.width - 40, c->win->area.height * 0.6));
    app_move_control(c, rect(20, 20, 0, 0));
    app_draw_rect(g, rect(0, 0, c->area.width, c->area.height));

    ui_day_names_draw(c, g);
    int day_width = c->area.width / ui_day_names_count_g;

    int i;
    for (i = 0; i < (int)spoor_objects_count; i++)
    {
        int start_minute = spoor_objects[i].schedule.start.tm_hour * 60 + spoor_objects[i].schedule.start.tm_min - offset + 60;
        int end_minute = spoor_objects[i].schedule.end.tm_hour * 60 + spoor_objects[i].schedule.end.tm_min - offset + 60;
        set_colour(g, MAGENTA);
        fill_rect(g, rect(55, start_minute, day_width - 55 - 5, end_minute - start_minute));
        set_colour(g, WHITE);
        app_draw_text(g, rect(60, start_minute + 5, day_width - 55 - 5, 20), 1, spoor_objects[i].title, strlen(spoor_objects[i].title));
    }
}

void spoor_objects_list_control_redraw_func(Control *c, Graphics *g)
{
    app_size_control(c, rect(0, 0, c->win->area.width - 40, c->win->area.height * 0.4 - 40 - 20));
    app_move_control(c, rect(20, c->win->area.height * 0.6 + 20 + 10, 0, 0)); 
    set_colour(g, GRAY);
    app_draw_rect(g, rect(0, 0, c->area.width, c->area.height));

    char title_format[30];
    char time_format_deadline[50] = { 0 };
    char time_format_schedule[50] = { 0 };

    app_draw_text(g, rect(5, 5, 200, 20), 1, "TITLE", 5);
    app_draw_text(g, rect(5 + 40 * 5, 5, 200, 20), 1, "DEADLINE", 8);
    app_draw_text(g, rect(5 + 80 * 5, 5, 200, 20), 1, "SCHEDULE", 8);
    draw_line(g, pt(5, 5 + 15), pt(600, 5 + 15));
    draw_line(g, pt(5, 5 + 15 + 1), pt(600, 5 + 15 + 1));


    int i;
    for (i = 0; i < (int)spoor_objects_count; i++)
    {
        title_format_parse(spoor_objects[i].title, title_format);
        time_format_parse_deadline(&spoor_objects[i + offset].deadline, time_format_deadline);
        time_format_parse_schedule(&spoor_objects[i + offset].schedule, time_format_schedule);

        app_draw_text(g, rect(5, 25 + i * 20, 500, 20), 1, title_format, strlen(title_format));
        app_draw_text(g, rect(5 + 40 * 5, 25 + i * 20, 200, 20), 1, time_format_deadline, strlen(time_format_deadline));
        app_draw_text(g, rect(5 + 80 * 5, 25 + i * 20, 200, 20), 1, time_format_schedule, strlen(time_format_schedule));
        draw_line(g, pt(5, 25 + i * 20 + 15), pt(600, 25 + i * 20 + 15));
    }
}


void scroll(Control *c, unsigned long key)
{
    if (key == 'b')
    {
        ui_day_names_count_g--;
        app_redraw_control(c);
    }

    if (key == 'm')
    {
        ui_day_names_count_g++;
        app_redraw_control(c);
    }

    if (key == 'n')
    {
        offset += 60;
        app_redraw_control(c);
    }

    if (key == 'r')
    {
        offset -= 60;
        app_redraw_control(c);
    }

    printf("%ld\n", key);
    printf("%c\n", (char)key);
    printf("Test");
}

void spoor_ui_graphapp_object_show(void)
{
    App *app = app_new_app(0, NULL);
    Window *window = app_new_window(app, rect(0, 0, 1000, 1000), "SPOOR", STANDARD_WINDOW);
    app_on_window_redraw(window, window_redraw_func);
    Control *c = app_new_control(window, rect(20, 20, 0, 0));
    app_on_control_redraw(c, week_control_redraw_func);
    app_on_control_key_down(c, scroll);
    app_set_focus(c);

    c = app_new_control(window, rect(0, 0, 0, 0));
    app_on_control_redraw(c, spoor_objects_list_control_redraw_func);

    spoor_objects_count = spoor_object_storage_load(spoor_objects, NULL);

    app_show_window(window);

    app_main_loop(app);
}
#endif
