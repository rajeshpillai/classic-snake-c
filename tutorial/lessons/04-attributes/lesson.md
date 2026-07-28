# Lesson 4 — Attributes

**Program:** `attrs.c` · **Links:** `-lncurses`

```sh
make 04
```

## A cell is not a character

Each cell in a curses window holds a `chtype`: an integer packing the
character *and* its styling together.

```
 chtype  =  [ colour pair ][ attribute bits ][ character ]
```

That's why you can OR them:

```c
mvaddch(y, x, 'X' | A_BOLD | A_REVERSE);
```

and why `addch()` takes a `chtype` rather than a `char`.

## The three ways to style

### 1. `attron` / `attroff` — add and remove

```c
attron(A_BOLD);
addstr("bold text");
attroff(A_BOLD);
```

These modify the window's *current* attribute set, which applies to
everything drawn afterwards. `attron` ORs bits in, `attroff` masks them out.
Combine with `|`:

```c
attron(A_BOLD | A_UNDERLINE);
```

### 2. `attrset` — replace wholesale

```c
attrset(A_UNDERLINE);   /* exactly this, nothing else */
attrset(A_NORMAL);      /* clear everything */
```

Use this when you want certainty rather than accumulation. Long functions
that `attron` in one branch and forget to `attroff` in another leave the rest
of the screen bold — `attrset(A_NORMAL)` at a known point is the cure.

### 3. Bake it into the character

```c
mvaddch(y, x, 'X' | A_REVERSE);
```

Cleanest when styling a single cell — no on/off pair to keep balanced.

## The standard attributes

| Attribute | Notes |
| --- | --- |
| `A_NORMAL` | zero; clears everything |
| `A_BOLD` | very widely supported, usually rendered as a brighter colour |
| `A_DIM` | half-bright; frequently ignored |
| `A_UNDERLINE` | very widely supported |
| `A_REVERSE` | swaps fg and bg; the most portable highlight there is |
| `A_STANDOUT` | "whatever this terminal's best highlight is" |
| `A_BLINK` | often disabled at the terminal level |
| `A_INVIS` | draws fg in the bg colour |
| `A_ITALIC` | ncurses 6+, terminal-dependent |
| `A_ALTCHARSET` | selects the line-drawing set (lesson 6) |

Ask before relying on any of them:

```c
attr_t have = termattrs();
if (have & A_ITALIC) { /* ... */ }
```

`A_REVERSE` and `A_BOLD` are safe everywhere. Everything else deserves a
fallback.

## `chgat` — restyle without redrawing

```c
mvchgat(row, 0, -1, A_REVERSE, 0, NULL);
```

Changes the attributes of characters already in the buffer, leaving the
characters themselves alone. The `-1` means "to end of line".

This is the natural way to highlight the selected row of a menu: draw the
text once, then move the highlight around with `chgat`. The arguments are
`(y, x, n, attr, color_pair, opts)` — `opts` is reserved and always `NULL`.

## Attributes are per-window

The current attribute set belongs to the window, not to the program. In a
multi-window program (lesson 7) each window tracks its own, and you use the
`w` variants:

```c
wattron(win, A_BOLD);
mvwaddstr(win, 1, 1, "bold, in win only");
wattroff(win, A_BOLD);
```

`attron(x)` is precisely `wattron(stdscr, x)`.

## The balance problem

`attron`/`attroff` pairs are easy to leave unbalanced across early returns
and error paths. Two habits that help:

1. `attrset(A_NORMAL)` at the top of each drawing function, so you never
   inherit state from whatever ran before.
2. Save and restore when writing a reusable helper:

```c
attr_t saved_attr;
short saved_pair;
attr_get(&saved_attr, &saved_pair, NULL);
/* ... mess with attributes ... */
attr_set(saved_attr, saved_pair, NULL);
```

## Where this shows up in the snake game

The snake game guards every attribute change on `use_color`:

```c
if (use_color) attron(COLOR_PAIR(CP_SNAKE));
for (int i = 0; i < snake_len; i++) mvaddch(...);
if (use_color) attroff(COLOR_PAIR(CP_SNAKE));
```

Colour pairs are attributes too — that's the next lesson.

## Exercises

1. Draw the same string with all nine attributes at once and see which
   survive on your terminal.
2. Replace an `attron`/`attroff` pair with `attrset` and note what leaks.
3. Write `highlight_row(int y)` using `mvchgat` and use it to build a
   two-item selector driven by the arrow keys.
4. Deliberately forget an `attroff` and observe how far the styling spreads.

---

Previous: **[Lesson 3](../03-input/lesson.md)** ·
Next: **[Lesson 5 — Colour](../05-color/lesson.md)**
