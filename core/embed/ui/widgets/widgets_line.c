#include "widgets_line.h"

static lv_point_precise_t g_points[2] = {0};


lv_obj_t *CreateLine(lv_obj_t *parent, uint16_t length)
{
    lv_obj_t *line;
    g_points[1].x = length - 1;

    line = lv_line_create(parent);
    lv_obj_remove_style_all(line);
    lv_line_set_points(line, g_points, 2);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_set_style_line_width(line, 3, 0);
    lv_obj_set_style_line_color(line, lv_color_black(), 0);
    lv_obj_set_style_line_opa(line, LV_OPA_COVER, 0);

    return line;
}
