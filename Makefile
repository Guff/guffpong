PREFIX     ?= /usr
INSTALLDIR := $(DESTDIR)$(PREFIX)

CC	?= gcc

PKGS := clutter-1.0
INCS := $(shell pkg-config --cflags $(PKGS))
LIBS := $(shell pkg-config --libs $(PKGS)) -lm

DEBUG    := -g -DDEBUG
CFLAGS   := -Wall -Wextra -std=gnu99 -I. $(INCS) $(CFLAGS) $(DEBUG)
CPPFLAGS := $(CPPFLAGS)
LDFLAGS  := $(LIBS) $(LDFLAGS)

SRCS  := $(wildcard *.c)
OBJS  := $(SRCS:.c=.o)


all: guffpong

debug: CFLAGS += $(DEBUG)
debug: all

.c.o:
	@echo $(CC) -c $< -o $@
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

guffpong: $(OBJS)
	@echo $(CC) -o $@ $(OBJS)
	@$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -rf guffpong $(OBJS)

