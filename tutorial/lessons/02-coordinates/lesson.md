# Lesson 2 — Coordinates

**Program:** `coords.c` · **Links:** `-lncurses`

```sh
make 02
```

Resize your terminal and run it again — the numbers follow.

## y comes first

```c
mvaddch(y, x, ch);     /* row, then column */
```

Every curses function that takes a position takes **row before column**. This
is the opposite of the (x, y) convention you know from graphics, and it is the
single most common source of "why is my screen blank" confusion.

The mnemonic that sticks: curses is descended from line-oriented terminal
handling, where the line number is the primary thing and the column is where
you are within it. `mvaddch(3, 10, 'x')` means "line 3, character 10".

The snake game keeps a `Point {int x, y;}` struct but always unpacks it in the
curses order:

```c
mvaddch(snake[i].y, snake[i].x, 'o');   /* .y first, always */
```

## Origin and bounds

The origin `(0, 0)` is the **top-left**. y increases *downward*. For a terminal
reporting `height` rows and `width` columns, the valid range is:

- `0 <= y <= height - 1`
- `0 <= x <= width - 1`

The off-by-one that bites: the bottom-right cell is `(height-1, width-1)`, not
`(height, width)`. That's why the snake game's border loop runs
`mvaddch(height - 1, x, '#')`.

## Getting the size

```c
int height, width;
getmaxyx(stdscr, height, width);   /* no ampersands! */
```

`getmaxyx` is a **macro**, not a function. It assigns to the variables you
name, so you pass the variables themselves. Writing `getmaxyx(stdscr, &h, &w)`
is a compile error, and a confusing one.

The same convention covers the whole family:

| Macro | Gives you |
| --- | --- |
| `getmaxyx(win, y, x)` | size of the window |
| `getyx(win, y, x)` | current cursor position |
| `getbegyx(win, y, x)` | window's top-left on the physical screen |
| `getparyx(win, y, x)` | position relative to the parent window |

There are also globals `LINES` and `COLS` holding the size of the terminal.
They're set by `initscr()` and updated on resize (lesson 13). Prefer
`getmaxyx(stdscr, ...)` — it works unchanged when you later switch to a real
window.

## Out of bounds fails silently

```c
int rc = mvaddch(999, 999, 'X');   /* returns ERR, draws nothing */
```

Curses does **not** crash, wrap, or clip-and-continue. It returns `ERR` and
does nothing. Since almost nobody checks the return value of `mvaddch`, a
coordinate bug shows up as *missing output*, not as an error.

When something you drew isn't there, check in this order:

1. Did you call `refresh()`?
2. Are your coordinates inside the window?
3. Did you swap y and x?

## The bottom-right corner problem

Writing to the very last cell `(height-1, width-1)` may return `ERR` even
though it's in range. After writing a character the cursor advances, and from
the last cell there is nowhere to advance to — many terminals scroll instead.
Curses refuses rather than scrolling your screen out from under you.

If you need that cell (drawing a full border, for example), the ways out are
`insch()`, which inserts without advancing, or simply accepting that the
corner is unreachable. The snake game sidesteps it: its border loop writes the
bottom row left-to-right and the `ERR` on the final cell is harmless, because
the wall is decorative.

## Cursor position vs. drawing position

`mvaddch(y, x, c)` is exactly `move(y, x)` followed by `addch(c)`. After it,
the cursor sits at `(y, x+1)`. This matters in two places:

- Consecutive `addstr` calls continue from wherever the last one stopped.
- Where you leave the cursor is where the terminal's blinking block ends up,
  which looks sloppy. Either park it somewhere sensible before `refresh()`,
  or hide it entirely with `curs_set(0)` (which the snake game does).

```c
curs_set(0);   /* invisible */
curs_set(1);   /* normal */
curs_set(2);   /* very visible, if the terminal supports it */
```

`curs_set` returns `ERR` if the terminal can't do it — safe to ignore.

## Centring text

There's no alignment support. You measure and subtract:

```c
mvprintw(height / 2, (width - (int) strlen(msg)) / 2, "%s", msg);
```

Cast `strlen`'s `size_t` to `int` before subtracting. If you don't and the
message is wider than the terminal, the subtraction underflows into a huge
unsigned value, and `(huge / 2)` is a nonsense column — the text vanishes with
no error. This is a real bug that appears in narrow terminals only.

## Exercises

1. Swap the arguments in one of the `mvaddstr` calls and see where it lands.
2. Remove `curs_set(0)` and watch where the cursor is left after drawing.
3. Write a `draw_centred(int y, const char *s)` helper and use it for all
   three centred strings.
4. Make the ruler handle a terminal narrower than 10 columns without
   misbehaving.

---

Previous: **[Lesson 1](../01-hello-curses/lesson.md)** ·
Next: **[Lesson 3 — Input](../03-input/lesson.md)**
