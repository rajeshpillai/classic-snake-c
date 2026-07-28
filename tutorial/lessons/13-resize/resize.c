/*
 * Lesson 13 -- surviving a terminal resize.
 *
 * Drag your terminal's corner while this runs. The layout reflows, the
 * counter shows how many resize events arrived, and the window is rebuilt
 * at the new size.
 *
 * The snake game does none of this: resize it mid-game and the playfield
 * still uses the old dimensions, so the snake can wrap into a wall that
 * is no longer there.
 *
 * 'q' quits.
 */

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

static int resize_count;

/* Windows have to be destroyed and recreated at the new size -- there is
 * no "resize this window" that preserves contents meaningfully. */
static WINDOW *rebuild(WINDOW *old, int h, int w, int y, int x) {
  if (old) delwin(old);          /* delwin(NULL) is not safe -- guard it */
  /* newwin() returns NULL for a degenerate size, and box(NULL) segfaults.
   * Clamp rather than propagate a NULL nobody downstream checks. */
  if (h < 3) h = 3;
  if (w < 6) w = 6;
  return newwin(h, w, y, x);
}

int main(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);   /* required: KEY_RESIZE arrives through getch() */
  curs_set(0);

  int use_color = has_colors();
  if (use_color) {
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED,   COLOR_BLACK);
  }

  int height, width;
  getmaxyx(stdscr, height, width);

  WINDOW *box_win = rebuild(NULL, height / 2, width / 2, 4, 2);

  int ch = 0;
  do {
    if (ch == KEY_RESIZE) {
      /* ncurses has already called resizeterm() for us by the time this
       * key is delivered, so LINES/COLS and stdscr are correct. What it
       * cannot do is know how YOUR windows should be laid out. */
      resize_count++;
      getmaxyx(stdscr, height, width);
      box_win = rebuild(box_win, height / 2, width / 2, 4, 2);

      /* The terminal may have left artefacts from the resize itself.
       * This is one of the few legitimate uses of clear(). */
      clear();
    }

    erase();

    if (height < 12 || width < 40) {
      /* Always handle "too small" -- users drag terminals to silly sizes,
       * and every mvprintw below would silently fail. */
      if (use_color) attron(COLOR_PAIR(2));
      mvprintw(0, 0, "too small: %dx%d", width, height);
      mvprintw(1, 0, "need 40x12");
      if (use_color) attroff(COLOR_PAIR(2));
      refresh();
      continue;
    }

    if (use_color) attron(COLOR_PAIR(1));
    mvprintw(1, 2, "Resize me -- drag the terminal corner.");
    if (use_color) attroff(COLOR_PAIR(1));

    mvprintw(2, 2, "size %dx%d   LINES=%d COLS=%d   resize events: %d",
             width, height, LINES, COLS, resize_count);

    /* A right-aligned element, to make the reflow obvious. */
    const char *tag = "[ right edge ]";
    mvprintw(1, width - (int) strlen(tag) - 1, "%s", tag);

    /* And a bottom-anchored one. */
    mvprintw(height - 1, 2, "q quits");

    /* Frame the whole screen so the new bounds are visible. */
    mvhline(3, 0, ACS_HLINE, width);
    mvvline(4, width - 1, ACS_VLINE, height - 5);

    wnoutrefresh(stdscr);

    box(box_win, 0, 0);
    mvwprintw(box_win, 0, 2, " rebuilt window ");
    mvwprintw(box_win, 2, 2, "this window is destroyed");
    mvwprintw(box_win, 3, 2, "and recreated on resize");
    {
      int bh, bw;
      getmaxyx(box_win, bh, bw);
      mvwprintw(box_win, 5, 2, "now %d x %d", bh, bw);
    }
    wnoutrefresh(box_win);

    doupdate();

  } while ((ch = getch()) != 'q');

  delwin(box_win);
  endwin();
  printf("handled %d resize events\n", resize_count);
  return 0;
}
