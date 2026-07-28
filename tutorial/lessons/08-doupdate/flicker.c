/*
 * Lesson 8 -- wnoutrefresh/doupdate, and why clear() flickers.
 *
 * Animates several windows at once under three different update
 * strategies, and counts how much work each one causes. Press SPACE to
 * switch strategy and watch both the counter and the actual smoothness.
 *
 * 'q' quits.
 */

#include <ncurses.h>
#include <stdio.h>

#define NWIN 4

enum Strategy { S_REFRESH_EACH, S_NOUT_DOUPDATE, S_CLEAR_EVERY, S_COUNT };

static const char *strategy_name[] = {
  "wrefresh() per window      -- one screen update per window, per frame",
  "wnoutrefresh() + doupdate() -- one screen update per FRAME",
  "clear() + wrefresh() each   -- full repaint every frame (the flicker bug)"
};

static const char *strategy_note[] = {
  "Works, but the terminal sees N separate updates and can tear between them.",
  "The right way. Compose all windows, then push one diff to the terminal.",
  "clear() forces curses to forget the screen, so every cell is resent."
};

int main(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  nodelay(stdscr, TRUE);

  int height, width;
  getmaxyx(stdscr, height, width);
  if (height < 20 || width < 64) {
    endwin();
    fprintf(stderr, "need at least 64x20, got %dx%d\n", width, height);
    return 1;
  }

  WINDOW *win[NWIN];
  int wh = 5, ww = width / 2 - 3;
  for (int i = 0; i < NWIN; i++) {
    win[i] = newwin(wh, ww, 6 + (i / 2) * (wh + 1), 2 + (i % 2) * (ww + 2));
  }

  enum Strategy strat = S_REFRESH_EACH;
  long frame = 0;
  int pos[NWIN] = { 0, 3, 6, 9 };

  for (;;) {
    int ch = getch();
    if (ch == 'q') break;
    if (ch == ' ') { strat = (strat + 1) % S_COUNT; frame = 0; }

    frame++;

    /* --- the header, drawn on stdscr ------------------------------- */
    /* erase() only marks cells blank in the buffer; curses still diffs
     * it, so unchanged cells cost nothing. clear() is the destructive
     * one -- see the S_CLEAR_EVERY branch. */
    erase();
    mvprintw(1, 2, "Strategy: %s", strategy_name[strat]);
    mvprintw(2, 2, "%s", strategy_note[strat]);
    mvprintw(3, 2, "frame %-8ld   screen updates so far: %ld", frame,
             strat == S_NOUT_DOUPDATE ? frame : frame * NWIN);
    mvprintw(height - 2, 2, "SPACE switches strategy, q quits.");

    /* --- animate the windows --------------------------------------- */
    for (int i = 0; i < NWIN; i++) {
      werase(win[i]);
      box(win[i], 0, 0);
      mvwprintw(win[i], 0, 2, " window %d ", i);
      pos[i] = (pos[i] + 1) % (ww - 4);
      mvwaddch(win[i], 2, 2 + pos[i], ACS_BLOCK | A_REVERSE);
      mvwprintw(win[i], 3, 2, "tick %ld", frame);
    }

    /* --- the three strategies -------------------------------------- */
    switch (strat) {
      case S_REFRESH_EACH:
        /* Each wrefresh() computes a diff AND writes to the terminal.
         * Four windows means four separate bursts of output per frame. */
        refresh();
        for (int i = 0; i < NWIN; i++) wrefresh(win[i]);
        break;

      case S_NOUT_DOUPDATE:
        /* wnoutrefresh() only copies the window into the virtual screen.
         * Nothing reaches the terminal until doupdate(), which sends one
         * combined diff. This is what wrefresh() does internally -- it
         * is literally wnoutrefresh() followed by doupdate(). */
        wnoutrefresh(stdscr);
        for (int i = 0; i < NWIN; i++) wnoutrefresh(win[i]);
        doupdate();
        break;

      case S_CLEAR_EVERY:
        /* clear() == erase() plus clearok(), which tells curses "throw
         * away what you think is on screen". The next refresh therefore
         * repaints every single cell instead of the handful that moved.
         * On a slow link this is visible flicker. */
        clear();
        mvprintw(1, 2, "Strategy: %s", strategy_name[strat]);
        mvprintw(2, 2, "%s", strategy_note[strat]);
        mvprintw(3, 2, "frame %-8ld   full repaints: %ld", frame, frame);
        mvprintw(height - 2, 2, "SPACE switches strategy, q quits.");
        refresh();
        for (int i = 0; i < NWIN; i++) { touchwin(win[i]); wrefresh(win[i]); }
        break;

      default: break;
    }

    napms(40);   /* ~25 fps */
  }

  for (int i = 0; i < NWIN; i++) delwin(win[i]);
  endwin();
  return 0;
}
