/*
 * The curses subset ../snake.c needs, implemented against a cell buffer
 * that JavaScript renders to a canvas.
 *
 * The design goal was that snake.c must not change. Everything awkward
 * about running a blocking terminal game loop in a browser is absorbed
 * here:
 *
 *   - usleep()  becomes emscripten_sleep(), which under ASYNCIFY unwinds
 *               the C stack, returns to the browser event loop, and
 *               resumes where it left off. That is what keeps the page
 *               responsive while snake.c thinks it is blocking.
 *   - getch()   reads a queue that JS fills from keydown handlers.
 *   - refresh() hands the cell buffer to JS to paint.
 *
 * Build: emcc -Iweb -Dusleep=shim_usleep snake.c web/curses_shim.c -sASYNCIFY
 */

#include "ncurses.h"

#include <emscripten.h>
#include <stdio.h>
#include <string.h>

#define MAX_ROWS 60
#define MAX_COLS 200

/* The single "window". snake.c only ever passes this to keypad() and
 * nodelay(), so it needs no contents -- just a stable address. */
static struct WINDOW { int unused; } the_screen;
WINDOW *stdscr = (WINDOW *) &the_screen;

/* One uint32 per cell: char | pair<<8 | attrs<<16. Laid out row-major so
 * JS can walk it linearly. */
static unsigned int cells[MAX_ROWS * MAX_COLS];

static int rows = 24;
static int cols = 80;
static int cur_y, cur_x;
static int cur_attr;          /* attributes + colour pair currently active */
static int nodelay_mode;
static int colors_started;

/* --- the JS side ---------------------------------------------------- */

/* Paint. JS reads the buffer straight out of the wasm heap rather than
 * receiving a copy -- at 80x24 that is 1920 words per frame. */
EM_JS(void, js_render, (const unsigned int *buf, int r, int c), {
  if (Module.renderScreen) Module.renderScreen(buf, r, c);
});

/* Colour pairs live on the JS side because that is where they are turned
 * into CSS colours. */
EM_JS(void, js_set_pair, (int pair, int fg, int bg), {
  if (Module.setPair) Module.setPair(pair, fg, bg);
});

EM_JS(void, js_beep, (void), {
  if (Module.beep) Module.beep();
});

/* Let the page choose the terminal size before main() runs. */
EM_JS(int, js_rows, (void), { return (Module.termRows | 0) || 24; });
EM_JS(int, js_cols, (void), { return (Module.termCols | 0) || 80; });

/* --- input queue ---------------------------------------------------- */

#define QUEUE_SIZE 64
static int key_queue[QUEUE_SIZE];
static int q_head, q_tail;

/* Called from JS on keydown. Exported so the page can reach it. */
EMSCRIPTEN_KEEPALIVE
void curses_push_key(int key) {
  int next = (q_tail + 1) % QUEUE_SIZE;
  if (next == q_head) return;        /* full -- drop, don't overwrite */
  key_queue[q_tail] = key;
  q_tail = next;
}

static int queue_pop(void) {
  if (q_head == q_tail) return ERR;
  int k = key_queue[q_head];
  q_head = (q_head + 1) % QUEUE_SIZE;
  return k;
}

/* --- setup / teardown ------------------------------------------------ */

WINDOW *initscr(void) {
  rows = js_rows();
  cols = js_cols();
  if (rows > MAX_ROWS) rows = MAX_ROWS;
  if (cols > MAX_COLS) cols = MAX_COLS;
  erase();
  return stdscr;
}

int endwin(void)                    { return OK; }
int cbreak(void)                    { return OK; }
int nocbreak(void)                  { return OK; }
int noecho(void)                    { return OK; }
int echo(void)                      { return OK; }
int curs_set(int v)                 { (void) v; return OK; }
int keypad(WINDOW *w, int bf)       { (void) w; (void) bf; return OK; }

int nodelay(WINDOW *w, int bf) {
  (void) w;
  nodelay_mode = bf;
  return OK;
}

int timeout(int delay) {
  nodelay_mode = (delay == 0);
  return OK;
}

int curses_rows(void) { return rows; }
int curses_cols(void) { return cols; }

/* --- colour ----------------------------------------------------------- */

/* Always true: the canvas can obviously do colour. This is what makes
 * snake.c take its coloured path rather than its monochrome fallback. */
int has_colors(void)       { return 1; }
int use_default_colors(void) { return OK; }

int start_color(void) {
  colors_started = 1;
  return OK;
}

int init_pair(short pair, short f, short b) {
  if (!colors_started) return ERR;
  js_set_pair(pair, f, b);
  return OK;
}

/* --- attributes -------------------------------------------------------- */

int attron(int a)  { cur_attr |= a;  return OK; }
int attroff(int a) { cur_attr &= ~a; return OK; }
int attrset(int a) { cur_attr = a;   return OK; }

/* --- output ------------------------------------------------------------ */

int erase(void) {
  /* Blank cells keep no attributes, matching curses' behaviour of
   * erasing to the background. */
  for (int i = 0; i < rows * cols; i++) cells[i] = (unsigned int) ' ';
  cur_y = cur_x = 0;
  return OK;
}

int clear(void) { return erase(); }

int move(int y, int x) {
  if (y < 0 || y >= rows || x < 0 || x >= cols) return ERR;
  cur_y = y;
  cur_x = x;
  return OK;
}

int addch(unsigned int ch) {
  /* Silently ignore out-of-bounds writes, exactly as curses does -- this
   * is what makes a coordinate bug show up as missing output rather than
   * a crash, in the browser as well as the terminal. */
  if (cur_y < 0 || cur_y >= rows || cur_x < 0 || cur_x >= cols) return ERR;

  /* A chtype may carry its own attributes (e.g. 'X' | A_REVERSE); they
   * combine with whatever attron() has set, which is why this is an OR
   * of the two rather than a choice between them. */
  unsigned int styling = (ch | (unsigned int) cur_attr) & 0xffff00u;
  cells[cur_y * cols + cur_x] = (ch & 0xffu) | styling;

  if (++cur_x >= cols) { cur_x = 0; if (++cur_y >= rows) cur_y = rows - 1; }
  return OK;
}

int mvaddch(int y, int x, unsigned int ch) {
  if (move(y, x) == ERR) return ERR;
  return addch(ch);
}

int addstr(const char *s) {
  for (; *s; s++) {
    if (*s == '\n') { cur_x = 0; if (++cur_y >= rows) break; continue; }
    addch((unsigned char) *s);
  }
  return OK;
}

int mvaddstr(int y, int x, const char *s) {
  if (move(y, x) == ERR) return ERR;
  return addstr(s);
}

static int vprintw_at(const char *fmt, va_list ap) {
  char buf[512];
  vsnprintf(buf, sizeof buf, fmt, ap);
  return addstr(buf);
}

int printw(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vprintw_at(fmt, ap);
  va_end(ap);
  return r;
}

int mvprintw(int y, int x, const char *fmt, ...) {
  if (move(y, x) == ERR) return ERR;
  va_list ap;
  va_start(ap, fmt);
  int r = vprintw_at(fmt, ap);
  va_end(ap);
  return r;
}

int refresh(void) {
  js_render(cells, rows, cols);
  return OK;
}

/* --- input -------------------------------------------------------------- */

int getch(void) {
  int k = queue_pop();
  if (k != ERR) return k;
  if (nodelay_mode) return ERR;

  /* Blocking read. We cannot actually block the browser's main thread,
   * so yield in short slices until a key shows up. ASYNCIFY makes this
   * look like a normal blocking call to the caller. */
  for (;;) {
    emscripten_sleep(16);
    k = queue_pop();
    if (k != ERR) return k;
  }
}

int flushinp(void) {
  q_head = q_tail = 0;
  return OK;
}

int beep(void)  { js_beep(); return OK; }
int flash(void) { return OK; }

int napms(int ms) {
  emscripten_sleep(ms < 0 ? 0 : ms);
  return OK;
}

/* Stands in for unistd.h's usleep via -Dusleep=shim_usleep. Rounds up so
 * a sub-millisecond sleep still yields rather than becoming a no-op. */
int shim_usleep(unsigned int usec) {
  unsigned int ms = usec / 1000u;
  emscripten_sleep(ms == 0 ? 1 : ms);
  return 0;
}
