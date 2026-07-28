# Lesson 1 — Hello, curses

**Programs:** `hello.c`, `no_refresh.c` · **Links:** `-lncurses`

```sh
make 01                        # or:
../../..$ cd tutorial && make
./lessons/01-hello-curses/hello
./lessons/01-hello-curses/no_refresh
```

## What curses actually is

ncurses is not a drawing library. It is a **screen differ**.

You draw into an in-memory buffer. When you call `refresh()`, curses compares
that buffer against its record of what the terminal is currently displaying,
works out the smallest set of escape sequences that would turn one into the
other, and sends those. If you redraw an entire 80x24 screen but only one
character changed, curses sends roughly one character.

That is the entire value proposition. Everything else — windows, colour,
input handling — is built on top of it.

There are two buffers involved:

| Buffer | Also called | What it is |
| --- | --- | --- |
| `stdscr` | the virtual screen | Where your `addch`/`printw` calls land |
| `curscr` | the physical screen | Curses' belief about what the terminal shows |

`refresh()` is the operation that reconciles the first into the second.

## The skeleton

```c
initscr();        /* take over the terminal, allocate stdscr */
/* ... draw ... */
refresh();        /* reconcile stdscr onto the real screen */
endwin();         /* give the terminal back */
```

### `initscr()`

Determines the terminal type from `$TERM`, loads its capabilities from
terminfo, allocates `stdscr` and `curscr`, and switches the terminal into a
mode curses controls. On most terminals it also switches to the *alternate
screen* — which is why your shell history reappears untouched when the program
exits.

If it fails (no `$TERM`, unknown terminal) it prints an error and calls
`exit()`. It does not return NULL. If you need to survive that, use
`newterm()` instead.

### `refresh()`

Nothing you draw is visible until this is called. This trips up everyone once.

Note in `no_refresh.c` that `refresh()` flushes *all* pending changes, not
just the ones since the last call — the buffer is a buffer, not a queue of
commands.

### `endwin()`

Restores the terminal modes. **Always call it**, including on error paths.
If your program crashes without it, the user is left in a terminal with no
echo, no line buffering, and possibly no cursor. (`reset` at the shell fixes
that, but they shouldn't have to.)

## The printf trap

```c
printf("hello");     /* goes straight to stdout */
addstr("hello");     /* goes into stdscr, where curses can see it */
```

`printf` bypasses curses entirely. Curses doesn't know the cursor moved or
that characters appeared, so its diff is computed against a wrong model and
the display corrupts. `no_refresh.c` demonstrates this directly.

The rule: **between `initscr()` and `endwin()`, never write to stdout.**
Use `printw()` where you'd use `printf()`. Debug logging goes to a file, or
to stderr redirected to a file — never to the screen.

## Output functions

The naming convention is worth learning now because it holds for the entire
library:

| Prefix | Meaning | Example |
| --- | --- | --- |
| *(none)* | acts on `stdscr` | `addch('x')` |
| `w` | takes a `WINDOW *` | `waddch(win, 'x')` |
| `mv` | move first, then act | `mvaddch(y, x, 'x')` |
| `mvw` | both | `mvwaddch(win, y, x, 'x')` |

So `mvwprintw(win, 3, 5, "%d", n)` is "move to (3,5) in `win`, then printw".
Once you see this you can predict the name of almost any function in ncurses.

The four output primitives:

| Function | Writes |
| --- | --- |
| `addch(ch)` | one character (plus attributes — see lesson 4) |
| `addstr(s)` | a string |
| `printw(fmt, ...)` | formatted, like `printf` |
| `addnstr(s, n)` | at most `n` characters |

## Exercises

1. Delete the `refresh()` in `hello.c`. Confirm nothing appears.
2. Delete the `endwin()` and run it. Your shell will stop echoing what you
   type. Type `reset` blind and press Enter to recover.
3. Replace `addstr` with `printf` throughout `hello.c` and observe the
   staircase effect — the terminal is in a mode where `\n` moves down but
   not back to column 0.
4. Run `./hello 2>/dev/null` under a terminal with `TERM=dumb` set and see
   what `initscr()` does.

## Compiling by hand

```sh
gcc -std=c11 -Wall -Wextra -o hello hello.c -lncurses
```

On some systems you also need `-ltinfo` (the terminfo half of ncurses is
sometimes a separate library). `pkg-config --libs ncurses` gets it right;
that's what the tutorial `Makefile` uses.

---

Next: **[Lesson 2 — Coordinates](../02-coordinates/lesson.md)**
