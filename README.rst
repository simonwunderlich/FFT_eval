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
on your system, as well as make and gcc.

Just type make to build the program:

.. code-block:: bash

  $ make
  gcc -O2 -Wall -pedantic -c -o fft_eval.o fft_eval.c
  gcc -lSDL2 -lSDL2_ttf -o fft_eval fft_eval.o
  $

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
  cat /sys/kernel/debug/ieee80211/phy0/ath9k/spectral_scan0 > samples
  echo disable > /sys/kernel/debug/ieee80211/phy0/ath9k/spectral_scan_ctl

For ath10k/AR98xx based cards, use:

.. code-block:: bash

  ip link set dev wlan0 up
  echo background > /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan_ctl
  echo trigger > /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan_ctl
  iw dev wlan0 scan
  echo disable > /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan_ctl
  cat /sys/kernel/debug/ieee80211/phy0/ath10k/spectral_scan0 > samples

For ath11k based cards, use:

.. code-block:: bash

  ip link set dev wlan0 up
  echo background > /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan_ctl
  echo trigger > /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan_ctl
  iw dev wlan0 scan
  echo disable > /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan_ctl
  cat /sys/kernel/debug/ieee80211/phy0/ath11k/spectral_scan0 > samples

There are some recorded samples in the "samples" directory to try it
out without actual hardware.

To view the FFT results, use:

.. code-block:: bash

  ./fft_eval_sdl /tmp/fft_results

Navigate through the currently selected datasets using the arrow keys (left
and right). Scroll through the spectrum using the Page Up/Down keys.


LICENSE
=======

Read the GPL v2 file 'COPYING'.

AUTHOR
======

This software has been written by Simon Wunderlich <sw@simonwunderlich.de>
for Fraunhofer-Gesellschaft zur Foerderung der angewandten Forschung e.V.
and then adapted (stripped SDL code) by Gui Iribarren <gui@altermundi.net>
and integrated as part of the codebase by Nico Pace <nicopace@altermundi.net>
