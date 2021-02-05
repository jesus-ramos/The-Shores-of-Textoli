CC	   = gcc
CFLAGS = -Wall
LD	   = $(CC)

BIN = sot
SRCS = $(wildcard *.c)
OBJS = ${SRCS:.c=.o}

.SUFFIXES:
.SUFFIXES: .o .c

all : $(BIN)

.c.o :
	$(CC) $(CFLAGS) -c $<

$(BIN) : $(OBJS)
	$(LD) -o $@ $(OBJS)

.PHONY : clean

clean:
	-rm *.o $(BIN) *.d

CFLAGS += -MMD
-include $(OBJS:.o=.d)
