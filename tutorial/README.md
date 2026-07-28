# Learning ncurses

A fifteen-lesson course in ncurses, built around working programs you can run,
break and fix. It starts from `initscr()` and ends with a rebuilt version of
the [snake game](../snake.c) in the parent directory.

Every lesson is a directory containing a `lesson.md` and the source it
discusses. The programs are meant to be *run* — most of them are interactive
and demonstrate something you can't see in a code listing.

## Setup

You need a C compiler and the ncurses development headers, including the
`panel`, `menu` and `form` add-ons (lessons 10 and 14).

```sh
# Debian / Ubuntu
sudo apt install build-essential libncurses-dev

# Fedora / RHEL
sudo dnf install gcc make ncurses-devel

# macOS
xcode-select --install
```

```sh
make            # build all 18 programs
make list       # show what got built
make 07         # build everything, then run lesson 7
make clean
```

## The lessons

| # | Lesson | Covers |
| --- | --- | --- |
| 01 | [Hello, curses](lessons/01-hello-curses/lesson.md) | `initscr`/`refresh`/`endwin`, the virtual screen, the printf trap |
| 02 | [Coordinates](lessons/02-coordinates/lesson.md) | (y, x) order, `getmaxyx`, silent out-of-bounds failure |
| 03 | [Input](lessons/03-input/lesson.md) | `cbreak`/`raw`, `noecho`, `keypad`, blocking vs `nodelay`, draining |
| 04 | [Attributes](lessons/04-attributes/lesson.md) | `attron`/`attrset`, `chtype`, `chgat`, `termattrs` |
| 05 | [Colour](lessons/05-color/lesson.md) | colour pairs, `use_default_colors`, 256 colours, degrading |
| 06 | [Lines and boxes](lessons/06-lines-and-boxes/lesson.md) | `ACS_*`, `hline`/`vline`, `box`, `wborder` |
| 07 | [Windows](lessons/07-windows/lesson.md) | `newwin`, clipping, the `w` family, `derwin`, refresh order |
| 08 | [doupdate](lessons/08-doupdate/lesson.md) | `wnoutrefresh`+`doupdate`, `erase` vs `clear`, flicker |
| 09 | [Pads](lessons/09-pads/lesson.md) | `newpad`, `prefresh`, scrolling viewports |
| 10 | [Panels](lessons/10-panels/lesson.md) | overlapping windows with a real stacking order |
| 11 | [The game loop](lessons/11-game-loop/lesson.md) | pacing strategies, monotonic time, input latency |
| 12 | [Mouse](lessons/12-mouse/lesson.md) | `mousemask`, `MEVENT`, dragging, `wmouse_trafo` |
| 13 | [Resize](lessons/13-resize/lesson.md) | `KEY_RESIZE`, `SIGWINCH`, relayout, "too small" |
| 14 | [Menus and forms](lessons/14-menus-and-forms/lesson.md) | the `menu` and `form` libraries, validation |
| 15 | [Mini snake](lessons/15-mini-snake/lesson.md) | capstone — all of the above in one game |

## Suggested paths

**Just want to draw something?** 1 → 2 → 4 → 5 → 6. That's enough for a
status display or a progress UI.

**Building a game?** 1 → 2 → 3 → 5 → 11 → 15. Lesson 11 is the one that
matters most for how the result *feels*.

**Building an application UI?** 1 → 2 → 3 → 7 → 8 → 10 → 13 → 14. Lesson 8
is the one that stops it flickering.

**Read the whole thing in order** if you have an afternoon — each lesson
assumes the previous ones.

## The ideas that matter most

If you take four things away from this course:

1. **Curses is a screen differ.** You draw into a buffer; `refresh()` works
   out the minimum the terminal needs to be told. Everything else follows.
2. **`erase()`, not `clear()`.** `clear()` throws away the diff and repaints
   everything — that's what flicker is (lesson 8).
3. **One `doupdate()` per frame.** With multiple windows, `wnoutrefresh` them
   all and update once (lesson 8).
4. **`timeout()` beats sleeping.** Combining the frame delay with the input
   wait removes input latency for free (lesson 11).

## Cross-references to the game

The lessons refer back to [`../snake.c`](../snake.c) throughout — as a
worked example where it does something well, and as a source of concrete
bugs where it doesn't:

| Lesson | What it says about `snake.c` |
| --- | --- |
| 3 | its input drain is right; the `while (ch == getch() != ERR)` form was not |
| 5 | its `has_colors()` fallback is worth copying; the black backgrounds are not |
| 8 | its use of `erase()` over `clear()` is correct |
| 11 | its speed ramp needs the clamp it has; its `usleep` pacing costs latency |
| 13 | it reads the terminal size once and breaks on resize |
| 15 | a side-by-side rebuild |

## Conventions in the code

- Every program compiles clean under `-Wall -Wextra -std=c11`.
- Comments explain *why*, not what — the API calls are in the `lesson.md`.
- Each program checks its minimum terminal size and exits with a message
  rather than drawing off-screen.
- No program leaves the terminal in a broken state, including on the error
  paths.

## Reference

```sh
man ncurses      # the function index, grouped by topic -- start here
man curs_getch   # curs_attr, curs_color, curs_refresh, curs_pad, ...
man panel        # menu, form
```

Two books worth knowing about: the *NCURSES Programming HOWTO* (dated but
still the best free tutorial) and Dan Gookin's *Programmer's Guide to
NCurses*.
