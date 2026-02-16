/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 20-Nov-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "app_utils/app_menu_util.h"

/*
static inline int app_menu_sortlist_check_duplicates(sys_slist_t *list, const char *name)
{
	struct menusort_data *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node)
	{
		if (!strcmp(cb->pkg_name, name)) {
			return -1;
		}
	}
	return 0;
}
*/

static void swap_data(struct menusort_data *sld1, struct menusort_data *sld2)
{
	struct menusort_data tmp;

	tmp.data_idx = sld2->data_idx;
	tmp.disp_order = sld2->disp_order;
	strcpy(tmp.pkg_key, sld2->pkg_key);
	strcpy(tmp.prev_path, sld2->prev_path);

	sld2->data_idx = sld1->data_idx;
	sld2->disp_order = sld1->disp_order;
	strcpy(sld2->pkg_key, sld1->pkg_key);
	strcpy(sld2->prev_path, sld1->prev_path);

	sld1->data_idx = tmp.data_idx;
	sld1->disp_order = tmp.disp_order;
	strcpy(sld1->pkg_key, tmp.pkg_key);
	strcpy(sld1->prev_path, tmp.prev_path);
}

void app_menu_sortlist_sort(sys_slist_t *list, int order)
{
	int swapped;
	sys_snode_t *ptr1;
	sys_snode_t *lptr = NULL;
	struct menusort_data *sld1, *sld2;

	/* Checking for empty list */
	if (sys_slist_peek_head(list) == NULL)
		return;

	do {
		swapped = 0;
		ptr1 = sys_slist_peek_head(list);

		while (ptr1->next != lptr) {
			sld1 = SYS_SLIST_CONTAINER(ptr1, sld1, node);
			sld2 = SYS_SLIST_CONTAINER(ptr1->next, sld2, node);
			if (sld1->disp_order > sld2->disp_order) {
				swap_data(sld1, sld2);
				swapped = 1;
			}
			ptr1 = ptr1->next;
		}
		lptr = ptr1;
	} while (swapped);
}
