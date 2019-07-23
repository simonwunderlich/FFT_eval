// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2012 Simon Wunderlich <sw@simonwunderlich.de>
 * Copyright (C) 2012 Fraunhofer-Gesellschaft zur Foerderung der angewandten Forschung e.V.
 * Copyright (C) 2013 Gui Iribarren <gui@altermundi.net>
 * Copyright (C) 2017 Nico Pace <nicopace@altermundi.net>
 */

#ifndef _FFT_EVAL_H
#define _FFT_EVAL_H

#include <stdint.h>


typedef int8_t s8;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint64_t u64;

enum ath_fft_sample_type {
        ATH_FFT_SAMPLE_HT20 = 1,
        ATH_FFT_SAMPLE_HT20_40 = 2,
	ATH_FFT_SAMPLE_ATH10K = 3,
};

enum nl80211_channel_type {
	NL80211_CHAN_NO_HT,
	NL80211_CHAN_HT20,
	NL80211_CHAN_HT40MINUS,
	NL80211_CHAN_HT40PLUS
};

/*
 * ath9k spectral definition
 */
#define SPECTRAL_HT20_NUM_BINS          56
#define SPECTRAL_HT20_40_NUM_BINS		128

struct fft_sample_tlv {
        u8 type;        /* see ath_fft_sample */
        u16 length;
        /* type dependent data follows */
} __attribute__((packed));

struct fft_sample_ht20 {
        struct fft_sample_tlv tlv;

        u8 max_exp;

        u16 freq;
        s8 rssi;
        s8 noise;

        u16 max_magnitude;
        u8 max_index;
        u8 bitmap_weight;

        u64 tsf;

        u8 data[SPECTRAL_HT20_NUM_BINS];
} __attribute__((packed));

struct fft_sample_ht20_40 {
	struct fft_sample_tlv tlv;

	u8 channel_type;
	u16 freq;

	s8 lower_rssi;
	s8 upper_rssi;

	u64 tsf;

	s8 lower_noise;
	s8 upper_noise;

	u16 lower_max_magnitude;
	u16 upper_max_magnitude;

	u8 lower_max_index;
	u8 upper_max_index;

	u8 lower_bitmap_weight;
	u8 upper_bitmap_weight;

	u8 max_exp;

	u8 data[SPECTRAL_HT20_40_NUM_BINS];
} __attribute__((packed));

/*
 * ath10k spectral sample definition
 */

#define SPECTRAL_ATH10K_MAX_NUM_BINS            256

struct fft_sample_ath10k {
	struct fft_sample_tlv tlv;
	u8 chan_width_mhz;
	uint16_t freq1;
	uint16_t freq2;
	int16_t noise;
	uint16_t max_magnitude;
	uint16_t total_gain_db;
	uint16_t base_pwr_db;
	uint64_t tsf;
	s8 max_index;
	u8 rssi;
	u8 relpwr_db;
	u8 avgpwr_db;
	u8 max_exp;

	u8 data[0];
} __attribute__((packed));


struct scanresult {
	union {
		struct fft_sample_tlv tlv;
		struct fft_sample_ht20 ht20;
		struct fft_sample_ht20_40 ht40;
		struct {
			struct fft_sample_ath10k header;
			u8 data[SPECTRAL_ATH10K_MAX_NUM_BINS];
		} ath10k;
	} sample;
	struct scanresult *next;
};

int fft_eval_init(char *fname);
void fft_eval_exit(void);
void fft_eval_usage(const char *prog);

extern struct scanresult *result_list;
extern int scanresults_n;

#endif
