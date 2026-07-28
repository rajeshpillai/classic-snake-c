/*
 * Lesson 11 -- timing, and the shape of a real game loop.
 *
 * Three bouncing balls, run under three different pacing strategies so
 * you can see what each one gets wrong. This is the lesson that explains
 * why the snake game's main loop looks the way it does -- and where it
 * could be better.
 *
 * SPACE cycles the strategy, +/- change the target rate, 'q' quits.
 */

#include <ncurses.h>
#include <stdio.h>
#include <time.h>

#define NBALL 3

enum Pacing { P_SLEEP_FIXED, P_SLEEP_COMPENSATED, P_TIMEOUT, P_COUNT };

static const char *pacing_name[] = {
  "napms(delay)           -- what the snake game does",
  "compensated sleep      -- subtract the time the frame took",
  "timeout(delay)+getch() -- let curses do the waiting"
};

static const char *pacing_note[] = {
  "Frame time = work + delay, so the real rate is always slower than asked.",
  "Measures the work, sleeps only the remainder. Rate holds steady.",
  "No separate sleep at all: getch() returns ERR when the timer expires."
};

typedef struct { double y, x, dy, dx; } Ball;

/* Everything the key handler is allowed to touch, in one place, so the
 * two call sites below stay in sync. */
typedef struct {
  enum Pacing pacing;
  int target_ms;
  int running;
  int reset_stats;
} State;

/* A monotonic clock in milliseconds. CLOCK_MONOTONIC rather than
 * gettimeofday() so that an NTP step or a DST change cannot make a frame
 * appear to take negative time. */
static long now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

static void handle_key(State *s, int ch) {
  switch (ch) {
    case 'q':
      s->running = 0;
      break;
    case ' ':
      s->pacing = (s->pacing + 1) % P_COUNT;
      s->reset_stats = 1;
      break;
    case '+': case '=':
      s->target_ms -= 5;
      break;
    case '-': case '_':
      s->target_ms += 5;
      break;
    default:
      return;
  }
  if (s->target_ms < 5)   s->target_ms = 5;
  if (s->target_ms > 200) s->target_ms = 200;
}

int main(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  nodelay(stdscr, TRUE);

  int height, width;
  getmaxyx(stdscr, height, width);
  if (height < 16 || width < 64) {
    endwin();
    fprintf(stderr, "need at least 64x16, got %dx%d\n", width, height);
    return 1;
  }

  int use_color = has_colors();
  if (use_color) {
    start_color();
    init_pair(1, COLOR_GREEN,  COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_CYAN,   COLOR_BLACK);
  }

  const int top = 6;                       /* playfield starts below the HUD */
  Ball ball[NBALL] = {
    { top + 2, 10, 0.55,  0.9 },
    { top + 5, 30, -0.4,  1.3 },
    { top + 3, 50, 0.7,  -1.1 },
  };

  State st = { P_SLEEP_FIXED, 33, 1, 0 };  /* ~30 fps to start */

  long frames = 0;
  long worst_frame = 0;
  double measured_fps = 0.0;
  long fps_window_start = now_ms(), fps_window_frames = 0;

  while (st.running) {
    long frame_start = now_ms();

    /* --- input ------------------------------------------------------ */
    /* Drain the queue. With nodelay on, this returns everything buffered
     * and then ERR. Without the drain, holding a key builds a backlog
     * that keeps acting after the key is released. */
    int ch;
    while ((ch = getch()) != ERR) handle_key(&st, ch);
    if (!st.running) break;

    /* --- simulate --------------------------------------------------- */
    for (int i = 0; i < NBALL; i++) {
      ball[i].y += ball[i].dy;
      ball[i].x += ball[i].dx;
      if (ball[i].y < top + 1)    { ball[i].y = top + 1;    ball[i].dy = -ball[i].dy; }
      if (ball[i].y > height - 3) { ball[i].y = height - 3; ball[i].dy = -ball[i].dy; }
      if (ball[i].x < 1)          { ball[i].x = 1;          ball[i].dx = -ball[i].dx; }
      if (ball[i].x > width - 2)  { ball[i].x = width - 2;  ball[i].dx = -ball[i].dx; }
    }

    /* --- draw ------------------------------------------------------- */
    erase();
    mvprintw(1, 2, "Pacing: %s", pacing_name[st.pacing]);
    mvprintw(2, 2, "%s", pacing_note[st.pacing]);
    mvprintw(3, 2, "target %3d ms (%.1f fps)   measured %.1f fps   worst frame %ld ms",
             st.target_ms, 1000.0 / st.target_ms, measured_fps, worst_frame);
    mvprintw(4, 2, "SPACE cycles pacing   +/- change rate   q quits");

    mvhline(top, 0, ACS_HLINE, width);
    mvhline(height - 2, 0, ACS_HLINE, width);

    for (int i = 0; i < NBALL; i++) {
      if (use_color) attron(COLOR_PAIR(i + 1));
      mvaddch((int) ball[i].y, (int) ball[i].x, 'O');
      if (use_color) attroff(COLOR_PAIR(i + 1));
    }
    refresh();

    frames++;
    fps_window_frames++;

    /* Recompute the observed rate once a second. */
    long t = now_ms();
    if (t - fps_window_start >= 1000) {
      measured_fps = fps_window_frames * 1000.0 / (t - fps_window_start);
      fps_window_start = t;
      fps_window_frames = 0;
    }

    long work_ms = now_ms() - frame_start;
    if (work_ms > worst_frame) worst_frame = work_ms;

    /* --- pace ------------------------------------------------------- */
    switch (st.pacing) {
      case P_SLEEP_FIXED:
        /* Simple and wrong: the frame takes work_ms + target_ms, so the
         * actual rate is always below target, by however long the work
         * took. Fine when the work is trivial, which is why the snake
         * game gets away with it. */
        napms(st.target_ms);
        break;

      case P_SLEEP_COMPENSATED: {
        /* Sleep only the time left in the budget. If the frame overran,
         * don't sleep at all -- but also don't try to "catch up" by
         * running extra frames, which produces a stutter spiral. */
        long remaining = st.target_ms - work_ms;
        if (remaining > 0) napms((int) remaining);
        break;
      }

      case P_TIMEOUT: {
        /* No sleep call at all. getch() blocks until either a key
         * arrives or the timer expires, so a keypress is acted on the
         * instant it happens instead of waiting out the sleep. Best of
         * the three for anything interactive. */
        long remaining = st.target_ms - work_ms;
        timeout(remaining > 0 ? (int) remaining : 0);
        int k = getch();
        if (k != ERR) handle_key(&st, k);
        nodelay(stdscr, TRUE);   /* restore for the drain at the top */
        break;
      }

      default: break;
    }

    if (st.reset_stats) {
      st.reset_stats = 0;
      frames = 0;
      worst_frame = 0;
      measured_fps = 0.0;
      fps_window_start = now_ms();
      fps_window_frames = 0;
      nodelay(stdscr, TRUE);
    }
  }

  endwin();
  printf("ran %ld frames\n", frames);
  return 0;
}
