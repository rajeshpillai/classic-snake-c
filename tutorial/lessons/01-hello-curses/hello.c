/*
 * Lesson 1 -- the smallest useful curses program.
 *
 * Four calls make up the skeleton of every curses program ever written:
 * initscr() to take over, some drawing, refresh() to make it visible,
 * endwin() to hand the terminal back.
 */

#include <ncurses.h>

int main(void) {
  /* Sets up the terminal for curses and allocates the standard screen,
   * `stdscr`. Everything you draw goes into that in-memory buffer -- the
   * real terminal hasn't been touched yet. */
  initscr();

  /* These write into stdscr, not to the terminal. Nothing appears yet. */
  addstr("Hello, curses!\n");
  addstr("\n");
  addstr("Everything above went into an in-memory buffer called stdscr.\n");
  addstr("It only reached your terminal when refresh() was called.\n");
  addstr("\n");
  addstr("Press any key to quit.");

  /* Now curses compares stdscr against what it believes is already on
   * screen and emits the minimum escape sequences needed to reconcile
   * the two. That diffing is the whole point of the library. */
  refresh();

  /* Without this the program would exit instantly and endwin() would
   * wipe the screen before you saw anything. */
  getch();

  /* Restores the terminal modes curses changed. Skip it and you leave the
   * user with a terminal that has no echo and a hidden cursor. */
  endwin();

  return 0;
}
