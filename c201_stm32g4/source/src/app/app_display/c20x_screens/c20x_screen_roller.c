/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 2-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <lvgl.h>

static lv_style_t m_rol_style_def;
static lv_style_t m_rol_style_foc;
static lv_style_t m_rol_style_sel;

void c20x_screen_roller_styles_set(lv_obj_t *roller)
{
#if (CONFIG_C20X_SCREENS_OLED)
	/* default */
	lv_style_set_border_opa(&m_rol_style_def, LV_OPA_COVER);
//	lv_style_set_radius(&m_rol_style_def, 0);
    lv_style_set_border_width(&m_rol_style_def, 1);
    lv_style_set_border_color(&m_rol_style_def, lv_color_white());
    lv_style_set_text_line_space(&m_rol_style_def, 5);

    /* focused */
    lv_style_set_border_width(&m_rol_style_foc, 2);

    /* selected*/
    lv_style_set_text_color(&m_rol_style_sel, lv_color_black());

    lv_obj_add_style(roller, &m_rol_style_def, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(roller, &m_rol_style_foc, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_style(roller, &m_rol_style_sel, LV_PART_SELECTED);
#endif
}

/* IMPORTANT: this function should be called only once */
void c20x_screen_roller_styles_init()
{
	lv_style_init(&m_rol_style_def);
	lv_style_init(&m_rol_style_foc);
	lv_style_init(&m_rol_style_sel);
}
