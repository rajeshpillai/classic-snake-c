/*
 * A drop-in stand-in for <ncurses.h>, covering exactly the subset that
 * ../snake.c uses.
 *
 * The web build puts this directory first on the include path, so
 * `#include <ncurses.h>` in snake.c resolves here instead of to the real
 * library. snake.c itself is compiled byte-for-byte identically for the
 * terminal and for the browser -- the only difference is -Iweb and a
 * -Dusleep on the command line.
 *
 * See curses_shim.c for the implementation and the reasoning behind the
 * cell packing.
 */

#ifndef WEB_NCURSES_SHIM_H
#define WEB_NCURSES_SHIM_H

#include <stdarg.h>

/* Opaque, because the shim only ever has one window: the whole screen.
 * snake.c passes stdscr around but never dereferences it. */
typedef struct WINDOW WINDOW;
extern WINDOW *stdscr;

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#define OK   0
#define ERR (-1)

/* Same numbering as real curses, so the values snake.c stores in
 * init_pair() calls mean the same thing here. */
#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

/*
 * A cell is packed into one uint32 so the renderer can read the whole
 * screen out of the wasm heap in a single typed-array pass:
 *
 *   bits  0-7   character
 *   bits  8-15  colour pair
 *   bits 16-23  attributes
 *
 * COLOR_PAIR(n) therefore shifts into the pair byte, exactly as real
 * curses ORs a pair into a chtype.
 */
#define A_NORMAL    0x000000
#define A_BOLD      0x010000
#define A_REVERSE   0x020000
#define A_UNDERLINE 0x040000
#define A_DIM       0x080000
#define A_BLINK     0x100000

#define COLOR_PAIR(n) (((n) & 0xff) << 8)

/* The standard curses keycodes. Real values, so anything comparing
 * against them behaves identically. */
#define KEY_DOWN   0402
#define KEY_UP     0403
#define KEY_LEFT   0404
#define KEY_RIGHT  0405
#define KEY_HOME   0406
#define KEY_BACKSPACE 0407
#define KEY_NPAGE  0522
#define KEY_PPAGE  0523
#define KEY_RESIZE 0632

/* --- setup / teardown --------------------------------------------- */
WINDOW *initscr(void);
int endwin(void);
int cbreak(void);
int nocbreak(void);
int noecho(void);
int echo(void);
int keypad(WINDOW *win, int bf);
int curs_set(int visibility);
int nodelay(WINDOW *win, int bf);
/* Declared as a real function rather than a macro -- a `#define timeout`
 * would collide with struct fields of that name in system headers. */
int timeout(int delay);

/* --- colour -------------------------------------------------------- */
int has_colors(void);
int start_color(void);
int init_pair(short pair, short f, short b);
int use_default_colors(void);

/* --- attributes ---------------------------------------------------- */
int attron(int attrs);
int attroff(int attrs);
int attrset(int attrs);

/* --- output -------------------------------------------------------- */
int erase(void);
int clear(void);
int refresh(void);
int move(int y, int x);
int addch(unsigned int ch);
int mvaddch(int y, int x, unsigned int ch);
int addstr(const char *s);
int mvaddstr(int y, int x, const char *s);
int printw(const char *fmt, ...);
int mvprintw(int y, int x, const char *fmt, ...);

/* --- input --------------------------------------------------------- */
int getch(void);
int flushinp(void);
int beep(void);
int flash(void);
int napms(int ms);

/* Screen size. A macro that assigns, like the real one -- so the
 * `getmaxyx(stdscr, height, width)` in snake.c compiles unchanged, with
 * no ampersands. */
int curses_rows(void);
int curses_cols(void);
#define getmaxyx(win, y, x) \
    do { (void)(win); (y) = curses_rows(); (x) = curses_cols(); } while (0)

/* The replacement for unistd.h's usleep(). The web build compiles with
 * -Dusleep=shim_usleep so the game's pacing yields to the browser event
 * loop instead of blocking the main thread. */
int shim_usleep(unsigned int usec);

#endif /* WEB_NCURSES_SHIM_H */
