SRC=$(wildcard *.c)
TARGET=$(patsubst %.c,%,$(SRC))

CC=gcc
CFLAGS=-g2 -gdwarf-2

LIBPATH=
LIBS=
INCLUDE=

all:$(TARGET)

$(TARGET):%:%.c
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDE) $(LIBPATH) $(LIBS)

info:
	echo $(SRC)
	echo $(TARGET)

clean:
	rm -rf $(TARGET)