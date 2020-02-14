#!/usr/bin/make -f
# -*- makefile -*-
# SPDX-License-Identifier: GPL-2.0-only
# SPDX-FileCopyrightText: 2008-2019, Sven Eckelmann <sven@narfation.org>
# SPDX-FileCopyrightText: 2012-2014, Simon Wunderlich <sw@simonwunderlich.de>
# SPDX-FileCopyrightText: 2013, Janusz Dziedzic <janusz.dziedzic@tieto.com>
# SPDX-FileCopyrightText: 2015, Adrian Chadd <adrian@freebsd.org>
# SPDX-FileCopyrightText: 2015, Steven Pease <spease@suitabletech.com>
# SPDX-FileCopyrightText: 2017, Nicolas Pace <nicopace@gmail.com>

# add a new binary and allow to disable/enable it using the CONFIG_$binaryname
# make parameter
# only allow as value y (enable) and n (disable)
define add_command
  CONFIG_$(1):=$(2)
  ifneq ($$(CONFIG_$(1)),y)
    ifneq ($$(CONFIG_$(1)),n)
      $$(warning invalid value for parameter CONFIG_$(1): $$(CONFIG_$(1)))
    endif
  endif

  obj-$$(CONFIG_$(1)) += $(1)
endef # add_command

# using the make parameter CONFIG_* (e.g. CONFIG_fft_eval_sdl) with the value
# 'y' enables the related feature and 'n' disables it
$(eval $(call add_command,fft_eval_sdl,y))
fft_eval_sdl-y += fft_eval.o
fft_eval_sdl-y += fft_eval_sdl.o

$(eval $(call add_command,fft_eval_json,y))
fft_eval_json-y += fft_eval.o
fft_eval_json-y += fft_eval_json.o

$(eval $(call add_command,fft_eval_rx_power,y))
fft_eval_rx_power-y += fft_eval.o
fft_eval_rx_power-y += fft_eval_rx_power.o

$(eval $(call add_command,fft_eval_rtl_power_fftw,y))
fft_eval_rtl_power_fftw-y += fft_eval.o
fft_eval_rtl_power_fftw-y += fft_eval_rtl_power_fftw.o

# fft_eval flags and options
CFLAGS += -Wall -W -std=gnu99 -fno-strict-aliasing -MD -MP
CPPFLAGS += -D_DEFAULT_SOURCE
LDLIBS += -lm

# disable verbose output
ifneq ($(findstring $(MAKEFLAGS),s),s)
ifndef V
	Q_CC = @echo '   ' CC $@;
	Q_LD = @echo '   ' LD $@;
	export Q_CC
	export Q_LD
endif
endif

ifeq ($(CONFIG_fft_eval_sdl),y)
  # test for presence of SDL
  ifeq ($(origin SDL_CFLAGS) $(origin SDL_LDLIBS), undefined undefined)
    SDL2_CONFIG = $(CROSS)sdl2-config
    ifeq ($(shell which $(SDL2_CONFIG) 2>/dev/null),)
      $(error No SDL development libraries found!)
    endif
    SDL_CFLAGS  += $(shell $(SDL2_CONFIG) --cflags)
    SDL_LDLIBS += $(shell $(SDL2_CONFIG) --libs)
  endif
  CFLAGS_fft_eval_sdl.o += $(SDL_CFLAGS)
  LDLIBS_fft_eval_sdl += $(SDL_LDLIBS)
  
  ifeq ($(origin PKG_CONFIG), undefined)
    PKG_CONFIG = $(CROSS)pkg-config
    ifeq ($(shell which $(PKG_CONFIG) 2>/dev/null),)
      $(error $(PKG_CONFIG) not found)
    endif
  endif
  
  ifeq ($(origin LIBSDL2TTF_CFLAGS) $(origin LIBSDL2TTF_LDLIBS), undefined undefined)
    LIBSDL2TTF_NAME ?= SDL2_ttf
    ifeq ($(shell $(PKG_CONFIG) --modversion $(LIBSDL2TTF_NAME) 2>/dev/null),)
      $(error No $(LIBSDL2TTF_NAME) development libraries found!)
    endif
    LIBSDL2TTF_CFLAGS += $(shell $(PKG_CONFIG) --cflags $(LIBSDL2TTF_NAME))
    LIBSDL2TTF_LDLIBS +=  $(shell $(PKG_CONFIG) --libs $(LIBSDL2TTF_NAME))
  endif
  CFLAGS_fft_eval_sdl.o += $(LIBSDL2TTF_CFLAGS)
  LDLIBS_fft_eval_sdl += $(LIBSDL2TTF_LDLIBS)
endif

CC = $(CROSS)gcc
RM ?= rm -f
INSTALL ?= install
MKDIR ?= mkdir -p
COMPILE.c = $(Q_CC)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
LINK.o = $(Q_LD)$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_ARCH)

# standard install paths
PREFIX = /usr/local
BINDIR = $(PREFIX)/sbin

# default target
all: $(obj-y)

# standard build rules
.SUFFIXES: .o .c
.c.o:
	$(COMPILE.c) $(CFLAGS_$(@)) -o $@ $<

$(obj-y):
	$(LINK.o) $^ $(LDLIBS) $(LDLIBS_$(@)) -o $@

clean:
	$(RM) $(BINARY_NAMES) $(OBJ) $(DEP) samples/*.test

install: $(obj-y)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(obj-y) $(DESTDIR)$(BINDIR)

ifeq ($(CONFIG_fft_eval_sdl),y)
test:: fft_eval_sdl
	set -e; \
	for i in $(wildcard samples/*.dump); do \
		echo $$i; \
		SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=disk $(TESTRUN_WRAPPER) ./fft_eval_sdl $$i; \
	done
endif

ifeq ($(CONFIG_fft_eval_json),y)
test:: fft_eval_json
	set -e; \
	for i in $(wildcard samples/*.dump); do \
		echo $$i; \
		$(TESTRUN_WRAPPER) ./fft_eval_json $$i > $$i.test; \
		cmp $$i.test $$i.json; \
	done
endif

ifeq ($(CONFIG_fft_eval_rx_power),y)
test:: fft_eval_rx_power
	set -e; \
	for i in $(wildcard samples/*.dump); do \
		echo $$i; \
		$(TESTRUN_WRAPPER) ./fft_eval_rx_power $$i > $$i.test; \
		cmp $$i.test $$i.csv; \
	done
endif

ifeq ($(CONFIG_fft_eval_rtl_power_fftw),y)
test:: fft_eval_rtl_power_fftw
	set -e; \
	for i in $(wildcard samples/*.dump); do \
		echo $$i; \
		$(TESTRUN_WRAPPER) ./fft_eval_rtl_power_fftw $$i > $$i.test; \
		cmp $$i.test $$i.fftw; \
	done
endif

# load dependencies
BINARY_NAMES = $(foreach binary,$(obj-y) $(obj-n), $(binary))
OBJ = $(foreach obj, $(BINARY_NAMES), $($(obj)-y))
DEP = $(OBJ:.o=.d)
-include $(DEP)

# add dependency of binary to object
define binary_dependency
$(1): $(2)
endef
$(foreach binary, $(obj-y), $(eval $(call binary_dependency, $(binary), $($(binary)-y))))

.PHONY: all clean install test
