/*
 * Lesson 9 -- pads: windows bigger than the screen.
 *
 * Builds a 200x200 pad, fills it with content, and shows a movable
 * viewport onto it. This is how you implement a scrollable log, a file
 * viewer, or a game map larger than the terminal.
 *
 * Arrows / PgUp / PgDn / Home scroll. 'q' quits.
 */

#include <ncurses.h>
#include <stdio.h>

#define PAD_H 200
#define PAD_W 200

int main(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  int height, width;
  getmaxyx(stdscr, height, width);
  if (height < 12 || width < 40) {
    endwin();
    fprintf(stderr, "need at least 40x12, got %dx%d\n", width, height);
    return 1;
  }

  int use_color = has_colors();
  if (use_color) {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  }

  /* newpad() takes only a size -- a pad has no position, because it is
   * not on the screen at all. It lives entirely in memory until you ask
   * for a rectangle of it to be displayed. */
  WINDOW *pad = newpad(PAD_H, PAD_W);

  /* Fill it with something whose position is obvious at a glance. */
  for (int y = 0; y < PAD_H; y++) {
    if (use_color) wattron(pad, COLOR_PAIR(y % 10 == 0 ? 2 : 1));
    mvwprintw(pad, y, 0, "%4d |", y);
    if (use_color) wattroff(pad, COLOR_PAIR(y % 10 == 0 ? 2 : 1));

    for (int x = 6; x < PAD_W - 1; x++) {
      /* A diagonal stripe, so scrolling in either axis is visible. */
      mvwaddch(pad, y, x, ((x + y) % 20 == 0) ? ACS_DIAMOND
                        : ((x % 10 == 0) ? '.' : ' '));
    }
    mvwprintw(pad, y, PAD_W - 12, "col%d", PAD_W - 12);
  }

  /* Viewport: which part of the pad is currently shown. */
  int pad_y = 0, pad_x = 0;

  /* The on-screen rectangle we project the pad into. Leave two rows at
   * the top for the header and one at the bottom for the status line. */
  const int view_top = 3, view_left = 2;
  int view_h = height - view_top - 2;
  int view_w = width - view_left - 2;

  int ch = 0;
  do {
    switch (ch) {
      case KEY_UP:    pad_y--;            break;
      case KEY_DOWN:  pad_y++;            break;
      case KEY_LEFT:  pad_x--;            break;
      case KEY_RIGHT: pad_x++;            break;
      case KEY_PPAGE: pad_y -= view_h;    break;
      case KEY_NPAGE: pad_y += view_h;    break;
      case KEY_HOME:  pad_y = pad_x = 0;  break;
      default: break;
    }

    /* Clamp. prefresh() returns ERR on an out-of-range origin and draws
     * nothing, so unclamped scrolling looks like a frozen screen. */
    if (pad_y < 0) pad_y = 0;
    if (pad_x < 0) pad_x = 0;
    if (pad_y > PAD_H - view_h) pad_y = PAD_H - view_h;
    if (pad_x > PAD_W - view_w) pad_x = PAD_W - view_w;

    erase();
    mvprintw(1, 2, "Pad %dx%d, viewport %dx%d, showing from (%d,%d)",
             PAD_H, PAD_W, view_h, view_w, pad_y, pad_x);
    mvprintw(height - 1, 2,
             "arrows / PgUp / PgDn / Home scroll   |   q quits");
    /* Frame the viewport so its extent is obvious. */
    mvhline(view_top - 1, view_left - 1, ACS_HLINE, view_w + 2);
    mvhline(view_top + view_h, view_left - 1, ACS_HLINE, view_w + 2);
    mvvline(view_top, view_left - 1, ACS_VLINE, view_h);
    mvvline(view_top, view_left + view_w, ACS_VLINE, view_h);
    wnoutrefresh(stdscr);

    /* pnoutrefresh(pad, pad_top, pad_left,
     *              screen_top, screen_left, screen_bottom, screen_right)
     *
     * Six coordinates: where in the PAD to start, and which rectangle of
     * the SCREEN to fill. The last two are inclusive corners, not a size
     * -- the most common mistake with pads. */
    pnoutrefresh(pad, pad_y, pad_x,
                 view_top, view_left,
                 view_top + view_h - 1, view_left + view_w - 1);

    doupdate();
  } while ((ch = getch()) != 'q');

  delwin(pad);
  endwin();
  return 0;
}
