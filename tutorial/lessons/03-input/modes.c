/*
 * Lesson 3, second example -- the four ways getch() can wait.
 *
 * Cycle through blocking, timeout, non-blocking and half-delay with the
 * space bar and watch the tick counter. The mode you pick decides the
 * shape of your whole main loop, so it is worth feeling the difference.
 *
 * 'q' quits.
 */

#include <ncurses.h>

enum Mode { M_BLOCK, M_TIMEOUT, M_NODELAY, M_HALFDELAY, M_COUNT };

static const char *mode_name[] = {
  "BLOCKING      getch() waits forever",
  "TIMEOUT 500   getch() waits 500ms, then returns ERR",
  "NODELAY       getch() returns ERR immediately if nothing is ready",
  "HALFDELAY 5   like timeout, but set in tenths of a second"
};

static const char *mode_note[] = {
  "Loop only runs when you press a key. Fine for a text editor, useless for a game.",
  "Loop runs at least twice a second. This is the sweet spot for most TUIs.",
  "Loop spins as fast as the CPU allows -- you MUST sleep yourself or burn a core.",
  "Same idea as timeout, older API, and it applies globally rather than per window."
};

/* Each mode is just a different call before getch(). They are mutually
 * exclusive -- setting one cancels the previous. */
static void apply_mode(enum Mode m) {
  nodelay(stdscr, FALSE);
  nocbreak();          /* halfdelay needs to be cancelled via nocbreak */
  cbreak();
  switch (m) {
    case M_BLOCK:     timeout(-1);           break;  /* -1 == block */
    case M_TIMEOUT:   timeout(500);          break;  /* milliseconds */
    case M_NODELAY:   nodelay(stdscr, TRUE); break;  /* == timeout(0) */
    case M_HALFDELAY: halfdelay(5);          break;  /* tenths of a second */
    default: break;
  }
}

int main(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  enum Mode mode = M_BLOCK;
  apply_mode(mode);

  long loops = 0, keys = 0, errs = 0;

  for (;;) {
    int ch = getch();
    loops++;

    if (ch == ERR) {
      errs++;             /* the timeout expired with nothing pressed */
    } else {
      keys++;
      if (ch == 'q') break;
      if (ch == ' ') {
        mode = (mode + 1) % M_COUNT;
        apply_mode(mode);
        loops = keys = errs = 0;
      }
    }

    erase();
    mvprintw(1, 2, "Mode: %s", mode_name[mode]);
    mvprintw(2, 2, "%s", mode_note[mode]);
    mvprintw(4, 2, "loop iterations : %ld", loops);
    mvprintw(5, 2, "real keypresses : %ld", keys);
    mvprintw(6, 2, "ERR returns     : %ld", errs);
    mvprintw(8, 2, "SPACE cycles the mode, 'q' quits.");
    mvprintw(9, 2, "Watch the counters when you stop touching the keyboard.");

    if (mode == M_NODELAY) {
      mvprintw(11, 2, "NOTE: this mode is spinning at full speed right now.");
      mvprintw(12, 2, "A real program adds usleep() or napms() here.");
      napms(20);   /* curses' own portable millisecond sleep */
    }

    refresh();
  }

  endwin();
  return 0;
}
