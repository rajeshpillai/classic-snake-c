/*
 * Lesson 1, second example -- proving that refresh() is what draws.
 *
 * This program writes two lines. Only one of them ever appears, because
 * only one of them is followed by a refresh() before the pause.
 *
 * It also shows why printf() and curses don't mix: printf writes straight
 * to stdout, bypassing stdscr, so curses' model of the screen goes stale
 * and the text lands wherever the cursor happened to be.
 */

#include <ncurses.h>
#include <stdio.h>

int main(void) {
  initscr();

  mvaddstr(2, 4, "(A) written to stdscr BEFORE the first refresh");
  refresh();   /* <-- only this makes (A) visible */

  mvaddstr(4, 4, "(B) written to stdscr but never refreshed - invisible");
  /* no refresh() here on purpose */

  mvaddstr(6, 4, "Press a key to see what happens next...");
  move(6, 4);  /* park the cursor somewhere sensible */
  refresh();   /* this one also flushes (B), so (B) does show up --   */
               /* refresh sends *everything* pending, not just the    */
               /* most recent call. Comment this line out and (B)     */
               /* disappears entirely.                                */
  getch();

  /* Now the classic mistake. printf goes to stdout directly and curses
   * has no idea it happened, so the next refresh() may well paint over
   * it, or leave the screen in a state neither side agrees on. */
  printf("(C) printed with printf - curses does not know this exists\n");
  fflush(stdout);

  mvaddstr(10, 4, "(D) curses text drawn after the printf");
  mvaddstr(11, 4, "Notice (C) is misaligned or gone. Use addstr/printw, not printf.");
  mvaddstr(13, 4, "Press a key to quit.");
  refresh();
  getch();

  endwin();
  return 0;
}
