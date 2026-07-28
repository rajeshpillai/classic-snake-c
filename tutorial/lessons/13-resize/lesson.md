# Lesson 13 — Resize

**Program:** `resize.c` · **Links:** `-lncurses`

```sh
make 13
```

Then drag your terminal's corner around.

## What happens on resize

The terminal emulator sends `SIGWINCH` to the foreground process group.
ncurses installs a handler for it, and by the time your next `getch()`
returns you get a synthetic key:

```c
if (ch == KEY_RESIZE) { /* relayout */ }
```

Before delivering it, ncurses has already:

- updated `LINES` and `COLS`,
- resized `stdscr` to the new dimensions,
- resized `curscr` and the virtual screen.

What it has **not** done, and cannot do, is know how your windows should be
laid out. That's the part you write.

`keypad(stdscr, TRUE)` is required — `KEY_RESIZE` comes through the same
path as the function keys.

## The handler

```c
case KEY_RESIZE:
    getmaxyx(stdscr, height, width);      /* re-read the size */
    /* destroy and recreate every window at its new geometry */
    clear();                              /* discard resize artefacts */
    break;
```

Windows are not automatically resized, and there's no useful "resize this
window in place". `wresize()` exists but leaves the contents in an awkward
state, and `mvwin()` only moves. In practice, `delwin` + `newwin` and redraw
is both simpler and what you'd end up doing anyway, since a different size
usually means a different layout.

This is one of the few places `clear()` is right (see lesson 8): the resize
itself may have left artefacts the diff can't know about.

## Always handle "too small"

```c
if (height < MIN_H || width < MIN_W) {
    mvprintw(0, 0, "too small: %dx%d", width, height);
    refresh();
    continue;
}
```

Users drag terminals to absurd sizes. Without this check every `mvprintw`
past the edge silently fails (lesson 2) and you get a blank screen with no
explanation.

Note it has to be checked **every frame**, not just at startup — which is
exactly what the snake game gets wrong. It checks once in `main()` and never
again.

## Beware of NULL windows

```c
WINDOW *w = newwin(0, 0, y, x);   /* returns NULL, not a 0x0 window */
box(w, 0, 0);                     /* segfault */
```

`newwin` returns `NULL` for a degenerate size. After a resize to a tiny
terminal, a layout computing `height / 2` can easily produce zero. Clamp
before calling, as `rebuild()` in `resize.c` does.

Also: `delwin(NULL)` is **not** safe. Guard it.

## Handling SIGWINCH yourself

If you're not sitting in `getch()` — say you're blocked on `select()` over
several file descriptors — you won't see `KEY_RESIZE` promptly. Then you
handle the signal directly:

```c
static volatile sig_atomic_t got_winch;
static void on_winch(int sig) { (void) sig; got_winch = 1; }

/* in main: */
signal(SIGWINCH, on_winch);

/* in the loop: */
if (got_winch) {
    got_winch = 0;
    endwin();               /* drop the old terminal state */
    refresh();              /* re-init at the new size */
    resizeterm(0, 0);       /* 0,0 = query the OS for the real size */
    relayout();
}
```

Set a flag and nothing else. A signal handler may only call async-signal-safe
functions, and essentially nothing in curses qualifies — calling `refresh()`
from inside the handler is a genuine (if intermittent) crash.

`resize_term()` is the variant that doesn't touch your windows; `resizeterm()`
also resizes `stdscr` and friends. The latter is usually what you want.

## Applying this to the snake game

The snake game reads its dimensions once:

```c
static void init_game(void) {
    getmaxyx(stdscr, height, width);   /* once, at startup */
    ...
}
```

Resize the terminal mid-game and `width`/`height` are stale. The wrap logic
then teleports the snake to coordinates outside the new screen, where
`mvaddch` silently fails — the snake vanishes.

A minimal fix in `handle_input()`:

```c
case KEY_RESIZE: {
    int nh, nw;
    getmaxyx(stdscr, nh, nw);
    if (nw < MIN_WIDTH || nh < MIN_HEIGHT) { paused = 1; break; }
    width = nw; height = nh;
    /* pull anything now out of bounds back inside */
    for (int i = 0; i < snake_len; i++) {
        if (snake[i].x >= width - 1)  snake[i].x = width - 2;
        if (snake[i].y >= height - 1) snake[i].y = height - 2;
    }
    for (int i = 0; i < obstacle_count; i++) { /* same */ }
    if (food.x >= width - 1 || food.y >= height - 1) spawn_food();
    clear();
    break;
}
```

Pausing when the terminal becomes too small is friendlier than exiting —
the player can drag it back and carry on.

## Exercises

1. Add the `KEY_RESIZE` case above to the snake game and resize mid-game.
2. Make `resize.c` keep the box window centred rather than top-left.
3. Resize to 10x5 and confirm the "too small" path.
4. Add a `SIGWINCH` handler that only sets a flag, and drive the relayout
   from the flag instead of `KEY_RESIZE`.

---

Previous: **[Lesson 12](../12-mouse/lesson.md)** ·
Next: **[Lesson 14 — Menus and forms](../14-menus-and-forms/lesson.md)**
