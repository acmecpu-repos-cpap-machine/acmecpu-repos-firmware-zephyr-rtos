/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr/device.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/settings/settings.h>
#include <errno.h>
#include <time.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_settings);

#include <zephyr/fs/fs.h>

#include "lib_events/lib_events.h"
#include "app_settings/app_settings.h"
//#include "app_settings/app_setting_values.h"
#include "app_settings/app_settings_value.h"
#include "app_settings/app_settings_paths.h"
#include "app_settings/app_settings_data.h"
#include "app_settings/app_settings_utils.h"

#include "app_time/app_time.h"
#include "app_net/app_net.h"
#include "app_blower/app_blower.h"

#if CONFIG_LIB_FILE_OPER
#include "lib_file_oper/lib_file_oper.h"
#endif


int root_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg);
int root_handle_commit(void);
int root_handle_export(int (*cb)(const char *name, const void *value, size_t val_len));

#if SETTINGS_ROOT
/* settings value object */
static struct settings_root_app m_root;
#else
static struct setting_value m_settings_load = {0, 0};;
//static uint8_t m_settings_load = 0;
#endif

#define SETTINGS_LOAD_TRUE					(1)	/* value to represent that settings are available */

static char m_last_saved_settings[SETTINGS_FULLPATH_LEN_MAX+1];
static struct k_sem m_settings_lock_rd;
static struct k_sem m_settings_lock_wr;
static struct k_sem m_lastsaved_lock;

/* dynamic root tree handler */
struct settings_handler root_handler = {
		.name = SETTINGS_KEY_ROOT,
		.h_get = NULL,
		.h_set = root_handle_set,
		.h_commit = root_handle_commit,
		.h_export = root_handle_export
};

static int m_settings_log_file_handle = -1;					/* log file static variables */
static struct lib_events_callback m_cb_settings_changed;	/* event cb */
static struct k_work m_settings_worker;						/* work object for handling events */

int root_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
	const char *next;
	int rc;
	char *key;

#if (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)
	struct app_settings_data const *asd = g_sdata;
	struct settings_runtime_value *svd = g_disp_val;
	int idx=-1, num_settings = SETTINGS_COUNT_MAX;

	/* nam */
	key = SETTINGS_KEY_FULL_NAM + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_NAM, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* bst */
	key = SETTINGS_KEY_FULL_BST + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_BST, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/mod */
	key = SETTINGS_KEY_FULL_TS_MOD + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_MOD, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/fic */
	key = SETTINGS_KEY_FULL_TS_FIC + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_FIC, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/mnc */
	key = SETTINGS_KEY_FULL_TS_MNC + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_MNC, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/mxc */
	key = SETTINGS_KEY_FULL_TS_MXC + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_MXC, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/mxi */
	key = SETTINGS_KEY_FULL_TS_MXI + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_MXI, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/ipa */
	key = SETTINGS_KEY_FULL_TS_IPA + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_IPA, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/fxe */
	key = SETTINGS_KEY_FULL_TS_FXE + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_FXE, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/mne */
	key = SETTINGS_KEY_FULL_TS_MNE + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_MNE, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/prs */
	key = SETTINGS_KEY_FULL_TS_PRS + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_PRS, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/rr */
	key = SETTINGS_KEY_FULL_TS_RR + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_RR, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/rit */
	key = SETTINGS_KEY_FULL_TS_RIT + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_RIT, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/fxt */
	key = SETTINGS_KEY_FULL_TS_FXT + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_FXT, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/mxt */
	key = SETTINGS_KEY_FULL_TS_MXT + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_MXT, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/mnt */
	key = SETTINGS_KEY_FULL_TS_MNT + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_MNT, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ts/edt */
	key = SETTINGS_KEY_FULL_TS_EDT + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_TS_EDT, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/srn */
	key = SETTINGS_KEY_FULL_DS_SRN + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_SRN, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/dat/yr */
	key = SETTINGS_KEY_FULL_DS_DAT_YR + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_DAT_YR, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/dat/mon */
	key = SETTINGS_KEY_FULL_DS_DAT_MON + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_DAT_MON, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/dat/day */
	key = SETTINGS_KEY_FULL_DS_DAT_DAY + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_DAT_DAY, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/tim/hr */
	key = SETTINGS_KEY_FULL_DS_TIM_HR + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_TIM_HR, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/tim/min */
	key = SETTINGS_KEY_FULL_DS_TIM_MIN + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_TIM_MIN, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/frs */
	key = SETTINGS_KEY_FULL_DS_FRS + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_FRS, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/pae */
	key = SETTINGS_KEY_FULL_DS_PAE + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_PAE, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/erd */
	key = SETTINGS_KEY_FULL_DS_ERD + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_ERD, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/lan */
	key = SETTINGS_KEY_FULL_DS_LAN + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_LAN, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/net/wiap */
	key = SETTINGS_KEY_FULL_DS_NET_WIAP + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_NET_WIAP, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/net/wpwd */
	key = SETTINGS_KEY_FULL_DS_NET_WPWD + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_NET_WPWD, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/net/wstacfg */
	key = SETTINGS_KEY_FULL_DS_NET_WSTACFG + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_NET_WSTACFG, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/net/wmac */
	key = SETTINGS_KEY_FULL_DS_NET_WMAC + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_NET_WMAC, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/net/wip */
	key = SETTINGS_KEY_FULL_DS_NET_WIP + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_NET_WIP, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/net/wssid */
	key = SETTINGS_KEY_FULL_DS_NET_WSSID + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_NET_WSSID, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/net/wssidcon */
	key = SETTINGS_KEY_FULL_DS_NET_WSSIDCON + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_NET_WSSIDCON, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* ds/net/wi */
	key = SETTINGS_KEY_FULL_DS_NET_WI + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DS_NET_WI, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/pou */
	key = SETTINGS_KEY_FULL_CS_POU + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_POU, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/hum */
	key = SETTINGS_KEY_FULL_CS_HUM + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_HUM, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/rtm */
	key = SETTINGS_KEY_FULL_CS_RTM + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_RTM, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/stm */
	key = SETTINGS_KEY_FULL_CS_STM + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_STM, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/aon */
	key = SETTINGS_KEY_FULL_CS_AON + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_AON, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/aof */
	key = SETTINGS_KEY_FULL_CS_AOF + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_AOF, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/lal */
	key = SETTINGS_KEY_FULL_CS_LAL + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_LAL, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/het/w24/hst */
	key = SETTINGS_KEY_FULL_CS_HET_W24HST + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_HET_W24HST, g_sdata);
		if (idx < 0)
			return -1;
		if (len != asd[idx].size)
			return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/het/w24/hum */
	key = SETTINGS_KEY_FULL_CS_HET_W24HUM + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_HET_W24HUM, g_sdata);
		if (idx < 0)
			return -1;
		if (len != asd[idx].size)
			return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/het/w12/hst */
	key = SETTINGS_KEY_FULL_CS_HET_W12HST + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_HET_W12HST, g_sdata);
		if (idx < 0)
			return -1;
		if (len != asd[idx].size)
			return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/het/w12/temp */
	key = SETTINGS_KEY_FULL_CS_HET_W12TEMP + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_HET_W12TEMP, g_sdata);
		if (idx < 0)
			return -1;
		if (len != asd[idx].size)
			return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/sp/press/inw */
	key = SETTINGS_KEY_FULL_CS_SP_PRESSINW + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_SP_PRESSINW, g_sdata);
		if (idx < 0)
			return -1;
		if (len != asd[idx].size)
			return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/sp/press/inpt */
	key = SETTINGS_KEY_FULL_CS_SP_PRESSINPT + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_SP_PRESSINPT, g_sdata);
		if (idx < 0)
			return -1;
		if (len != asd[idx].size)
			return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/sp/press/exw */
	key = SETTINGS_KEY_FULL_CS_SP_PRESSEXW + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_SP_PRESSEXW, g_sdata);
		if (idx < 0)
			return -1;
		if (len != asd[idx].size)
			return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/sp/press/expt */
	key = SETTINGS_KEY_FULL_CS_SP_PRESSEXPT + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_SP_PRESSEXPT, g_sdata);
		if (idx < 0)
			return -1;
		if (len != asd[idx].size)
			return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/sp/press/amb */
	key = SETTINGS_KEY_FULL_CS_SP_PRESSAMB + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_SP_PRESSAMB, g_sdata);
		if (idx < 0)
			return -1;
		if (len != asd[idx].size)
			return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/sp/humi/inch */
	key = SETTINGS_KEY_FULL_CS_SP_HUMIINCH 	+ (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_SP_HUMIINCH, g_sdata);
		if (idx < 0)
			return -1;
		if (len != asd[idx].size)
			return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* cs/sp/dist/wch */
	key = SETTINGS_KEY_FULL_CS_SP_DISTWCH + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_CS_SP_DISTWCH, g_sdata);
		if (idx < 0)
			return -1;
		if (len != asd[idx].size)
			return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* as/lal */
	key = SETTINGS_KEY_FULL_AS_LAL + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_AS_LAL, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* as/bs */
	key = SETTINGS_KEY_FULL_AS_BS + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_AS_BS, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* as/trg */
	key = SETTINGS_KEY_FULL_AS_TRG + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_AS_TRG, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* as/cyc */
	key = SETTINGS_KEY_FULL_AS_CYC + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_AS_CYC, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* as/tln */
	key = SETTINGS_KEY_FULL_AS_TLN + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_AS_TLN, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* as/mxs */
	key = SETTINGS_KEY_FULL_AS_MXS + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_AS_MXS, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* dev/bmode */
	key = SETTINGS_KEY_FULL_DEV_BMODE + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DEV_BMODE, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* dev/brpm */
	key = SETTINGS_KEY_FULL_DEV_BRPM + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DEV_BRPM, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* dev/bramp */
	key = SETTINGS_KEY_FULL_DEV_BRAMP + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DEV_BRAMP, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* dev/usb */
	key = SETTINGS_KEY_FULL_DEV_USB + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_DEV_USB, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}

	/* lod */
	key = SETTINGS_KEY_FULL_LOD + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		idx = app_settings_lookup_array_idx_get(num_settings, SETTINGS_KEY_FULL_LOD, g_sdata);
		if (idx < 0)	return -1;
		if (len != asd[idx].size)	return -EINVAL;

		rc = app_settings_value_get(&asd[idx], &svd[idx], read_cb, cb_arg);
		if (rc >= 0) {
			memcpy(&m_settings_load, svd[idx].val, sizeof(m_settings_load));
			LOG_DBG("%d) %s: %s", idx, key, svd[idx].disp_val);
			return 0;
		}
		return rc;
	}
#else
	/* load */
	key = SETTINGS_KEY_FULL_LOD + (strlen(SETTINGS_KEY_ROOT) + strlen(SETTINGS_SEPARATOR));
	if (settings_name_steq(name, key, &next) && !next) {
		if (len != sizeof(m_settings_load)) {
			return -EINVAL;
		}
		rc = read_cb(cb_arg, &m_settings_load, sizeof(m_settings_load));
		if (rc >= 0) {
			return 0;
		}
		return rc;
	}
#endif	/* CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM */

	return 0;
}

int root_handle_commit(void) {
	printk("loading all settings under <root> handler is done\n");
	return 0;
}

int root_handle_export(int (*cb)(const char *name, const void *value, size_t val_len)) {
	/* check if settings are available */
	if (m_settings_load.val1 == 0) {
		/* settings already available, do nothing */
		return 0;
	}

	LOG_INF("export all keys under <root> handler");
	/* settings not available, first time boot, store all settings */
	m_settings_load.val1 = 0;
//	uint8_t blw_state = 0;

	char tmp[SETTING_VAL_SRN_LEN_MAX] = {0x00}; strcpy(tmp, "demo");
	char tmpIP[SETTING_VAL_SRN_LEN_MAX] = {0x00};// strcpy(tmpIP, "NA");
	char tmpSSID[SETTING_VAL_WIFI_SSID_LEN_MAX] = {0x00};
	char tmp_wifiPwd[SETTING_VAL_WIFI_PWD_LEN_MAX] = {0x00};
	uint32_t tmp_rpm=10000, tmp_ramp=3000;

	struct setting_value val; val.val1=0; val.val2=0;
	struct setting_value mode; mode.val1=MODE_CPAP; mode.val2=0;

	struct wifi_sta_config wifi_sta_cfg[SETTING_VAL_WIFI_CRED_SAVED_NUM_MAX];
	for (int i=0; i<SETTING_VAL_WIFI_CRED_SAVED_NUM_MAX; i++) {
		memset(wifi_sta_cfg, 0x00, sizeof(struct wifi_sta_config));
	}

	/********************************************************************
	 * The below lines are generated by codegen.
	 * Do not edit them! The sequence must be maintained
	 *********************************************************************/
	(void) cb(SETTINGS_KEY_FULL_NAM, tmp, 20);
        (void) cb(SETTINGS_KEY_FULL_BST, &val, sizeof(struct setting_value));

        (void) cb(SETTINGS_KEY_FULL_DEV_BMODE, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_DEV_BRPM, &tmp_rpm, sizeof(uint32_t));
        (void) cb(SETTINGS_KEY_FULL_DEV_BRAMP, &tmp_ramp, sizeof(uint32_t));
        (void) cb(SETTINGS_KEY_FULL_DEV_USB, &val, sizeof(struct setting_value));

        (void) cb(SETTINGS_KEY_FULL_TS_MOD, &mode, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_FIC, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_MNC, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_MXC, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_MXI, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_IPA, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_FXE, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_MNE, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_PRS, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_RR, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_RIT, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_FXT, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_MXT, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_MNT, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_TS_EDT, &val, sizeof(struct setting_value));

        (void) cb(SETTINGS_KEY_FULL_DS_SRN, tmp, 20);

        (void) cb(SETTINGS_KEY_FULL_DS_DAT_YR, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_DS_DAT_MON, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_DS_DAT_DAY, &val, sizeof(struct setting_value));

        (void) cb(SETTINGS_KEY_FULL_DS_TIM_HR, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_DS_TIM_MIN, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_DS_FRS, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_DS_PAE, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_DS_ERD, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_DS_LAN, &val, sizeof(struct setting_value));

        (void) cb(SETTINGS_KEY_FULL_DS_NET_WIAP, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_DS_NET_WPWD, tmp_wifiPwd, 64);
        (void) cb(SETTINGS_KEY_FULL_DS_NET_WSTACFG, wifi_sta_cfg, sizeof(struct wifi_sta_config));
        (void) cb(SETTINGS_KEY_FULL_DS_NET_WMAC, tmp, 20);
        (void) cb(SETTINGS_KEY_FULL_DS_NET_WIP, tmpIP, 20);
        (void) cb(SETTINGS_KEY_FULL_DS_NET_WSSID, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_DS_NET_WSSIDCON, tmpSSID, 32);
        (void) cb(SETTINGS_KEY_FULL_DS_NET_WI, &val, sizeof(struct setting_value));

        (void) cb(SETTINGS_KEY_FULL_CS_POU, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_HUM, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_RTM, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_STM, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_AON, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_AOF, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_LAL, &val, sizeof(struct setting_value));


        (void) cb(SETTINGS_KEY_FULL_CS_HET_W24HST, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_HET_W24HUM, &val, sizeof(struct setting_value));

        (void) cb(SETTINGS_KEY_FULL_CS_HET_W12HST, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_HET_W12TEMP, &val, sizeof(struct setting_value));


        (void) cb(SETTINGS_KEY_FULL_CS_SP_PRESSINW, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_SP_PRESSINPT, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_SP_PRESSEXW, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_SP_PRESSEXPT, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_CS_SP_PRESSAMB, &val, sizeof(struct setting_value));

        (void) cb(SETTINGS_KEY_FULL_CS_SP_HUMIINCH, &val, sizeof(struct setting_value));

        (void) cb(SETTINGS_KEY_FULL_CS_SP_DISTWCH, &val, sizeof(struct setting_value));

        (void) cb(SETTINGS_KEY_FULL_AS_LAL, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_AS_BS, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_AS_TRG, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_AS_CYC, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_AS_TLN, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_AS_MXS, &val, sizeof(struct setting_value));
        (void) cb(SETTINGS_KEY_FULL_LOD, &m_settings_load, sizeof(struct setting_value));

	/* end */

	return 0;
}

static void app_settings_default_set() {
#if SETTINGS_ROOT

	/* log */
	m_root.log.log_store = SETTINGS_LOG_STORE;
	m_root.log.log_tx_dur = SETTINGS_LOG_TX_DUR_NONE;
	m_root.log.log_tx_med = SETTINGS_LOG_TX_MED_NONE;
	m_root.log.data_store = SETTINGS_DATA_STORE;
	m_root.log.data_tx_dur = SETTINGS_DATA_TX_DUR_NONE;
	m_root.log.data_tx_med = SETTINGS_DATA_TX_MED_NONE;

	/* pwr_chrg */
	m_root.pwr_chrg.powered_on = SETTINGS_POWERED_ON_RESUME;
	m_root.pwr_chrg.powered_off = SETTINGS_POWERED_OFF_SHOW_OPTIONS;
	m_root.pwr_chrg.chrg_dev_on = SETTINGS_CHRG_DEV_ON_SCREEN_ON;
	m_root.pwr_chrg.chrg_dev_off = SETTINGS_CHRG_DEV_OFF_DEV_SCREEN_ON;

	/* presc */
	m_root.presc.presc_change = SETTINGS_PRESC_CHANGE_UPDATE_CONF;

	/* blower */
	m_root.blower.voltage_mv = SETTINGS_BLOWER_VOLTAGE_MV;
	m_root.blower.speed_rpm = SETTINGS_BLOWER_SPEED_RPM;
	m_root.blower.duty_percent = SETTINGS_BLOWER_DUTY;
	m_root.blower.state = 1;//SETTINGS_BLOWER_STATE;

	/* stepper */
	m_root.stepper.reset_pos = SETTINGS_RESET_POS;
	m_root.stepper.step_speed_hz = SETTINGS_STEP_SPEED_HZ;
	m_root.stepper.reset_rot_cnt = SETTINGS_RESET_ROT_CNT;
	m_root.stepper.reset_dir = SETTINGS_RESET_DIR;
	m_root.stepper.step_mode = SETTINGS_STEP_MODE;
	m_root.stepper.step_angle = SETTINGS_STEP_ANGLE;
#endif
}

static void settings_workq_handler(struct k_work *work)
{
	int ret = 0;
	char changed_setting[SETTINGS_FULLPATH_LEN_MAX] = { 0x00 };
	app_settings_changed_latest_get(changed_setting);

	if ((strcmp(changed_setting, SETTINGS_KEY_FULL_LOD) == 0)) {
		struct setting_value val;
		ret = app_settings_load_single(SETTINGS_KEY_FULL_LOD, &val, sizeof(struct setting_value));
		if ((ret == 0) && (val.val1 == 1)) {
			// reboot to load new settings
			lib_events_report_event(LIB_EVENT_REBOOT);
		}
	}

	if ((strcmp(changed_setting, SETTINGS_KEY_FULL_DS_ERD) == 0)) {
		struct setting_value val;
		ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_ERD, &val, sizeof(struct setting_value));
		if ((ret == 0) && (val.val1 == 1)) {
			memset(&val, 0x00, sizeof(struct setting_value));
			app_settings_save_single_with_retry(SETTINGS_KEY_FULL_DS_ERD, &val, sizeof(struct setting_value), 10, true);
#if CONFIG_LIB_FILE_OPER
//	  ret = lib_file_oper_delete_file(m_settings_log_file_handle, SETTINGS_LOG_CURR_FILE_PATH, true);
	  ret = lib_file_oper_delete_file(m_settings_log_file_handle, SETTINGS_LOG_CURR_FILE_PATH, false);
	  if (ret < 0) {
	      LOG_ERR("file %s delete failed, %d", SETTINGS_LOG_CURR_FILE_PATH, ret);
	  } else {
		  m_settings_log_file_handle = -1;	/* invalidate the file handle, get new handle to read/write */
	      LOG_INF("file delete %s success", SETTINGS_LOG_CURR_FILE_PATH);

	      /* TODO below is done for testing purpose, delete when tesing is done */
//	      k_sleep(K_MSEC(100));
//	      sys_reboot(SYS_REBOOT_COLD);

//	      m_settings_log_file_handle = lib_file_oper_create_open_file(
//	    		  SETTINGS_LOG_DIRECTORY_PATH,
//				  SETTINGS_LOG_CURR_FILE_NAME,
//				  SETTINGS_LOG_CURR_FILE_PATH,
//				  SETTINGS_LOG_CURR_FILE_MAX_SIZE_BYTES,
//				  SETTINGS_LOG_MAX_FILE_COUNT);
	  }
#endif
		}
	}

	/* TODO this is done temporarily, remove when testing is done */
	if ((strcmp(changed_setting, SETTINGS_KEY_FULL_DS_PAE) == 0)) {
#if CONFIG_LIB_FILE_OPER
		struct lib_file_oper_rw *rd;
		struct k_fifo* rd_fifo = lib_file_oper_read_whole_file(m_settings_log_file_handle);
		if (rd_fifo != NULL) {
			k_sleep(K_MSEC(100));	// let it read some data into the fifo first
			while (!k_fifo_is_empty(rd_fifo)) {
				rd = k_fifo_get(rd_fifo, K_FOREVER);
				if (rd == NULL) {
					LOG_ERR("read fifo returned NULL");
					continue;
				}
				LOG_HEXDUMP_INF(rd->data, rd->len, "");
				free(rd->data);
				free(rd);
			}
		}
#endif
	}
}

static void lib_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event)
{
	switch (event) {
	case LIB_EVENT_SETTINGS_CHANGED: {
		k_work_submit(&m_settings_worker);
		break;
	}
	default:
		break;
	}
}

int app_settings_init() {
	int ret = 0;

	/* initialize locks for thread safe operation */
	k_sem_init(&m_settings_lock_rd, 1, 1);
	k_sem_init(&m_settings_lock_wr, 1, 1);
	k_sem_init(&m_lastsaved_lock, 1, 1);

	/* Initialize the settings sub system */
	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("settings subsys initialization: fail (err %d)\n", ret);
		return ret;
	}

	LOG_INF("settings subsys initialization: OK.");

	/* Register the root settings handler */
	ret = settings_register(&root_handler);
	if (ret) {
		LOG_ERR("subtree <%s> handler registered: fail (err %d)\n",
				root_handler.name, ret);
	}

	LOG_INF("subtree <%s> handler registered: OK", root_handler.name);
#if (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)
	/* Allocation memory for all setting values */
	ret = app_settings_mem_alloc_displayable_val(SETTINGS_COUNT_MAX, g_disp_val);
#endif
	/* First we set default values to the settings variables */
	app_settings_default_set();

	/* Try to load the settings from persistent memory and set the settings variables */
	int64_t start, delta=0;
	start = k_uptime_get();
	ret = app_settings_load();
	delta = k_uptime_delta(&start);
	LOG_INF("app_settings_load time: %lld ms", delta);

	/* Save the settings into persistent memory.
	 * If there was nothing loaded from persistent memory, then this will call save the default values */
	ret = app_settings_save();

	app_settings_value_init();

#if CONFIG_LIB_FILE_OPER
	/* create / open the settings log file */
	m_settings_log_file_handle = lib_file_oper_create_open_file(
			SETTINGS_LOG_DIRECTORY_PATH,
			SETTINGS_LOG_CURR_FILE_NAME,
			SETTINGS_LOG_CURR_FILE_PATH,
			SETTINGS_LOG_CURR_FILE_MAX_SIZE_BYTES,
			SETTINGS_LOG_MAX_FILE_COUNT,
			(FS_O_CREATE | FS_O_READ | FS_O_WRITE | FS_O_APPEND));
#endif

	ret = lib_events_callback_add(&m_cb_settings_changed, lib_event_handler, LIB_EVENT_SETTINGS_CHANGED);

	/* work queue thread to handle events */
	k_work_init(&m_settings_worker, settings_workq_handler);

	return ret;
}

int app_settings_load() {
	int ret = 0;

	ret = settings_load();

	return ret;
}

int app_settings_save() {
	int ret = 0;

	ret = settings_save();

	return ret;
}

struct direct_immediate_value {
	size_t len;
	void *dest;
	uint8_t fetched;
};
static int direct_loader_immediate_value(const char *name, size_t len,
		settings_read_cb read_cb, void *cb_arg, void *param) {
	const char *next;
	size_t name_len;
	int rc;
	struct direct_immediate_value *one_value =
			(struct direct_immediate_value*) param;

	name_len = settings_name_next(name, &next);

	if (name_len == 0) {
		if (len == one_value->len) {
//			int64_t start, delta=0;
//			start = k_uptime_get();
			rc = read_cb(cb_arg, one_value->dest, len);
//			delta = k_uptime_delta(&start);
//			LOG_INF("read_cb time: %lld ms", delta);
			if (rc >= 0) {
				one_value->fetched = 1;
				LOG_DBG("immediate load: OK.");
				return 0;
			}

			LOG_ERR("immediate failed: %d.", rc);
			return rc;
		}
		return -EINVAL;
	}

	/* other keys aren't served by the calback
	 * Return success in order to skip them
	 * and keep storage processing.
	 */
	return 0;
}

int app_settings_changed_latest_get(char *settings_path)
{
	if (settings_path == NULL)	return -EINVAL;

	k_sem_take(&m_lastsaved_lock, K_MSEC(10));
	strcpy(settings_path, m_last_saved_settings);
	k_sem_give(&m_lastsaved_lock);
	return 0;
}

static void app_settings_last_saved_update(const char *name)
{
	k_sem_take(&m_lastsaved_lock, K_MSEC(10));
	strcpy(m_last_saved_settings, name);
	k_sem_give(&m_lastsaved_lock);
}

int app_settings_load_single(const char *name, void *dest, size_t len) {
	int ret = 0;
	/* special case: don't load date time values from settings, get from app_time module */
	if ((strstr(name, SETTINGS_KEY_FULL_DS_DAT))
			|| strstr(name, SETTINGS_KEY_FULL_DS_TIM)) {
		ret = app_time_get_from_settings(name, dest, len);
		return ret;
	}

#if (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)
	k_sem_take(&m_settings_lock_rd, K_MSEC(1));
	int idx = app_settings_lookup_array_idx_get(SETTINGS_COUNT_MAX, name, g_sdata);
	if (idx < 0) {
		k_sem_give(&m_settings_lock_rd);
		return -ENOENT;
	}
	struct settings_runtime_value *srv = &g_disp_val[idx];
	memcpy(dest, srv->val, len);
	k_sem_give(&m_settings_lock_rd);
#else
	struct direct_immediate_value dov;

	dov.fetched = 0;
	dov.len = len;
	dov.dest = dest;

//	int64_t start, delta=0;
//	start = k_uptime_get();
	k_sem_take(&m_settings_lock_rd, K_MSEC(1));
	ret = settings_load_subtree_direct(name, direct_loader_immediate_value,
			(void*) &dov);
	if (ret == 0) {
		if (!dov.fetched) {
			ret = -ENOENT;
		}
	}
	k_sem_give(&m_settings_lock_rd);
//	delta = k_uptime_delta(&start);
//	LOG_INF("load_subtree_direct time: %lld ms", delta);
#endif	/*(CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)*/
	return ret;
}

static int settings_log_create_and_save(const char *path, void *dest, size_t len)
{
	int asd_idx = app_settings_array_idx_get(path);
	int op_idx = -1, ret=0;
	struct app_settings_data const *asd = app_settings_data_obj_get(asd_idx);
	uint8_t datatype = asd->datatype;
	char str_val[SETTING_VAL_STR_LEN_MAX] = {0x00};

	struct lib_file_oper_rw *wr = (struct lib_file_oper_rw*)calloc(1, sizeof(struct lib_file_oper_rw));
	if (wr == NULL) {
		LOG_ERR("calloc failed at %s", __func__);
		return -ENOMEM;
	}

	if (datatype == SETTING_DATATYPE_SETTING_VALUE) {
		ret = app_settings_value_to_option_idx(path, asd->options, &op_idx);
		if (ret == 0) {
			struct app_settings_value const *data = asd->options->op_val;
			strcpy(str_val, data[op_idx].key);
		}
	} else if (datatype == SETTING_DATATYPE_STRING) {
		memcpy(str_val, dest, len);
	} else if (datatype == SETTING_DATATYPE_DATE) {
		char date[11]= {0x00};	// format yyyy-mm-dd (10 bytes) needed for html format
		app_time_html_formatted_date_get(date);
		strcpy(str_val, date);
	} else if (datatype == SETTING_DATATYPE_TIME) {
		char time[6]= {0x00};	// format hr:mn (6 bytes) needed for html format
		app_time_html_formatted_time_get(time);
		strcpy(str_val, time);
	}

	/* timestamp in millis */
	time_t sec = app_time_value_get_secs();
	char ts_arr[20] = {0x00};
	sprintf(ts_arr, "%lld", sec);

	/* make array to write to file */
	wr->len = strlen(ts_arr) + strlen(path) + strlen(str_val) + 4;	// 2 x comma, 1 x newline, 1 x NULL

	wr->data = (char*)calloc(1, wr->len);
	if (wr->data == NULL) {
		LOG_ERR("calloc failed at %s", __func__);
		free(wr);
		return -ENOMEM;
	}

	sprintf(wr->data, "%s,%s,%s\n", ts_arr, path, str_val);

	/* write to file if we have a valid file handle */
	if (m_settings_log_file_handle >= 0)
		ret = lib_file_oper_write(m_settings_log_file_handle, wr);

	return ret;
}

int app_settings_save_single(const char *name, void *dest, size_t len, bool report_event) {
	int ret = 0;

	/* special case: don't save date time values forward them to app_time module */
	if ((strstr(name, SETTINGS_KEY_FULL_DS_DAT))
			|| strstr(name, SETTINGS_KEY_FULL_DS_TIM)) {
		ret = app_time_change_from_settings(name, dest, len);
		if (ret) {
			LOG_ERR("app_time_change_from_settings failed, %d", ret);
			return ret;
		}
	} else {
		k_sem_take(&m_settings_lock_wr, K_MSEC(1));
		ret = settings_save_one(name, (const void*) dest, len);
		if (ret) {
			LOG_ERR("settings_save_one failed, %d", ret);
			k_sem_give(&m_settings_lock_wr);
			return ret;
		}
#if (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)
		int idx = app_settings_lookup_array_idx_get(SETTINGS_COUNT_MAX, name, g_sdata);
		if (idx < 0) {
			k_sem_give(&m_settings_lock_wr);
			return -ENOENT;
		}
		struct settings_runtime_value *srv = &g_disp_val[idx];
		memcpy(srv->val, dest, len);
#endif
		k_sem_give(&m_settings_lock_wr);
	}

	/* store the setting path which was saved */
	app_settings_last_saved_update(name);

	if (report_event) {
	/* update the settings log file */
#if CONFIG_LIB_FILE_OPER
	settings_log_create_and_save(name, dest, len);
#endif

		/* Report SETTINGS_CHANGED event */
		lib_events_report_event(LIB_EVENT_SETTINGS_CHANGED);
	}

	return ret;
}

int app_settings_save_single_with_retry(const char *name, void *dest, size_t len, int retry_max, bool report_event) {
	int count=0, ret=0;
	do {
		ret = app_settings_save_single(name, dest, len, report_event);
		if (ret)
			k_sleep(K_MSEC(10));
	} while ((ret != 0) && (++count < retry_max));
	return ret;
}

struct subtree_get_value {
	char prev_path[SETTINGS_FULLPATH_LEN_MAX];
	settings_subtree_get_cb cb;
	bool subtree_ok;
};
static int subtree_get_cb(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg, void *param) {

	struct subtree_get_value *sgv = (struct subtree_get_value *) param;

	if (name == NULL) {
		sgv->subtree_ok = false;
		return  0;
	}

	if (sgv->cb != NULL) {
		sgv->subtree_ok = true;
		sgv->cb(sgv->prev_path, name);
	}

	return 0;
}

int app_settings_subtree_get(const char *name, settings_subtree_get_cb cb) {
	int ret = 0;

	if (name == NULL)	return -1;

	struct subtree_get_value sgv;

	memset(&sgv, 0x00, sizeof(sgv));
	strcpy(sgv.prev_path, name);
	sgv.cb = cb;
	sgv.subtree_ok = true;

	ret = settings_load_subtree_direct(name, subtree_get_cb, (void*) &sgv);
	if (ret == 0) {
		if (!sgv.subtree_ok) {
			ret = -ENOENT;
		}
	} else {
		LOG_ERR("settings_load_subtree_direct failed, %d", ret);
	}

	return ret;
}

int app_settings_array_idx_get(const char *fullpath) {
	int idx = -1;
	int num_settings = SETTINGS_COUNT_MAX;
	struct app_settings_data const *asd = &g_sdata[0];
	for (int i=0; i<num_settings; i++) {
		if (!strcmp(fullpath, (asd+i)->fullpath)) {
			idx = i;
			break;
		}
	}
	return idx;
}

int app_settings_datatype_get(int idx) {
	if (idx < 0)	return -1;
	struct app_settings_data const *asd = &g_sdata[0];
	return (asd+idx)->datatype;
}

int app_settings_displayable_get(int idx) {
	if (idx < 0)	return -1;
	struct app_settings_data const *asd = &g_sdata[0];
	return (asd+idx)->displayable;
}

int app_settings_option_key_to_val(struct setting_value_options *options,
									const char *key,
									struct setting_value *out_val)
{
	int idx = -1;
	for (int i = 0; i < options->num_options; i++) {
		if (strcmp(options->op_val[i].key, key) == 0) {
//		if (memcmp(&options->op_val[i].val, &val, sizeof(struct setting_value)) == 0) {
			idx = i;
			memcpy(out_val, &options->op_val[i].val, sizeof (struct setting_value));
			break;
		}
	}
	LOG_INF("app_settings_option_key_to_val: key = %s, val1 = %d, val2 = %d",
			key, out_val->val1, out_val->val2);
	return idx;
}

int app_settings_option_val_to_key(struct setting_value_options *options,
									struct setting_value *in_val, char *out_key)
{
	int idx = -1;
	for (int i = 0; i < options->num_options; i++) {
//		if (strcmp(options->op_val[i].key, key) == 0) {
		if (memcmp(&options->op_val[i].val, in_val, sizeof(struct setting_value)) == 0) {
			idx = i;
//			memcpy(out_val, &options->op_val[i].val, sizeof (struct setting_value));
			strcpy(out_key, options->op_val[i].key);
			break;
		}
	}
	LOG_DBG("app_settings_option_val_to_key: val1 = %d, val2 = %d, out_key = %s",
			in_val->val1, in_val->val2, out_key);
	return idx;
}

/*
 * Function to get array index of a setting value from the saved value
 * This function only works for settings having datatype of struct setting_value
 */
int app_settings_value_to_option_idx(const char* settings_path, struct setting_value_options *options,
											int *pidx)
{
	struct setting_value val;
	*pidx = 0;
	int match = 0;
	int ret = app_settings_load_single(settings_path, &val, sizeof(struct setting_value));
	if (ret == 0) {
		for (int i = 0; i < options->num_options; i++) {
			if (memcmp(&options->op_val[i].val, &val, sizeof(struct setting_value)) == 0) {
				*pidx = i;
				match = 1;
				break;
			}
		}
	}

	if (match == 1)	ret = 0;
	else			ret = -1;

	return ret;
}

struct app_settings_data const* app_settings_data_obj_get(int idx)
{
	if (idx < 0)	return NULL;
	return &g_sdata[idx];
}
