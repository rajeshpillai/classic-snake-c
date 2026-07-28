# Lesson 3 — Input

**Programs:** `keylog.c`, `modes.c` · **Links:** `-lncurses`

```sh
make 03                              # keylog
./lessons/03-input/modes
```

## Terminal input modes

By default a Unix terminal is in **cooked** mode: the kernel buffers a whole
line, handles backspace for you, and only hands your program the text when the
user presses Enter. That's right for `cat`, useless for a game.

Curses gives you three alternatives:

| Call | Line buffering | Signals (Ctrl-C, Ctrl-Z) | Flow control (Ctrl-S) |
| --- | --- | --- | --- |
| `nocbreak()` | on (cooked, the default) | handled | handled |
| `cbreak()` | **off** | handled | handled |
| `raw()` | **off** | **passed to you as keys** | **passed to you** |

`cbreak()` is what you want almost always. Keys arrive immediately, but Ctrl-C
still kills the program — which users expect, and which saves you when your
event loop hangs.

`raw()` is for when you genuinely need Ctrl-C as an input character (a
terminal emulator, an editor with its own keymap). Use it knowing you've just
taken responsibility for every escape hatch the user had.

## Echo

```c
noecho();      /* the tty does not print what the user types */
echo();        /* it does (the default) */
```

With `noecho()`, *you* decide what appears. Any interface where the keystroke
isn't literal text — a game, a menu, a password prompt — wants this on.

## keypad — decoding the arrow keys

Press the up arrow and the terminal sends three bytes: `ESC`, `[`, `A`. Read
those raw and you get three separate `getch()` returns.

```c
keypad(stdscr, TRUE);
```

With this on, ncurses recognises the sequences from your terminal's terminfo
entry and collapses each into one value above 255:

| Constant | Key |
| --- | --- |
| `KEY_UP` `KEY_DOWN` `KEY_LEFT` `KEY_RIGHT` | arrows |
| `KEY_HOME` `KEY_END` | home / end |
| `KEY_NPAGE` `KEY_PPAGE` | page down / up |
| `KEY_BACKSPACE` `KEY_DC` `KEY_IC` | backspace, delete, insert |
| `KEY_F(n)` | function key *n* |
| `KEY_RESIZE` | the terminal was resized (lesson 13) |
| `KEY_MOUSE` | a mouse event is waiting (lesson 12) |

Run `keylog.c` and toggle it with F2 to watch one arrow press turn from three
events into one.

**This is why `getch()` returns `int`.** Storing it in a `char` truncates
`KEY_UP` (259) to garbage — and on platforms where `char` is signed, a byte
of 0xFF compares equal to `ERR` (-1) and your program thinks input ended.

```c
char ch = getch();     /* WRONG -- silently broken */
int  ch = getch();     /* right */
```

## The ESC ambiguity

`ESC` alone and the start of an arrow sequence are the same byte. Curses
resolves this with a timer: after `ESC` it waits ~1 second (tunable via
`set_escdelay()` or `$ESCDELAY`) to see if more bytes follow.

The consequence is that a bare Escape keypress is reported *late*. If Escape
is a hot key in your program, set `set_escdelay(25)` — 25ms is plenty for a
local terminal and imperceptible to the user. Over ssh you may need more.

## Blocking, timeout, non-blocking

This decision shapes your main loop, so `modes.c` exists to let you feel it:

| Call | `getch()` behaviour |
| --- | --- |
| `timeout(-1)` | blocks forever (the default) |
| `timeout(n)` | waits *n* ms, then returns `ERR` |
| `timeout(0)` / `nodelay(win, TRUE)` | returns `ERR` immediately if nothing is ready |
| `halfdelay(n)` | waits *n* **tenths** of a second |

The tradeoff:

- **Blocking** — the loop only runs when a key is pressed. Correct for an
  editor or a menu. Nothing can animate.
- **Timeout** — the loop runs at a floor rate even with no input. This is the
  right default for most TUIs: no busy-waiting, and you still get to update
  clocks and progress bars.
- **Non-blocking** — the loop runs flat out. You *must* sleep yourself or you
  will pin a CPU core at 100%. This is what the snake game uses, because it
  wants to control the tick rate precisely (see lesson 11).

`napms(ms)` is curses' portable sleep, and it's preferable to `usleep()`
inside a curses program.

## Draining the queue

If a key is held down, the terminal buffers repeats. With `nodelay` on, this
loop consumes everything pending in one pass:

```c
int ch;
while ((ch = getch()) != ERR) {
  /* handle ch */
}
```

The snake game does exactly this, so holding an arrow key doesn't queue up a
backlog of turns that keep executing after you let go.

Watch the parenthesisation. The bug the original snake code had:

```c
while (ch == getch() != ERR)      /* parses as (ch == getch()) != ERR */
while ((ch = getch()) != ERR)     /* correct */
```

The first compiles with a warning, uses `ch` uninitialised, and never assigns
to it. Always build with `-Wall`.

To throw away buffered input rather than process it — after a slow operation,
or before a "press any key" prompt — use `flushinp()`.

## Reading whole strings

For line input there's `getnstr(buf, n)`, which gives you the tty's line
editing back temporarily. Always the `n` variant: `getstr()` has no bound and
is a buffer overflow waiting to happen.

Usually you're better off writing your own input field (lesson 14 shows the
`form` library, which does it properly) because `getnstr` blocks your entire
loop while it runs.

## Exercises

1. In `keylog.c`, comment out `keypad(stdscr, TRUE)` and press an arrow key.
   Count the events.
2. Add `set_escdelay(25)` and compare how quickly a bare Escape registers.
3. In `modes.c`, delete the `napms(20)` from the nodelay branch and watch a
   CPU core with `top`.
4. Change `cbreak()` to `raw()` in either program and try Ctrl-C.

---

Previous: **[Lesson 2](../02-coordinates/lesson.md)** ·
Next: **[Lesson 4 — Attributes](../04-attributes/lesson.md)**
