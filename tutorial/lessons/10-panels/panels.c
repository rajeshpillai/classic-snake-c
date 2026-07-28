/*
 * Lesson 10 -- panels: overlapping windows that actually work.
 *
 * Five overlapping windows. TAB brings the next one to the front, arrows
 * move the top one around, 'h' hides and shows it. Try doing any of that
 * with bare windows and you will spend the afternoon on touchwin().
 *
 * Link with -lpanel (before -lncurses).
 *
 * 'q' quits.
 */

#include <ncurses.h>
#include <panel.h>
#include <stdio.h>

#define NPANEL 5

int main(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  int height, width;
  getmaxyx(stdscr, height, width);
  if (height < 18 || width < 60) {
    endwin();
    fprintf(stderr, "need at least 60x18, got %dx%d\n", width, height);
    return 1;
  }

  int use_color = has_colors();
  if (use_color) {
    start_color();
    for (int i = 0; i < NPANEL; i++) {
      init_pair(i + 1, COLOR_WHITE, (short) (i + 1));   /* red, green, ... */
    }
  }

  WINDOW *win[NPANEL];
  PANEL  *pan[NPANEL];

  for (int i = 0; i < NPANEL; i++) {
    win[i] = newwin(7, 26, 4 + i, 4 + i * 6);

    if (use_color) wbkgd(win[i], COLOR_PAIR(i + 1));   /* fill the window */
    box(win[i], 0, 0);
    mvwprintw(win[i], 0, 2, " panel %d ", i);
    mvwprintw(win[i], 2, 2, "TAB   raise the next");
    mvwprintw(win[i], 3, 2, "arrows move this one");
    mvwprintw(win[i], 4, 2, "h     hide / show");

    /* new_panel() attaches the window to the panel stack. From this
     * moment on you must NOT call wrefresh() on the window -- the panel
     * library owns the drawing order. */
    pan[i] = new_panel(win[i]);

    /* Stash the index on the panel so we can find it again later. */
    set_panel_userptr(pan[i], &win[i]);
  }

  int top = NPANEL - 1;       /* which panel is currently on top */
  int hidden[NPANEL] = { 0 };

  mvaddstr(1, 2, "Panels -- overlapping windows with a real stacking order.");
  mvaddstr(2, 2, "TAB raise next   arrows move top   h hide/show   q quit");

  int ch = 0;
  do {
    switch (ch) {
      case '\t':
        /* Walk to the next non-hidden panel and raise it. */
        for (int n = 1; n <= NPANEL; n++) {
          int cand = (top + n) % NPANEL;
          if (!hidden[cand]) { top = cand; break; }
        }
        top_panel(pan[top]);      /* the whole trick, in one call */
        break;

      case 'h':
        if (hidden[top]) {
          show_panel(pan[top]);
          hidden[top] = 0;
        } else {
          hide_panel(pan[top]);   /* still exists, just not drawn */
          hidden[top] = 1;
        }
        break;

      case KEY_UP:
      case KEY_DOWN:
      case KEY_LEFT:
      case KEY_RIGHT: {
        /* move_panel() repositions on screen. Use it rather than mvwin()
         * -- the panel library needs to know the window moved so it can
         * repair whatever was underneath. */
        int y, x;
        getbegyx(win[top], y, x);
        int wh, ww;
        getmaxyx(win[top], wh, ww);

        if (ch == KEY_UP    && y > 3)               y--;
        if (ch == KEY_DOWN  && y + wh < height)     y++;
        if (ch == KEY_LEFT  && x > 0)               x--;
        if (ch == KEY_RIGHT && x + ww < width)      x++;

        move_panel(pan[top], y, x);
        break;
      }
      default: break;
    }

    mvprintw(height - 2, 2, "top panel: %d %-10s",
             top, hidden[top] ? "(hidden)" : "");
    /* Panels sit above stdscr, so stdscr must be pushed into the virtual
     * screen before update_panels() stacks the panels on top of it. */
    wnoutrefresh(stdscr);

    /* update_panels() walks the stack bottom-to-top and does the
     * wnoutrefresh()/touchwin() bookkeeping for every panel. It does NOT
     * touch the terminal -- doupdate() still does that. */
    update_panels();
    doupdate();

  } while ((ch = getch()) != 'q');

  /* del_panel() detaches; the window is yours to free afterwards. */
  for (int i = 0; i < NPANEL; i++) {
    del_panel(pan[i]);
    delwin(win[i]);
  }

  endwin();
  return 0;
}
