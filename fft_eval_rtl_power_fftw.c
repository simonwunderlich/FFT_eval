/* SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2012 Simon Wunderlich <sw@simonwunderlich.de>
 * SPDX-FileCopyrightText: 2012 Fraunhofer-Gesellschaft zur Foerderung der angewandten Forschung e.V.
 * SPDX-FileCopyrightText: 2013 Gui Iribarren <gui@altermundi.net>
 * SPDX-FileCopyrightText: 2017 Nico Pace <nicopace@altermundi.net>
 * SPDX-FileCopyrightText: 2019 Kirill Lukonin (EvilWirelessMan) <klukonin@gmail.com>
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

#include <time.h>

#include "fft_eval.h"

/*
 * print_values - spit out the analyzed values in text form, compatible with rtl_power csv format.
 */
static int print_values(void) {
    int i;
    struct scanresult * result;
    struct tm * cal_time;

    time_t time_now;
    char t_str[50];

    for (result = result_list; result; result = result-> next) {

        time_now = time(NULL);
        cal_time = localtime( & time_now);
        strftime(t_str, 50, "# Acquisition start: %Y-%m-%d %H:%M:%S", cal_time);
        printf("%s\n", t_str);
        strftime(t_str, 50, "# Acquisition end: %Y-%m-%d %H:%M:%S", cal_time);
        printf("%s\n", t_str);

        switch (result-> sample.tlv.type) {
        case ATH_FFT_SAMPLE_HT20:
            {
                int datasquaresum = 0;

                printf("#\n");

                for (i = 0; i < SPECTRAL_HT20_NUM_BINS; i++) {
                    int data;
                    data = (result-> sample.ht20.data[i] << result-> sample.ht20.max_exp);
                    if (data == 0)
                        data = 1;
                    data *= data;
                    datasquaresum += data;
                }
                /* prints some statistical data about the data sample and auxiliary data. */
                printf("# datasquaresum = %d\n", datasquaresum);
                printf("# noise = %d\n", result-> sample.ht20.noise);
                printf("# max_exp = %d\n", result-> sample.ht20.max_exp);
                printf("# rssi = %d\n", result-> sample.ht20.rssi);

                for (i = 0; i < SPECTRAL_HT20_NUM_BINS; i++) {
                    float freq;
                    float signal;
                    int data;
                    freq = (result-> sample.ht20.freq - 10.0 + ((20.0 * i) / SPECTRAL_HT20_NUM_BINS)) / 1000;

                    /* This is where the "magic" happens: interpret the signal
                     * to output some kind of data which looks useful.  */

                    data = (result-> sample.ht20.data[i] << result-> sample.ht20.max_exp);

                    if (data == 0)
                        data = 1;

                    signal = result-> sample.ht20.noise + result-> sample.ht20.rssi + 20 * log10(data) - log10(datasquaresum) * 10;

                    printf("%f"
                        "e+09  %f\n", freq, signal);
                }

                printf("\n");
            }
            break;
        case ATH_FFT_SAMPLE_HT20_40:
            {
                int datasquaresum_lower = 0;
                int datasquaresum_upper = 0;
                int datasquaresum;
                int i;
                int centerfreq;
                s8 noise;
                s8 rssi;
                //todo build average

                printf("#\n");

                for (i = 0; i < SPECTRAL_HT20_40_NUM_BINS / 2; i++) {
                    int data;

                    data = result-> sample.ht40.data[i];
                    data <<= result-> sample.ht40.max_exp;
                    data *= data;
                    datasquaresum_lower += data;
                }

                for (i = SPECTRAL_HT20_40_NUM_BINS / 2; i < SPECTRAL_HT20_40_NUM_BINS; i++) {
                    int data;

                    data = result-> sample.ht40.data[i];
                    data <<= result-> sample.ht40.max_exp;
                    datasquaresum_upper += data;

                }
                /* prints some statistical data about the data sample and auxiliary data. */
                printf("# datasquaresum_lower = %d\n", datasquaresum_lower);
                printf("# datasquaresum_upper = %d\n", datasquaresum_upper);
                printf("# noise_lower = %d\n", result-> sample.ht40.lower_noise);
                printf("# noise_upper = %d\n", result-> sample.ht40.upper_noise);
                printf("# max_exp = %d\n", result-> sample.ht40.max_exp);
                printf("# rssi_lower = %d\n", result-> sample.ht40.lower_rssi);
                printf("# rssi_upper = %d\n", result-> sample.ht40.upper_rssi);

                switch (result-> sample.ht40.channel_type) {
                case NL80211_CHAN_HT40PLUS:
                    centerfreq = result-> sample.ht40.freq + 10;
                    break;
                case NL80211_CHAN_HT40MINUS:
                    centerfreq = result-> sample.ht40.freq - 10;
                    break;
                default:
                    return -1;
                }

                for (i = 0; i < SPECTRAL_HT20_40_NUM_BINS; i++) {
                    float freq;
                    int data;

                    freq = (centerfreq - (40.0 * SPECTRAL_HT20_40_NUM_BINS / 128.0) / 2 + (40.0 * (i + 0.5) / 128.0)) / 1000;

                    if (i < SPECTRAL_HT20_40_NUM_BINS / 2) {
                        noise = result-> sample.ht40.lower_noise;
                        datasquaresum = datasquaresum_lower;
                        rssi = result-> sample.ht40.lower_rssi;
                    } else {
                        noise = result-> sample.ht40.upper_noise;
                        datasquaresum = datasquaresum_upper;
                        rssi = result-> sample.ht40.upper_rssi;
                    }

                    data = (result-> sample.ht40.data[i] << result-> sample.ht40.max_exp);

                    if (data == 0)
                        data = 1;

                    float signal = noise + rssi + 20 * log10(data) - log10(datasquaresum) * 10;

                    printf("%f"
                        "e+09  %f\n", freq, signal);
                }

                printf("\n");
            }

            break;
        case ATH_FFT_SAMPLE_ATH10K:
            {
                int datasquaresum = 0;
                int i,
                bins;

                bins = result-> sample.tlv.length - (sizeof(result-> sample.ath10k.header) - sizeof(result-> sample.ath10k.header.tlv));

                printf("#\n");

                for (i = 0; i < bins; i++) {
                    int data;

                    data = (result-> sample.ath10k.data[i] << result-> sample.ath10k.header.max_exp);
                    if (data == 0)
                        data = 1;
                    data *= data;
                    datasquaresum += data;
                }
                /* prints some statistical data about the data sample and auxiliary data. */
                printf("# freq1 = %d\n", result-> sample.ath10k.header.freq1);
                printf("# freq2 = %d\n", result-> sample.ath10k.header.freq2);
                printf("# total_gain_db = %d\n", result-> sample.ath10k.header.total_gain_db);
                printf("# relpwr_db = %d\n", result-> sample.ath10k.header.relpwr_db);
                printf("# avgpwr_db = %d\n", result-> sample.ath10k.header.avgpwr_db);
                printf("# base_pwr_db = %d\n", result-> sample.ath10k.header.base_pwr_db);
                printf("# max_exp = %d\n", result-> sample.ath10k.header.max_exp);
                printf("# max_magnitude = %d\n", result-> sample.ath10k.header.max_magnitude);
                printf("# datasquaresum = %d\n", datasquaresum);
                printf("# rssi = %d\n", result-> sample.ath10k.header.rssi);
                printf("# noise = %d\n", result-> sample.ath10k.header.noise);

                for (i = 0; i < bins; i++) {
                    float freq;
                    int data;
                    float signal;
                    freq = (result-> sample.ath10k.header.freq1 - (result-> sample.ath10k.header.chan_width_mhz) / 2 + (result-> sample.ath10k.header.chan_width_mhz * (i + 0.5) / bins)) / 1000;

                    data = (result-> sample.ath10k.data[i] << result-> sample.ath10k.header.max_exp);

                    if (data == 0)
                        data = 1;

                    signal = result-> sample.ath10k.header.noise + result-> sample.ath10k.header.rssi + 20 * log10(data) - log10(datasquaresum) * 10;
                    printf("%f"
                        "e+09  %f\n", freq, signal);
                }

                printf("\n");

            }
            break;
        case ATH_FFT_SAMPLE_ATH11K:
            {
                int datasquaresum = 0;
                int i,
                bins;

                bins = result-> sample.tlv.length - (sizeof(result-> sample.ath11k.header) - sizeof(result-> sample.ath11k.header.tlv));

                printf("#\n");

                for (i = 0; i < bins; i++) {
                    int data;

                    data = result-> sample.ath11k.data[i];
                    if (data == 0)
                        data = 1;
                    data *= data;
                    datasquaresum += data;
                }

                /* prints some statistical data about the data sample and auxiliary data. */
                printf("# freq1 = %d\n", result-> sample.ath11k.header.freq1);
                printf("# freq2 = %d\n", result-> sample.ath11k.header.freq2);
                printf("# max_exp = %d\n", result-> sample.ath11k.header.max_exp);
                printf("# max_magnitude = %d\n", result-> sample.ath11k.header.max_magnitude);
                printf("# rssi = %d\n", result-> sample.ath11k.header.rssi);
                printf("# noise = %d\n", result-> sample.ath11k.header.noise);

                for (i = 0; i < bins; i++) {
                    float freq;
                    int data;
                    float signal;

                    freq = (result-> sample.ath11k.header.freq1 - (result-> sample.ath11k.header.chan_width_mhz) / 2 + (result-> sample.ath11k.header.chan_width_mhz * (i + 0.5) / bins)) / 1000;

                    data = result-> sample.ath11k.data[i];

                    if (data == 0)
                        data = 1;

                    signal = result-> sample.ath11k.header.noise + result-> sample.ath11k.header.rssi + 20 * log10f(data) - log10f(datasquaresum) * 10;
                    printf("%f"
                        "e+09  %f\n", freq, signal);
                }

                printf("\n");

            }
            break;
        }
    }

    return 0;
}

static void usage(const char * prog) {
    if (!prog)
        prog = "fft_eval";

    fprintf(stderr, "Usage: %s scanfile\n", prog);
    fft_eval_usage(prog);
}

int main(int argc, char * argv[]) {
    char * ss_name = NULL;
    char * prog = NULL;

    if (argc >= 1)
        prog = argv[0];

    if (argc >= 2)
        ss_name = argv[1];

    if (fft_eval_init(ss_name) < 0) {
        fprintf(stderr, "Couldn't read scanfile ...\n");
        usage(prog);
        return -1;
    }

    print_values();
    fft_eval_exit();

    return 0;
}
