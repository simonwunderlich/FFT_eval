#!/usr/bin/make -f
# -*- makefile -*-

# batctl build
BINARY_NAME = fft_eval
OBJ = fft_eval.o

# batctl flags and options
CFLAGS += -Wall -W -std=gnu99 -fno-strict-aliasing -MD -MP
CPPFLAGS += -D_GNU_SOURCE
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

# test for presence of SDL
ifeq ($(origin SDL_CFLAGS) $(origin SDL_LDLIBS), undefined undefined)
  SDL_CONFIG = sdl-config
  ifeq ($(shell which $(SDL_CONFIG) 2>/dev/null),)
    $(error No SDL development libraries found!)
  endif
  SDL_CFLAGS  += $(shell $(SDL_CONFIG) --cflags)
  SDL_LDLIBS += $(shell $(SDL_CONFIG) --libs)
endif
CFLAGS += $(SDL_CFLAGS)
LDLIBS += $(SDL_LDLIBS)

ifeq ($(origin PKG_CONFIG), undefined)
  PKG_CONFIG = pkg-config
  ifeq ($(shell which $(PKG_CONFIG) 2>/dev/null),)
    $(error $(PKG_CONFIG) not found)
  endif
endif

ifeq ($(origin LIBSDLTTF_CFLAGS) $(origin LIBSDLTTF_LDLIBS), undefined undefined)
  LIBSDLTTF_NAME ?= SDL_ttf
  ifeq ($(shell $(PKG_CONFIG) --modversion $(LIBSDLTTF_NAME) 2>/dev/null),)
    $(error No $(LIBSDLTTF_NAME) development libraries found!)
  endif
  LIBSDLTTF_CFLAGS += $(shell $(PKG_CONFIG) --cflags $(LIBSDLTTF_NAME))
  LIBSDLTTF_LDLIBS +=  $(shell $(PKG_CONFIG) --libs $(LIBSDLTTF_NAME))
endif
CFLAGS += $(LIBSDLTTF_CFLAGS)
LDLIBS += $(LIBSDLTTF_LDLIBS)

CC ?= cc
RM ?= rm -f
INSTALL ?= install
MKDIR ?= mkdir -p
COMPILE.c = $(Q_CC)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
LINK.o = $(Q_LD)$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_ARCH)

# standard install paths
PREFIX = /usr/local
BINDIR = $(PREFIX)/sbin

# default target
all: $(BINARY_NAME)

# standard build rules
.SUFFIXES: .o .c
.c.o:
	$(COMPILE.c) -o $@ $<

$(BINARY_NAME): $(OBJ)
	$(LINK.o) $^ $(LDLIBS) -o $@

clean:
	$(RM) $(BINARY_NAME) $(OBJ) $(OBJ_BISECT) $(DEP)

install: $(BINARY_NAME)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(BINARY_NAME) $(DESTDIR)$(BINDIR)

# load dependencies
DEP = $(OBJ:.o=.d) $(OBJ_BISECT:.o=.d)
-include $(DEP)

.PHONY: all clean install
