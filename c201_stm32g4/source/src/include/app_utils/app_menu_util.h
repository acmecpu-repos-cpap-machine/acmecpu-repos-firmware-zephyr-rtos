/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 20-Nov-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_UTILS_APP_MENU_UTIL_H_
#define SRC_INCLUDE_APP_UTILS_APP_MENU_UTIL_H_

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "app_settings/app_settings_paths.h"

struct menusort_data {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	/* Data */
	char prev_path[SETTINGS_FULLPATH_LEN_MAX];
	char pkg_key[SETTINGS_FULLPATH_LEN_MAX];
	int data_idx;
	int disp_order;
};

/**
 * @brief	Add a object to slist
 * @param list		The list to add to
 * @param obj		The object
 * @return
 */
static inline int app_menu_sortlist_add_data(sys_slist_t *list, struct menusort_data *obj)
{
	sys_slist_append(list, &obj->node);
	return 0;
}

/**
 * @brief	Remove a slist and container objects of type struct menusort_data
 * @param list
 */
static inline void app_menu_sortlist_remove(sys_slist_t *list)
{
	struct menusort_data *mdata, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, mdata, tmp, node)
	{
		if (mdata) {
			sys_slist_remove(list, NULL, &mdata->node);
			free(mdata);
		}
	}
}

/**
 * @brief		Sort a list of struct menusort_data objects
 * @param list	The slist to sort
 * @param order	Sort order (0 ascending, 1 descending)
 */
void app_menu_sortlist_sort(sys_slist_t *list, int order);

#endif /* SRC_INCLUDE_APP_UTILS_APP_MENU_UTIL_H_ */
