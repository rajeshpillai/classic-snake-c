# Lesson 7 — Windows

**Program:** `windows.c` · **Links:** `-lncurses`

```sh
make 07
```

TAB cycles focus, arrows move the marker.

## What a window is

A `WINDOW` is a rectangular buffer of cells with its own cursor, its own
attribute state, and its own coordinate system. `stdscr` is one — a window
covering the whole terminal, created for you by `initscr()`.

```c
WINDOW *w = newwin(rows, cols, begin_y, begin_x);
```

Size first, then position, both in (y, x) order. All four are in *screen*
coordinates, but everything you subsequently draw into `w` is in
**window-relative** coordinates: `(0, 0)` is the window's own top-left.

```c
mvwaddch(w, 0, 0, 'X');   /* top-left OF THE WINDOW, not of the screen */
```

Free it when you're done:

```c
delwin(w);
```

`delwin` frees the structure but does not erase the window's area from the
screen. Erase it first if that matters.

## Two reasons to bother

**Clipping.** Anything you draw outside a window's bounds is discarded
rather than spilling into whatever is next to it. `windows.c` writes a
deliberately over-long string in each pane to show this. Without windows
you'd have to bounds-check every string yourself.

**Independent state.** Each window has its own cursor position and current
attributes, so a function that draws into one window can't leave the rest
of the screen bold.

## The `w` naming convention, again

Every drawing function has a window variant:

| stdscr | window | with move |
| --- | --- | --- |
| `addch` | `waddch` | `mvwaddch` |
| `printw` | `wprintw` | `mvwprintw` |
| `attron` | `wattron` | — |
| `refresh` | `wrefresh` | — |
| `erase` | `werase` | — |
| `getch` | `wgetch` | — |

The non-`w` version is a macro for the `w` version applied to `stdscr`.
Learn one, you know both.

Note the argument order for the `mvw` family: **window first, then y, then
x**. `mvwaddch(win, y, x, ch)`. Getting `mvwaddch(y, x, win, ch)` past the
compiler is unlikely, but `mvwprintw` with its varargs will happily accept
nonsense.

## Refresh order determines what's on top

Curses keeps **no stacking order**. Windows are painted in the order you
refresh them, and the last one wins:

```c
refresh();        /* stdscr -- paints its blanks over the whole screen */
wrefresh(w);      /* now w, on top */
```

Refresh `stdscr` *before* the windows that sit on it, never after.

This is also why `windows.c` calls `refresh()` once at the start, then only
ever `wrefresh()`s individual panes. If it called `refresh()` inside the
loop, the panes would be erased on every keypress.

Two related functions for when things get out of sync:

| Function | Effect |
| --- | --- |
| `touchwin(w)` | mark the whole window as changed, forcing a full repaint next refresh |
| `redrawwin(w)` | as above, plus discard what curses believes is on screen |

You need `touchwin` when one window overlaps another: curses only sends what
*it* thinks changed, and it doesn't know window B painted over window A.

## Windows do not overlap gracefully

Two overlapping windows will scribble over each other, and the only way to
fix it is to refresh them in the right order and `touchwin` the one
underneath. With three or more it becomes unmanageable by hand.

That's what the `panel` library is for — lesson 10.

## Subwindows

```c
WINDOW *sub = derwin(parent, rows, cols, rel_y, rel_x);   /* relative */
WINDOW *sub = subwin(parent, rows, cols, abs_y, abs_x);   /* absolute */
```

These **share the parent's memory** rather than owning their own. Writing to
the subwindow changes the parent's buffer directly.

The usual use is carving the interior out of a bordered window so you don't
have to keep offsetting by 1:

```c
WINDOW *frame = newwin(10, 40, 2, 2);
box(frame, 0, 0);
WINDOW *body = derwin(frame, 8, 38, 1, 1);   /* inside the border */
mvwaddstr(body, 0, 0, "starts just inside the box");
```

The catch: because they share memory, changes in one require `touchwin` on
the other before a refresh will show them. Delete children before parents.

## Input from a window

```c
int ch = wgetch(win);
```

`wgetch` implicitly refreshes `win` first, which surprises people — it's why
a stray `wgetch` on the wrong window can make it jump to the front. It also
means `keypad()` and `nodelay()` are **per-window** settings:

```c
keypad(win, TRUE);       /* arrow keys decoded for wgetch(win) */
nodelay(win, TRUE);      /* non-blocking for wgetch(win) */
```

Setting `keypad(stdscr, TRUE)` and then reading with `wgetch(other)` gives
you raw escape sequences again. In practice, keep reading input from
`stdscr` (as `windows.c` does) and use the other windows for output only —
it's simpler and avoids the accidental refresh.

## Exercises

1. Remove the `touchwin`-free assumption: make two panes overlap and watch
   them corrupt each other.
2. Give each pane a `derwin` interior and drop the manual `+1` offsets.
3. Move `refresh()` inside the key loop and see the panes vanish.
4. Add a pane whose `wgetch` you read directly, with `keypad` off, and
   observe the arrow keys break.

---

Previous: **[Lesson 6](../06-lines-and-boxes/lesson.md)** ·
Next: **[Lesson 8 — doupdate and flicker](../08-doupdate/lesson.md)**
