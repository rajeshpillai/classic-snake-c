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

# --- WebAssembly build -------------------------------------------------
# Compiles the SAME snake.c to wasm. The only differences are -Iweb, which
# makes <ncurses.h> resolve to our shim, and -Dusleep, which routes the
# game's pacing through emscripten_sleep(). snake.c itself is untouched.
EMCC     ?= emcc
WEB_OUT  := docs/index.html
WEB_SRCS := snake.c web/curses_shim.c

# SINGLE_FILE inlines the wasm as base64 so docs/ is one self-contained
# file -- no MIME configuration, and it opens over file:// as well as
# from GitHub Pages.
# ASYNCIFY is what lets snake.c keep its blocking while(running) loop:
# emscripten_sleep() unwinds to the browser event loop and resumes.
EMFLAGS := -O3 -Iweb -Dusleep=shim_usleep \
           -sASYNCIFY \
           -sASYNCIFY_STACK_SIZE=16384 \
           -sSINGLE_FILE=1 \
           -sEXPORTED_FUNCTIONS=_main,_curses_push_key \
           -sEXPORTED_RUNTIME_METHODS=HEAPU32,FS,IDBFS,ENV,addRunDependency,removeRunDependency \
           -sFORCE_FILESYSTEM=1 \
           -lidbfs.js \
           -sALLOW_MEMORY_GROWTH=1 \
           -sMODULARIZE=0 \
           -sEXIT_RUNTIME=1 \
           --shell-file web/shell.html

.PHONY: all run debug clean clean-web install uninstall web web-serve

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

# Needs the Emscripten SDK on PATH:  source ~/emsdk/emsdk_env.sh
web: $(WEB_OUT)

# web/find-emsdk.sh puts emcc on PATH if it isn't already, so this works in a
# fresh shell. It has to be sourced in the SAME shell as the compile -- make
# runs each recipe line in its own shell, hence the one long command.
$(WEB_OUT): $(WEB_SRCS) web/ncurses.h web/shell.html web/find-emsdk.sh
	@mkdir -p docs
	@bash -c '. web/find-emsdk.sh; \
	  command -v $(EMCC) >/dev/null || { \
	    echo "emcc not found -- install emsdk (see README) or set EMSDK=/path/to/emsdk"; \
	    exit 1; }; \
	  echo "$(EMCC) $(EMFLAGS) -o $@ $(WEB_SRCS)"; \
	  $(EMCC) $(EMFLAGS) -o $@ $(WEB_SRCS)'
	@echo "built $@ ($$(du -h $@ | cut -f1), self-contained)"

# GitHub Pages serves docs/ over HTTP; this mimics it locally.
web-serve: web
	@echo "http://localhost:8000/  (Ctrl-C to stop)"
	@cd docs && python3 -m http.server 8000

clean:
	$(RM) $(TARGET) $(OBJS)

clean-web:
	$(RM) $(WEB_OUT)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(TARGET)
