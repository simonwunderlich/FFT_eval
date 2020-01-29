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

#include "fft_eval.h"

#if defined(__APPLE__)
  #include <libkern/OSByteOrder.h>
  #define CONVERT_BE16(val)	val = OSSwapBigToHostInt16(val)
  #define CONVERT_BE32(val)	val = OSSwapBigToHostInt32(val)
  #define CONVERT_BE64(val)	val = OSSwapBigToHostInt64(val)
#elif defined(_WIN32)
  #define __USE_MINGW_ANSI_STDIO 1
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    #define CONVERT_BE16(val)	val = _byteswap_ushort(val)
    #define CONVERT_BE32(val)	val = _byteswap_ulong(val)
    #define CONVERT_BE64(val)	val = _byteswap_uint64(val)
  #elif __BYTE_ORDER == __BIG_ENDIAN
    #define CONVERT_BE16(val)
    #define CONVERT_BE32(val)
    #define CONVERT_BE64(val)
  #else
    #error Endianess undefined
  #endif
#else
  #ifdef	__FreeBSD__
    #include <sys/endian.h>
  #else
    #include <endian.h>
  #endif	/* __FreeBSD__ */
  #define CONVERT_BE16(val)	val = be16toh(val)
  #define CONVERT_BE32(val)	val = be32toh(val)
  #define CONVERT_BE64(val)	val = be64toh(val)
#endif
#include <errno.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct scanresult *result_list;
int scanresults_n;

/* read_file - reads an file into a big buffer and returns it
 *
 * @fname: file name
 *
 * returns the buffer with the files content
 */
static char *read_file(char *fname, size_t *size)
{
	FILE *fp;
	char *buf = NULL;
	char *newbuf;
	size_t ret;

	fp = fopen(fname, "rb");

	if (!fp)
		return NULL;

	*size = 0;
	while (!feof(fp)) {

		newbuf = realloc(buf, *size + 4097);
		if (!newbuf) {
			free(buf);
			return NULL;
		}

		buf = newbuf;

		ret = fread(buf + *size, 1, 4096, fp);
		*size += ret;
	}
	fclose(fp);

	if (buf)
		buf[*size] = '\0';

	return buf;
}

/*
 * read_scandata - reads the fft scandata and compiles a linked list of datasets
 *
 * @fname: file name
 *
 * returns 0 on success, -1 on error.
 */
int fft_eval_init(char *fname)
{
	char *pos, *scandata;
	size_t len, sample_len;
	size_t rel_pos, remaining_len;
	struct scanresult *result;
	struct fft_sample_tlv *tlv;
	struct scanresult *tail = result_list;
	int handled, bins;

	scandata = read_file(fname, &len);
	if (!scandata)
		return -1;

	pos = scandata;

	while ((uintptr_t)(pos - scandata) < len) {
		rel_pos = pos - scandata;
		remaining_len = len - rel_pos;

		if (remaining_len < sizeof(*tlv)) {
			fprintf(stderr, "Found incomplete TLV header at position 0x%zx\n", rel_pos);
			break;
		}

		tlv = (struct fft_sample_tlv *) pos;
		CONVERT_BE16(tlv->length);
		sample_len = sizeof(*tlv) + tlv->length;
		pos += sample_len;

		if (remaining_len < sample_len) {
			fprintf(stderr, "Found incomplete TLV at position 0x%zx\n", rel_pos);
			break;
		}

		if (sample_len > sizeof(*result)) {
			fprintf(stderr, "sample length %zu too long\n", sample_len);
			continue;
		}

		result = malloc(sizeof(*result));
		if (!result)
			continue;

		memset(result, 0, sizeof(*result));
		memcpy(&result->sample, tlv, sample_len);

		handled = 0;
		switch (tlv->type) {
		case ATH_FFT_SAMPLE_HT20:
			if (sample_len != sizeof(result->sample.ht20)) {
				fprintf(stderr, "wrong sample length (have %zd, expected %zd)\n",
					sample_len, sizeof(result->sample.ht20));
				break;
			}

			CONVERT_BE16(result->sample.ht20.freq);
			CONVERT_BE16(result->sample.ht20.max_magnitude);
			CONVERT_BE64(result->sample.ht20.tsf);

			handled = 1;
			break;
		case ATH_FFT_SAMPLE_HT20_40:
			if (sample_len != sizeof(result->sample.ht40)) {
				fprintf(stderr, "wrong sample length (have %zd, expected %zd)\n",
					sample_len, sizeof(result->sample.ht40));
				break;
			}

			CONVERT_BE16(result->sample.ht40.freq);
			CONVERT_BE64(result->sample.ht40.tsf);
			CONVERT_BE16(result->sample.ht40.lower_max_magnitude);
			CONVERT_BE16(result->sample.ht40.upper_max_magnitude);

			handled = 1;
			break;
		case ATH_FFT_SAMPLE_ATH10K:
			if (sample_len < sizeof(result->sample.ath10k.header)) {
				fprintf(stderr, "wrong sample length (have %zd, expected at least %zd)\n",
					sample_len, sizeof(result->sample.ath10k.header));
				break;
			}

			bins = sample_len - sizeof(result->sample.ath10k.header);

			if (bins != 64 &&
			    bins != 128 &&
			    bins != 256) {
				fprintf(stderr, "invalid bin length %d\n", bins);
				break;
			}

			CONVERT_BE16(result->sample.ath10k.header.freq1);
			CONVERT_BE16(result->sample.ath10k.header.freq2);
			CONVERT_BE16(result->sample.ath10k.header.noise);
			CONVERT_BE16(result->sample.ath10k.header.max_magnitude);
			CONVERT_BE16(result->sample.ath10k.header.total_gain_db);
			CONVERT_BE16(result->sample.ath10k.header.base_pwr_db);
			CONVERT_BE64(result->sample.ath10k.header.tsf);

			handled = 1;
			break;
		case ATH_FFT_SAMPLE_ATH11K:
			if (sample_len < sizeof(result->sample.ath11k.header)) {
				fprintf(stderr, "wrong sample length (have %zd, expected at least %zd)\n",
					sample_len, sizeof(result->sample.ath11k.header));
				break;
			}

			bins = sample_len - sizeof(result->sample.ath11k.header);

			if (bins != 32 &&
			    bins != 64 &&
			    bins != 128 &&
			    bins != 256) {
				fprintf(stderr, "invalid bin length %d\n", bins);
				break;
			}

			CONVERT_BE16(result->sample.ath11k.header.freq1);
			CONVERT_BE16(result->sample.ath11k.header.freq2);
			CONVERT_BE16(result->sample.ath11k.header.max_magnitude);
			CONVERT_BE16(result->sample.ath11k.header.rssi);
			CONVERT_BE32(result->sample.ath11k.header.tsf);
			CONVERT_BE32(result->sample.ath11k.header.noise);

			handled = 1;
			break;
		default:
			fprintf(stderr, "unknown sample type (%d)\n", tlv->type);
			break;
		}

		if (!handled) {
			free(result);
			continue;
		}

		if (tail)
			tail->next = result;
		else
			result_list = result;

		tail = result;

		scanresults_n++;
	}

	fprintf(stderr, "read %d scan results\n", scanresults_n);
	free(scandata);

	return 0;
}

void fft_eval_exit(void)
{
	struct scanresult *list = result_list;
	struct scanresult *next;

	while (list) {
		next = list->next;
		free(list);
		list = next;
	}

	result_list = NULL;
}

void fft_eval_usage(const char *prog)
{
	if (!prog)
		prog = "fft_eval";

	fprintf(stderr, "\n");
	fprintf(stderr, "scanfile is generated by the spectral analyzer feature\n");
	fprintf(stderr, "of your wifi card. If you have a AR92xx or AR93xx based\n");
	fprintf(stderr, "card, try:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "ip link set dev wlan0 up\n");
	fprintf(stderr, "echo chanscan > /sys/kernel/debug/ieee80211/phy0/ath9k/spectral_scan_ctl\n");
	fprintf(stderr, "iw dev wlan0 scan\n");
	fprintf(stderr, "cat /sys/kernel/debug/ieee80211/phy0/ath9k/spectral_scan0 > /tmp/fft_results\n");
	fprintf(stderr, "echo disable > /sys/kernel/debug/ieee80211/phy0/ath9k/spectral_scan_ctl\n");
	fprintf(stderr, "%s /tmp/fft_results\n", prog);
	fprintf(stderr, "\n");
	fprintf(stderr, "for AR98xx based cards, you may use:\n");
	fprintf(stderr, "ip link set dev wlan0 up\n");
	fprintf(stderr, "echo background > /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan_ctl\n");
	fprintf(stderr, "echo trigger > /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan_ctl\n");
	fprintf(stderr, "iw dev wlan0 scan\n");
	fprintf(stderr, "echo disable > /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan_ctl\n");
	fprintf(stderr, "cat /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan0 > samples\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "(NOTE: maybe debugfs must be mounted first: mount -t debugfs none /sys/kernel/debug/ )\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "For ath11k based cards, use:\n");
	fprintf(stderr, "ip link set dev wlan0 up\n");
	fprintf(stderr, "echo background > /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan_ctl\n");
	fprintf(stderr, "echo trigger > /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan_ctl\n");
	fprintf(stderr, "iw dev wlan0 scan\n");
	fprintf(stderr, "echo disable > /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan_ctl\n");
	fprintf(stderr, "cat /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan0 > samples\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "(NOTE: maybe debugfs must be mounted first: mount -t debugfs none /sys/kernel/debug/ )\n");
	fprintf(stderr, "\n");

}
