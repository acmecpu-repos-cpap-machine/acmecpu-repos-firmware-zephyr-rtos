/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 08-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <lvgl.h>

#include "c20x_screen_config.h"

static lv_style_t m_lvstyle_text_squeezed;
static lv_style_t m_lvstyle_text_squeezed_high;

void c20x_screen_cmn_styles_set()
{
#if (CONFIG_C20X_SCREENS_OLED)
	lv_style_set_text_letter_space(&m_lvstyle_text_squeezed, C20X_SCREEN_STYLE_TEXT_LETTER_SPACE_SQUEEZED);
	lv_style_set_text_letter_space(&m_lvstyle_text_squeezed_high, C20X_SCREEN_STYLE_TEXT_LETTER_SPACE_SQUEEZED_HIGH);
#endif
}


lv_style_t* c20x_screen_cmn_style_textsqueezed_obj_get()
{
	return &m_lvstyle_text_squeezed;
}

lv_style_t* c20x_screen_cmn_style_textSqueezedHigh_obj_get()
{
	return &m_lvstyle_text_squeezed_high;
}

void c20x_screen_cmn_styles_init()
{
	lv_style_init(&m_lvstyle_text_squeezed);
	lv_style_init(&m_lvstyle_text_squeezed_high);

	c20x_screen_cmn_styles_set();
}
