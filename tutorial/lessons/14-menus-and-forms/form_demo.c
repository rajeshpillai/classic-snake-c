/*
 * Lesson 14b -- the form library.
 *
 * A settings dialog with validated text fields. The form library gives
 * you cursor movement, editing, field validation and tabbing between
 * fields; you supply the layout and read the buffers at the end.
 *
 * Link with -lform (before -lncurses).
 *
 * TAB / arrows move between fields, ENTER submits, ESC cancels.
 */

#include <ncurses.h>
#include <form.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NFIELD 4

/* Field buffers come back space-padded to the full field width, which is
 * almost never what you want. */
static char *trimmed(const char *s) {
  static char out[64];
  size_t n = strlen(s);
  while (n > 0 && isspace((unsigned char) s[n - 1])) n--;
  if (n >= sizeof out) n = sizeof out - 1;
  memcpy(out, s, n);
  out[n] = '\0';
  return out;
}

int main(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  int use_color = has_colors();
  if (use_color) {
    start_color();
    init_pair(1, COLOR_CYAN,  COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    init_pair(3, COLOR_RED,   COLOR_BLACK);
  }

  int height, width;
  getmaxyx(stdscr, height, width);
  if (height < 18 || width < 56) {
    endwin();
    fprintf(stderr, "need at least 56x18, got %dx%d\n", width, height);
    return 1;
  }

  /* new_field(rows, cols, top, left, offscreen_rows, extra_buffers)
   * The last two are almost always 0. Coordinates are relative to the
   * form's sub-window. */
  FIELD *field[NFIELD + 1];
  field[0] = new_field(1, 20, 1, 18, 0, 0);   /* player name  */
  field[1] = new_field(1, 6,  3, 18, 0, 0);   /* board width  */
  field[2] = new_field(1, 6,  5, 18, 0, 0);   /* board height */
  field[3] = new_field(1, 6,  7, 18, 0, 0);   /* tick ms      */
  field[NFIELD] = NULL;

  for (int i = 0; i < NFIELD; i++) {
    /* O_AUTOSKIP would jump to the next field when this one fills up --
     * usually irritating, so it stays off. */
    set_field_back(field[i], use_color ? COLOR_PAIR(2) : A_UNDERLINE);
    field_opts_off(field[i], O_AUTOSKIP);
  }

  /* Validators. The library enforces these when you leave the field --
   * it will simply refuse to move on if the content doesn't match. */
  set_field_type(field[0], TYPE_ALNUM, 1);              /* min 1 char   */
  set_field_type(field[1], TYPE_INTEGER, 0, 20, 200);   /* pad, min, max */
  set_field_type(field[2], TYPE_INTEGER, 0, 10, 100);
  set_field_type(field[3], TYPE_INTEGER, 0, 20, 500);

  set_field_buffer(field[0], 0, "player");
  set_field_buffer(field[1], 0, "80");
  set_field_buffer(field[2], 0, "24");
  set_field_buffer(field[3], 0, "100");

  FORM *form = new_form(field);

  /* Like menus: an outer window for decoration, a sub-window for fields. */
  int fh, fw;
  scale_form(form, &fh, &fw);          /* asks the form how big it needs to be */
  WINDOW *fwin = newwin(fh + 6, fw + 6, 3, 3);
  WINDOW *fsub = derwin(fwin, fh, fw, 3, 3);

  keypad(fwin, TRUE);
  set_form_win(form, fwin);
  set_form_sub(form, fsub);
  post_form(form);

  box(fwin, 0, 0);
  mvwprintw(fwin, 0, 2, " Game settings ");
  mvwaddstr(fwin, 1, 3, "TAB moves, ENTER submits, ESC cancels");

  /* Labels live on the form window, not the sub-window -- the sub-window
   * belongs to the fields. */
  mvwaddstr(fwin, 4,  3, "Player name:");
  mvwaddstr(fwin, 6,  3, "Board width:");
  mvwaddstr(fwin, 8,  3, "Board height:");
  mvwaddstr(fwin, 10, 3, "Tick (ms):");

  mvprintw(1, 3, "The form library: editing, tabbing and validation for free.");
  mvprintw(height - 3, 3, "width 20-200, height 10-100, tick 20-500");
  refresh();
  wrefresh(fwin);

  int ch, submitted = 0, running = 1;
  while (running && (ch = wgetch(fwin)) != 27 /* ESC */) {
    switch (ch) {
      case KEY_DOWN:
      case '\t':
        form_driver(form, REQ_NEXT_FIELD);
        form_driver(form, REQ_END_LINE);   /* put the cursor after the text */
        break;

      case KEY_UP:
      case KEY_BTAB:
        form_driver(form, REQ_PREV_FIELD);
        form_driver(form, REQ_END_LINE);
        break;

      case KEY_LEFT:   form_driver(form, REQ_PREV_CHAR);  break;
      case KEY_RIGHT:  form_driver(form, REQ_NEXT_CHAR);  break;
      case KEY_HOME:   form_driver(form, REQ_BEG_LINE);   break;
      case KEY_END:    form_driver(form, REQ_END_LINE);   break;

      case KEY_BACKSPACE:
      case 127:
      case 8:
        form_driver(form, REQ_DEL_PREV);
        break;

      case KEY_DC:
        form_driver(form, REQ_DEL_CHAR);
        break;

      case '\n':
        /* REQ_VALIDATION forces the current field to be checked and its
         * buffer synced. Without it the last field you typed in is still
         * sitting in the internal edit buffer and reads back stale. */
        if (form_driver(form, REQ_VALIDATION) == E_OK) {
          submitted = 1;
          running = 0;
        } else {
          if (use_color) attron(COLOR_PAIR(3));
          mvprintw(height - 2, 3, "invalid value in the current field");
          if (use_color) attroff(COLOR_PAIR(3));
          refresh();
        }
        break;

      default:
        /* Anything else is a literal character for the field. */
        form_driver(form, ch);
        break;
    }
    wrefresh(fwin);
  }

  char name[64], w[16], h[16], tick[16];
  snprintf(name, sizeof name, "%s", trimmed(field_buffer(field[0], 0)));
  snprintf(w,    sizeof w,    "%s", trimmed(field_buffer(field[1], 0)));
  snprintf(h,    sizeof h,    "%s", trimmed(field_buffer(field[2], 0)));
  snprintf(tick, sizeof tick, "%s", trimmed(field_buffer(field[3], 0)));

  unpost_form(form);
  free_form(form);
  for (int i = 0; i < NFIELD; i++) free_field(field[i]);
  delwin(fsub);
  delwin(fwin);
  endwin();

  if (submitted) {
    printf("settings: name=%s board=%sx%s tick=%sms\n", name, w, h, tick);
  } else {
    printf("cancelled\n");
  }
  return 0;
}
