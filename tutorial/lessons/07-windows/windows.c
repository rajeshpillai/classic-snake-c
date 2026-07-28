/*
 * Lesson 7 -- windows.
 *
 * Three bordered windows sharing the screen. TAB moves focus, the arrow
 * keys move the marker inside the focused window, and everything you draw
 * is clipped to that window automatically.
 *
 * 'q' quits.
 */

#include <ncurses.h>

#define NWIN 3

typedef struct {
  WINDOW *win;          /* the outer window, including its border */
  const char *title;
  int cy, cx;           /* marker position, in window-relative coords */
} Pane;

/* Redraw one pane from scratch. Note every call is a w-variant taking the
 * window, and every coordinate is relative to the window's own top-left. */
static void draw_pane(Pane *p, int focused) {
  werase(p->win);

  /* The border eats the outermost ring of cells, so the usable interior
   * is (1,1) to (h-2, w-2). */
  if (focused) wattron(p->win, A_BOLD);
  box(p->win, 0, 0);
  mvwprintw(p->win, 0, 2, " %s%s ", p->title, focused ? " *" : "");
  if (focused) wattroff(p->win, A_BOLD);

  int h, w;
  getmaxyx(p->win, h, w);
  mvwprintw(p->win, 1, 2, "%d x %d interior", h - 2, w - 2);
  mvwprintw(p->win, 2, 2, "marker at %d,%d", p->cy, p->cx);

  mvwaddch(p->win, p->cy, p->cx, '@' | A_REVERSE);

  /* This deliberately writes past the right edge. Curses clips it to the
   * window instead of spilling into the neighbour -- that clipping is the
   * main reason to use windows at all. */
  mvwaddstr(p->win, h - 2, 2, "this long line is clipped at the border");

  wrefresh(p->win);
}

int main(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  int height, width;
  getmaxyx(stdscr, height, width);

  if (height < 16 || width < 60) {
    endwin();
    fprintf(stderr, "need at least 60x16, got %dx%d\n", width, height);
    return 1;
  }

  mvaddstr(0, 2, "Windows -- TAB cycles focus, arrows move the marker, q quits.");
  mvaddstr(1, 2, "Each window clips its own drawing. stdscr is a window too.");
  refresh();   /* refresh stdscr FIRST, before drawing the windows over it */

  /* newwin(rows, cols, begin_y, begin_x) -- size first, then position,
   * and both in (y, x) order like everything else. */
  int wh = (height - 4) / 2;
  Pane panes[NWIN] = {
    { newwin(wh, width / 2 - 2, 3, 2),                "left top",  1, 1 },
    { newwin(wh, width / 2 - 2, 3, width / 2 + 1),    "right top", 1, 1 },
    { newwin(wh, width - 4,     3 + wh, 2),           "bottom",    1, 1 },
  };

  int focus = 0;
  for (int i = 0; i < NWIN; i++) draw_pane(&panes[i], i == focus);

  int ch;
  while ((ch = getch()) != 'q') {
    Pane *p = &panes[focus];
    int h, w;
    getmaxyx(p->win, h, w);

    switch (ch) {
      case '\t':
        focus = (focus + 1) % NWIN;
        /* Repaint both the old and new focus so the bold title updates. */
        for (int i = 0; i < NWIN; i++) draw_pane(&panes[i], i == focus);
        continue;

      /* Clamp to the interior so the marker never eats the border. */
      case KEY_UP:    if (p->cy > 1)     p->cy--; break;
      case KEY_DOWN:  if (p->cy < h - 3) p->cy++; break;
      case KEY_LEFT:  if (p->cx > 1)     p->cx--; break;
      case KEY_RIGHT: if (p->cx < w - 2) p->cx++; break;
      default: continue;
    }

    draw_pane(p, 1);
  }

  /* Windows are heap-allocated. Free them before endwin(). */
  for (int i = 0; i < NWIN; i++) delwin(panes[i].win);

  endwin();
  return 0;
}
