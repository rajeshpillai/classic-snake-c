/*
 * Lesson 6 -- drawing lines and boxes with the alternate character set.
 *
 * The snake game draws its border out of '#' characters. Curses can do
 * better: terminals carry a line-drawing character set, and the ACS_*
 * constants give you proper corners and junctions that work back to a
 * physical VT100.
 *
 * Any key quits.
 */

#include <ncurses.h>

int main(void) {
  initscr();
  cbreak();
  noecho();
  curs_set(0);

  int height, width;
  getmaxyx(stdscr, height, width);

  mvaddstr(0, 2, "Line drawing -- ASCII on the left, ACS on the right");

  /* --- The manual way, the way the snake game does it ---------------- */
  mvaddstr(2, 2, "hand-rolled with '#':");
  int t = 3, l = 2, b = 9, r = 24;
  for (int x = l; x <= r; x++) { mvaddch(t, x, '#'); mvaddch(b, x, '#'); }
  for (int y = t; y <= b; y++) { mvaddch(y, l, '#'); mvaddch(y, r, '#'); }
  mvaddstr(5, 6, "portable, ugly");

  /* --- ACS: the terminal's own line-drawing glyphs ------------------- */
  mvaddstr(2, 30, "same box with ACS_*:");
  t = 3; l = 30; b = 9; r = 52;
  mvaddch(t, l, ACS_ULCORNER);
  mvaddch(t, r, ACS_URCORNER);
  mvaddch(b, l, ACS_LLCORNER);
  mvaddch(b, r, ACS_LRCORNER);
  /* hline/vline repeat a character n times without a loop. */
  mvhline(t, l + 1, ACS_HLINE, r - l - 1);
  mvhline(b, l + 1, ACS_HLINE, r - l - 1);
  mvvline(t + 1, l, ACS_VLINE, b - t - 1);
  mvvline(t + 1, r, ACS_VLINE, b - t - 1);
  mvaddstr(5, 34, "proper corners");

  /* A T-junction, for splitting a box into panes. */
  mvaddch(t, l + 11, ACS_TTEE);
  mvaddch(b, l + 11, ACS_BTEE);
  mvvline(t + 1, l + 11, ACS_VLINE, b - t - 1);
  mvaddstr(7, 32, "tee'd");
  mvaddstr(7, 43, "pane");

  /* --- box(): the one-liner ------------------------------------------ */
  /* box() only works on a WINDOW, so make one. More on windows next. */
  WINDOW *w = newwin(7, 24, 3, 58);
  box(w, 0, 0);          /* 0,0 means "use ACS_VLINE and ACS_HLINE" */
  mvwaddstr(w, 2, 2, "box(win, 0, 0)");
  mvwaddstr(w, 3, 2, "one call, done");
  wrefresh(w);
  mvaddstr(2, 58, "box() on a window:");

  /* --- The ACS catalogue --------------------------------------------- */
  int row = 11;
  mvaddstr(row++, 2, "The alternate character set:");

  /* NOT static. ACS_ULCORNER and friends are macros for acs_map['l'] --
   * a runtime array lookup filled in by initscr(), not a compile-time
   * constant. A `static` table of them fails to compile, and a global one
   * would be initialised before initscr() ever ran. */
  const struct { chtype ch; const char *name; } acs[] = {
    { ACS_ULCORNER, "ULCORNER" }, { ACS_URCORNER, "URCORNER" },
    { ACS_LLCORNER, "LLCORNER" }, { ACS_LRCORNER, "LRCORNER" },
    { ACS_LTEE,     "LTEE"     }, { ACS_RTEE,     "RTEE"     },
    { ACS_TTEE,     "TTEE"     }, { ACS_BTEE,     "BTEE"     },
    { ACS_HLINE,    "HLINE"    }, { ACS_VLINE,    "VLINE"    },
    { ACS_PLUS,     "PLUS"     }, { ACS_DIAMOND,  "DIAMOND"  },
    { ACS_CKBOARD,  "CKBOARD"  }, { ACS_DEGREE,   "DEGREE"   },
    { ACS_PLMINUS,  "PLMINUS"  }, { ACS_BULLET,   "BULLET"   },
    { ACS_LARROW,   "LARROW"   }, { ACS_RARROW,   "RARROW"   },
    { ACS_DARROW,   "DARROW"   }, { ACS_UARROW,   "UARROW"   },
    { ACS_BOARD,    "BOARD"    }, { ACS_BLOCK,    "BLOCK"    },
    { ACS_S1,       "S1"       }, { ACS_S9,       "S9"       },
  };

  int per_row = width / 14;
  if (per_row < 1) per_row = 1;
  for (unsigned i = 0; i < sizeof acs / sizeof acs[0]; i++) {
    int y = row + (int) i / per_row;
    int x = 2 + ((int) i % per_row) * 14;
    if (y >= height - 2) break;
    move(y, x);
    addch(acs[i].ch);
    printw(" ACS_%-8s", acs[i].name);
  }

  mvaddstr(height - 1, 2, "Press any key to quit.");
  refresh();
  wrefresh(w);      /* stdscr's refresh painted over the window; redraw it */
  getch();

  delwin(w);
  endwin();
  return 0;
}
