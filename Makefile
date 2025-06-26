CC = gcc
TARGET = ex2
SRCS = main.c
LIBS = -lmta_crypt -lmta_rand -lpthread

.PHONY: all clean

all:
	$(CC) -o $(TARGET) $(SRCS) $(LIBS)

clean:
	rm -f $(TARGET)
