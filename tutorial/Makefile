# Tutorial build. Every .c under lessons/*/ becomes a binary beside its source.
#
#   make          build everything
#   make list     show what got built
#   make clean    remove the binaries
#   make 03       build and run the first program in lesson 03
#
# We link panel/menu/form for every example even though only lessons 10 and 14
# need them -- the linker's --as-needed default drops the ones a given program
# doesn't reference. Each lesson.md states the libraries it actually requires.

CC      ?= gcc
CFLAGS  ?= -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -O2
CURSES  := $(shell pkg-config --libs ncurses 2>/dev/null || echo -lncurses)
LDLIBS  := -lform -lmenu -lpanel $(CURSES)

SRCS := $(sort $(wildcard lessons/*/*.c))
BINS := $(SRCS:.c=)

.PHONY: all list clean

all: $(BINS)

# One rule covers every lesson -- they're all single-file programs.
lessons/%: lessons/%.c
	@$(CC) $(CFLAGS) -o $@ $< $(LDLIBS) && echo "  CC  $@"

list: all
	@echo
	@echo "Built $(words $(BINS)) programs:"
	@printf '  ./%s\n' $(BINS)
	@echo

clean:
	@$(RM) $(BINS)
	@echo "cleaned $(words $(BINS)) binaries"

LESSONS := 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15

# Shorthand: `make 07` builds everything, then runs lesson 07's first program.
.PHONY: $(LESSONS)
$(LESSONS): all
	@bin=$$(ls lessons/$@-*/* | grep -v '\.c$$' | head -1); \
	 echo "running $$bin"; exec ./$$bin
