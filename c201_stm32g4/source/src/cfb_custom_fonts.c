/*
 * Copyright (c) 2021 Acme CPU
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This is a wrapper to Adafruit's glcdfont.c file.
 * It provides a mean of representing the mentioned font in a Zephyr project
 * Licensed under the Apache License, Version 2.0
 */

#include <zephyr.h>
#include <display/cfb.h>

#define CFB_FONTS_FIRST_CHAR	0
#define CFB_FONTS_LAST_CHAR		255

extern char *adafruit_glcd_font_0507[];

FONT_ENTRY_DEFINE(font0507,
		  	  	  5,
				  8,
				  CFB_FONT_MONO_VPACKED,
				  adafruit_glcd_font_0507,
				  CFB_FONTS_FIRST_CHAR,
				  CFB_FONTS_LAST_CHAR
				);
