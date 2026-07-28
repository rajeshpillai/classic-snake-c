# Lesson 15 — Capstone: mini snake

**Program:** `minisnake.c` · **Links:** `-lncurses`

```sh
make 15
```

Same game as [`../../../snake.c`](../../../snake.c), rebuilt with everything
from the tutorial. About 150 lines of logic against the original's 260, and
it does *more*.

## What each lesson contributed

| Lesson | Applied as |
| --- | --- |
| 3 | `cbreak`/`noecho`/`keypad`, and draining the input queue |
| 4 | `A_BOLD` and `A_REVERSE` in the HUD |
| 5 | colour pairs with `use_default_colors()` and a mono fallback |
| 6 | `box()` and `ACS_DIAMOND` instead of `#` and `*` |
| 7 | separate `hud` and `field` windows |
| 8 | `wnoutrefresh` × 2 then one `doupdate` |
| 11 | `timeout()` pacing — zero input latency |
| 13 | `KEY_RESIZE` handling |

## The four real differences from `snake.c`

### 1. Two windows instead of one screen

The original draws the scoreboard *on top of* the border:

```c
mvprintw(0, 2, " Score: %d High: %d ", score, high_score);   /* over the wall */
```

Here the HUD is its own window above the playfield. The playfield window
then starts at `(0, 0)` in its own coordinates, so there's no offset
arithmetic anywhere — `field`'s row 1 is the first playable row, full stop.

That's the practical argument for windows: not clipping, but **not having to
add an offset to every coordinate**.

### 2. `timeout()` instead of `usleep()`

The original:

```c
if (!handle_input()) break;
if (!paused) { step(); draw(); }
usleep(useconds);           /* keys pressed during this wait sit idle */
```

Here the wait and the input read are one operation:

```c
timeout(paused ? 80 : tick_ms);
ch = getch();               /* returns the instant a key arrives */
nodelay(stdscr, TRUE);
do { /* handle ch */ } while ((ch = getch()) != ERR);   /* drain the rest */
```

The first `getch()` does the frame's waiting; the drain loop then runs
non-blocking to consume any buffered repeats. At a 110 ms tick this removes
up to 110 ms of input lag, and it's the single most noticeable improvement.

### 3. Resize handling

The original reads the terminal size once and never again. Resize it
mid-game and the wrap logic sends the snake to coordinates off the new
screen, where `mvaddch` silently fails.

Here `layout()` rebuilds both windows and `clamp_into_field()` pulls
anything now out of bounds back in. If the terminal becomes too small the
game pauses rather than exiting, so dragging it back resumes play.

### 4. One `doupdate` per frame

With two windows, `wrefresh` on each would mean two terminal updates per
frame and possible tearing between the HUD and the playfield. Composing and
pushing once is both faster and correct (lesson 8).

## Things the original does better

Not a one-way comparison:

- **High-score persistence.** `minisnake.c` drops it entirely.
- **Obstacles.** Also dropped, to keep the example readable.
- **Golden food.** Same.
- **The `Point`/`Direction` types are cleaner** in the original, and the
  file-scope-static style makes the state genuinely easy to follow.

Read them side by side. The original is a straightforward, honest curses
program; this one shows what the library gives you once you use more of it.

## Where to go next

**Finish this one.** Port the obstacles, golden food and high score across.
The interesting part is that `spawn_obstacles` now works in window
coordinates, which removes the border special-casing.

**Add an options screen** with the menu library from lesson 14, driving the
constants at the top of the file.

**Make the board bigger than the terminal** with a pad (lesson 9), scrolling
to keep the head centred.

**Add a pause dialog** as a panel (lesson 10) floating over the playfield
instead of a HUD flag.

**Go wide-character.** Link `-lncursesw`, call `setlocale(LC_ALL, "")`, and
draw the snake with `▓` and the food with `●`. That's the biggest visual
upgrade for the least code.

## Reference

The manual pages are genuinely good and are the right place to settle
details:

```sh
man ncurses          # overview and the full function index
man curs_getch       # input
man curs_attr        # attributes
man curs_color       # colour
man curs_refresh     # refresh / doupdate
man curs_pad         # pads
man panel            # panel library
man menu             # menu library
man form             # form library
```

`man ncurses` in particular lists every function grouped by topic — it's the
fastest way to find the call you half-remember.

## Exercises

1. Port obstacles from `snake.c` into `minisnake.c`.
2. Add the high-score file back, and show it in the HUD.
3. Replace the HUD's pause flag with a centred panel.
4. Add golden food with a rot timer, drawn in `A_BLINK` where supported.
5. Run it under `-fsanitize=address,undefined` and confirm it's clean.

---

Previous: **[Lesson 14](../14-menus-and-forms/lesson.md)** ·
Back to the **[index](../../README.md)**
