# classic-snake -- `make` to build, `make run` to play, `make clean` to tidy up.

CC      ?= gcc
# _DEFAULT_SOURCE: strict -std=c11 hides POSIX declarations, and we want usleep().
CFLAGS  ?= -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -O2
LDLIBS  := $(shell pkg-config --libs ncurses 2>/dev/null || echo -lncurses)

PREFIX  ?= /usr/local
BINDIR  := $(PREFIX)/bin

TARGET  := snake
SRCS    := snake.c
OBJS    := $(SRCS:.c=.o)

.PHONY: all run debug clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Nothing fancy -- one translation unit, so the implicit .c.o rule is plenty.
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

# Unoptimised build with symbols and the sanitizers, for chasing bugs.
debug: CFLAGS := -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -g -O0 -fsanitize=address,undefined
debug: clean $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJS)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(TARGET)
