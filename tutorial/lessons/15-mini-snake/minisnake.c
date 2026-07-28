/*
 * Lesson 15 -- the capstone: snake in about 150 lines.
 *
 * Every technique from the tutorial, applied at once:
 *
 *   lesson 3  cbreak/noecho/keypad, draining the input queue
 *   lesson 4  attributes for the HUD
 *   lesson 5  colour pairs, with a no-colour fallback
 *   lesson 6  an ACS border instead of '#'
 *   lesson 7  a separate window for the playfield and the HUD
 *   lesson 8  wnoutrefresh x2 + one doupdate per frame
 *   lesson 11 timeout()-based pacing, so input latency is zero
 *   lesson 13 KEY_RESIZE handling -- the thing ../../../snake.c lacks
 *
 * Compare this with ../../../snake.c: same game, but drawn through
 * windows and paced properly.
 *
 * Arrows or wasd to steer, p pauses, q quits.
 */

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_LEN   1024
#define HUD_H     2          /* rows reserved at the top for the scoreboard */
#define MIN_W     30
#define MIN_H     12

typedef struct { int x, y; } Point;
typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

enum { CP_SNAKE = 1, CP_FOOD, CP_WALL, CP_HUD };

static WINDOW *field;        /* the playfield, including its border */
static WINDOW *hud;

static Point snake[MAX_LEN];
static int   len;
static Direction dir;
static Point food;
static int   score;
static int   paused;
static int   use_color;
static int   fw, fh;         /* playfield window size, border included */

static int occupied(int x, int y) {
  for (int i = 0; i < len; i++)
    if (snake[i].x == x && snake[i].y == y) return 1;
  return 0;
}

static void spawn_food(void) {
  int tries = 0;
  do {
    /* Interior only: (1,1) to (fh-2, fw-2), because the border owns the
     * outermost ring of the window (lesson 7). */
    food.x = rand() % (fw - 2) + 1;
    food.y = rand() % (fh - 2) + 1;
  } while (occupied(food.x, food.y) && ++tries < 1000);
}

/* Build (or rebuild) the two windows for the current terminal size.
 * Called at startup and again on every resize. */
static int layout(void) {
  int H, W;
  getmaxyx(stdscr, H, W);
  if (H < MIN_H || W < MIN_W) return 0;

  if (hud)   { delwin(hud);   hud = NULL; }
  if (field) { delwin(field); field = NULL; }

  hud   = newwin(HUD_H, W, 0, 0);
  field = newwin(H - HUD_H, W, HUD_H, 0);
  getmaxyx(field, fh, fw);
  return 1;
}

static void reset(void) {
  len = 3;
  dir = RIGHT;
  score = 0;
  paused = 0;
  for (int i = 0; i < len; i++) {
    snake[i].x = fw / 2 - i;      /* head on the right, matching RIGHT */
    snake[i].y = fh / 2;
  }
  spawn_food();
}

/* Pull anything now outside the playfield back in. Cheaper and less
 * jarring than restarting the game when the terminal is resized. */
static void clamp_into_field(void) {
  for (int i = 0; i < len; i++) {
    if (snake[i].x > fw - 2) snake[i].x = fw - 2;
    if (snake[i].y > fh - 2) snake[i].y = fh - 2;
    if (snake[i].x < 1) snake[i].x = 1;
    if (snake[i].y < 1) snake[i].y = 1;
  }
  if (food.x > fw - 2 || food.y > fh - 2) spawn_food();
}

/* Returns 0 on death. */
static int step(void) {
  Point next = snake[0];
  switch (dir) {
    case UP:    next.y--; break;
    case DOWN:  next.y++; break;
    case LEFT:  next.x--; break;
    case RIGHT: next.x++; break;
  }

  /* Wrap at the border, which is at 0 and fw-1 / fh-1. */
  if (next.x < 1)       next.x = fw - 2;
  if (next.x > fw - 2)  next.x = 1;
  if (next.y < 1)       next.y = fh - 2;
  if (next.y > fh - 2)  next.y = 1;

  int grew = (next.x == food.x && next.y == food.y);

  /* The tail vacates this tick unless we're growing (lesson: see the
   * self-collision fix in ../../../snake.c). */
  int body = grew ? len : len - 1;
  for (int i = 0; i < body; i++)
    if (snake[i].x == next.x && snake[i].y == next.y) return 0;

  int new_len = len;
  if (grew) {
    score += 10;
    if (new_len < MAX_LEN) {
      snake[new_len] = snake[new_len - 1];   /* seed on the old tail tip */
      new_len++;
    }
  }

  for (int i = new_len - 1; i > 0; i--) snake[i] = snake[i - 1];
  snake[0] = next;
  len = new_len;

  if (grew) spawn_food();
  return 1;
}

static void draw(void) {
  /* --- HUD ---------------------------------------------------------- */
  werase(hud);
  if (use_color) wattron(hud, COLOR_PAIR(CP_HUD));
  wattron(hud, A_BOLD);
  mvwprintw(hud, 0, 2, "score %-6d", score);
  wattroff(hud, A_BOLD);
  mvwprintw(hud, 0, 16, "length %-4d", len);
  mvwaddstr(hud, 0, 30, "arrows/wasd  p pause  q quit");
  if (paused) {
    wattron(hud, A_REVERSE);
    mvwaddstr(hud, 0, 62, " PAUSED ");
    wattroff(hud, A_REVERSE);
  }
  if (use_color) wattroff(hud, COLOR_PAIR(CP_HUD));
  mvwhline(hud, 1, 0, ACS_HLINE, fw);

  /* --- playfield ---------------------------------------------------- */
  werase(field);

  if (use_color) wattron(field, COLOR_PAIR(CP_WALL));
  box(field, 0, 0);                                  /* lesson 6 */
  if (use_color) wattroff(field, COLOR_PAIR(CP_WALL));

  if (use_color) wattron(field, COLOR_PAIR(CP_FOOD));
  mvwaddch(field, food.y, food.x, ACS_DIAMOND);
  if (use_color) wattroff(field, COLOR_PAIR(CP_FOOD));

  if (use_color) wattron(field, COLOR_PAIR(CP_SNAKE));
  for (int i = 0; i < len; i++)
    mvwaddch(field, snake[i].y, snake[i].x, i == 0 ? 'O' : 'o');
  if (use_color) wattroff(field, COLOR_PAIR(CP_SNAKE));

  /* One update for both windows -- lesson 8. */
  wnoutrefresh(hud);
  wnoutrefresh(field);
  doupdate();
}

int main(void) {
  srand((unsigned) time(NULL));

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  use_color = has_colors();
  if (use_color) {
    start_color();
    use_default_colors();                            /* lesson 5 */
    init_pair(CP_SNAKE, COLOR_GREEN,  -1);
    init_pair(CP_FOOD,  COLOR_RED,    -1);
    init_pair(CP_WALL,  COLOR_CYAN,   -1);
    init_pair(CP_HUD,   COLOR_YELLOW, -1);
  }

  if (!layout()) {
    endwin();
    fprintf(stderr, "terminal too small: need %dx%d\n", MIN_W, MIN_H);
    return 1;
  }
  reset();

  int tick_ms = 110;
  int alive = 1;

  while (alive) {
    /* --- input, drained (lesson 3) ---------------------------------- */
    int ch;
    /* timeout() gives us the frame delay AND the input wait in one call,
     * so a keypress is acted on immediately rather than after a sleep
     * (lesson 11). The first getch() of the frame does the waiting; the
     * drain loop below runs non-blocking. */
    timeout(paused ? 80 : tick_ms);
    ch = getch();
    nodelay(stdscr, TRUE);

    do {
      if (ch == ERR) break;
      switch (ch) {
        case 'w': case KEY_UP:    if (dir != DOWN)  dir = UP;    break;
        case 's': case KEY_DOWN:  if (dir != UP)    dir = DOWN;  break;
        case 'a': case KEY_LEFT:  if (dir != RIGHT) dir = LEFT;  break;
        case 'd': case KEY_RIGHT: if (dir != LEFT)  dir = RIGHT; break;
        case 'p': paused = !paused; break;
        case 'q': alive = 0; break;

        case KEY_RESIZE:                              /* lesson 13 */
          if (layout()) {
            clamp_into_field();
            clear();
            refresh();
          } else {
            paused = 1;    /* too small -- wait for them to drag it back */
          }
          break;
      }
    } while ((ch = getch()) != ERR);

    if (!alive) break;

    /* --- simulate --------------------------------------------------- */
    if (!paused) {
      if (!step()) break;
      tick_ms = 110 - score / 4;
      if (tick_ms < 45) tick_ms = 45;                 /* clamp both ends */
    }

    /* --- draw ------------------------------------------------------- */
    if (fw >= MIN_W && fh >= 4) draw();
  }

  /* --- game over ---------------------------------------------------- */
  nodelay(stdscr, FALSE);
  timeout(-1);
  flushinp();                                         /* lesson 11 */

  werase(field);
  box(field, 0, 0);
  mvwprintw(field, fh / 2 - 1, (fw - 9) / 2, "GAME OVER");
  mvwprintw(field, fh / 2 + 1, (fw - 14) / 2, "final score %d", score);
  mvwaddstr(field, fh / 2 + 2, (fw - 22) / 2, "press any key to exit");
  wnoutrefresh(field);
  doupdate();
  getch();

  delwin(field);
  delwin(hud);
  endwin();

  printf("final score: %d\n", score);
  return 0;
}
