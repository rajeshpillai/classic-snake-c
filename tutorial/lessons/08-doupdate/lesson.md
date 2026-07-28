# Lesson 8 — `doupdate`, and why things flicker

**Program:** `flicker.c` · **Links:** `-lncurses`

```sh
make 08
```

Press SPACE to cycle the three strategies. The difference is most obvious
over ssh or in a slow terminal.

## `wrefresh` is two operations

```c
wrefresh(win)  ==  wnoutrefresh(win);  doupdate();
```

| Call | What it does |
| --- | --- |
| `wnoutrefresh(win)` | copy the window into the **virtual screen** — no I/O |
| `doupdate()` | diff the virtual screen against the physical one, write the difference |

Only `doupdate()` talks to the terminal.

So refreshing four windows with `wrefresh` produces **four** separate diffs
and four bursts of terminal output per frame. The terminal may render
between them, which is what tearing looks like.

The fix is to compose first and transmit once:

```c
wnoutrefresh(stdscr);
for (int i = 0; i < n; i++) wnoutrefresh(win[i]);
doupdate();                                  /* one update, one diff */
```

**Rule: with more than one window, always `wnoutrefresh` × N then
`doupdate` once.** With a single window `refresh()` is already optimal, which
is why the snake game gets away with it.

## Three buffers, not two

Lesson 1 said there were two. There are really three:

| Buffer | Written by | Read by |
| --- | --- | --- |
| your windows | `addch`, `printw`, … | `wnoutrefresh` |
| the virtual screen | `wnoutrefresh` | `doupdate` |
| the physical screen (`curscr`) | `doupdate` | — |

`wnoutrefresh` is the cheap step; `doupdate` is the one that costs I/O.

## `erase()` vs `clear()`

These look interchangeable. They are not.

```c
erase();    /* blank the window's cells in the buffer */
clear();    /* erase() + clearok() -- forget what's on screen too */
```

`erase()` sets the buffer to blanks. The next `refresh()` still diffs, so if
you redraw the same content, curses sends almost nothing.

`clear()` additionally sets a flag telling curses to discard its model of the
physical screen. The next refresh therefore repaints **every cell** — often
emitting a terminal clear-screen sequence first, which is the visible flash.

Calling `clear()` every frame is *the* classic curses flicker bug. Run
`flicker.c` and cycle to the third strategy to see it.

```
per frame:   erase()  ->  ~20 cells changed  ->  ~60 bytes sent
             clear()  ->  every cell "changed" -> ~4000 bytes + flash
```

Use `clear()` exactly once, at startup, or after something outside your
control has corrupted the screen (a subprocess wrote to the terminal). Use
`erase()` everywhere else.

The snake game does this correctly — `erase()` at the top of `draw()`.

## Redrawing overlapped windows

Curses tracks changes per window and has no idea window B painted over
window A. When B moves away, A's cells look unchanged, so nothing is resent
and you're left with a hole.

```c
touchwin(win);       /* "assume everything changed" */
wrefresh(win);
```

`touchwin` forces a full repaint of that window on the next refresh.
`redrawwin` goes further and also discards the physical-screen model for
those lines. There's also `touchline(win, start, count)` when you know which
rows are affected.

## Other performance levers

```c
leaveok(win, TRUE);
```

Tells curses you don't care where the hardware cursor ends up, so it can skip
the cursor-positioning sequence at the end of each update. Combined with
`curs_set(0)` it's a genuine saving in an animation loop.

```c
idlok(win, TRUE);      /* allow hardware insert/delete-line */
scrollok(win, TRUE);   /* let writes past the bottom scroll the window */
immedok(win, TRUE);    /* refresh on every single change -- almost always wrong */
```

`immedok` is a trap: it makes every `addch` do a screen update. It exists for
tiny status windows; using it on anything busy is catastrophic.

## What a frame should look like

```c
for (;;) {
    handle_input();
    update_state();

    werase(a); draw_a();
    werase(b); draw_b();

    wnoutrefresh(a);
    wnoutrefresh(b);
    doupdate();          /* exactly one per frame */

    napms(33);
}
```

One `doupdate` per frame, `erase` not `clear`, and no I/O in the middle of
composing. Lesson 11 adds proper timing to this skeleton.

## Exercises

1. Run `flicker.c` over ssh (or `ssh localhost`) and compare the strategies.
2. Add `leaveok(stdscr, TRUE)` and see whether you can tell the difference.
3. Instrument it: run under `script` and compare output byte counts per
   strategy with `wc -c`.
4. Change `clear()` to `erase()` in the third strategy and confirm the
   flicker disappears.

---

Previous: **[Lesson 7](../07-windows/lesson.md)** ·
Next: **[Lesson 9 — Pads](../09-pads/lesson.md)**
