/*
 * Lesson 5 -- colour.
 *
 * Curses colour works through numbered *pairs* of (foreground, background)
 * rather than by naming a colour at draw time. You define the pairs up
 * front, then switch between them with COLOR_PAIR(n), which is just another
 * attribute.
 *
 * Any key quits.
 */

#include <ncurses.h>

static const char *color_name[] = {
  "BLACK", "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "CYAN", "WHITE"
};

int main(void) {
  initscr();
  cbreak();
  noecho();
  curs_set(0);

  /* Always ask first. A terminal with TERM=vt100 has no colour at all, and
   * calling init_pair() on it is an error you would otherwise ignore. */
  if (!has_colors()) {
    mvaddstr(1, 2, "This terminal reports no colour support. Press a key.");
    refresh();
    getch();
    endwin();
    return 1;
  }

  start_color();   /* must come after initscr(), before any pair is defined */

  /* Lets you use -1 to mean "whatever the terminal's own default is",
   * which is how you write a program that respects the user's colour
   * scheme instead of forcing black backgrounds on everyone. */
  int have_default = (use_default_colors() == OK);

  /* Pair 0 is hardwired to the terminal default and cannot be redefined.
   * Useful pairs therefore start at 1. */
  for (int i = 0; i < 8; i++) {
    init_pair(i + 1, i, COLOR_BLACK);
  }
  if (have_default) init_pair(9, COLOR_CYAN, -1);   /* -1 == default bg */

  mvprintw(0, 2, "COLORS=%d  COLOR_PAIRS=%d  can_change_color()=%s",
           COLORS, COLOR_PAIRS, can_change_color() ? "yes" : "no");

  mvaddstr(2, 2, "The eight base colours, as foreground on black:");
  for (int i = 0; i < 8; i++) {
    attron(COLOR_PAIR(i + 1));
    mvprintw(3 + i, 4, "%-10s COLOR_PAIR(%d)", color_name[i], i + 1);
    attroff(COLOR_PAIR(i + 1));
  }

  /* A_BOLD historically doubles as "bright" for the foreground colour, so
   * eight colours effectively become sixteen on most terminals. */
  mvaddstr(2, 40, "The same eight with A_BOLD:");
  for (int i = 0; i < 8; i++) {
    attron(COLOR_PAIR(i + 1) | A_BOLD);
    mvprintw(3 + i, 42, "%-10s + A_BOLD", color_name[i]);
    attroff(COLOR_PAIR(i + 1) | A_BOLD);
  }

  int row = 12;

  if (have_default) {
    attron(COLOR_PAIR(9));
    mvaddstr(row, 2, "use_default_colors(): cyan on the terminal's own background");
    attroff(COLOR_PAIR(9));
    row++;
  }

  /* Anything beyond the base 8 depends on the terminal. Most modern ones
   * report 256; check COLORS rather than assuming. */
  if (COLORS >= 256 && COLOR_PAIRS > 40) {
    row++;
    mvaddstr(row++, 2, "256-colour mode detected. The 6x6x6 colour cube:");
    int pair = 20;
    for (int block = 0; block < 6 && pair < COLOR_PAIRS - 1; block++) {
      move(row + block, 4);
      for (int i = 0; i < 36 && pair < COLOR_PAIRS - 1; i++) {
        int c = 16 + block * 36 + i;
        init_pair(pair, COLOR_BLACK, c);
        attron(COLOR_PAIR(pair));
        addch(' ');
        attroff(COLOR_PAIR(pair));
        pair++;
      }
    }
    row += 7;
  } else {
    mvprintw(++row, 2, "Only %d colours here -- set TERM=xterm-256color for more.",
             COLORS);
    row += 2;
  }

  mvaddstr(row, 2, "Press any key to quit.");
  refresh();
  getch();

  endwin();
  return 0;
}
