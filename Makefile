BIN=fft_eval
OBJ=fft_eval.o
LIBS=-lSDL -lSDL_ttf
CC=gcc -O2 -Wall -pedantic
LD=gcc
.SUFFIXES: .o .c
.c.o:
	$(CC) -c -o $@ $<

default:	all
all:	$(BIN)

$(BIN): $(OBJ)
	$(LD) $(LIBS) -o $@ $(OBJ)

clean:
	rm -rf $(BIN) $(OBJ)

