/*
 * Lesson 2 -- coordinates, and the (y, x) order that trips everybody up.
 *
 * Draws a ruler along both axes, marks the four corners and the centre,
 * and reports the terminal size. Resize your terminal and re-run it to see
 * the numbers change.
 */

#include <ncurses.h>
#include <string.h>

int main(void) {
  initscr();
  noecho();
  curs_set(0);

  int height, width;
  /* Note: NOT a pointer -- getmaxyx is a macro and takes the variables
   * themselves. Passing &height here is a classic compile error. */
  getmaxyx(stdscr, height, width);

  /* Column ruler across the top: a digit every column, tens marked. */
  for (int x = 0; x < width; x++) {
    mvaddch(0, x, (x % 10 == 0) ? '|' : '.');
    if (x % 10 == 0 && x + 2 < width) mvprintw(1, x, "%d", x);
  }

  /* Row ruler down the left. Rows 0 and 1 are used by the column ruler. */
  for (int y = 2; y < height; y++) {
    mvaddch(y, 0, (y % 5 == 0) ? '-' : ':');
    if (y % 5 == 0) mvprintw(y, 1, "%d", y);
  }

  /* The corners. Remember: y first, x second. */
  mvaddstr(2, 6, "top-left is (y=2, x=6)");
  mvaddstr(height - 2, 6, "bottom area");

  /* Right-aligning text means measuring it yourself -- curses has no
   * concept of alignment. This is the idiom you'll write constantly. */
  const char *right = "right-aligned";
  mvaddstr(4, width - (int) strlen(right), right);

  /* Dead centre. Same arithmetic the snake game uses for "GAME OVER". */
  const char *mid = "[ centre ]";
  mvprintw(height / 2, (width - (int) strlen(mid)) / 2, "%s", mid);

  mvprintw(height / 2 + 2, 4, "terminal is %d rows x %d cols", height, width);
  mvprintw(height / 2 + 3, 4, "valid y: 0..%d   valid x: 0..%d",
           height - 1, width - 1);

  /* Where is the cursor after all that? getyx() answers, same macro
   * convention -- no ampersands. */
  int cy, cx;
  getyx(stdscr, cy, cx);
  mvprintw(height / 2 + 4, 4, "cursor sits at y=%d x=%d after that printw", cy, cx);

  /* Writing out of bounds is not a crash -- curses just returns ERR and
   * ignores it. Silent failure, which is why a blank screen usually means
   * bad coordinates rather than a missing refresh. */
  int rc = mvaddch(height + 50, 0, 'X');
  mvprintw(height / 2 + 5, 4, "mvaddch() off-screen returned %s (ERR is %d)",
           rc == ERR ? "ERR" : "OK", ERR);

  mvaddstr(height - 1, 0, "press any key");
  refresh();
  getch();

  endwin();
  return 0;
}
