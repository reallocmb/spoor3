#include"spoor_internal.h"

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<raylib.h>

#define MODE_INSERT_TOGGLE 'i'
#define MODE_NORMAL_TOGGLE 0x1b

enum {
    MODE_NORMAL,
    MODE_INSERT
};

char mode_str[2][7] = {
    "NORMAL",
    "INSERT",
};

uint32_t mode = MODE_NORMAL;

Font font_liberation;

typedef struct {
    uint32_t top;
    uint32_t right;
    uint32_t bottom;
    uint32_t left;
} UIInsets;

typedef struct {
    uint32_t x;
    uint32_t y;
} UIVector2;

#define CHILDS_ALLOC_SIZE 50

#define UI_STATUS_BAR_HEIGHT 20

/* UI Area Flgas */
#define UI_AREA_FLAG_PARENT 0b1
#define UI_AREA_FLAG_CHILD 0b10
#define UI_AREA_FLAG_LAYOUT_VERTICAL 0b100
#define UI_AREA_FLAG_LAYOUT_HORIZONTAL 0b1000
#define UI_AREA_FLAG_CURRENT 0b10000

typedef struct UIArea {
    UIInsets margin;
    UIVector2 position;
    UIVector2 size;
    uint8_t flags;
    void (*draw_func)(struct UIArea *ui_area);
    struct UIArea *childs;
    uint32_t childs_count;
    uint32_t child_index;
    struct UIArea *parent;
} UIArea;

time_t week_start_time;

void _padding(uint32_t padding)
{
    while (padding--)
        putchar(' ');
}

void ui_area_debug_print(UIArea *ui_area_head)
{
    static uint32_t padding = 4;
    _padding(padding - 4);
    puts("{");
    _padding(padding);
    printf("Child Index %d\n",
           (ui_area_head->child_index == 0xffffffff) ?0xffffffff :ui_area_head->child_index);
    _padding(padding);
    printf("Position: %d %d\n",
           ui_area_head->position.x,
           ui_area_head->position.y);
    _padding(padding);
    printf("Size: %d %d\n",
           ui_area_head->size.x,
           ui_area_head->size.y);
    _padding(padding);
    printf("Size: %d %d\n",
           ui_area_head->size.x,
           ui_area_head->size.y);
    _padding(padding);
    printf("Flags:\n");
    _padding(padding);
    printf("%s",
           (ui_area_head->flags & UI_AREA_FLAG_PARENT) ?"P, " :"");
    printf("%s",
           (ui_area_head->flags & UI_AREA_FLAG_CHILD) ?"C, " :"");
    printf("%s",
           (ui_area_head->flags & UI_AREA_FLAG_LAYOUT_VERTICAL) ?"LV, " :"");
    printf("%s",
           (ui_area_head->flags & UI_AREA_FLAG_LAYOUT_HORIZONTAL) ?"LH, " :"");
    printf("%s\n",
           (ui_area_head->flags & UI_AREA_FLAG_CURRENT) ?"Curr, " :"");
    _padding(padding);
    printf("Draw Func: %p\n",
           ui_area_head->draw_func);

    uint32_t i;
    for (i = 0; i < ui_area_head->childs_count; i++)
    {
        padding += 4;
        ui_area_debug_print(&ui_area_head->childs[i]);
        padding -= 4;
    }

    _padding(padding - 4);
    puts("}");
}

UIArea *ui_area_create(void)
{
    UIArea *ui_area = malloc(sizeof(*ui_area));
    ui_area->margin = (UIInsets){ 0, 0, 0, 0};
    ui_area->position = (UIVector2){ 0, 0 };
    ui_area->size = (UIVector2){ 0, 0};
    ui_area->flags = 0;
    ui_area->draw_func = NULL;
    ui_area->childs = NULL;
    ui_area->child_index = 0xffffffff;
    ui_area->childs_count = 0;
    ui_area->parent = NULL;

    return ui_area;
}

void test_ui_area_draw_func(UIArea *ui_area)
{
    Color border_color = BLACK;
    if (ui_area->flags & UI_AREA_FLAG_CURRENT)
        border_color = VIOLET;

    uint32_t margin = 5;
    DrawRectangle(ui_area->position.x + margin,
                  ui_area->position.y + margin,
                  ui_area->size.x - 2 * margin,
                  ui_area->size.y - 2 * margin,
                  border_color);
}

char ui_day_names[7][10] = {
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
    "Sunday",
};

uint32_t ui_day_names_count = 7;

#if 1
uint32_t time_offset_y = 7;
void ui_page_days_draw(UIArea *ui_area)
{
    uint32_t i;
    for (i = 0; i < ui_day_names_count; i++)
    {
        uint32_t height = ui_area->size.y;
        uint32_t width = ui_area->size.x / ui_day_names_count;
        uint32_t x = ui_area->position.x + i * width;
        uint32_t y = ui_area->position.y;

        DrawLine(x + width,
                 y,
                 x + width,
                 y + height,
                 BLACK);

        DrawTextEx(font_liberation,
                   ui_day_names[i],
                   (Vector2){ x + 5,
                   y + 5 },
                   (float)font_liberation.baseSize,
                   2,
                   BLACK);
        time_t time_in_sec = week_start_time + 60 * 60 * 24 * i;
        SpoorTime spoor_time = spoor_time_today_get(&time_in_sec);
        char date[20];
        sprintf(date, "%d.%d.%d", spoor_time.day, spoor_time.mon + 1, spoor_time.year + 1900);
        DrawTextEx(font_liberation,
                   date,
                   (Vector2){ x + 5,
                   y + 25 },
                   (float)font_liberation.baseSize,
                   2,
                   BLACK);

        y += 60;
        height -= 60;

        char time[20];
        uint32_t k;
        for (k = 0; k < height; k += 5)
        {
            if (k % 60 == 0)
            {
                uint32_t hours = k / 60 + time_offset_y;
                sprintf(time,
                        "%s%u:00",
                        (hours < 10) ?"0" :"",
                        hours);

                DrawLine(x,
                         y + k,
                         x + width,
                         y + k,
                         BLACK);
                DrawTextEx(font_liberation,
                           time,
                           (Vector2){ x + 5 + 2, y + k + 2 },
                           (float)font_liberation.baseSize,
                           2,
                           BLACK);
            }
            else if (k % 30 == 0)
            {
                DrawLine(x + 40,
                         y + k,
                         x + width,
                         y + k,
                         BLACK);
            }
            else
                DrawLine(x + 40,
                         y + k,
                         x + width,
                         y + k,
                         LIGHTGRAY);
        }

        int i;
        for (i = 0; i < spoor_objects_count; i++)
        {
            if (spoor_objects[i].schedule.start.year != -1) 
            {
                if (spoor_time_compare_day(&spoor_objects[i].schedule.start, &spoor_time) == 0)
                {
                    int minute_start = spoor_objects[i].schedule.start.hour * 60 + spoor_objects[i].schedule.start.min + 60 - time_offset_y * 60;
                    int minute_end = spoor_objects[i].schedule.end.hour * 60 + spoor_objects[i].schedule.end.min + 60 - time_offset_y * 60;
                    if (minute_start >= 60)
                    {
                        DrawRectangle(x + 45, minute_start, width - 50, minute_end - minute_start, DARKPURPLE);
                        DrawText(spoor_objects[i].title, x + 50, minute_start, 3, BLACK);
                    }
                }
            }
        }
    }
}
#endif

void main_page_draw_func(UIArea *ui_area)
{
    Color border_color = BLACK;
    if (ui_area->flags & UI_AREA_FLAG_CURRENT)
        border_color = VIOLET;

    uint32_t margin = 2;
    DrawRectangleLines(ui_area->position.x + margin,
                       ui_area->position.y + margin,
                       ui_area->size.x - 2 * margin,
                       ui_area->size.y - 2 * margin,
                       border_color);
    DrawRectangleLines(ui_area->position.x + margin - 1,
                       ui_area->position.y + margin - 1,
                       ui_area->size.x - 2 * margin + 2,
                       ui_area->size.y - 2 * margin + 2,
                       border_color);

#if 1
    ui_page_days_draw(ui_area);
#endif
}


UIArea *ui_area_child_append(UIArea *ui_area_head,
                             uint8_t flags)
{
    UIArea *ui_area_child = ui_area_create();
    ui_area_child->flags = flags;
    ui_area_child->draw_func = test_ui_area_draw_func;
    ui_area_child->draw_func = main_page_draw_func;

    if (ui_area_head->flags & UI_AREA_FLAG_PARENT)
    {
        ui_area_child->parent = ui_area_head;

        if (ui_area_head->childs_count % CHILDS_ALLOC_SIZE == 0)
            ui_area_head->childs = realloc(ui_area_head->childs,
                                           (ui_area_head->childs_count + CHILDS_ALLOC_SIZE) * sizeof(*ui_area_head->childs));

        ui_area_head->childs[ui_area_head->childs_count] = *ui_area_child;
        ui_area_head->childs[ui_area_head->childs_count].child_index = ui_area_head->childs_count;
        ui_area_head->flags |= 0b1100 & flags; 
        free(ui_area_child);

        return &ui_area_head->childs[ui_area_head->childs_count++];
    }
    else
    {
        if ((ui_area_child->flags & 0b1100) & (ui_area_head->flags & 0b1100))
        {
            ui_area_child->parent = ui_area_head->parent;
            UIArea *parent = ui_area_head->parent;

            if (parent->childs_count % CHILDS_ALLOC_SIZE == 0)
                parent->childs = realloc(parent->childs,
                                         (parent->childs_count + CHILDS_ALLOC_SIZE) * sizeof(*parent->childs));

            parent->childs[parent->childs_count] = *ui_area_child;
            parent->childs[parent->childs_count].child_index = parent->childs_count;
            free(ui_area_child);

            return &parent->childs[parent->childs_count++];
        }

        UIArea backup = *ui_area_head;
        *ui_area_head = *ui_area_create();
        ui_area_head->child_index = backup.child_index;


        ui_area_head->flags = 0b1100 & backup.flags;
        ui_area_head->flags |= UI_AREA_FLAG_CURRENT;

        backup.flags ^= 0b1100;
        backup.flags ^= UI_AREA_FLAG_CURRENT;


        /* parent */
        ui_area_head->parent = backup.parent;
        backup.parent = ui_area_head;
        ui_area_child->parent = ui_area_head;

        if (ui_area_head->childs_count % CHILDS_ALLOC_SIZE == 0)
            ui_area_head->childs = realloc(ui_area_head->childs,
                                           (ui_area_head->childs_count + CHILDS_ALLOC_SIZE) * sizeof(*ui_area_head->childs));

        ui_area_head->childs[ui_area_head->childs_count] = backup;
        ui_area_head->childs[ui_area_head->childs_count].child_index = ui_area_head->childs_count;
        ui_area_head->childs_count++;


        if (ui_area_head->childs_count % CHILDS_ALLOC_SIZE == 0)
            ui_area_head->childs = realloc(ui_area_head->childs,
                                           (ui_area_head->childs_count + CHILDS_ALLOC_SIZE) * sizeof(*ui_area_head->childs));

        ui_area_head->childs[ui_area_head->childs_count] = *ui_area_child;
        ui_area_head->childs[ui_area_head->childs_count].child_index = ui_area_head->childs_count;
        ui_area_head->flags |= UI_AREA_FLAG_PARENT; 
        free(ui_area_child);

        return &ui_area_head->childs[ui_area_head->childs_count++];
    }

    return NULL;
}
void ui_area_current_update(UIArea **ui_area_current, UIArea *ui_area_child)
{
    ui_area_child->flags |= UI_AREA_FLAG_CURRENT;
    (*ui_area_current)->flags ^= UI_AREA_FLAG_CURRENT;
    *ui_area_current = ui_area_child;
}

void ui_area_draw(UIArea *ui_area)
{
    if (ui_area->draw_func != NULL)
        ui_area->draw_func(ui_area);

    uint32_t i;
    for (i = 0; i < ui_area->childs_count; i++)
        ui_area_draw(&ui_area->childs[i]);
}

void ui_area_child_resize_update(UIArea *ui_area_head)
{
    uint32_t i;
    for (i = 0; i < ui_area_head->childs_count; i++)
    {
        ui_area_head->childs[i].position = ui_area_head->position;
        ui_area_head->childs[i].size = ui_area_head->size;

        if (ui_area_head->childs[i].flags &
            UI_AREA_FLAG_LAYOUT_VERTICAL)
        {
            ui_area_head->childs[i].size.y = ui_area_head->size.y / ui_area_head->childs_count;
            ui_area_head->childs[i].position.y += ui_area_head->size.y / ui_area_head->childs_count * i;
        }

        if (ui_area_head->childs[i].flags & UI_AREA_FLAG_LAYOUT_HORIZONTAL)
        {
            ui_area_head->childs[i].size.x = ui_area_head->size.x / ui_area_head->childs_count;
            ui_area_head->childs[i].position.x += ui_area_head->size.x / ui_area_head->childs_count * i;
        }

        ui_area_child_resize_update(&ui_area_head->childs[i]);
    }
}

void ui_area_resize_update(UIArea *ui_area_head)
{
    ui_area_head->size = (UIVector2){ GetScreenWidth(), GetScreenHeight() - UI_STATUS_BAR_HEIGHT };
    uint32_t i;
    for (i = 0; i < ui_area_head->childs_count; i++)
    {
        ui_area_head->childs[i].position = ui_area_head->position;
        ui_area_head->childs[i].size = ui_area_head->size;

        if (ui_area_head->childs[i].flags & UI_AREA_FLAG_LAYOUT_VERTICAL)
        {
            ui_area_head->childs[i].size.y = ui_area_head->size.y / ui_area_head->childs_count;
            ui_area_head->childs[i].position.y = ui_area_head->size.y / ui_area_head->childs_count * i;
        }

        if (ui_area_head->childs[i].flags & UI_AREA_FLAG_LAYOUT_HORIZONTAL)
        {
            ui_area_head->childs[i].size.x = ui_area_head->size.x / ui_area_head->childs_count;
            ui_area_head->childs[i].position.x = ui_area_head->size.x / ui_area_head->childs_count * i;
        }

        ui_area_child_resize_update(&ui_area_head->childs[i]);
    }
}

void ui_status_bar_draw(void)
{
    Color border_color = GRAY;

    DrawRectangle(0,
                  GetScreenHeight() - UI_STATUS_BAR_HEIGHT,
                  GetScreenWidth(),
                  UI_STATUS_BAR_HEIGHT,
                  border_color);

    DrawText(mode_str[mode],
             10,
             GetScreenHeight() - UI_STATUS_BAR_HEIGHT + UI_STATUS_BAR_HEIGHT / 4,
             UI_STATUS_BAR_HEIGHT / 4, BLACK);
}

void ui_status_bar_draw_command_mode(char *buffer_command)
{
    Color border_color = GRAY;

    DrawRectangle(0,
                  GetScreenHeight() - UI_STATUS_BAR_HEIGHT,
                  GetScreenWidth(),
                  UI_STATUS_BAR_HEIGHT,
                  border_color);

    DrawText(buffer_command,
             10,
             GetScreenHeight() - UI_STATUS_BAR_HEIGHT + UI_STATUS_BAR_HEIGHT / 4,
             UI_STATUS_BAR_HEIGHT / 4, BLACK);
}

void ui_area_close(UIArea *ui_area_current)
{
    UIArea *parent = ui_area_current->parent;
    uint32_t i;
    for (i = ui_area_current->child_index + 1; i < parent->childs_count; i++)
        parent->childs[i - 1] = parent->childs[i];

    parent->childs_count--;
}

void ui_area_current_move_left(UIArea **ui_area_current)
{
    uint32_t flags = (*ui_area_current)->flags;

    if (flags & UI_AREA_FLAG_LAYOUT_HORIZONTAL &&
        (*ui_area_current)->child_index != 0)
    {
        UIArea *current_new = &(*ui_area_current)->parent->childs[(*ui_area_current)->child_index - 1];
        if (!current_new)
            return;

        while (!(current_new->flags & UI_AREA_FLAG_CHILD))
            current_new = &current_new->childs[current_new->childs_count - 1];

        ui_area_current_update(ui_area_current,
                               current_new);
    }
    else
    {
        UIArea *parent = (*ui_area_current)->parent;
        if (!parent || parent->child_index == 0xffffffff)
            return;

        while (!(parent->flags & UI_AREA_FLAG_LAYOUT_HORIZONTAL))
        {
            if (!parent || parent->child_index == 0xffffffff)
                return;
            parent = parent->parent;
        }

        if (parent->child_index == 0)
            return;
        parent = &parent->parent->childs[parent->child_index - 1];

        while (!(parent->flags & UI_AREA_FLAG_CHILD))
            parent = &parent->childs[parent->childs_count - 1];

        ui_area_current_update(ui_area_current,
                               parent);
    }
}

void ui_area_current_move_right(UIArea **ui_area_current)
{
    uint32_t flags = (*ui_area_current)->flags;

    if (flags & UI_AREA_FLAG_LAYOUT_HORIZONTAL &&
        (*ui_area_current)->child_index != (*ui_area_current)->parent->childs_count - 1)
    {
        UIArea *current_new = &(*ui_area_current)->parent->childs[(*ui_area_current)->child_index + 1];
        if (!current_new)
            return;

        while (!(current_new->flags & UI_AREA_FLAG_CHILD))
            current_new = &current_new->childs[0];

        ui_area_current_update(ui_area_current,
                               current_new);
    }
    else
    {
        UIArea *parent = (*ui_area_current)->parent;
        if (!parent || parent->child_index == 0xffffffff)
            return;

        while (!(parent->flags & UI_AREA_FLAG_LAYOUT_HORIZONTAL))
        {
            if (!parent || parent->child_index == 0xffffffff)
                return;
            parent = parent->parent;
        }

        if (parent->child_index + 1 >= parent->parent->childs_count)
            return;
        parent = &parent->parent->childs[parent->child_index + 1];

        while (!(parent->flags & UI_AREA_FLAG_CHILD))
            parent = &parent->childs[0];

        ui_area_current_update(ui_area_current,
                               parent);
    }
}

void ui_area_current_move_up(UIArea **ui_area_current)
{
    uint32_t flags = (*ui_area_current)->flags;

    if (flags & UI_AREA_FLAG_LAYOUT_VERTICAL &&
        (*ui_area_current)->child_index != 0)
    {
        UIArea *current_new = &(*ui_area_current)->parent->childs[(*ui_area_current)->child_index - 1];
        if (!current_new)
            return;

        while (!(current_new->flags & UI_AREA_FLAG_CHILD))
            current_new = &current_new->childs[current_new->childs_count - 1];

        ui_area_current_update(ui_area_current,
                               current_new);
    }
    else
    {
        UIArea *parent = (*ui_area_current)->parent;
        if (!parent || parent->child_index == 0xffffffff)
            return;

        while (!(parent->flags & UI_AREA_FLAG_LAYOUT_VERTICAL))
        {
            if (!parent || parent->child_index == 0xffffffff)
                return;
            parent = parent->parent;
        }

        if (parent->child_index == 0)
            return;
        parent = &parent->parent->childs[parent->child_index - 1];

        while (!(parent->flags & UI_AREA_FLAG_CHILD))
            parent = &parent->childs[parent->childs_count - 1];

        ui_area_current_update(ui_area_current,
                               parent);
    }
}

void ui_area_current_move_down(UIArea **ui_area_current)
{
    uint32_t flags = (*ui_area_current)->flags;

    if (flags & UI_AREA_FLAG_LAYOUT_VERTICAL &&
        (*ui_area_current)->child_index != (*ui_area_current)->parent->childs_count - 1)
    {
        UIArea *current_new = &(*ui_area_current)->parent->childs[(*ui_area_current)->child_index + 1];
        if (!current_new)
            return;

        while (!(current_new->flags & UI_AREA_FLAG_CHILD))
            current_new = &current_new->childs[0];

        ui_area_current_update(ui_area_current,
                               current_new);
    }
    else
    {
        UIArea *parent = (*ui_area_current)->parent;
        if (!parent || parent->child_index == 0xffffffff)
            return;

        while (!(parent->flags & UI_AREA_FLAG_LAYOUT_VERTICAL))
        {
            if (!parent || parent->child_index == 0xffffffff)
                return;
            parent = parent->parent;
        }

        if (parent->child_index + 1 >= parent->parent->childs_count)
            return;
        parent = &parent->parent->childs[parent->child_index + 1];

        while (!(parent->flags & UI_AREA_FLAG_CHILD))
            parent = &parent->childs[0];

        ui_area_current_update(ui_area_current,
                               parent);
    }
}

void spoor_ui_raylib_object_show(void)
{
    int screenWidth = 800;
    int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "SPOOR BY REALLOCMB");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    SetTargetFPS(60);

    UIArea *ui_area_head = ui_area_create();
    ui_area_head->flags = UI_AREA_FLAG_PARENT | UI_AREA_FLAG_CURRENT;
    UIArea *ui_area_current = ui_area_head;

    /* temp */
    UIArea *ui_area_child = ui_area_child_append(ui_area_current,
                                                 UI_AREA_FLAG_LAYOUT_VERTICAL |
                                                 UI_AREA_FLAG_CHILD);
    ui_area_current_update(&ui_area_current,
                           ui_area_child);
    ui_area_resize_update(ui_area_head);

    /* load font */
    font_liberation = LoadFontEx("LiberationMono-Regular.ttf",
                                 32,
                                 0,
                                 250);

    spoor_objects_count = spoor_object_storage_load(NULL);

    /* week start date */
    time_t current_time;
    week_start_time = time(NULL);

    /* command */
    char buffer_command[200] = { '#' };
    uint16_t buffer_command_length = 0;
    bool command_mode = false;

    _Bool leader = 0;
    while (!WindowShouldClose() || IsKeyPressed(KEY_ESCAPE))
    {
        BeginDrawing();
        {
            ClearBackground(BEIGE);

            if (IsWindowResized())
                ui_area_resize_update(ui_area_head);

            ui_area_draw(ui_area_head);
            if (command_mode)
                ui_status_bar_draw_command_mode(buffer_command);
            else
            {
                ui_status_bar_draw();
                DrawText(buffer_command,
                         150,
                         GetScreenHeight() - UI_STATUS_BAR_HEIGHT + UI_STATUS_BAR_HEIGHT / 4,
                         UI_STATUS_BAR_HEIGHT / 4, BLACK);
            }
        }
        EndDrawing();

        uint32_t input = GetCharPressed();
        if (input == 0)
        {
            if (IsKeyPressed(KEY_ESCAPE))
                mode = MODE_NORMAL;
            if (IsKeyPressed(KEY_BACKSPACE))
            {
                printf("back\n");
                buffer_command_length -= 2;
                buffer_command[buffer_command_length] = 0;
            }

            continue;
        }
        else
            printf("input: %c\n", (char)input);
        if (mode == MODE_NORMAL)
        {
            if (input == MODE_INSERT_TOGGLE)
                mode = MODE_INSERT;
        }
        else /* INSERT mode */
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
                case 'n':
                {
                    time_offset_y++;
                } break;

                case 'r':
                {
                    time_offset_y--;
                } break;

                case 'R':
                {
                    spoor_objects_count = spoor_object_storage_load(NULL);
                } break;
            }
        }

#if 0
        if (leader)
        {
            switch (c)
            {
                case 'i':
                {
                    UIArea *ui_area_child = ui_area_child_append(ui_area_current,
                                                                 UI_AREA_FLAG_LAYOUT_VERTICAL |
                                                                 UI_AREA_FLAG_CHILD);
                    ui_area_current_update(&ui_area_current,
                                           ui_area_child);
                    ui_area_resize_update(ui_area_head);
                    leader = 0;
                } break;

                case 'a':
                {
                    UIArea *ui_area_child = ui_area_child_append(ui_area_current,
                                                                 UI_AREA_FLAG_LAYOUT_HORIZONTAL |
                                                                 UI_AREA_FLAG_CHILD);
                    ui_area_current_update(&ui_area_current,
                                           ui_area_child);
                    ui_area_resize_update(ui_area_head);
                    leader = 0;
                } break;
                case 'c':
                {
                    ui_area_close(ui_area_current);
                    ui_area_resize_update(ui_area_head);
                } break;
                case 's':
                {
                    ui_area_current_move_left(&ui_area_current);
                } break;
                case 'n':
                {
                    ui_area_current_move_down(&ui_area_current);
                } break;
                case 'r':
                {
                    ui_area_current_move_up(&ui_area_current);
                } break;
                case 't':
                {
                    ui_area_current_move_right(&ui_area_current);
                } break;
            }
        }

        switch (c)
        {
            case 'n':
                ui_day_names_count--;
                break;
            case 'r':
                ui_day_names_count++;
                break;
            case 'd':
                printf("\n\n\t\tUIArea Debug Start\n---\n");
                ui_area_debug_print(ui_area_head);
                printf("---\n\t\tUIArea Debug End\n\n");
                break;
            case ' ':
                leader = 1;
                break;
        }
#endif
    }

    CloseWindow();
}
