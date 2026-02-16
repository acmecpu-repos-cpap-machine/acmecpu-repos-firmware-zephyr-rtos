/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 30-May-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */
#define AUDIO_FROM_FILES 1

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdlib.h>

#include <zephyr/fs/fs.h>
// #include <zephyr/fs/littlefs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lib_audio);

// #if CONFIG_CMX655D
// #include "cmx655d.h"
// #endif
#include "codec_apis.h"
#include "lib_audio.h"
// #include "app_storage/app_storage.h"

// #define ARR_SZ 50000
// extern const char raw_aud[ARR_SZ];

// #define AUDIO_CODEC_DEV     DT_PROP(DT_ALIAS(audio_codec), label)

#if DT_NODE_EXISTS(DT_NODELABEL(i2s_rxtx))
#define I2S_RX_NODE  DT_NODELABEL(i2s_rxtx)
#define I2S_TX_NODE  I2S_RX_NODE
#else
#define I2S_RX_NODE  DT_NODELABEL(i2s_rx)
#define I2S_TX_NODE  DT_NODELABEL(i2s_tx)
#endif

#define SAMPLE_FREQUENCY    CONFIG_LIB_AUDIO_SAMPLE_RATE
#define SAMPLE_BIT_WIDTH    CONFIG_LIB_AUDIO_BPS
#define BYTES_PER_SAMPLE    (SAMPLE_BIT_WIDTH / 8) //sizeof(int16_t)
#define NUMBER_OF_CHANNELS  CONFIG_LIB_AUDIO_NUM_CHANNEL
#define SAMPLES_PER_BLOCK   (SAMPLE_FREQUENCY / 10 * NUMBER_OF_CHANNELS)
#define INITIAL_BLOCKS      2
#define TIMEOUT             SYS_FOREVER_MS//1000
#define BLOCK_SIZE  		(BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT 		(INITIAL_BLOCKS + 4)
K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

typedef enum {
    AUDIO_STATE_IDLE = 0,
    AUDIO_STATE_STOP,
    AUDIO_STATE_RECORD,
    AUDIO_STATE_PLAYBACK,
} AUDIO_STATES;

typedef enum {
    FILE_OPER_IDLE = 0,
	FILE_OPER_READING,
    FILE_OPER_WRITING,
} FILE_OPER_STATES;

// WAV header spec information:
//https://web.archive.org/web/20140327141505/https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
//http://www.topherlee.com/software/pcm-tut-wavformat.html
struct __attribute__((__packed__)) wav_header {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    int wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"
    
    // Format Header
    char fmt_header[4]; // Contains "fmt " (includes trailing space)
    int fmt_chunk_size; // Should be 16 for PCM
    short audio_format; // Should be 1 for PCM. 3 for IEEE Float
    short num_channels;
    int sample_rate;
    int byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    short sample_alignment; // num_channels * Bytes Per Sample
    short bit_depth; // Number of bits per sample
    
    // Data
    char data_header[4]; // Contains "data"
    int data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
    // uint8_t bytes[]; // Remainder of wave file is bytes
};

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
	AUDIO_STATES aud_state;
	struct fs_file_t file;
	const char* fname;
	uint32_t fsize_max;		// max record file size in bytes
	// struct k_work work;
	struct k_fifo file_fifo;
	int write_count;
	uint32_t record_time;
	uint8_t file_oper_state;
};
static struct file_info	m_fi;

struct aud_data {
	void *fifo_reserved;
	void *pbuf;
	uint32_t buf_sz;
};
struct aud_data m_ad;

/* callback to application */
static lib_audio_cb m_user_cb = NULL;

/* other static variables */
struct audio_timer {
	struct k_timer timer;
	uint32_t res_ms;		// timer resolution in ms
	uint32_t elapsed_ms;	// time elapsed in ms sine start of audio streams (rec or pb)
};
struct audio_timer m_audio_timer;

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

static void audio_timer_handler(struct k_timer *tmr)
{
	struct audio_timer *atmr = CONTAINER_OF(tmr, struct audio_timer, timer);
	atmr->elapsed_ms += atmr->res_ms;

	if ((atmr->elapsed_ms % 1000) == 0)		// (debug) print time every sec
		LOG_DBG("%d", (atmr->elapsed_ms / 1000));
}

static void audio_timer_stop_handler(struct k_timer *tmr)
{
	struct audio_timer *atmr = CONTAINER_OF(tmr, struct audio_timer, timer);
	LOG_INF("audio duration: %d.%d secs", (atmr->elapsed_ms/1000), (atmr->elapsed_ms%1000));
}

static void fs_thread(void *p1, void *p2, void *p3)
{
	struct file_info *fi = (struct file_info *)p1;
	int rc, write_count=1;
	struct aud_data *pad = NULL;
	fi->file_oper_state = FILE_OPER_IDLE;
	while (1) {
		if (write_count == fi->write_count) {
			fi->file_oper_state = FILE_OPER_IDLE;
			LOG_INF("end of write, count = %d", write_count);
			fi->write_count = -1;
			write_count = 1;
		}

		/* dequeue an audio packet from the fifo */
		if (fi->file_oper_state == FILE_OPER_IDLE)
			pad = k_fifo_get(&fi->file_fifo, K_FOREVER);
		else if (fi->file_oper_state == FILE_OPER_WRITING)
			pad = k_fifo_get(&fi->file_fifo, K_MSEC(1));
		
		if (pad == NULL) {
			continue;
		}
		fi->file_oper_state = FILE_OPER_WRITING;

		// if (k_fifo_is_empty(&fi->file_fifo) != 0) {
		// 	continue;
		// }

		// save recorded data to file
		rc = fs_write(&fi->file, pad->pbuf, pad->buf_sz);
		if (rc < 0) {
			LOG_ERR("FAIL: write %s: %d", fi->fname, rc);
			// break;
		} else {
			LOG_DBG("%d. wrote %s: %d bytes", write_count, fi->fname, pad->buf_sz);
		}
		// printk("%d. free: %p\n", count, pad->pbuf);
		free(pad->pbuf);
		free(pad);
		write_count++;
	}
}

static void audio_thread(void *p1, void *p2, void *p3)
{
	int ret=0, rc;
	struct file_info *fi = (struct file_info *)p1;
	lib_audio_cb user_cb = (lib_audio_cb) p2;
	struct aud_data *ad;

	/* file save thread */
	k_fifo_init(&fi->file_fifo);
	fi->write_count = -1;
	m_fs_tid = k_thread_create(&m_fs_data, m_fs_stack, K_THREAD_STACK_SIZEOF(m_fs_stack), fs_thread, fi, NULL, NULL, 1, 0, K_NO_WAIT);
	ret = k_thread_name_set(m_fs_tid, "fs");
	if (user_cb != NULL) user_cb(LIB_AUDIO_EVENT_INIT_DONE);

	for(;;) {
		uint32_t rec_fsize=0;
		uint32_t record_count=1;
		int err=0;
		fi->aud_state = AUDIO_STATE_IDLE;
		m_audio_timer.elapsed_ms = 0;

		/* obtain the semaphore to start recording or playback, else wait */
		k_sem_take(&toggle_transfer, K_FOREVER);

		int state = fi->aud_state;
		if (state == AUDIO_STATE_RECORD) {
			fi->write_count = -1;
			fs_unlink(fi->fname);
			fs_file_t_init(&fi->file);
			rc = fs_open(&fi->file, fi->fname, FS_O_CREATE | FS_O_WRITE);
			if (rc < 0) {
				LOG_ERR("FAIL: open %s: %d", fi->fname, rc);
				return;
			}
			
			/* TODO: write with proper wav attributes */
			struct wav_header whdr;
			memset(&whdr, 0x00, sizeof(struct wav_header));
			fs_write(&fi->file, &whdr, sizeof(struct wav_header));
		} else if (state == AUDIO_STATE_PLAYBACK) {
			fs_file_t_init(&fi->file);
			rc = fs_open(&fi->file, fi->fname, FS_O_READ);
			if (rc < 0) {
				LOG_ERR("FAIL: open %s: %d", fi->fname, rc);
				continue;
			}

			/* TODO: parse wav */
			struct wav_header whdr;
			memset(&whdr, 0x00, sizeof(struct wav_header));
			rc = fs_read(&fi->file, &whdr, sizeof(struct wav_header));

			if (!prepare_transfer_for_play(i2s_dev_rx, i2s_dev_tx, state)) {
				LOG_ERR("prepare_transfer failed");
				return;
			}
		}
#if (CMX655D_PLL_CLK)
		/* start the codec clock */
		codec_startclk();
#endif
		if (state == AUDIO_STATE_RECORD) {
			/* disable power amp when recording */
			// codec_pa_disable();

			ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, I2S_TRIGGER_START);
			if (ret < 0) {
				printk("Failed to trigger command %d on RX: %d\n", I2S_TRIGGER_START, ret);
				return;
			}
			k_timer_start(&m_audio_timer.timer, K_MSEC(m_audio_timer.res_ms), K_MSEC(m_audio_timer.res_ms));
			if (user_cb != NULL) user_cb(LIB_AUDIO_EVENT_RECORD_STARTED);
		} else if (state == AUDIO_STATE_PLAYBACK) {
#if (CMX655D_PLL_CLK)
			/* enable power amp */
			codec_pa_enable();
#endif
			ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_START);
			if (ret < 0) {
				printk("Failed to trigger command %d on TX: %d\n", I2S_TRIGGER_START, ret);
				return ;
			}
			k_timer_start(&m_audio_timer.timer, K_MSEC(m_audio_timer.res_ms), K_MSEC(m_audio_timer.res_ms));
			if (user_cb != NULL) user_cb(LIB_AUDIO_EVENT_PLAYBACK_STARTED);
		}
		
		LOG_INF("Streams started");

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

				memcpy(ad->pbuf, buf, size);
				ad->buf_sz = size;
				k_fifo_put(&fi->file_fifo, ad);

				free(buf);
				record_count++;
				// LOG_INF("PUT: %d", record_count);

				rec_fsize += size;
				if (rec_fsize >= fi->fsize_max) {
					LOG_INF("max size reached, %d", rec_fsize);
					break;
				}
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
			}
			else if (state == AUDIO_STATE_STOP) {
				break;
			}
			else {
				break;
			}
		}
#if (CMX655D_PLL_CLK)
		/* stop the codec clock */
		codec_stopclk();
#endif
		if (err == 1) {
			err = 0;
			k_timer_stop(&m_audio_timer.timer);
			if (state == AUDIO_STATE_RECORD) {
				ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
				if (ret < 0) {
					printk("Failed to trigger command %d on RX: %d\n", I2S_TRIGGER_PREPARE, ret);
					return;
				}
				if (user_cb != NULL) user_cb(LIB_AUDIO_EVENT_RECORD_ERROR);
			} else if (state == AUDIO_STATE_PLAYBACK) {
				ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
				if (ret < 0) {
					printk("Failed to trigger command %d on TX: %d\n", I2S_TRIGGER_PREPARE, ret);
					return ;
				}
				if (user_cb != NULL) user_cb(LIB_AUDIO_EVENT_PLAYBACK_ERROR);
			}
		} else {
			if (state == AUDIO_STATE_RECORD) {
				ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, I2S_TRIGGER_DROP);
				if (ret < 0) {
					printk("Failed to trigger command %d on RX: %d\n", I2S_TRIGGER_DROP, ret);
					return;
				}
				k_timer_stop(&m_audio_timer.timer);
				/* number of blocks put in the fifo */
				fi->write_count = record_count;
				LOG_INF("end of record %d", record_count);
				/* let file operations end */
				while (fi->file_oper_state != FILE_OPER_IDLE) {
					k_sleep(K_MSEC(10));
				}

				/* notify app that recording has been completed */
				if (user_cb != NULL) user_cb(LIB_AUDIO_EVENT_RECORD_COMPLETE);
			} else if (state == AUDIO_STATE_PLAYBACK) {
				ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
				if (ret < 0) {
					printk("Failed to trigger command %d on TX: %d\n", I2S_TRIGGER_DROP, ret);
					return ;
				}
				k_timer_stop(&m_audio_timer.timer);
				if (user_cb != NULL) user_cb(LIB_AUDIO_EVENT_PLAYBACK_COMPLETE);
			}
		}
		
		/* close the file */
		ret = fs_close(&fi->file);
		if (ret < 0) {
			LOG_ERR("FAIL: close %s: %d", fi->fname, ret);
		return;
		}
		record_count = 1;
	}
}

int lib_audio_init(uint8_t rec_pb, lib_audio_cb user_cb)
{
    int ret=0;

	/* set default values and the application callback */
	m_fi.aud_state = AUDIO_STATE_IDLE;
	m_fi.file_oper_state = FILE_OPER_IDLE;
	m_fi.fsize_max = (CONFIG_LIB_AUDIO_MAX_REC_FILE_SIZE_KB * 1024);
	m_user_cb = user_cb;

	/* audio timer init */
	m_audio_timer.elapsed_ms = 0;
	m_audio_timer.res_ms = 10;
	k_timer_init(&m_audio_timer.timer, audio_timer_handler, audio_timer_stop_handler);

	/* get the codec device */
	codec_dev = codec_device_get();
    if (codec_dev == NULL) {
        LOG_ERR("Device not found %s", codec_dev->name);
        return -1;
    }

	/* setup and start the codec clock */
	ret = codec_clock_setup(codec_dev, NUMBER_OF_CHANNELS, SAMPLE_FREQUENCY);
    if (ret < 0) {
        LOG_ERR("codec_clock_setup failed, ret = %d", ret);
        return ret;
    }

    /* Init and configure the I2S peripheral */
	struct i2s_config config;
	i2s_dev_rx = DEVICE_DT_GET(I2S_RX_NODE);
	i2s_dev_tx = DEVICE_DT_GET(I2S_TX_NODE);

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
	config.options = I2S_OPT_BIT_CLK_SLAVE | I2S_OPT_FRAME_CLK_SLAVE; //I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
	config.frame_clk_freq = SAMPLE_FREQUENCY;
	config.mem_slab = &mem_slab;
	config.block_size = BLOCK_SIZE;
	config.timeout = TIMEOUT;
	if (!configure_streams(i2s_dev_rx, i2s_dev_tx, &config)) {
		return -1;
	}

	/* prepare the code and enable streams */
	ret = codec_prepare(codec_dev);
    if (ret < 0) {
        LOG_ERR("codec_prepare failed, ret = %d", ret);
        return -1;
    }
	codec_enable_streams(NUMBER_OF_CHANNELS, rec_pb);

	// k_sleep(K_MSEC(10));
	// codec_print_regs();
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

    return ret;
}

int lib_audio_start()
{
	int ret = 0;
	/* audio record / playback thread */
	m_aud_tid = k_thread_create(&m_aud_data,
			m_aud_stack,
			K_THREAD_STACK_SIZEOF(m_aud_stack),
			audio_thread, &m_fi, m_user_cb, NULL, 1,
			0, K_NO_WAIT);
	ret = k_thread_name_set(m_aud_tid, "libaud");
	return ret;
}

int lib_audio_record_till_time(const char* fname, uint32_t secs)
{
	if (m_fi.aud_state != AUDIO_STATE_IDLE) {
		LOG_ERR("lib audio is busy, state = %d", m_fi.aud_state);
		return -EBUSY;
	}
	m_fi.aud_state = AUDIO_STATE_RECORD;
	m_fi.fname = fname;
	m_fi.record_time = secs;
	k_sem_give(&toggle_transfer);
	return 0;
}

int lib_audio_record(const char* fname)
{
	if (m_fi.aud_state != AUDIO_STATE_IDLE) {
		LOG_ERR("lib audio is busy, state = %d", m_fi.aud_state);
		return -EBUSY;
	}
	m_fi.aud_state = AUDIO_STATE_RECORD;
	m_fi.fname = fname;
	m_fi.record_time = 0;
	k_sem_give(&toggle_transfer);
	return 0;
}

int lib_audio_playback(const char* fname)
{
	if (m_fi.aud_state != AUDIO_STATE_IDLE) {
		LOG_ERR("lib audio is busy, state = %d", m_fi.aud_state);
		return -EBUSY;
	}

	CMX655SetVolume(92);
	m_fi.aud_state = AUDIO_STATE_PLAYBACK;
	m_fi.fname = fname;
	k_sem_give(&toggle_transfer);
	return 0;	
}

int lib_audio_stop()
{
	m_fi.aud_state = AUDIO_STATE_STOP;
	return 0;	
}

int lib_audio_playback_volume_change(uint32_t volume)
{
	CMX655SetVolume(volume);
    CMX655SetVolSmooth(1);
	return 0;
}

int lib_audio_record_gain_change(uint32_t gain)
{
	// todo
	return 0;
}

#if 0
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
#endif