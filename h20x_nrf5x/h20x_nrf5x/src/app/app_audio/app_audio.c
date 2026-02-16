/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 25-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */
#define AUDIO_FROM_FILES 1

// #include <zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_audio);

#if CONFIG_CMX655D
#include "cmx655d.h"
#endif
#include "codec_apis.h"

#include "app_audio/app_audio.h"
#include "app_storage/app_storage.h"

// #define ARR_SZ 50000
// extern const char raw_aud[ARR_SZ];

#define AUDIO_CODEC_DEV     DT_PROP(DT_ALIAS(audio_codec), label)

#if DT_NODE_EXISTS(DT_NODELABEL(i2s_rxtx))
#define I2S_RX_NODE  DT_NODELABEL(i2s_rxtx)
#define I2S_TX_NODE  I2S_RX_NODE
#else
#define I2S_RX_NODE  DT_NODELABEL(i2s_rx)
#define I2S_TX_NODE  DT_NODELABEL(i2s_tx)
#endif

#define SAMPLE_FREQUENCY    32000//32000//44100
#define SAMPLE_BIT_WIDTH    16
#define BYTES_PER_SAMPLE    sizeof(int16_t)
#define NUMBER_OF_CHANNELS  1
/* Such block length provides an echo with the delay of 100 ms. */
// #define SAMPLES_PER_BLOCK   ((SAMPLE_FREQUENCY / 20) * NUMBER_OF_CHANNELS)
#define SAMPLES_PER_BLOCK   (SAMPLE_FREQUENCY / 10 * NUMBER_OF_CHANNELS)
#define INITIAL_BLOCKS      2
#define TIMEOUT             SYS_FOREVER_MS//1000

#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT (INITIAL_BLOCKS + 4)
K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

/* file saving thread variables */
K_THREAD_STACK_DEFINE(m_fs_stack, 2048);
static struct k_thread m_fs_data;
static k_tid_t m_fs_tid;

/* audio record / playback thread variables */
K_THREAD_STACK_DEFINE(m_aud_stack, 4096);
static struct k_thread m_aud_data;
static k_tid_t m_aud_tid;

static K_SEM_DEFINE(toggle_transfer, 0, 1);

/* device objects */
const struct device *codec_dev;
const struct device *i2s_dev_rx;
const struct device *i2s_dev_tx;

struct file_info {
	AUDIO_STATES m_aud_state;
	struct fs_file_t file;
	const char* fname;
	uint32_t fsize_max;
	// struct k_work work;
	struct k_fifo file_fifo;
	int count;
};
static struct file_info	m_fi;

struct aud_data {
	void *fifo_reserved;
	void *pbuf;
	uint32_t buf_sz;
};
struct aud_data m_ad;

// static void file_info_work_handler(struct k_work *work);



static bool configure_streams(const struct device *i2s_dev_rx,
			      const struct device *i2s_dev_tx,
			      const struct i2s_config *config)
{
	int ret;

	if (i2s_dev_rx == i2s_dev_tx) {
		ret = i2s_configure(i2s_dev_rx, I2S_DIR_BOTH, config);
		if (ret == 0) {
			return true;
		}
		/* -ENOSYS means that the RX and TX streams need to be
		 * configured separately.
		 */
		if (ret != -ENOSYS) {
			LOG_ERR("Failed to configure streams: %d\n", ret);
			return false;
		}
	}

	ret = i2s_configure(i2s_dev_rx, I2S_DIR_RX, config);
	if (ret < 0) {
		LOG_ERR("Failed to configure RX stream: %d\n", ret);
		return false;
	}

	ret = i2s_configure(i2s_dev_tx, I2S_DIR_TX, config);
	if (ret < 0) {
		LOG_ERR("Failed to configure TX stream: %d\n", ret);
		return false;
	}

	return true;
}


static int lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		LOG_ERR("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	LOG_PRINTK("\nListing dir %s ...\n", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			if (res < 0) {
				LOG_ERR("Error reading dir [%d]\n", res);
			}
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			LOG_PRINTK("[DIR ] %s\n", entry.name);
		} else {
			LOG_PRINTK("[FILE] %s (size = %zu)\n",
				   entry.name, entry.size);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}

#if AUDIO_FROM_FILES
static bool prepare_transfer_for_play(const struct device *i2s_dev_rx,
			     const struct device *i2s_dev_tx, int state)
{
	int ret;

	for (int i = 0; i < INITIAL_BLOCKS; ++i) {
		void *mem_block;

		ret = k_mem_slab_alloc(&mem_slab, &mem_block, K_NO_WAIT);
		if (ret < 0) {
			printk("Failed to allocate TX block %d: %d\n", i, ret);
			return false;
		}

		memset(mem_block, 0, BLOCK_SIZE);

		ret = i2s_write(i2s_dev_tx, mem_block, BLOCK_SIZE);
		if (ret < 0) {
			printk("Failed to write block %d: %d\n", i, ret);
			return false;
		}
	}

	return true;
}

// static volatile bool echo_enabled = true;
// static int16_t echo_block[SAMPLES_PER_BLOCK];
// static void process_block_data(void *mem_block, uint32_t number_of_samples)
// {
// 	static bool clear_echo_block;

// 	if (echo_enabled) {
// 		for (int i = 0; i < number_of_samples; ++i) {
// 			int16_t *sample = &((int16_t *)mem_block)[i];
// 			*sample += echo_block[i];
// 			echo_block[i] = (*sample) / 2;
// 		}

// 		clear_echo_block = true;
// 	} else if (clear_echo_block) {
// 		clear_echo_block = false;
// 		memset(echo_block, 0, sizeof(echo_block));
// 	}
// }

static void fs_thread(void *p1, void *p2, void *p3)
{
	struct file_info *fi = (struct file_info *)p1;
	
	int rc, count=1;
	struct aud_data *pad;
	while (1) {
		/* dequeue a switch press request from the fifo */
		pad = k_fifo_get(&fi->file_fifo, K_FOREVER);
		if (pad == NULL) {
			continue;
		}

		// if (k_fifo_is_empty(&fi->file_fifo) != 0) {
		// 	continue;
		// }

		// save recorded data to file
		rc = fs_write(&fi->file, pad->pbuf, pad->buf_sz);
		if (rc < 0) {
			LOG_ERR("FAIL: write %s: %d", fi->fname, rc);
			// break;
		} else {
			LOG_INF("%d. wrote %s: %d bytes", count, fi->fname, pad->buf_sz);
		}
		// printk("%d. free: %p\n", count, pad->pbuf);
		free(pad->pbuf);
		free(pad);
		count++;
	}
}

static void audio_thread(void *p1, void *p2, void *p3)
{
	int ret=0;
	struct file_info *fi = (struct file_info *)p1;
	int rc;
	struct aud_data *ad;

	/* file save thread */
	k_fifo_init(&fi->file_fifo);
	m_fs_tid = k_thread_create(&m_fs_data,
			m_fs_stack,
			K_THREAD_STACK_SIZEOF(m_fs_stack),
			fs_thread, fi, NULL, NULL, 2,
			0, K_NO_WAIT);
	ret = k_thread_name_set(m_fs_tid, "fs");

	for(;;) {
		uint32_t offset=0, count=1, rec_fsize=0;
		int err=0;

		k_sem_take(&toggle_transfer, K_FOREVER);

		int state = fi->m_aud_state;
		if (state == AUDIO_STATE_RECORD) {
			fs_unlink(fi->fname);
			lsdir("/lfs1");
			fs_file_t_init(&fi->file);
			rc = fs_open(&fi->file, fi->fname, FS_O_CREATE | FS_O_WRITE);
			if (rc < 0) {
				LOG_ERR("FAIL: open %s: %d", fi->fname, rc);
				return;
			}
		} else if (state == AUDIO_STATE_PLAYBACK) {
			lsdir("/lfs1");
			fs_file_t_init(&fi->file);
			rc = fs_open(&fi->file, fi->fname, FS_O_READ);
			if (rc < 0) {
				LOG_ERR("FAIL: open %s: %d", fi->fname, rc);
				continue;
			}
			if (!prepare_transfer_for_play(i2s_dev_rx, i2s_dev_tx, state)) {
				LOG_ERR("prepare_transfer failed");
				return;
			}
		}

		/* start the codec clock */
		codec_startclk();

		// if (!trigger_command(i2s_dev_rx, i2s_dev_tx, I2S_TRIGGER_START)) {
		// 	LOG_ERR("trigger_command I2S_TRIGGER_START failed");
		// 	return;
		// }

		if (state == AUDIO_STATE_RECORD) {
			/* disable power amp when recording */
			codec_pa_disable();

			ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, I2S_TRIGGER_START);
			if (ret < 0) {
				printk("Failed to trigger command %d on RX: %d\n", I2S_TRIGGER_START, ret);
				return;
			}
		} else if (state == AUDIO_STATE_PLAYBACK) {
			/* enable power amp */
			codec_pa_enable();

			ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_START);
			if (ret < 0) {
				printk("Failed to trigger command %d on TX: %d\n", I2S_TRIGGER_START, ret);
				return ;
			}
		}
		
		LOG_INF("Streams started\n");

		/* wait for the codec clock to stabilize */
		k_sleep(K_MSEC(10));

		// uint16_t *sample;
		while (1) {
			if (state == AUDIO_STATE_RECORD) {
				void *buf;
				size_t size;
				buf = calloc(1, BLOCK_SIZE);
				if (buf == NULL) {
					LOG_ERR("calloc failed ad %d bytes!!", BLOCK_SIZE);
					break;
				}

				ret = i2s_buf_read(i2s_dev_rx, buf, &size);
				if (ret < 0) {
					LOG_ERR("Failed to read data: %d\n", ret);
					err = 1;
					break;
				}

				// ret = i2s_read(i2s_dev_rx, &buf, &block_size);
				// if (ret < 0) {
				// 	LOG_ERR("Failed to read data: %d\n", ret);
				// 	break;
				// }
				
				/* allocate memory and copy data */
				ad = (struct aud_data*) calloc(1, sizeof(struct aud_data));
				if (ad == NULL) {
					LOG_ERR("calloc failed ad %d bytes!!", sizeof(struct aud_data));
					break;
				}
				ad->pbuf = calloc(1, size);
				if (ad->pbuf == NULL) {
					LOG_ERR("calloc failed ad->pbuf %d bytes!!", size);
					break;
				}
				// printk("%d. calloc: %p\n", count++, ad->pbuf);
				memcpy(ad->pbuf, buf, size);
				ad->buf_sz = size;
				k_fifo_put(&fi->file_fifo, ad);

				free(buf);
				// LOG_INF("PUT: %d", count++);

				// for (int i=0; i<block_size; i++) {
				// 	sample = &((uint16_t*) buf)[i];
				// 	printk("0x%x ", *sample);
				// }

				rec_fsize += size;
				if (rec_fsize >= fi->fsize_max) {
					LOG_INF("max size reached, %d", rec_fsize);
					break;
				}

				// LOG_INF("used block = %d", k_mem_slab_num_used_get(&mem_slab));

				// process_block_data(buf, SAMPLES_PER_BLOCK);

				// k_mem_slab_free(&mem_slab, &buf);
				// ret = i2s_write(i2s_dev_tx, buf, block_size);
				// if (ret < 0) {
				// 	LOG_ERR("Failed to write data: %d\n", ret);
				// 	break;
				// }
				// k_sleep(K_USEC(100));
			}
			else if (state == AUDIO_STATE_PLAYBACK) {
				size_t block_size = BLOCK_SIZE;
				void *buf = calloc(1, block_size);
				if (buf == NULL) {
					LOG_ERR("calloc failed ad %d bytes!!", block_size);
					break;
				}

				// memcpy(buf, (raw_aud + offset), block_size);

				rc = fs_read(&fi->file, buf, block_size);
				if (rc <= 0) {
					free(buf);
					LOG_INF("no more data in file");
					break;	
				}

				ret = i2s_buf_write(i2s_dev_tx, buf, block_size);
				if (ret < 0) {
					free(buf);
					LOG_ERR("Failed to write data: %d\n", ret);
					err = 1;
					break;
				}

				free(buf);

				// ret = i2s_write(i2s_dev_tx, buf, block_size);
				// if (ret < 0) {
				// 	LOG_ERR("Failed to write data: %d\n", ret);
				// 	break;
				// }
				// offset += block_size;
				// if (offset >= ARR_SZ) {
				// 	LOG_INF("max size reached, %d", offset);
				// 	break;
				// }
			}
		}

		/* stop the codec clock */
		codec_stopclk();

		// if (!trigger_command(i2s_dev_rx, i2s_dev_tx, I2S_TRIGGER_DROP)) {
		// 	LOG_ERR("trigger_command I2S_TRIGGER_DROP failed");
		// 	return;
		// }

		if (err == 1) {
			err = 0;
			if (state == AUDIO_STATE_RECORD) {
				ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
				if (ret < 0) {
					printk("Failed to trigger command %d on RX: %d\n", I2S_TRIGGER_PREPARE, ret);
					return;
				}
			} else if (state == AUDIO_STATE_PLAYBACK) {
				ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
				if (ret < 0) {
					printk("Failed to trigger command %d on TX: %d\n", I2S_TRIGGER_PREPARE, ret);
					return ;
				}
			}
		} else {
			if (state == AUDIO_STATE_RECORD) {
				ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, I2S_TRIGGER_DROP);
				if (ret < 0) {
					printk("Failed to trigger command %d on RX: %d\n", I2S_TRIGGER_DROP, ret);
					return;
				}
			} else if (state == AUDIO_STATE_PLAYBACK) {
				ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
				if (ret < 0) {
					printk("Failed to trigger command %d on TX: %d\n", I2S_TRIGGER_DROP, ret);
					return ;
				}
			}
		}

		/* let file operations end */
		k_sleep(K_MSEC(1000));
		ret = fs_close(&fi->file);
		if (ret < 0) {
			LOG_ERR("FAIL: close %s: %d", fi->fname, ret);
			return;
		}
		count = 1; 	
	}
}
#endif /* AUDIO_FROM_FILES */

#if (!AUDIO_FROM_FILES)
static bool prepare_transfer(const struct device *i2s_dev_rx,
			     const struct device *i2s_dev_tx)
{
	int ret;

	for (int i = 0; i < INITIAL_BLOCKS; ++i) {
		void *mem_block;

		ret = k_mem_slab_alloc(&mem_slab, &mem_block, K_NO_WAIT);
		if (ret < 0) {
			printk("Failed to allocate TX block %d: %d\n", i, ret);
			return false;
		}

		memset(mem_block, 0, BLOCK_SIZE);

		ret = i2s_write(i2s_dev_tx, mem_block, BLOCK_SIZE);
		if (ret < 0) {
			printk("Failed to write block %d: %d\n", i, ret);
			return false;
		}
	}

	return true;
}

static bool trigger_command(const struct device *i2s_dev_rx,
			    const struct device *i2s_dev_tx,
			    enum i2s_trigger_cmd cmd)
{
	int ret;

	if (i2s_dev_rx == i2s_dev_tx) {
		ret = i2s_trigger(i2s_dev_rx, I2S_DIR_BOTH, cmd);
		if (ret == 0) {
			return true;
		}
		/* -ENOSYS means that commands for the RX and TX streams need
		 * to be triggered separately.
		 */
		if (ret != -ENOSYS) {
			printk("Failed to trigger command %d: %d\n", cmd, ret);
			return false;
		}
	}

	ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, cmd);
	if (ret < 0) {
		printk("Failed to trigger command %d on RX: %d\n", cmd, ret);
		return false;
	}

	ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, cmd);
	if (ret < 0) {
		printk("Failed to trigger command %d on TX: %d\n", cmd, ret);
		return false;
	}

	return true;
}
#endif /* (!AUDIO_FROM_FILES) */

int app_audio_init()
{
    int ret=0;
    
	codec_dev = device_get_binding(AUDIO_CODEC_DEV);
    if (codec_dev == NULL) {
        LOG_ERR("Device not found %s", AUDIO_CODEC_DEV);
        return -1;
    }

	ret = codec_clock_setup(codec_dev, NUMBER_OF_CHANNELS, SAMPLE_FREQUENCY);
    if (ret < 0) {
        LOG_ERR("codec_clock_setup failed, ret = %d", ret);
        return ret;
    }

	// ret = codec_init(codec_dev, NUMBER_OF_CHANNELS, SAMPLE_FREQUENCY);
    // if (ret < 0) {
    //     LOG_ERR("codec_init failed, ret = %d", ret);
    //     return ret;
    // }

    /* Init the I2S peripheral */
	i2s_dev_rx = DEVICE_DT_GET(I2S_RX_NODE);
	i2s_dev_tx = DEVICE_DT_GET(I2S_TX_NODE);
	struct i2s_config config;

	if (!device_is_ready(i2s_dev_rx)) {
		LOG_ERR("%s is not ready\n", i2s_dev_rx->name);
		return -1;
	}

	if (i2s_dev_rx != i2s_dev_tx && !device_is_ready(i2s_dev_tx)) {
		LOG_ERR("%s is not ready\n", i2s_dev_tx->name);
		return -1;
	}

    config.word_size = SAMPLE_BIT_WIDTH;
	config.channels = NUMBER_OF_CHANNELS;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
	config.frame_clk_freq = SAMPLE_FREQUENCY;
	config.mem_slab = &mem_slab;
	config.block_size = BLOCK_SIZE;
	config.timeout = TIMEOUT;
	if (!configure_streams(i2s_dev_rx, i2s_dev_tx, &config)) {
		return -1;
	}

#if AUDIO_FROM_FILES
	// codec prepare
	ret = codec_prepare(codec_dev);
    if (ret < 0) {
        LOG_ERR("codec_prepare failed, ret = %d", ret);
        return -1;
    }

	// LOG_INF("codec_startclk failed, ret = %d", ret);
	// LOG_INF("srate = %d", CMX655GetSampleRate());
	// LOG_INF("ClkSrc = %d", CMX655GetClkSrc());
	// LOG_INF("Sai = %d", CMX655GetSai());
	// LOG_INF("SaiM = %d", CMX655GetSaiM());
	// LOG_INF("SysCtrl = %d", CMX655GetSysCtrl());
	// LOG_INF("PllCtrl = %d", CMX655GetPllCtrl());
	// LOG_INF("ISR = %d", CMX655GetIsr());
	// LOG_INF("Rdiv = %d", CMX655GetRdiv());
	// LOG_INF("Ndiv = %d", CMX655GetNdiv());

	/* audio record / playback thread */
	m_aud_tid = k_thread_create(&m_aud_data,
			m_aud_stack,
			K_THREAD_STACK_SIZEOF(m_aud_stack),
			audio_thread, &m_fi, NULL, NULL, 1,
			0, K_NO_WAIT);
	ret = k_thread_name_set(m_aud_tid, "audio");

#else

	// codec prepare
	// ret = codec_prepare(codec_dev);
    // if (ret < 0) {
    //     LOG_ERR("codec_prepare failed, ret = %d", ret);
    //     return -1;
    // }

	// k_sleep(K_MSEC(10));

	if (!prepare_transfer(i2s_dev_rx, i2s_dev_tx)) {
		return -1;
	}

	// k_sleep(K_MSEC(10));

	if (!trigger_command(i2s_dev_rx, i2s_dev_tx, I2S_TRIGGER_START)) {
			return -1;
	}

	k_sleep(K_MSEC(100));


	LOG_INF("Streams started\n");

	codec_enable_streams();
	k_sleep(K_MSEC(10));
	codec_print_regs();

	// const struct device *pin_dev = device_get_binding(TEST_GPIO_DEV_NAME);
	// gpio_pin_configure(pin_dev, TEST_GPIO_PIN, TEST_GPIO_FLAGS);
	while (1) {
		// gpio_pin_set(pin_dev, TEST_GPIO_PIN, 1);

		void *mem_block;
		uint32_t block_size;

		ret = i2s_read(i2s_dev_rx, &mem_block, &block_size);
		if (ret < 0) {
			LOG_ERR("Failed to read data: %d\n", ret);
			break;
		}

		// process_block_data(mem_block, SAMPLES_PER_BLOCK);

		ret = i2s_write(i2s_dev_tx, mem_block, block_size);
		if (ret < 0) {
			LOG_ERR("Failed to write data: %d\n", ret);
			break;
		}

		// k_sleep(K_USEC(50));
		// gpio_pin_set(pin_dev, TEST_GPIO_PIN, 0);
	}
#endif
    return ret;
}

#if 0
static void audio_work_handler(struct k_work *work)
{
	struct app_audio *aa = CONTAINER_OF(work, struct app_audio, m_audio_work);
	int state = aa->m_aud_state;
	int ret, rc, rec_fsize=0;
	struct fs_file_t file;

	if ((state == AUDIO_STATE_RECORD) || (state == AUDIO_STATE_PLAYBACK)) {
		fs_file_t_init(&file);
		rc = fs_open(&file, aa->fname, FS_O_CREATE | FS_O_RDWR);
		if (rc < 0) {
			LOG_ERR("FAIL: open %s: %d", aa->fname, rc);
			return;
		}
	}

	if (!prepare_transfer(i2s_dev_rx, i2s_dev_tx)) {
		LOG_ERR("prepare_transfer failed");
		return;
	}

	if (!trigger_command(i2s_dev_rx, i2s_dev_tx, I2S_TRIGGER_START)) {
		LOG_ERR("trigger_command I2S_TRIGGER_START failed");
		return;
	}
	LOG_INF("Streams started\n");

	while (1) {
		if (state == AUDIO_STATE_RECORD) {
			void *mem_block;
			uint32_t block_size;
			ret = i2s_read(i2s_dev_rx, &mem_block, &block_size);
			if (ret < 0) {
				LOG_ERR("Failed to read data: %d\n", ret);
				break;
			}

			// save recorded data to file
			rc = fs_write(&file, mem_block, block_size);
			if (rc < 0) {
				LOG_ERR("FAIL: write %s: %d", (aa->fname), rc);
				break;
			}
			rec_fsize += rc;
			if (rec_fsize >= aa->fsize_max) {
				LOG_INF("max size reached, %d", rec_fsize);
				break;
			}
		} else if (state == AUDIO_STATE_PLAYBACK) {
			void *mem_block;
			uint32_t block_size;

			rc = fs_read(&file, mem_block, block_size);
			if (rc < 0) {
				LOG_ERR("FAIL: read %s: [rd:%d]", (aa->fname), rc);
				break;
			}	
			ret = i2s_write(i2s_dev_tx, mem_block, block_size);
			if (ret < 0) {
				LOG_ERR("Failed to write data: %d\n", ret);
				break;
			}
			// if (aa->m_aud_state != AUDIO_STATE_PLAYBACK) break;
		} else if (state == AUDIO_STATE_STOP) {
			break;
		}
	}

	if (!trigger_command(i2s_dev_rx, i2s_dev_tx, I2S_TRIGGER_DROP)) {
		LOG_ERR("trigger_command I2S_TRIGGER_DROP failed");
		return;
	}
}
#endif

int app_audio_record_start(const char* fname, uint32_t fsize_max)
{
	// CMX655SetVolume(80);
	m_fi.m_aud_state = AUDIO_STATE_RECORD;
	m_fi.fname = fname;
	m_fi.fsize_max = fsize_max;
	// k_work_submit(&m_aa.m_audio_work);
	k_sem_give(&toggle_transfer);
	return 0;
}

int app_audio_playback_start(const char* fname)
{
	CMX655SetVolume(92);
	m_fi.m_aud_state = AUDIO_STATE_PLAYBACK;
	m_fi.fname = fname;
	// k_work_submit(&m_aa.m_audio_work);
	k_sem_give(&toggle_transfer);
	return 0;	
}

int app_audio_stop()
{
	m_fi.m_aud_state = AUDIO_STATE_STOP;
	// k_work_submit(&m_aa.m_audio_work);
	return 0;	
}

int app_audio_volume_set(uint32_t volume)
{
	CMX655SetVolume(volume);
    CMX655SetVolSmooth(1);
	return 0;
}

int app_audio_printfile(const char* fname)
{
	lsdir("/lfs1");

	struct fs_dirent dirent;
	struct fs_file_t file;

	fs_file_t_init(&file);
	int rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (rc < 0) {
		LOG_ERR("FAIL: open %s: %d", fname, rc);
		return -1;
	}

	rc = fs_stat(fname, &dirent);
	if (rc < 0) {
		LOG_ERR("FAIL: stat %s: %d", fname, rc);
		goto end;
	}

	/* Check file size */
	// if (rc == 0 && dirent.type == FS_DIR_ENTRY_FILE && dirent.size == 0) {
	// 	LOG_INF("file %s, size %d \n", (fname), dirent.size);
	// }

	if (dirent.size == 0) {
		LOG_INF("file %s, size %d \n", fname, dirent.size);
		goto end;
	} else {
#if 0
		char buf[64];
		while((rc = fs_read(&file, buf, sizeof(buf))) > 0) {
			LOG_HEXDUMP_WRN(buf, sizeof(buf), "");
		}
#else
		// int offset=0;
		// int i=0;
		char buf;
		// while (i++ < 100) {
		// printk("\n-----------------------------\n");
		printk("const char raw[] = {");
		while (1) {
			rc = fs_read(&file, &buf, sizeof(buf));
			if (rc <= 0) {
				LOG_ERR("FAIL: read %s: [rd:%d]", fname, rc);
				break;
				// goto end;
			}
			printk("0x%x, ", buf);
			// printk("rc = %d\n", rc);
			// printk("%d: 0x%x\n", fs_tell(&file), buf);
			// fs_seek(&file, rc, FS_SEEK_CUR);
		}
		printk("\b\b};\n");
		// printk("\n-----------------------------\n");
		// printk("\n\n");
#endif
	}

end:
	rc = fs_close(&file);
	if (rc < 0) {
		LOG_ERR("FAIL: close %s: %d", fname, rc);
		return -1;
	}
	return 0;
}