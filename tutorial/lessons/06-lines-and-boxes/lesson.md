# Lesson 6 — Lines and boxes

**Program:** `boxes.c` · **Links:** `-lncurses`

```sh
make 06
```

## The alternate character set

Terminals have carried a second character set for line drawing since the
VT100. Curses exposes it as the `ACS_*` constants — each is a `chtype` with
`A_ALTCHARSET` already set, so you use them exactly like ordinary characters:

```c
mvaddch(y, x, ACS_ULCORNER);
```

They degrade sensibly. If the terminal has no line-drawing set, ncurses
substitutes an ASCII approximation (`+`, `-`, `|`) automatically, so ACS is
*more* portable than hard-coding Unicode box characters, not less.

| Group | Constants |
| --- | --- |
| corners | `ACS_ULCORNER` `ACS_URCORNER` `ACS_LLCORNER` `ACS_LRCORNER` |
| lines | `ACS_HLINE` `ACS_VLINE` |
| junctions | `ACS_LTEE` `ACS_RTEE` `ACS_TTEE` `ACS_BTEE` `ACS_PLUS` |
| symbols | `ACS_DIAMOND` `ACS_BULLET` `ACS_DEGREE` `ACS_PLMINUS` |
| blocks | `ACS_BLOCK` `ACS_BOARD` `ACS_CKBOARD` |
| arrows | `ACS_LARROW` `ACS_RARROW` `ACS_UARROW` `ACS_DARROW` |

## `ACS_*` are not constants

Worth knowing before it bites you:

```c
#define ACS_ULCORNER  (acs_map['l'])
```

They're **runtime array lookups**, and `acs_map` is populated by `initscr()`
from the terminal's terminfo entry. Two consequences:

```c
static const chtype corner = ACS_ULCORNER;   /* compile error */
static struct { chtype ch; ... } tbl[] = { { ACS_HLINE, ... } };  /* same */
```

- You cannot use them in any static or global initialiser — the compiler
  rejects them as non-constant. Make the table a local (C99 allows
  non-constant initialisers for automatic aggregates), as `boxes.c` does.
- You cannot use them **before `initscr()`**. Doing so reads an
  uninitialised map and draws garbage.

## Repeating a character

```c
mvhline(y, x, ACS_HLINE, n);   /* n copies, horizontally  */
mvvline(y, x, ACS_VLINE, n);   /* n copies, vertically    */
```

Cleaner than the loop, and clipped to the window automatically — passing an
`n` that runs past the edge truncates rather than failing.

Any character works, not just ACS: `mvhline(y, x, '-', 20)` or
`mvhline(y, x, ' ', 20)` to clear a run.

## `box()` — the shortcut

```c
box(win, 0, 0);
```

Draws a complete border around a **window** (which is why `boxes.c` has to
create one — `box()` doesn't take `stdscr`-relative coordinates, it borders
the whole window). The two zeros mean "use the defaults", `ACS_VLINE` and
`ACS_HLINE`. Pass other characters to override:

```c
box(win, '|', '-');       /* ASCII border */
box(win, 0, 0);           /* proper lines */
```

For full control there's `wborder()`, which takes all eight pieces:

```c
wborder(win, ls, rs, ts, bs, tl, tr, bl, br);
```

Any argument of `0` falls back to the sensible ACS default.

**The border occupies the window's outermost cells.** A 10x40 window with a
box has 8x38 of usable interior, starting at (1, 1). Forgetting this and
writing to row 0 erases your own top border.

## The overlap problem

Note the ordering at the end of `boxes.c`:

```c
refresh();        /* stdscr repaint -- covers the window's area */
wrefresh(w);      /* so the window must be redrawn after it     */
```

`stdscr` is a full-screen window like any other. Refreshing it paints its
contents — including the blanks where your subwindow sits — over whatever
was there. Curses does **not** track stacking order.

This is the central difficulty with multiple windows, and it's what the
`panel` library exists to solve (lesson 10).

## Unicode box characters

You *can* write `"┌─┐"` instead, but only with the wide-character build:
link `-lncursesw`, call `setlocale(LC_ALL, "")` before `initscr()`, and use
`addwstr`/`add_wch` rather than `addstr`/`addch`. Then you get rounded
corners, double lines, and the full Unicode box set.

The tradeoff: it breaks on terminals in a non-UTF-8 locale, whereas ACS
degrades to ASCII on its own. For a game like snake, ACS is the better
default; for a modern developer tool where you control the environment,
wide characters are worth the setup.

## Applying it to the snake game

The snake border is drawn as `#`. Swapping to ACS is a small change:

```c
mvaddch(0, 0, ACS_ULCORNER);
mvaddch(0, width - 1, ACS_URCORNER);
mvaddch(height - 1, 0, ACS_LLCORNER);
mvaddch(height - 1, width - 1, ACS_LRCORNER);
mvhline(0, 1, ACS_HLINE, width - 2);
mvhline(height - 1, 1, ACS_HLINE, width - 2);
mvvline(1, 0, ACS_VLINE, height - 2);
mvvline(1, width - 1, ACS_VLINE, height - 2);
```

Eight calls instead of two loops, and it looks like a real border. Note the
bottom-right corner may fail to draw (lesson 2) — one reason games often
leave the last column alone.

## Exercises

1. Replace the snake game's `#` border with ACS and compare.
2. Split a boxed window into two panes with `ACS_TTEE`, `ACS_VLINE` and
   `ACS_BTEE`.
3. Write `draw_box(y, x, h, w)` that boxes an arbitrary rectangle of
   `stdscr` without creating a window.
4. Draw a progress bar from `ACS_BLOCK` and `ACS_CKBOARD`.

---

Previous: **[Lesson 5](../05-color/lesson.md)** ·
Next: **[Lesson 7 — Windows](../07-windows/lesson.md)**
