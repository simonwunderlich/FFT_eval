/* SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2012 Simon Wunderlich <sw@simonwunderlich.de>
 * SPDX-FileCopyrightText: 2012 Fraunhofer-Gesellschaft zur Foerderung der angewandten Forschung e.V.
 * SPDX-FileCopyrightText: 2013 Gui Iribarren <gui@altermundi.net>
 * SPDX-FileCopyrightText: 2017 Nico Pace <nicopace@altermundi.net>
 */

/*
 * This program has been created to aid open source spectrum
 * analyzer development for Qualcomm/Atheros AR92xx and AR93xx
 * based chipsets.
 */

#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "fft_eval.h"

/*
 * print_values - spit out the analyzed values in text form, JSON-like.
 */
static int print_values(void)
{
	int i, rnum;
	struct scanresult *result;

	printf("[");
	rnum = 0;
	for (result = result_list; result; result = result->next) {

		switch (result->sample.tlv.type) {

		case ATH_FFT_SAMPLE_HT20:
			{
				uint64_t datamax = 0, datamin = UINT64_MAX;
				uint64_t datasquaresum = 0;

				/* prints some statistical data about the
				 * data sample and auxiliary data. */
				printf("\n{ \"tsf\": %" PRIu64 ", \"central_freq\": %d, \"rssi\": %d, \"noise\": %d, \"data\": [ ", result->sample.ht20.tsf, result->sample.ht20.freq, result->sample.ht20.rssi,
				       result->sample.ht20.noise);
				for (i = 0; i < SPECTRAL_HT20_NUM_BINS; i++) {
					uint64_t data;
					data = (result->sample.ht20.data[i] << result->sample.ht20.max_exp);
					data *= data;
					datasquaresum += data;
					if (data > datamax)
						datamax = data;
					if (data < datamin)
						datamin = data;
				}
				for (i = 0; i < SPECTRAL_HT20_NUM_BINS; i++) {
					float freq;
					float signal;
					int data;

					/*
					 * According to Dave Aragon from University of Washington,
					 * formerly Trapeze/Juniper Networks, in 2.4 GHz it should
					 * divide 22 MHz channel width into 64 subcarriers but
					 * only report the middle 56 subcarriers. The DC subcarrier
					 * is the bin in the middle.
					 *
					 * For 5 GHz we do not know (Atheros claims it does not support
					 * this frequency band, but it works). Use the same assumption.
					 */
					freq = fft_eval_bin_freq(result->sample.ht20.freq, 22.0, 64,
								 SPECTRAL_HT20_NUM_BINS / 2, i);

					/* This is where the "magic" happens: interpret the signal
					 * to output some kind of data which looks useful.  */

					data = result->sample.ht20.data[i] << result->sample.ht20.max_exp;
					if (data == 0)
						data = 1;
					signal = result->sample.ht20.noise + result->sample.ht20.rssi + 20 * log10(data) - log10(datasquaresum) * 10;

					printf("[ %f, %f ]", freq, signal);
					if (i < SPECTRAL_HT20_NUM_BINS - 1)
						printf(", ");
				}
			}
			break;
		case ATH_FFT_SAMPLE_HT20_40:
			{
				uint64_t datamax = 0, datamin = UINT64_MAX;
				uint64_t datasquaresum_lower = 0;
				uint64_t datasquaresum_upper = 0;
				uint64_t datasquaresum;
				int i;
				int centerfreq;
				s8 noise;
				s8 rssi;
				//todo build average

				printf("\n{ \"tsf\": %" PRIu64 ", \"central_freq\": %d, \"rssi\": %d, \"noise\": %d, \"data\": [ ", result->sample.ht40.tsf, result->sample.ht40.freq, result->sample.ht40.lower_rssi,
				       result->sample.ht40.lower_noise);
				for (i = 0; i < SPECTRAL_HT20_40_NUM_BINS / 2; i++) {
					uint64_t data;

					data = result->sample.ht40.data[i];
					data <<= result->sample.ht40.max_exp;
					data *= data;
					datasquaresum_lower += data;

					if (data > datamax)
						datamax = data;
					if (data < datamin)
						datamin = data;
				}

				for (i = SPECTRAL_HT20_40_NUM_BINS / 2; i < SPECTRAL_HT20_40_NUM_BINS; i++) {
					uint64_t data;

					data = result->sample.ht40.data[i];
					data <<= result->sample.ht40.max_exp;
					data *= data;
					datasquaresum_upper += data;

					if (data > datamax)
						datamax = data;
					if (data < datamin)
						datamin = data;
				}

				switch (result->sample.ht40.channel_type) {
				case NL80211_CHAN_HT40PLUS:
					centerfreq = result->sample.ht40.freq + 10;
					break;
				case NL80211_CHAN_HT40MINUS:
					centerfreq = result->sample.ht40.freq - 10;
					break;
				default:
					return -1;
				}

				for (i = 0; i < SPECTRAL_HT20_40_NUM_BINS; i++) {
					float freq;
					int data;

					/*
					 * Like for HT20, the FFT samples 44 MHz (and not 40 MHz) with
					 * the DC subcarrier in the middle.
					 */
					freq = fft_eval_bin_freq(centerfreq, 44.0, 128,
								 SPECTRAL_HT20_40_NUM_BINS / 2, i);

					if (i < SPECTRAL_HT20_40_NUM_BINS / 2) {
						noise = result->sample.ht40.lower_noise;
						datasquaresum = datasquaresum_lower;
						rssi = result->sample.ht40.lower_rssi;
					} else {
						noise = result->sample.ht40.upper_noise;
						datasquaresum = datasquaresum_upper;
						rssi = result->sample.ht40.upper_rssi;
					}

					data = result->sample.ht40.data[i];
					data <<= result->sample.ht40.max_exp;
					if (data == 0)
						data = 1;

					float signal = noise + rssi + 20 * log10(data) - log10(datasquaresum) * 10;

					printf("[ %f, %f ]", freq, signal);
					if (i < SPECTRAL_HT20_40_NUM_BINS - 1)
						printf(", ");
				}
			}
			break;
		case ATH_FFT_SAMPLE_ATH10K:
			{
				uint64_t datamax = 0, datamin = UINT64_MAX;
				uint64_t datasquaresum = 0;
				int i, bins;
				printf("\n{ \"tsf\": %" PRIu64 ", \"central_freq\": %d, \"rssi\": %d, \"noise\": %d, \"data\": [ ", result->sample.ath10k.header.tsf, result->sample.ath10k.header.freq1,
				       result->sample.ath10k.header.rssi, result->sample.ath10k.header.noise);

				bins = result->sample.tlv.length - (sizeof(result->sample.ath10k.header) - sizeof(result->sample.ath10k.header.tlv));

				for (i = 0; i < bins; i++) {
					uint64_t data;

					data = (result->sample.ath10k.data[i] << result->sample.ath10k.header.max_exp);
					data *= data;
					datasquaresum += data;
					if (data > datamax)
						datamax = data;
					if (data < datamin)
						datamin = data;
				}

				for (i = 0; i < bins; i++) {
					float freq;
					int data;
					float signal;
					/*
					 * For 20/40/80 MHz channels, ath10k reports the bandwidth which
					 * is sampled by the FFT (22/44/88 MHz). The DC bin is in the
					 * middle.
					 */
					freq = fft_eval_bin_freq(result->sample.ath10k.header.freq1,
								 result->sample.ath10k.header.chan_width_mhz,
								 bins, bins / 2, i);

					data = result->sample.ath10k.data[i] << result->sample.ath10k.header.max_exp;
					if (data == 0)
						data = 1;
					signal = result->sample.ath10k.header.noise + result->sample.ath10k.header.rssi + 20 * log10(data) - log10(datasquaresum) * 10;
					printf("[ %f, %f ]", freq, signal);
					if (i < bins - 1)
						printf(", ");

				}

			}
			break;
		case ATH_FFT_SAMPLE_ATH11K:
			{
				uint64_t datamax = 0, datamin = UINT64_MAX;
				uint64_t datasquaresum = 0;
				int i, bins;
				printf("\n{ \"tsf\": %" PRIu32 ", \"central_freq\": %d, \"rssi\": %d, \"noise\": %d, \"data\": [ ", result->sample.ath11k.header.tsf, result->sample.ath11k.header.freq1,
				       result->sample.ath11k.header.rssi, result->sample.ath11k.header.noise);

				bins = result->sample.tlv.length - (sizeof(result->sample.ath11k.header) - sizeof(result->sample.ath11k.header.tlv));

				for (i = 0; i < bins; i++) {
					uint64_t data;

					data = (result->sample.ath11k.data[i] << result->sample.ath11k.header.max_exp);
					data *= data;
					datasquaresum += data;
					if (data > datamax)
						datamax = data;
					if (data < datamin)
						datamin = data;
				}

				for (i = 0; i < bins; i++) {
					float freq;
					int data;
					float signal;
					/*
					 * Like for ath10k, the DC bin is in the middle (the max_index of
					 * ath11k is relative to it).
					 */
					freq = fft_eval_bin_freq(result->sample.ath11k.header.freq1,
								 result->sample.ath11k.header.chan_width_mhz,
								 bins, bins / 2, i);

					data = result->sample.ath11k.data[i] << result->sample.ath11k.header.max_exp;
					if (data == 0)
						data = 1;
					signal = (float)result->sample.ath11k.header.noise + result->sample.ath11k.header.rssi + 20 * log10f(data) - log10f(datasquaresum) * 10;
					printf("[ %f, %f ]", freq, signal);
					if (i < bins - 1)
						printf(", ");

				}
			}
			break;
		case MT7996_SPECTRUM_TYPE:
			{
				uint32_t center_freq = result->sample.mt7996.header.center_freq_mhz;
				uint32_t bw = result->sample.mt7996.header.bandwidth_mhz;
				int nfft = mt7996_spectrum_capture_sample_count(result->sample.mt7996.header);
				int i;

				printf("\n{ \"central_freq\": %u, \"data\": [ ", center_freq);

				for (i = 0; i < nfft; i++) {
					float freq, signal, re, im;

					freq = center_freq + (i - nfft/2.) * bw * 2 / nfft;

					re = result->sample.mt7996.fft[i].re;
					im = result->sample.mt7996.fft[i].im;
					signal = 10 * log10(re*re+im*im);

					if (i > 0)
						printf(", ");
					printf("[ %f, %f ]", freq, signal);
				}
			}
			break;
		}

		printf(" ] }");
		if (result->next)
			printf(",");
		rnum++;
	}
	printf("\n]\n");

	return 0;
}

static void usage(const char *prog)
{
	if (!prog)
		prog = "fft_eval";

	fprintf(stderr, "Usage: %s scanfile\n", prog);
	fft_eval_usage(prog);
}

int main(int argc, char *argv[])
{
	char *ss_name = NULL;
	char *prog = NULL;

	if (argc >= 1)
		prog = argv[0];

	if (argc >= 2)
		ss_name = argv[1];

	fprintf(stderr, "WARNING: Experimental Software! Don't trust anything you see. :)\n");
	fprintf(stderr, "\n");

	if (!ss_name) {
		fprintf(stderr, "ERROR: need scan file\n");
		usage(prog);
		return 127;
	}

	if (fft_eval_init(ss_name) < 0) {
		fprintf(stderr, "Couldn't read scanfile ...\n");
		usage(prog);
		return -1;
	}

	print_values();
	fft_eval_exit();

	return 0;
}
