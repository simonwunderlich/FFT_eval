/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2012 Simon Wunderlich <sw@simonwunderlich.de>
 * SPDX-FileCopyrightText: 2012 Fraunhofer-Gesellschaft zur Foerderung der angewandten Forschung e.V.
 * SPDX-FileCopyrightText: 2013 Gui Iribarren <gui@altermundi.net>
 * SPDX-FileCopyrightText: 2017 Nico Pace <nicopace@altermundi.net>
 */

#ifndef _FFT_EVAL_H
#define _FFT_EVAL_H

#include <stdint.h>
#include <stddef.h>
#include <kiss_fft.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint64_t u64;

enum ath_fft_sample_type {
        ATH_FFT_SAMPLE_HT20 = 1,
        ATH_FFT_SAMPLE_HT20_40 = 2,
	ATH_FFT_SAMPLE_ATH10K = 3,
	ATH_FFT_SAMPLE_ATH11K = 4,
	MT7996_SPECTRUM_TYPE = 64,
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
/*
 * Limit for the exponent which has to be applied to the FFT bins. The sum of
 * the squares of up to 512 bins (8 bit << max_exp) must fit into 64 bit.
 */
#define SPECTRAL_MAX_EXP                19

#define SPECTRAL_HT20_NUM_BINS          56
#define SPECTRAL_HT20_40_NUM_BINS		128

#if defined(__WIN32__)
#pragma pack(push,1)
#endif

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

        u8 data[0];
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

	u8 data[0];
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

/*
 * ath11k spectral sample definition
 */

#define SPECTRAL_ATH11K_MAX_NUM_BINS            512

struct fft_sample_ath11k {
	struct fft_sample_tlv tlv;
	u8 chan_width_mhz;
	s8 max_index;
	u8 max_exp;
	uint16_t freq1;
	uint16_t freq2;
	uint16_t max_magnitude;
	uint16_t rssi;
	uint32_t tsf;
	int32_t noise;

	u8 data[0];
} __attribute__((packed));

/*
 * mt7996 spectrum capture definition
 */

#define MT7996_SPECTRUM_MAX_SAMPLES	2048

enum mt7996_spectrum_gain {
	MT7996_SPECTRUM_GAIN_AUTO = 0,
	MT7996_SPECTRUM_GAIN_HIGH = 1,
	MT7996_SPECTRUM_GAIN_MID = 2,
	MT7996_SPECTRUM_GAIN_LOW = 3,
};

struct mt7996_spectrum_capture {
	struct fft_sample_tlv tlv;
	u8 _rsv;

	uint32_t seq;
	uint64_t timestamp;
	uint32_t capture_node;
	uint32_t center_freq_mhz;
	uint16_t bandwidth_mhz;
	u8 band;
	u8 gain;

	struct {
		int32_t i, q;
	} data[0];
} __attribute__((packed));

#if defined(__WIN32__)
#pragma pack(pop)
#endif


struct scanresult {
	struct scanresult *next;
	union {
		struct fft_sample_tlv tlv;
		struct fft_sample_ht20 ht20;
		struct fft_sample_ht20_40 ht40;
		struct {
			struct fft_sample_ath10k header;
			u8 data[0];
		} ath10k;
		struct {
			struct fft_sample_ath11k header;
			u8 data[0];
		} ath11k;
		struct {
			struct mt7996_spectrum_capture header;
			struct {
				int32_t i, q;
			} raw[MT7996_SPECTRUM_MAX_SAMPLES];
			kiss_fft_cpx fft[MT7996_SPECTRUM_MAX_SAMPLES];
		} mt7996;
	} sample;
};

int fft_eval_init(char *fname);
void fft_eval_exit(void);
void fft_eval_usage(const char *prog);

/**
 * fft_eval_bin_freq() - Calculate the frequency of an FFT bin
 * @center: frequency of the DC bin in MHz
 * @bandwidth: bandwidth which is sampled by the FFT in MHz
 * @fft_size: number of points of the FFT
 * @dc_bin: index of the DC bin in the reported bins
 * @bin: index of the reported bin
 *
 * Return: center frequency of the bin in MHz
 */
static inline double fft_eval_bin_freq(double center, double bandwidth,
				       int fft_size, int dc_bin, int bin)
{
	return center + bandwidth * (bin - dc_bin) / fft_size;
}

static inline size_t
mt7996_spectrum_capture_sample_count(struct mt7996_spectrum_capture capture)
{
	return (capture.tlv.length - (sizeof(capture) - sizeof(capture.tlv))) /
	       sizeof(capture.data[0]);
}

extern struct scanresult *result_list;
extern int scanresults_n;

#endif
