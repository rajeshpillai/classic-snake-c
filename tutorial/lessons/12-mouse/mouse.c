/*
 * Lesson 12 -- the mouse.
 *
 * A tiny paint program. Click and drag to draw, right-click to erase,
 * the scroll wheel changes the brush character. The event log on the
 * right shows exactly what curses reports.
 *
 * 'c' clears, 'q' quits.
 */

#include <ncurses.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define LOGLINES 12

static char log_line[LOGLINES][80];
static int log_count;

static void log_event(const char *fmt, ...) {
  /* Scroll the buffer up by one and append at the bottom. */
  if (log_count == LOGLINES) {
    memmove(log_line[0], log_line[1], sizeof(log_line) - sizeof(log_line[0]));
    log_count--;
  }
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(log_line[log_count], sizeof(log_line[0]), fmt, ap);
  va_end(ap);
  log_count++;
}

int main(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);      /* required -- mouse events arrive as KEY_MOUSE */
  curs_set(0);

  int height, width;
  getmaxyx(stdscr, height, width);
  if (height < 18 || width < 70) {
    endwin();
    fprintf(stderr, "need at least 70x18, got %dx%d\n", width, height);
    return 1;
  }

  /* Ask the terminal for mouse reporting. The return value is the mask
   * actually granted, which may be less than you asked for -- always
   * check it rather than assuming. */
  mmask_t wanted = ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION;
  mmask_t granted = 0;
  mousemask(wanted, &granted);

  if (granted == 0) {
    mvaddstr(2, 2, "This terminal reports no mouse support. Press a key.");
    refresh();
    getch();
    endwin();
    return 1;
  }

  /* Terminals send a mouse-down and mouse-up separately, and synthesise a
   * "click" only if they arrive close together. Setting the interval to 0
   * disables click detection entirely and gives you the raw press/release
   * pairs -- which is what you want for dragging. */
  mouseinterval(0);

  int use_color = has_colors();
  if (use_color) {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  }

  const int canvas_w = width - 34;
  const char *brushes = "#*+.oO@";
  int brush = 0;
  int painting = 0;

  /* The canvas lives in its own window so drawing is clipped for free. */
  WINDOW *canvas = newwin(height - 5, canvas_w, 3, 2);
  WINDOW *side   = newwin(height - 5, 28, 3, canvas_w + 4);

  int ch = 0;
  do {
    if (ch == KEY_MOUSE) {
      MEVENT ev;
      /* getmouse() pulls the event details out of the queue. It must be
       * called immediately after KEY_MOUSE, and it can fail. */
      if (getmouse(&ev) == OK) {
        /* ev.y and ev.x are SCREEN coordinates. Translate them into the
         * window's own space before using them. wmouse_trafo() does this
         * and tells you whether the click was inside at all. */
        int wy = ev.y, wx = ev.x;
        int inside = wmouse_trafo(canvas, &wy, &wx, FALSE);

        const char *what = "";
        if (ev.bstate & BUTTON1_PRESSED)       { what = "BUTTON1_PRESSED";  painting = 1; }
        else if (ev.bstate & BUTTON1_RELEASED) { what = "BUTTON1_RELEASED"; painting = 0; }
        else if (ev.bstate & BUTTON3_PRESSED)  { what = "BUTTON3_PRESSED";  painting = 2; }
        else if (ev.bstate & BUTTON3_RELEASED) { what = "BUTTON3_RELEASED"; painting = 0; }
        else if (ev.bstate & BUTTON4_PRESSED)  { what = "BUTTON4 (wheel up)";
                                                 brush = (brush + 1) % (int) strlen(brushes); }
        else if (ev.bstate & BUTTON5_PRESSED)  { what = "BUTTON5 (wheel down)";
                                                 brush = (brush + (int) strlen(brushes) - 1)
                                                         % (int) strlen(brushes); }
        else if (ev.bstate & REPORT_MOUSE_POSITION) what = "MOUSE_POSITION (drag)";

        log_event("%-20s y=%2d x=%2d %s", what, ev.y, ev.x,
                  inside ? "in" : "out");

        if (inside && painting == 1) {
          if (use_color) wattron(canvas, COLOR_PAIR(1));
          mvwaddch(canvas, wy, wx, brushes[brush]);
          if (use_color) wattroff(canvas, COLOR_PAIR(1));
        } else if (inside && painting == 2) {
          mvwaddch(canvas, wy, wx, ' ');
        }
      }
    } else if (ch == 'c') {
      werase(canvas);
      log_event("canvas cleared");
    }

    erase();
    mvprintw(1, 2, "Mouse paint -- drag to draw, right-drag to erase, "
                   "wheel changes brush");
    mvprintw(height - 1, 2, "brush '%c'   c clears   q quits   "
                            "granted mask 0x%lx",
             brushes[brush], (unsigned long) granted);
    wnoutrefresh(stdscr);

    box(canvas, 0, 0);
    mvwprintw(canvas, 0, 2, " canvas ");
    wnoutrefresh(canvas);

    werase(side);
    box(side, 0, 0);
    mvwprintw(side, 0, 2, " events ");
    for (int i = 0; i < log_count; i++) {
      if (use_color) wattron(side, COLOR_PAIR(2));
      mvwaddnstr(side, i + 1, 1, log_line[i], 26);
      if (use_color) wattroff(side, COLOR_PAIR(2));
    }
    wnoutrefresh(side);

    doupdate();
  } while ((ch = getch()) != 'q');

  /* Hand mouse reporting back to the terminal, or the user's shell will
   * keep receiving escape sequences when they click. */
  mousemask(0, NULL);

  delwin(canvas);
  delwin(side);
  endwin();
  return 0;
}
