#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "app_settings_paths.h"

#define HTTP_GET_URI				"/g"
#define HTTP_POST_URI				"/p"

/* h tag */
#define HTML_HTAG_OPEN_1			"<h1>"
#define HTML_HTAG_CLOSE_1			"</h1>"
#define HTML_HTAG_OPEN_2			"<h2>"
#define HTML_HTAG_CLOSE_2			"</h2>"
#define HTML_HTAG_OPEN_3			"<h3>"
#define HTML_HTAG_CLOSE_3			"</h3>"
#define HTML_HTAG_OPEN_4			"<h4>"
#define HTML_HTAG_CLOSE_4			"</h4>"

/* links */
#define HTML_LIST_ITEM_PREFIX		"<a href=\""HTTP_GET_URI"?pa="
#define HTML_LIST_ITEM_MIDFIX		"\">"
#define HTML_LIST_ITEM_POSTFIX		"</a>"
#define HTML_PAGE_SIZE_MAX			(10*1024)

/* space */
#define HTML_REGULAR_SPACE			"&nbsp;"
#define HTML_TWO_SPACE				"&ensp;"
#define HTML_FOUR_SPACE				"&emsp;"

#define SETTINGS_FULLPATH_LEN_MAX	32
#define SETTINGS_NAME_LEN_MAX		32

struct app_settings_data {
	const char fullpath[SETTINGS_FULLPATH_LEN_MAX];	// path of a setting in the tree
	char name[SETTINGS_NAME_LEN_MAX];				// name of the settings (can be used for displaying)
};

#if 0
#define SETTINGS_COUNT_MAX 		5
const struct app_settings_data g_sdata[SETTINGS_COUNT_MAX] =
{
		{SETTINGS_KEY_FULL_DS_DAT_YR, "Year"},        //39
		{SETTINGS_KEY_FULL_DS_SRN, "SN"},        //42
		{SETTINGS_KEY_FULL_CS_HUM, "Humidity"},        //49
		{SETTINGS_KEY_FULL_CS_POU, "Place of use"},        //50
		{SETTINGS_KEY_FULL_NAM, "User"},        //52

		/* end */
};

#else
#define SETTINGS_COUNT_MAX 		53
const struct app_settings_data g_sdata[SETTINGS_COUNT_MAX] =
{
		{SETTINGS_KEY_FULL_LOD, "Load"},        //0
		{SETTINGS_KEY_FULL_BST, "Blower State"},        //1
		{SETTINGS_KEY_FULL_DEV_USB, "USB select"},        //2
		{SETTINGS_KEY_FULL_DEV, "Developer"},        //3
		{SETTINGS_KEY_FULL_AS_MXS, "max Settle"},        //4
		{SETTINGS_KEY_FULL_AS_TLN, "tube length"},        //5
		{SETTINGS_KEY_FULL_AS_CYC, "Cycle"},        //6
		{SETTINGS_KEY_FULL_AS_TRG, "Trigger"},        //7
		{SETTINGS_KEY_FULL_AS_LAL, "Leak Alert"},        //8
		{SETTINGS_KEY_FULL_AS, "Adv Tech Settings"},        //9
		{SETTINGS_KEY_FULL_TS_EDT, "exhalation detection threshold"},        //10
		{SETTINGS_KEY_FULL_TS_MNT, "ti min"},        //11
		{SETTINGS_KEY_FULL_TS_MXT, "ti max"},        //12
		{SETTINGS_KEY_FULL_TS_FXT, "ti (fixed)"},        //13
		{SETTINGS_KEY_FULL_TS_RIT, "rise time"},        //14
		{SETTINGS_KEY_FULL_TS_RR, "rr"},        //15
		{SETTINGS_KEY_FULL_TS_PRS, "Pressure Support"},        //16
		{SETTINGS_KEY_FULL_TS_MNE, "minEPAP, minCPAP"},        //17
		{SETTINGS_KEY_FULL_TS_FXE, "fixed epap, cpap"},        //18
		{SETTINGS_KEY_FULL_TS_IPA, "ipap"},        //19
		{SETTINGS_KEY_FULL_TS_MXI, "maxIPAP"},        //20
		{SETTINGS_KEY_FULL_TS_MXC, "max CPAP"},        //21
		{SETTINGS_KEY_FULL_TS_MNC, "min CPAP"},        //22
		{SETTINGS_KEY_FULL_TS_FIC, "CPAP"},        //23
		{SETTINGS_KEY_FULL_TS_MOD, "Mode"},        //24
		{SETTINGS_KEY_FULL_TS, "Tech Settings"},        //25
		{SETTINGS_KEY_FULL_DS_NET, "Network"},        //26
		{SETTINGS_KEY_FULL_DS_NET_WI, "Wi-Fi"},        //27
		{SETTINGS_KEY_FULL_DS_NET_WSSID, "Select SSID"},        //28
		{SETTINGS_KEY_FULL_DS_NET_WPWD, "Password"},        //29
		{SETTINGS_KEY_FULL_DS_NET_WIAP, "Hotspot"},        //30
		{SETTINGS_KEY_FULL_DS_LAN, "language"},        //31
		{SETTINGS_KEY_FULL_DS_ERD, "Erase Data"},        //32
		{SETTINGS_KEY_FULL_DS_PAE, "pt access exh"},        //33
		{SETTINGS_KEY_FULL_DS_FRS, "Factory Reset"},        //34
		{SETTINGS_KEY_FULL_DS_TIM, "Time"},        //35
		{SETTINGS_KEY_FULL_DS_TIM_HR, "Hour"},        //36
		{SETTINGS_KEY_FULL_DS_TIM_MIN, "Min"},        //37
		{SETTINGS_KEY_FULL_DS_DAT, "Date"},        //38
		{SETTINGS_KEY_FULL_DS_DAT_YR, "Year"},        //39
		{SETTINGS_KEY_FULL_DS_DAT_MON, "Month"},        //40
		{SETTINGS_KEY_FULL_DS_DAT_DAY, "Day"},        //41
		{SETTINGS_KEY_FULL_DS_SRN, "SN"},        //42
		{SETTINGS_KEY_FULL_DS, "Device Settings"},        //43
		{SETTINGS_KEY_FULL_CS_LAL, "Leak Alert"},        //44
		{SETTINGS_KEY_FULL_CS_AOF, "AutoOff"},        //45
		{SETTINGS_KEY_FULL_CS_AON, "AutoOn"},        //46
		{SETTINGS_KEY_FULL_CS_STM, "Settle Time"},        //47
		{SETTINGS_KEY_FULL_CS_RTM, "Ramp Time"},        //48
		{SETTINGS_KEY_FULL_CS_HUM, "Humidity"},        //49
		{SETTINGS_KEY_FULL_CS_POU, "Place of use"},        //50
		{SETTINGS_KEY_FULL_CS, "Comfort Settings"},        //51
		{SETTINGS_KEY_FULL_NAM, "User"},        //52

		/* end */
};
#define PATHS_MAX	5
const char g_paths[][SETTINGS_FULLPATH_LEN_MAX] =
{
		{SETTINGS_KEY_FULL_DS_DAT_YR},        //39
		{SETTINGS_KEY_FULL_DS_SRN},        //42
		{SETTINGS_KEY_FULL_CS_HUM},        //49
		{SETTINGS_KEY_FULL_CS_POU},        //50
		{SETTINGS_KEY_FULL_NAM},        //52

		/* end */
};

#endif

static inline int lookup_array_idx_get(int num_settings, const char *fullpath, struct app_settings_data const *hmd) {
	int idx = -1;
	for (int i=0; i<num_settings; i++) {
		if (!strcmp(fullpath, (hmd+i)->fullpath)) {
			idx = i;
			break;
		}
	}
	return idx;
}

#define MAX_DATA	20
char *html_str = NULL;
int hsidx = 0;
struct tree_level {
	char path[SETTINGS_FULLPATH_LEN_MAX];
};
static struct tree_level lvl1[MAX_DATA];
//static struct tree_level lvl2[MAX_DATA];
//static struct tree_level lvl3[MAX_DATA];
//static struct tree_level lvl4[MAX_DATA];
//static struct tree_level lvl5[MAX_DATA];

static int idx_l1 = 0;
//static int idx_l2 = 0;
//static int idx_l3 = 0;
//static int idx_l4 = 0;
//static int idx_l5 = 0;

int check_and_copy(struct tree_level* tl, int *pidx, const char *data)
{
	int match = 0;
	for (int i=0; i<MAX_DATA; i++) {
		if (strcmp(tl[i].path, data) == 0) {
			match = 1;
			break;
		}
	}
	if (match == 0) {
		strcpy(tl[*pidx].path, data);
		(*pidx)++;
	}
	return match;
}

void add_space(char *space, int num)
{
	memset(space, 0x00, 50);
	for (int i=0; i<num; i++) {
		strcat(space, HTML_FOUR_SPACE);
	}
}

void add_htag(char *htag_open, char *htag_close, int level)
{
	memset(htag_open, 0x00, 20);
	memset(htag_close, 0x00, 20);

	switch (level) {
	case 1:
		strcpy(htag_open, HTML_HTAG_OPEN_1);
		strcpy(htag_close, HTML_HTAG_CLOSE_1);
		break;
	case 2:
		strcpy(htag_open, HTML_HTAG_OPEN_2);
		strcpy(htag_close, HTML_HTAG_CLOSE_2);
		break;
	case 3:
		strcpy(htag_open, HTML_HTAG_OPEN_3);
		strcpy(htag_close, HTML_HTAG_CLOSE_3);
		break;
	case 4:
		strcpy(htag_open, HTML_HTAG_OPEN_4);
		strcpy(htag_close, HTML_HTAG_CLOSE_4);
		break;
	}
}

void settings_to_html(const char *pkg_key)
{
	char space[50] = {0x00};
	char htag_open[20] = {0x00};
	char htag_close[20] = {0x00};

	int level_cnt = 0;

	/* get the index from the lookup table array */
	int idx = lookup_array_idx_get(	(sizeof(g_sdata) / sizeof(struct app_settings_data)), pkg_key, g_sdata);
	if (idx < 0)	return;

	struct app_settings_data const *asd = &g_sdata[idx];

	char path[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
	strcpy(path, asd->fullpath);

	char *ptr = path;
	while (1) {
		ptr = strstr(ptr, "/");
		if (ptr == NULL) break;

		level_cnt++;

		char lvl[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
		strncpy(lvl, path, (ptr-path));
		ptr = ptr + 1;
		if (check_and_copy(&lvl1[0], &idx_l1, lvl) == 0) {
			int tmpidx = lookup_array_idx_get(	(sizeof(g_sdata) / sizeof(struct app_settings_data)), lvl, g_sdata);
			if (tmpidx >= 0) {
				struct app_settings_data const *tmp = &g_sdata[tmpidx];
				add_htag(htag_open, htag_close, level_cnt);
				add_space(space, level_cnt);
				hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s%s%s%s", htag_open, HTML_LIST_ITEM_PREFIX, tmp->fullpath,
						HTML_LIST_ITEM_MIDFIX, space, tmp->name, HTML_LIST_ITEM_POSTFIX, htag_close);
			}
		}
	}

	add_htag(htag_open, htag_close, ++level_cnt);
	add_space(space, level_cnt);
	hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s%s%s%s", htag_open, HTML_LIST_ITEM_PREFIX, asd->fullpath,
			HTML_LIST_ITEM_MIDFIX, space, asd->name, HTML_LIST_ITEM_POSTFIX, htag_close);
}

#if 0
void settings_to_html(const char *pkg_key)
{
	/* get the index from the lookup table array */
	int idx = lookup_array_idx_get(	(sizeof(g_sdata) / sizeof(struct app_settings_data)), pkg_key, g_sdata);


	/* if we found the item on the lookup array, we add it to the display slist */
	if ((idx >= 0) /*&& (g_html_menu[idx].settings_data->displayable != 0)*/) {
		struct app_settings_data const *asd = &g_sdata[idx];

		char path[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
		strcpy(path, asd->fullpath);

		char *have_level = strstr(path, "/");
		char *tok = strtok(path, "/");		// ---------------------- check level 1
		if ((have_level==NULL) || (tok == NULL)) {	// no levels
			hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s", HTML_LIST_ITEM_PREFIX, asd->fullpath,
					HTML_LIST_ITEM_MIDFIX, asd->name, HTML_LIST_ITEM_POSTFIX);
		} else {
			if (check_and_copy(&lvl1[0], &idx_l1, tok) == 0) { // level 1
				int tmpidx = lookup_array_idx_get(	(sizeof(g_sdata) / sizeof(struct app_settings_data)), tok, g_sdata);
				if (tmpidx >= 0) {
					struct app_settings_data const *tmp = &g_sdata[tmpidx];
					hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s", HTML_LIST_ITEM_PREFIX, tmp->fullpath,
							HTML_LIST_ITEM_MIDFIX, tmp->name, HTML_LIST_ITEM_POSTFIX);
				}
			}

			char *tok = strtok(NULL, "/");	// ---------------------- check level 2
			if (tok == NULL) {					// no more levels
				hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s", HTML_LIST_ITEM_PREFIX, asd->fullpath,
						HTML_LIST_ITEM_MIDFIX, asd->name, HTML_LIST_ITEM_POSTFIX);
			} else {
				if (check_and_copy(&lvl2[0], &idx_l2, tok) == 0) { // level 2
					int tmpidx = lookup_array_idx_get(	(sizeof(g_sdata) / sizeof(struct app_settings_data)), tok, g_sdata);
					if (tmpidx >= 0) {
						struct app_settings_data const *tmp = &g_sdata[tmpidx];
						hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s", HTML_LIST_ITEM_PREFIX, tmp->fullpath,
								HTML_LIST_ITEM_MIDFIX, tmp->name, HTML_LIST_ITEM_POSTFIX);
					}
				}
				char *tok = strtok(NULL, "/");	// ---------------------- check level 3
				if (tok == NULL) {					// no more levels
					hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s", HTML_LIST_ITEM_PREFIX, asd->fullpath,
							HTML_LIST_ITEM_MIDFIX, asd->name, HTML_LIST_ITEM_POSTFIX);
				}
			}
		}
	}
}
#endif

int main()
{
	html_str = (char*)calloc(1, HTML_PAGE_SIZE_MAX);

	for (int i=0; i<PATHS_MAX; i++) {
		settings_to_html(&g_paths[i][0]);
	}

	if (html_str != NULL) {
		printf("html_len = %ld\n", strlen(html_str));
		printf("%s\n", html_str);
	}
	free(html_str);
}

