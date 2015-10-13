BIN=fft_eval
OBJ=fft_eval.o
LIBS=$(shell pkg-config sdl --libs) -lSDL_ttf -lm
CC=cc $(shell pkg-config sdl --cflags) -std=c99 -O2 -Wall
LD=cc
.SUFFIXES: .o .c
.c.o:
	$(CC) -c -o $@ $<

default:	all
all:	$(BIN)

$(BIN): $(OBJ)
	$(LD) -o $@ $(OBJ) $(LIBS)

clean:
	rm -rf $(BIN) $(OBJ)

