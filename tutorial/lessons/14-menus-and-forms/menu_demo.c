/*
 * Lesson 14a -- the menu library.
 *
 * A scrollable, multi-select menu. The menu library handles navigation,
 * scrolling, marking and the highlight; you supply the items and read the
 * result.
 *
 * Link with -lmenu (before -lncurses).
 *
 * Arrows / PgUp / PgDn navigate, SPACE toggles, ENTER confirms, q quits.
 */

#include <ncurses.h>
#include <menu.h>
#include <stdio.h>
#include <stdlib.h>

static const char *choices[] = {
  "Wrap at walls",        "the snake reappears on the far side",
  "Obstacles",            "scatter %% rocks around the board",
  "Golden food",          "worth 50, but it rots after 60 ticks",
  "Speed ramp",           "the game accelerates as your score climbs",
  "Sound",                "beep() on every pickup",
  "Colour",               "use colour pairs if the terminal has them",
  "Persist high score",   "write ~/.snake_highscore",
  "Show FPS",             "draw the measured frame rate",
  "Vim keys",             "hjkl as well as the arrows",
  "Pause on focus loss",  "auto-pause when the terminal is unfocused",
  "Big playfield",        "use a pad larger than the screen",
  "Debug overlay",        "draw the collision grid",
};
#define NCHOICE ((int) (sizeof choices / sizeof choices[0]) / 2)

int main(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  int use_color = has_colors();
  if (use_color) {
    start_color();
    init_pair(1, COLOR_CYAN,  COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_CYAN);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
  }

  int height, width;
  getmaxyx(stdscr, height, width);
  if (height < 20 || width < 64) {
    endwin();
    fprintf(stderr, "need at least 64x20, got %dx%d\n", width, height);
    return 1;
  }

  /* The items array must be NULL-terminated, and the strings must stay
   * alive for as long as the menu does -- new_item() does NOT copy them. */
  ITEM **items = calloc(NCHOICE + 1, sizeof *items);
  for (int i = 0; i < NCHOICE; i++) {
    items[i] = new_item(choices[i * 2], choices[i * 2 + 1]);
  }
  items[NCHOICE] = NULL;

  MENU *menu = new_menu(items);

  /* A menu needs two windows: the outer one for the border and title,
   * and a "sub" window that the items are actually drawn into. */
  WINDOW *mwin = newwin(14, 58, 4, 3);
  WINDOW *msub = derwin(mwin, 10, 54, 3, 2);

  keypad(mwin, TRUE);
  set_menu_win(menu, mwin);
  set_menu_sub(menu, msub);

  /* Show 8 rows at a time; the library scrolls the rest. */
  set_menu_format(menu, 8, 1);

  /* O_ONEVALUE is on by default (single select). Turning it off makes
   * the menu multi-select, driven by REQ_TOGGLE_ITEM. */
  menu_opts_off(menu, O_ONEVALUE);

  set_menu_mark(menu, " [x] ");        /* shown against selected items */

  if (use_color) {
    set_menu_fore(menu, COLOR_PAIR(2) | A_BOLD);   /* the cursor row */
    set_menu_back(menu, COLOR_PAIR(1));            /* everything else */
    set_menu_grey(menu, COLOR_PAIR(1) | A_DIM);    /* disabled items */
  }

  box(mwin, 0, 0);
  mvwprintw(mwin, 0, 2, " Options ");
  mvwaddstr(mwin, 1, 2, "SPACE toggles, arrows move, ENTER confirms, q quits");
  mvwhline(mwin, 2, 1, ACS_HLINE, 56);

  post_menu(menu);      /* draws it; nothing appears before this */
  wrefresh(mwin);

  mvprintw(1, 3, "The menu library: scrolling, marking and navigation for free.");
  refresh();

  int ch, done = 0;
  while (!done && (ch = wgetch(mwin)) != 'q') {
    switch (ch) {
      case KEY_DOWN:  menu_driver(menu, REQ_DOWN_ITEM);   break;
      case KEY_UP:    menu_driver(menu, REQ_UP_ITEM);     break;
      case KEY_NPAGE: menu_driver(menu, REQ_SCR_DPAGE);   break;
      case KEY_PPAGE: menu_driver(menu, REQ_SCR_UPAGE);   break;
      case KEY_HOME:  menu_driver(menu, REQ_FIRST_ITEM);  break;
      case KEY_END:   menu_driver(menu, REQ_LAST_ITEM);   break;
      case ' ':       menu_driver(menu, REQ_TOGGLE_ITEM); break;
      case '\n':      done = 1;                           break;
      default: break;
    }

    /* item_description() of the item under the cursor -- a status line
     * like this is the main reason to set descriptions at all. */
    ITEM *cur = current_item(menu);
    move(height - 3, 3);
    clrtoeol();
    if (use_color) attron(COLOR_PAIR(3));
    printw("%s: %s", item_name(cur), item_description(cur));
    if (use_color) attroff(COLOR_PAIR(3));
    refresh();

    wrefresh(mwin);
  }

  /* Read the result BEFORE unposting and freeing. */
  int chosen[NCHOICE], nchosen = 0;
  for (int i = 0; i < NCHOICE; i++) {
    if (item_value(items[i])) chosen[nchosen++] = i;
  }

  /* Teardown order matters: unpost, free the menu, then the items. */
  unpost_menu(menu);
  free_menu(menu);
  for (int i = 0; i < NCHOICE; i++) free_item(items[i]);
  free(items);
  delwin(msub);
  delwin(mwin);
  endwin();

  printf("selected %d option%s:\n", nchosen, nchosen == 1 ? "" : "s");
  for (int i = 0; i < nchosen; i++) printf("  - %s\n", choices[chosen[i] * 2]);
  return 0;
}
