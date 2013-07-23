BIN=fft_eval
OBJ=fft_eval.o
LIBS=-lSDL -lSDL_ttf
CC=gcc -std=c99 -O2 -Wall -pedantic
LD=gcc
.SUFFIXES: .o .c
.c.o:
	$(CC) -c -o $@ $<

default:	all
all:	$(BIN)

$(BIN): $(OBJ)
	$(LD) -o $@ $(OBJ) $(LIBS)

clean:
	rm -rf $(BIN) $(OBJ)

