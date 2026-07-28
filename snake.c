/*
 * classic-snake -- a terminal Snake clone built on ncurses.
 *
 * Rules of the house:
 *   - walls wrap you around instead of killing you
 *   - the '%' obstacles and your own body do kill you
 *   - '*' is normal food (+10), '$' is golden food (+50) but it rots away
 *
 * Build with `make`, play with the arrow keys or wasd, 'p' pauses, 'q' quits.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>

#define MAX_SNAKE_LEN		1024
#define MAX_OBSTACLES		40
#define GOLDEN_CHANCE		5	/* 1 in N food spawns is golden */
#define GOLDEN_LIFETIME 	60	/* ticks before golden food expires */
#define HIGH_SCORE_FILE		".snake_highscore"
#define INITIAL_DELAY_US 	100000

/* Below this the playfield has no room for a snake, let alone obstacles. */
#define MIN_WIDTH		20
#define MIN_HEIGHT		10

typedef struct {
  int x, y;
} Point;

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;

/* ncurses colour pair ids -- 0 is reserved by curses, so we start at 1. */
enum {
  CP_SNAKE = 1,
  CP_FOOD,
  CP_GOLDEN,
  CP_WALL,
  CP_OBSTACLE,
  CP_TEXT
};

/* snake[0] is the head, snake[snake_len - 1] the tip of the tail. */
static Point snake [MAX_SNAKE_LEN];
static int snake_len;
static Direction dir;

static Point food;
static int food_is_golden;
static int golden_ticks_left;

static Point obstacles[MAX_OBSTACLES];
static int obstacle_count;

static int score;
static int high_score;
static int width, height;
static int paused;
static int use_color;

//high score persistence

/* Returns a pointer to a static buffer -- fine here, we only ever use it
 * immediately in fopen() and never hold on to it. */
static const char *high_score_path(void) {
  static char path[512];
  const char *home = getenv("HOME");
  if (home) {
    snprintf(path, sizeof(path), "%s/%s", home, HIGH_SCORE_FILE);
  } else {
    /* No HOME (cron, weird shell)? Settle for the current directory. */
    snprintf(path, sizeof(path), "./%s", HIGH_SCORE_FILE);
  }

  return path;
}

static void load_high_score(void) {
  FILE *f = fopen(high_score_path(), "r");
  high_score = 0;
  /* A missing or corrupt file just means "no high score yet". */
  if (f) {
    if (fscanf(f, "%d", &high_score) != 1) high_score = 0;
    fclose(f);
  }
}

static void save_high_score(void) {
  if (score <= high_score) return;
  high_score = score;
  FILE *f = fopen(high_score_path(), "w");
  if (f) {
    fprintf(f, "%d\n", high_score);
    fclose(f);
  }
}

// placement holders

/* Is this cell already taken? Food is optional because spawn_food() is
 * allowed to land on the spot the old food just vacated. */
static int occupied(int x, int y, int check_food) {
  for (int i = 0; i < snake_len; i++) {
    if (snake[i].x == x && snake[i].y == y) return 1;
  }

  for (int i = 0; i < obstacle_count; i++) {
    if (obstacles[i].x == x && obstacles[i].y == y) return 1;
  }

  if (check_food && food.x == x && food.y == y) return 1;

  return 0;
}

static void spawn_food(void) {
  /* Rejection sampling. The board can in principle fill up, so cap the
   * number of tries rather than spinning forever. */
  int tries = 0;
  do {
    food.x = rand() % (width - 2) + 1;
    food.y = rand() % (height - 2) + 1;
  } while (occupied(food.x, food.y, 0) && ++tries < 1000);

  food_is_golden = (rand() % GOLDEN_CHANCE == 0);
  golden_ticks_left = GOLDEN_LIFETIME;
}


static void spawn_obstacles(void) {
  int target = height / 4;
  if (target > MAX_OBSTACLES) target = MAX_OBSTACLES;

  obstacle_count = 0;
  for (int i = 0; i < target; i++) {
    int x, y;
    int tries = 0;
    /* Keep a clear pocket around the middle so the snake doesn't spawn
     * nose-first into a rock it never had a chance to dodge. */
    do {
      x = rand() % (width - 2) + 1;
      y = rand() % (height - 2) + 1;
    } while ((occupied(x, y, 1) ||
              (abs(x - width / 2) < 8 && abs(y - height / 2) < 3)) &&
             ++tries < 1000);

    obstacles[i].x = x;
    obstacles[i].y = y;
    /* Publish each rock as we go, so occupied() sees the ones already
     * placed and we never stack two on the same cell. */
    obstacle_count = i + 1;
  }
}

static void init_game(void) {
  getmaxyx(stdscr, height, width);
  snake_len = 3;
  dir = DIR_RIGHT;
  score = 0;
  paused = 0;

  /* Lay the snake out horizontally with the head on the right, matching
   * the initial DIR_RIGHT heading. */
  int start_x = width / 2;
  int start_y = height / 2;
  for (int i = 0; i < snake_len; i++) {
    snake[i].x = start_x - i;
    snake[i].y = start_y;
  }

  /* Park the food off-board so spawn_obstacles() doesn't dodge a stale
   * position left over from the previous game. */
  food.x = -1;
  food.y = -1;

  obstacle_count = 0; // place before food so food avoids them
  spawn_obstacles();
  spawn_food();
}

// drawing
static void draw(void) {
  erase();

  /* Border. Purely decorative -- movement wraps, it never collides. */
  if (use_color) attron(COLOR_PAIR(CP_WALL));
  for (int x = 0; x < width; x++) {
    mvaddch(0, x, '#');
    mvaddch(height - 1, x, '#');
  }

  for (int y = 0; y < height; y++) {
    mvaddch(y, 0, '#');
    mvaddch(y, width - 1, '#');
  }

  if (use_color) attroff(COLOR_PAIR(CP_WALL));

  if (use_color) attron(COLOR_PAIR(CP_OBSTACLE));

  for (int i = 0; i < obstacle_count; i++) {
    mvaddch(obstacles[i].y, obstacles[i].x, '%');
  }

  if (use_color) attroff(COLOR_PAIR(CP_OBSTACLE));

  if (use_color) attron(COLOR_PAIR(food_is_golden ? CP_GOLDEN : CP_FOOD));
  mvaddch(food.y, food.x, food_is_golden ? '$' : '*');

  if (use_color) attroff(COLOR_PAIR(food_is_golden ? CP_GOLDEN : CP_FOOD));

  if (use_color) attron(COLOR_PAIR(CP_SNAKE));

  /* Capital 'O' for the head so you can tell which way you're pointing. */
  for (int i = 0; i < snake_len; i++) {
    mvaddch(snake[i].y, snake[i].x, i == 0 ? 'O' : 'o');
  }

  if (use_color) attroff(COLOR_PAIR(CP_SNAKE));

  if (use_color) attron(COLOR_PAIR(CP_TEXT));

  /* Scoreboard sits on the top border -- it's just decoration anyway. */
  mvprintw(0, 2, " Score: %d High: %d ", score, high_score);
  if (paused) {
    static const char *msg = "PAUSED";
    mvprintw(height / 2, (width - 6) / 2, "%s", msg);
  }

  if (use_color) attroff(COLOR_PAIR(CP_TEXT));

  refresh();
}

// input

/* Drains everything curses has buffered, so holding a key down doesn't
 * queue up a stack of turns. Returns 0 when the player asked to quit. */
static int handle_input(void) {
  int ch;

  while ((ch = getch()) != ERR) {
    switch (ch) {
      case 'w':
      case KEY_UP:
          /* Reversing straight into your own neck is instant death, so
           * we quietly ignore a 180-degree turn. */
          if (dir != DIR_DOWN) dir = DIR_UP;
          break;

      case 's':
      case KEY_DOWN:
          if (dir != DIR_UP) dir = DIR_DOWN;
          break;

      case 'a':
      case KEY_LEFT:
          if (dir != DIR_RIGHT) dir = DIR_LEFT;
          break;

      case 'd':
      case KEY_RIGHT:
          if (dir != DIR_LEFT) dir = DIR_RIGHT;
          break;

      case 'p':
          paused = !paused;
          break;

      case 'q':
          return 0;
    }
  }
  return 1;
}


// simulation -> return 0 if game over

static int step (void) {
  Point next = snake[0];
  switch (dir) {
    case DIR_UP: next.y--; break;
    case DIR_DOWN: next.y++; break;
    case DIR_LEFT: next.x--; break;
    case DIR_RIGHT: next.x++; break;
  }

  // Wrap around instead of dying on walls
  if (next.x <= 0) next.x = width - 2;
  else if (next.x >= width - 1) next.x = 1;

  if (next.y <= 0) next.y = height - 2;
  else if (next.y >= height - 1) next.y = 1;

  /* Work out whether we're eating first -- it decides whether the tail
   * cell is about to be vacated, which the self-collision test needs. */
  int grew = (next.x == food.x && next.y == food.y);

  // obstacle collision
  for (int i = 0; i < obstacle_count; i++) {
    if (obstacles[i].x == next.x && obstacles[i].y == next.y)
      return 0;
  }

  /* Self collision. When we're not growing the last segment moves out of
   * the way this same tick, so chasing your own tail tip is legal. */
  int body_len = grew ? snake_len : snake_len - 1;
  for (int i = 0; i < body_len; i++) {
    if (snake[i].x == next.x && snake[i].y == next.y)
      return 0;
  }

  int new_len = snake_len;

  if (grew) {
    int add = food_is_golden ? 3 : 1;
    score += food_is_golden ? 50 : 10;

    /* Clamp against the fixed-size array -- a maxed-out snake just stops
     * growing rather than scribbling past the end of it. */
    if (new_len + add > MAX_SNAKE_LEN) add = MAX_SNAKE_LEN - new_len;

    /* Seed the fresh segments on top of the current tail tip. They stay
     * hidden under it and unspool naturally as the snake moves on. */
    for (int i = new_len; i < new_len + add; i++) snake[i] = snake[new_len - 1];
    new_len += add;
  }

  // Shuffle every segment into the one ahead of it, then plant the head.
  for (int i = new_len - 1; i > 0; i--) {
    snake[i] = snake[i - 1];
  }

  snake[0] = next;
  snake_len = new_len;

  if (grew) {
    beep();
    spawn_food();
  } else if (food_is_golden) {
    /* Golden food is on a timer; when it rots we roll a fresh piece. */
    if (--golden_ticks_left <= 0) spawn_food();
  }

  return 1;
}

// screens
static void game_over_screen(void) {
  save_high_score();
  nodelay(stdscr, FALSE);   /* block on getch() so the screen actually waits */
  erase();
  if (use_color) attron(COLOR_PAIR(CP_TEXT));
  mvprintw(height / 2 - 1, (width - 10) / 2, "GAME OVER");
  mvprintw(height / 2, (width - 20) / 2, "Final score: %d", score);
  mvprintw(height / 2 + 1, (width - 20) / 2, "High score: %d", high_score);
  mvprintw(height / 2 + 2, (width - 20) / 2, "Press any key to exit...");

  if (use_color) attroff(COLOR_PAIR(CP_TEXT));
  refresh();

  flushinp();   /* drop keys mashed during play so we don't exit instantly */
  getch();
}


// main

int main(void) {
  srand(time(NULL));
  load_high_score();

  initscr();
  cbreak();                 /* deliver keys immediately, no line buffering */
  noecho();                 /* don't echo the player's keystrokes */
  keypad(stdscr, TRUE);     /* decode the arrow keys into KEY_* */
  curs_set(0);              /* hide the hardware cursor */
  nodelay(stdscr, TRUE);    /* getch() returns ERR instead of blocking */

  getmaxyx(stdscr, height, width);
  if (width < MIN_WIDTH || height < MIN_HEIGHT) {
    endwin();
    fprintf(stderr, "Terminal too small: need at least %dx%d, got %dx%d.\n",
            MIN_WIDTH, MIN_HEIGHT, width, height);
    return 1;
  }

  use_color = has_colors();
  if (use_color) {
    start_color();
    init_pair(CP_SNAKE, COLOR_GREEN, COLOR_BLACK);
    init_pair(CP_FOOD, COLOR_RED, COLOR_BLACK);
    init_pair(CP_GOLDEN, COLOR_YELLOW, COLOR_BLACK);
    init_pair(CP_WALL, COLOR_CYAN, COLOR_BLACK);
    init_pair(CP_OBSTACLE, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CP_TEXT, COLOR_WHITE, COLOR_BLACK);
  }

  init_game();
  draw();   /* show the board before the first tick moves anything */

  int useconds = INITIAL_DELAY_US;
  int running = 1;

  while (running) {
    if (!handle_input()) break;

    if (!paused) {
      if (!step()) break;
      draw();
      /* Speed up as the score climbs, but never past the floor. */
      useconds = INITIAL_DELAY_US - (score * 500);
      if (useconds < 50000) useconds = 50000;
    } else {
      draw();
    }
    /* While paused we poll faster so unpausing feels instant. */
    usleep(paused ? 50000 : useconds);
  }

  game_over_screen();
  endwin();

  printf("Final score: %d (high score: %d)\n", score, high_score);
  return 0;
}
