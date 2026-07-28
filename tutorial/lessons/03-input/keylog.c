/*
 * Lesson 3 -- reading the keyboard.
 *
 * A live key logger. Press keys and watch what getch() actually returns.
 * Press F2 to toggle keypad() mode on and off so you can see the arrow
 * keys change from a three-byte escape sequence into a single KEY_UP.
 *
 * 'q' quits.
 */

#include <ncurses.h>
#include <string.h>

/* getch() returns an int, not a char, precisely so it can return values
 * above 255 for the function keys. Translate the ones we care about. */
static const char *key_name_of(int ch) {
  switch (ch) {
    case KEY_UP:        return "KEY_UP";
    case KEY_DOWN:      return "KEY_DOWN";
    case KEY_LEFT:      return "KEY_LEFT";
    case KEY_RIGHT:     return "KEY_RIGHT";
    case KEY_HOME:      return "KEY_HOME";
    case KEY_END:       return "KEY_END";
    case KEY_NPAGE:     return "KEY_NPAGE (page down)";
    case KEY_PPAGE:     return "KEY_PPAGE (page up)";
    case KEY_BACKSPACE: return "KEY_BACKSPACE";
    case KEY_DC:        return "KEY_DC (delete)";
    case KEY_IC:        return "KEY_IC (insert)";
    case KEY_F(1):      return "KEY_F(1)";
    case KEY_F(2):      return "KEY_F(2)";
    case KEY_RESIZE:    return "KEY_RESIZE (terminal was resized)";
    case KEY_MOUSE:     return "KEY_MOUSE";
    case '\n':          return "newline";
    case '\t':          return "tab";
    case 27:            return "ESC (or the start of an escape sequence)";
    case ERR:           return "ERR (nothing was waiting)";
    default:            return NULL;
  }
}

int main(void) {
  initscr();
  cbreak();               /* keys arrive immediately, no waiting for Enter */
  noecho();               /* we decide what appears on screen, not the tty */
  keypad(stdscr, TRUE);   /* decode escape sequences into single KEY_* codes */

  int keypad_on = 1;
  int row = 6;

  mvaddstr(0, 0, "Key logger -- press keys. F2 toggles keypad(), 'q' quits.");
  mvaddstr(1, 0, "Try the arrow keys with keypad on, then off.");
  refresh();

  int ch;
  while ((ch = getch()) != 'q') {
    int height, width;
    getmaxyx(stdscr, height, width);

    /* Scroll our little log by hand once it reaches the bottom. */
    if (row >= height - 2) {
      move(6, 0);
      clrtobot();        /* erase from the cursor to the end of the screen */
      row = 6;
    }

    const char *name = key_name_of(ch);
    char printable[16];

    if (ch >= 32 && ch < 127) {
      snprintf(printable, sizeof(printable), "'%c'", ch);
    } else {
      snprintf(printable, sizeof(printable), "--");
    }

    move(row, 0);
    clrtoeol();          /* erase the old line before writing the new one */
    printw("code %-6d  %-4s  %s", ch, printable, name ? name : "");

    if (ch == KEY_F(2)) {
      keypad_on = !keypad_on;
      keypad(stdscr, keypad_on ? TRUE : FALSE);
    }

    /* Status line, redrawn every keypress. */
    move(3, 0);
    clrtoeol();
    printw("keypad(): %s   |   with it OFF, an arrow key arrives as "
           "27 then '[' then 'A'", keypad_on ? "ON " : "OFF");

    (void) width;
    row++;
    refresh();
  }

  endwin();
  return 0;
}
