BIN=fft_eval
OBJ=fft_eval.o
LIBS=-L/usr/local/lib -lSDL -lSDL_ttf -lm
CC=cc -std=c99 -O2 -Wall -I/usr/local/include
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

