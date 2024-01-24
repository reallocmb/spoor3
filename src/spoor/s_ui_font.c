#include"spoor_internal.h"

#include<ft2build.h>
#include FT_FREETYPE_H

struct UIFont UIFontGlobal;

void ui_font_size_set(uint32_t font_size)
{
    UIFontGlobal.font_size = font_size;
    FT_Error err;
    err = FT_Set_Pixel_Sizes(UIFontGlobal.face, 0, font_size);
    if (err != 0) {
        printf("Failed to set pixel size\n");
        return;
    }
}

void spoor_ui_font_init(const char *path, uint32_t font_size)
{
    FT_Error err = FT_Init_FreeType(&UIFontGlobal.ft);
    if (err != 0) {
        printf("Failed to initialize FreeType\n");
        return;
    }

    FT_Library_Version(UIFontGlobal.ft,
                       &UIFontGlobal.major,
                       &UIFontGlobal.minor,
                       &UIFontGlobal.patch);

    err = FT_New_Face(UIFontGlobal.ft, path, 0, &UIFontGlobal.face);
    if (err != 0) {
        printf("Failed to load face\n");
        return;
    }
    ui_font_size_set(font_size);
}
