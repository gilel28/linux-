# Makefile – קומפילציה בלבד של קבצי C

CC      = gcc
CFLAGS  = -Wall
LDFLAGS = -lmta_crypt -lmta_rand

ENCRYPTER_SRC = my_encrypter.c
ENCRYPTER_BIN = my_encrypter

DECRYPTER_SRC = my_decrypter.c
DECRYPTER_BIN = my_decrypter

.PHONY: all clean

all: $(ENCRYPTER_BIN) $(DECRYPTER_BIN)

$(ENCRYPTER_BIN): $(ENCRYPTER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(DECRYPTER_BIN): $(DECRYPTER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(ENCRYPTER_BIN) $(DECRYPTER_BIN)
