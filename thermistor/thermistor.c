#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>
#include "thermistor.h"

LOG_MODULE_REGISTER(NTC_THERMISTOR, CONFIG_SENSOR_LOG_LEVEL);

struct ntc_thermistor_data {
	struct k_mutex mutex;
	int16_t raw;
	int16_t sample_val;
};

struct ntc_thermistor_config {
	const struct adc_dt_spec adc_channel;
	const struct ntc_config ntc_cfg;
};

static int thermistor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ntc_thermistor_data *data = dev->data;
	const struct ntc_thermistor_config *cfg = dev->config;
	enum pm_device_state pm_state;
	int32_t val_mv;
	int res;
	struct adc_sequence sequence = {
		.options = NULL,
		.buffer = &data->raw,
		.buffer_size = sizeof(data->raw),
		.calibrate = false,
	};

	(void)pm_device_state_get(dev, &pm_state);
	if (pm_state != PM_DEVICE_STATE_ACTIVE) {
		return -EIO;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	adc_sequence_init_dt(&cfg->adc_channel, &sequence);
	res = adc_read(cfg->adc_channel.dev, &sequence);
	if (!res) {
		val_mv = data->raw;
		res = adc_raw_to_millivolts_dt(&cfg->adc_channel, &val_mv);
		data->sample_val = val_mv;
	}

	k_mutex_unlock(&data->mutex);

	return res;
}

static int thermistor_channel_get(const struct device *dev, enum sensor_channel chan,
				      struct sensor_value *val)
{
	struct ntc_thermistor_data *data = dev->data;
	const struct ntc_thermistor_config *cfg = dev->config;
	uint32_t ohm;
	int32_t temp;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		ohm = thermistor_get_ohm(&cfg->ntc_cfg, data->sample_val);
		temp = thermistor_get_temp_mc(&cfg->ntc_cfg.type, ohm);
		val->val1 = temp / 1000;
		val->val2 = (temp % 1000) * 1000;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static const struct sensor_driver_api thermistor_driver_api = {
	.sample_fetch = thermistor_sample_fetch,
	.channel_get = thermistor_channel_get,
};

int thermistor_init(const struct device *dev)
{
	const struct ntc_thermistor_config *cfg = dev->config;
	int err;

	if (!adc_is_ready_dt(&cfg->adc_channel)) {
		LOG_ERR("ADC controller device is not ready\n");
		return -ENODEV;
	}

	err = adc_channel_setup_dt(&cfg->adc_channel);
	if (err < 0) {
		LOG_ERR("Could not setup channel err(%d)\n", err);
		return err;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_init_suspended(dev);

	err = pm_device_runtime_enable(dev);
	if (err) {
		LOG_ERR("Failed to enable runtime power management");
		return err;
	}
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int thermistor_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_RESUME:
	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_SUSPEND:
		return 0;
	default:
		return -ENOTSUP;
	}
}
#endif

#define NTC_THERMISTOR_DEFINE0(inst, id, _comp, _n_comp)                                           \
	static struct ntc_thermistor_data ntc_thermistor_driver_##id##inst;                        \
                                                                                                   \
	static const struct ntc_thermistor_config ntc_thermistor_cfg_##id##inst = {                \
		.adc_channel = ADC_DT_SPEC_INST_GET(inst),                                         \
		.ntc_cfg =                                                                         \
			{                                                                          \
				.pullup_uv = DT_INST_PROP(inst, pullup_uv),                        \
				.pullup_ohm = DT_INST_PROP(inst, pullup_ohm),                      \
				.pulldown_ohm = DT_INST_PROP(inst, pulldown_ohm),                  \
				.connected_positive = DT_INST_PROP(inst, connected_positive),      \
				.type = {                                                          \
					.comp = _comp,                                             \
					.n_comp = _n_comp,                                         \
				},                                                                 \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, thermistor_pm_action);                                  \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(                                                              \
		inst, thermistor_init, PM_DEVICE_DT_INST_GET(inst),                            \
		&ntc_thermistor_driver_##id##inst, &ntc_thermistor_cfg_##id##inst, POST_KERNEL,    \
		CONFIG_SENSOR_INIT_PRIORITY, &thermistor_driver_api);

#define NTC_THERMISTOR_DEFINE(inst, id, comp) \
	NTC_THERMISTOR_DEFINE0(inst, id, comp, ARRAY_SIZE(comp))

/* ntc-thermistor-generic */
#define DT_DRV_COMPAT ntc_thermistor_generic

#define NTC_THERMISTOR_GENERIC_DEFINE(inst)                                                        \
	static const uint32_t comp_##inst[] = DT_INST_PROP(inst, zephyr_compensation_table);       \
	NTC_THERMISTOR_DEFINE0(inst, DT_DRV_COMPAT, (struct ntc_compensation *)comp_##inst,        \
		ARRAY_SIZE(comp_##inst) / 2)

DT_INST_FOREACH_STATUS_OKAY(NTC_THERMISTOR_GENERIC_DEFINE)

/* murata,NXFT15XH103FEAB050 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT murata_NXFT15XH103FEAB050

static __unused const struct ntc_compensation comp_murata_NXFT15XH103FEAB050[] = {
	        {195652,-40},
		{184917,-39},
		{174845,-38},
		{165391,-37},
		{156513,-36},
		{148171,-35},
		{140330,-34},
		{132958,-33},
		{126022,-32},
		{119494,-31},
		{113347,-30},
		{107565,-29},
		{102116,-28},
		{96978,-27},
		{92132,-26},
		{87559,-25},
		{83242,-24},
		{79166,-23},
		{75316,-22},
		{71677,-21},
		{68237,-20},
		{64991,-19},
		{61919,-18},
		{59011,-17},
		{56258,-16},
		{53650,-15},
		{51178,-14},
		{48835,-13},
		{46613,-12},
		{44506,-11},
		{42506,-10},
		{40600,-9},
		{38791,-8},
		{37073,-7},
		{35442,-6},
		{33892,-5},
		{32420,-4},
		{31020,-3},
		{29689,-2},
		{28423,-1},
		{27219,0},
		{26076,1},
		{24988,2},
		{23951,3},
		{22963,4},
		{22021,5},
		{21123,6},
		{20267,7},
		{19450,8},
		{18670,9},
		{17926,10},
		{17214,11},
		{16534,12},
		{15886,13},
		{15266,14},
		{14674,15},
		{14108,16},
		{13566,17},
		{13049,18},
		{12554,19},
		{12081,20},
		{11628,21},
		{11195,22},
		{10780,23},
		{10382,24},
		{10000,25},
		{9634,26},
		{9284,27},
		{8947,28},
		{8624,29},
		{8315,30},
		{8018,31},
		{7734,32},
		{7461,33},
		{7199,34},
		{6948,35},
		{6707,36},
		{6475,37},
		{6253,38},
		{6039,39},
		{5834,40},
		{5636,41},
		{5445,42},
		{5262,43},
		{5086,44},
		{4917,45},
		{4754,46},
		{4597,47},
		{4446,48},
		{4301,49},
		{4161,50},
		{4026,51},
		{3896,52},
		{3771,53},
		{3651,54},
		{3535,55},
		{3423,56},
		{3315,57},
		{3211,58},
		{3111,59},
		{3014,60},
		{2922,61},
		{2834,62},
		{2748,63},
		{2666,64},
		{2586,65},
		{2509,66},
		{2435,67},
		{2364,68},
		{2294,69},
		{2228,70},
		{2163,71},
		{2100,72},
		{2040,73},
		{1981,74},
		{1925,75},
		{1870,76},
		{1817,77},
		{1766,78},
		{1716,79},
		{1669,80},
		{1622,81},
		{1578,82},
		{1535,83},
		{1493,84},
		{1452,85},
		{1413,86},
		{1375,87},
		{1338,88},
		{1303,89},
		{1268,90},
		{1234,91},
		{1202,92},
		{1170,93},
		{1139,94},
		{1110,95},
		{1081,96},
		{1053,97},
		{1026,98},
		{999,99},
		{974,100},
		{949,101},
		{925,102},
		{902,103},
		{880,104},
		{858,105},
		{837,106},
		{816,107},
		{796,108},
		{777,109},
		{758,110},
		{740,111},
		{722,112},
		{705,113},
		{688,114},
		{672,115},
		{656,116},
		{640,117},
		{625,118},
		{611,119},
		{596,120},
		{583,121},
		{569,122},
		{556,123},
		{544,124},
		{531,125},
};

DT_INST_FOREACH_STATUS_OKAY_VARGS(NTC_THERMISTOR_DEFINE, DT_DRV_COMPAT,
				  comp_murata_NXFT15XH103FEAB050)
