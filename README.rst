.. SPDX-License-Identifier: CC0-1.0
.. SPDX-FileCopyrightText: 2012-2016, Simon Wunderlich <sw@simonwunderlich.de>
.. SPDX-FileCopyrightText: 2017, Nico Pace <nicopace@altermundi.net>
.. SPDX-FileCopyrightText: 2019, Sven Eckelmann <sven@narfation.org>
.. SPDX-FileCopyrightText: 2019, John Crispin <john@phrozen.org>

==============
FFT evaluation
==============

DESCRIPTION
===========

This program has been created to aid open source spectrum
analyzer development for Qualcomm/Atheros AR92xx and AR93xx
based chipsets. 

It visualizes the FFT data reported by the chips to help interpreting
and understanding the data.

TODO: The interpreted data format is unknown! Please help
investigating the data, or help acquiring information about the
data format from Qualcomm Atheros!

BUILD
=====

You need to have SDL2 and SDL2_ttf development packages installed
on your system, as well as make, pkg-config and gcc.

Just type make to build the programs fft_eval_sdl (graphical viewer) and
fft_eval_json (converter to JSON):

.. code-block:: bash

  $ make
      CC fft_eval.o
      CC fft_eval_sdl.o
      LD fft_eval_sdl
      CC fft_eval_json.o
      LD fft_eval_json
  $

fft_eval_json doesn't need SDL2. The viewer can be disabled when the SDL2
development packages are not available:

.. code-block:: bash

  $ make CONFIG_fft_eval_sdl=n

USAGE
=====

First, you need to acquire sample data from your spectral-scan enabled
Atheros WiFi card. If the patches are applied correctly, you can use
the following commands:

For ath9k/AR92xx or AR93xx based cards, use:

.. code-block:: bash

  ip link set dev wlan0 up
  echo chanscan > /sys/kernel/debug/ieee80211/phy0/ath9k/spectral_scan_ctl
  iw dev wlan0 scan
  cat /sys/kernel/debug/ieee80211/phy0/ath9k/spectral_scan0 > /tmp/fft_results
  echo disable > /sys/kernel/debug/ieee80211/phy0/ath9k/spectral_scan_ctl

For ath10k/AR98xx based cards, use:

.. code-block:: bash

  ip link set dev wlan0 up
  echo background > /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan_ctl
  echo trigger > /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan_ctl
  iw dev wlan0 scan
  echo disable > /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan_ctl
  cat /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan0 > /tmp/fft_results

For ath11k based cards, use:

.. code-block:: bash

  ip link set dev wlan0 up
  echo background > /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan_ctl
  echo trigger > /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan_ctl
  iw dev wlan0 scan
  echo disable > /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan_ctl
  cat /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan0 > /tmp/fft_results

There are some recorded samples in the "samples" directory to try it
out without actual hardware.

The ``*_tones.dump`` samples were recorded with a single test tone (CW) from
an AntSDR E200 about 20 cm away from the receiving antenna. They can be used
to check the frequencies which are assigned to the FFT bins. Each file
contains 4 samples per tone, sorted by the frequency of the tone:

================================ ========== ========== ====================================
File                             Channel    Center     Tones (MHz relative to the center)
================================ ========== ========== ====================================
ar9287_ch11_noht_tones.dump      11 (NOHT)  2462 MHz   -9, -6, -3, -1, +1, +3, +6, +9
ar9287_ch11_ht20_tones.dump      11 (HT20)  2462 MHz   -9, -6, -3, -1, +1, +3, +6, +9
ar9287_ch11_ht40minus_tones.dump 11 (HT40-) 2452 MHz   -19, -14.5, -11, -7, -3, -1, +1, +3,
                                                       +7, +11, +15, +19
ar9287_ch7_ht40plus_tones.dump   7 (HT40+)  2452 MHz   -19, -15, -11, -7, -3, -1, +1, +3,
                                                       +7, +11, +15, +19
================================ ========== ========== ====================================

The two halves of an HT20/40 sample are scaled with their own rssi. A tone
close to the center can therefore look weaker than the bins of the other
half (for example -1 MHz in ar9287_ch11_ht40minus_tones.dump).

To view the FFT results, use:

.. code-block:: bash

  ./fft_eval_sdl /tmp/fft_results

Navigate through the currently selected datasets using the arrow keys (left
and right). Scroll through the spectrum using the Page Up/Down keys.

To convert the FFT results to JSON, use:

.. code-block:: bash

  ./fft_eval_json /tmp/fft_results > /tmp/fft_results.json


LICENSE
=======

Read the GPL v2 file 'LICENSES/GPL-2.0-only.txt'.

AUTHOR
======

This software has been written by Simon Wunderlich <sw@simonwunderlich.de>
for Fraunhofer-Gesellschaft zur Foerderung der angewandten Forschung e.V.
and then adapted (stripped SDL code) by Gui Iribarren <gui@altermundi.net>
and integrated as part of the codebase by Nico Pace <nicopace@altermundi.net>
