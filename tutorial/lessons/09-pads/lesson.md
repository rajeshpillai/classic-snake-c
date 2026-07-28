# Lesson 9 — Pads

**Program:** `pad.c` · **Links:** `-lncurses`

```sh
make 09
```

Arrows scroll, PgUp/PgDn page, Home returns to the origin.

## A pad is a window with no position

```c
WINDOW *pad = newpad(rows, cols);
```

No `begin_y`, no `begin_x` — because a pad isn't on the screen. It's a buffer
that can be **larger than the terminal**, and you display a rectangle of it
on demand.

That's the whole idea. Build your content once at full size, then move a
viewport over it. Scrolling becomes a matter of changing two integers, with
no redrawing at all.

Everything else works normally: `mvwaddch`, `wattron`, `box` all take a pad
just like a window, since a pad *is* a `WINDOW`. Free it with `delwin`.

## Displaying part of it

```c
prefresh(pad, pad_y, pad_x, top, left, bottom, right);
pnoutrefresh(pad, pad_y, pad_x, top, left, bottom, right);   /* + doupdate() */
```

Six coordinates, in two groups:

| Arguments | Meaning |
| --- | --- |
| `pad_y, pad_x` | which cell of the **pad** appears at the top-left of the view |
| `top, left` | top-left corner of the **screen** rectangle to fill |
| `bottom, right` | bottom-right corner of that rectangle, **inclusive** |

The last pair is inclusive corners, not a width and height. This is the
single most common pad bug:

```c
/* WRONG -- treats the last two as a size */
prefresh(pad, 0, 0, 3, 2, view_h, view_w);

/* right -- inclusive bottom-right corner */
prefresh(pad, 0, 0, 3, 2, 3 + view_h - 1, 2 + view_w - 1);
```

Note you must use `prefresh`/`pnoutrefresh` — calling plain `wrefresh()` on a
pad is undefined, because curses has no idea where to put it.

## Clamp your origin

`prefresh` returns `ERR` and draws **nothing** if the requested rectangle
doesn't fit inside the pad. Since nobody checks the return value, an
unclamped scroll manifests as the screen simply freezing at the edge.

```c
if (pad_y < 0) pad_y = 0;
if (pad_y > PAD_H - view_h) pad_y = PAD_H - view_h;
```

Also worth guarding: if the pad is *smaller* than the viewport,
`PAD_H - view_h` goes negative and clamps your origin to a negative number.
Real code should `max(0, ...)` the result.

## When to use a pad

Good fits:

- A scrollback log where the content is generated once and read many times.
- A file or man-page viewer.
- A game map or diagram larger than the screen.
- Anything with horizontal scrolling — wide tables especially.

Poor fits:

- Content that changes constantly. You pay to render the whole pad whether
  it's visible or not; a 10,000-line pad is 10,000 lines of memory and of
  drawing work.
- Very large data sets. At some point the right answer is a normal window
  plus an index into your own data — render only the visible rows. A pad is
  a convenience, not a virtualisation strategy.

The crossover is roughly: if the content fits comfortably in memory and
doesn't change every frame, use a pad; otherwise render the viewport
yourself.

## Mixing pads with normal windows

Pads participate in the virtual-screen system, so they compose with
`doupdate` exactly as windows do:

```c
wnoutrefresh(stdscr);                          /* header and frame */
pnoutrefresh(pad, py, px, t, l, b, r);         /* the scrolling body */
doupdate();                                    /* one update */
```

`pad.c` does this so the header, the viewport frame and the pad contents all
land in a single terminal update.

## Subpads

```c
WINDOW *sp = subpad(pad, rows, cols, y, x);
```

The pad equivalent of `derwin` — shares the parent's memory. Same caveats:
`touchwin` when you cross between them, delete children first.

## Exercises

1. Break the inclusive-corner rule deliberately and see the viewport
   misdraw.
2. Remove the clamping and scroll to the edge — note the freeze, not a
   crash.
3. Add a scrollbar down the right edge showing `pad_y / PAD_H`.
4. Rewrite `pad.c` to render only the visible rows into a normal window,
   and compare the memory use for a 100,000-row document.

---

Previous: **[Lesson 8](../08-doupdate/lesson.md)** ·
Next: **[Lesson 10 — Panels](../10-panels/lesson.md)**
