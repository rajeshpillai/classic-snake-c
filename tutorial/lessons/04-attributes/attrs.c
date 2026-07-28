/*
 * Lesson 4 -- attributes: bold, underline, reverse and friends.
 *
 * Shows every standard attribute, then demonstrates the three ways to
 * apply them and the difference between attron/attrset/chtype.
 *
 * Any key quits.
 */

#include <ncurses.h>

static const struct {
  attr_t attr;
  const char *name;
  const char *note;
} attrs[] = {
  { A_NORMAL,     "A_NORMAL",     "the default, clears everything else" },
  { A_BOLD,       "A_BOLD",       "widely supported; often rendered as bright" },
  { A_DIM,        "A_DIM",        "half-bright; many terminals ignore it" },
  { A_UNDERLINE,  "A_UNDERLINE",  "widely supported" },
  { A_REVERSE,    "A_REVERSE",    "swaps foreground and background" },
  { A_STANDOUT,   "A_STANDOUT",   "'best highlight this terminal has' - usually reverse" },
  { A_BLINK,      "A_BLINK",      "often disabled by the terminal, and rightly so" },
  { A_INVIS,      "A_INVIS",      "foreground painted in the background colour" },
  { A_ITALIC,     "A_ITALIC",     "ncurses 6+, and only on terminals that advertise it" },
};

int main(void) {
  initscr();
  cbreak();
  noecho();
  curs_set(0);

  mvaddstr(0, 2, "Attributes -- what your terminal actually renders:");

  int row = 2;
  for (unsigned i = 0; i < sizeof attrs / sizeof attrs[0]; i++) {
    mvprintw(row, 2, "%-14s", attrs[i].name);

    /* attron() adds an attribute to whatever is already active. */
    attron(attrs[i].attr);
    addstr("  Sample Text 123  ");
    attroff(attrs[i].attr);

    printw("  %s", attrs[i].note);
    row++;
  }

  row++;
  mvaddstr(row++, 2, "Attributes combine with bitwise OR:");
  mvaddstr(row, 2, "  ");
  attron(A_BOLD | A_UNDERLINE);
  addstr("bold + underline");
  attroff(A_BOLD | A_UNDERLINE);
  row += 2;

  /* attrset() REPLACES the whole attribute set rather than adding to it.
   * Use it when you want to be certain nothing is left over from earlier. */
  mvaddstr(row++, 2, "attron() adds, attrset() replaces:");

  attron(A_BOLD);
  mvaddstr(row, 2, "  bold... ");
  attron(A_UNDERLINE);
  addstr("bold+underline... ");
  attrset(A_UNDERLINE);          /* drops the bold */
  addstr("underline only");
  attrset(A_NORMAL);             /* the reliable way to reset everything */
  row += 2;

  /* A chtype packs a character and its attributes into one integer. This
   * is what a cell in the window buffer actually holds, and you can build
   * one yourself when it reads more clearly than attron/attroff pairs. */
  mvaddstr(row++, 2, "A chtype is character | attributes in a single value:");
  mvaddch(row, 4, 'X' | A_REVERSE);
  mvaddch(row, 6, 'Y' | A_BOLD | A_UNDERLINE);
  mvaddstr(row, 10, "<- drawn with mvaddch('X' | A_REVERSE)");
  row += 2;

  /* chgat() restyles characters already on screen without redrawing them:
   * -1 means "to the end of the line". Handy for highlighting a menu row. */
  mvaddstr(row, 2, "  chgat() restyles text already drawn - this whole line");
  mvchgat(row, 2, -1, A_REVERSE, 0, NULL);
  row += 2;

  mvprintw(row++, 2, "termattrs() reports what this terminal claims to support.");
  attr_t supported = termattrs();
  mvprintw(row++, 2, "  bold:%s  underline:%s  reverse:%s  blink:%s  italic:%s",
           (supported & A_BOLD)      ? "yes" : "no ",
           (supported & A_UNDERLINE) ? "yes" : "no ",
           (supported & A_REVERSE)   ? "yes" : "no ",
           (supported & A_BLINK)     ? "yes" : "no ",
           (supported & A_ITALIC)    ? "yes" : "no ");

  mvaddstr(row + 1, 2, "Press any key to quit.");
  refresh();
  getch();

  endwin();
  return 0;
}
