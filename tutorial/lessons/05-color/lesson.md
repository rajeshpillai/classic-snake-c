# Lesson 5 — Colour

**Program:** `colors.c` · **Links:** `-lncurses`

```sh
make 05
TERM=xterm-256color ./lessons/05-color/colors    # to see the colour cube
```

## Colour comes in pairs

This is the part that surprises people. You don't say "draw this in red".
You define a numbered **pair** of (foreground, background) up front, then
select the pair when drawing.

```c
start_color();
init_pair(1, COLOR_RED, COLOR_BLACK);   /* pair 1 is now red-on-black */

attron(COLOR_PAIR(1));
addstr("red text");
attroff(COLOR_PAIR(1));
```

`COLOR_PAIR(n)` produces an attribute, so it ORs with the ones from lesson 4:

```c
attron(COLOR_PAIR(1) | A_BOLD);
```

The reason is historical and physical: terminals had a fixed table of colour
combinations, and the escape sequence selected a table slot. Curses exposes
that model directly.

## The setup sequence

```c
initscr();
if (has_colors()) {
    start_color();
    use_default_colors();          /* optional but recommended */
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    /* ... */
}
```

Order matters, and each step has a failure mode:

| Call | Why | If you skip it |
| --- | --- | --- |
| `has_colors()` | not every terminal has colour | `init_pair` fails, text is unstyled |
| `start_color()` | initialises the colour subsystem | `init_pair` returns `ERR` |
| `use_default_colors()` | enables `-1` = terminal default | forced to pick a background |

**Pair 0 is reserved** for the terminal's default and cannot be redefined.
Your pairs start at 1. Give them names rather than scattering literals — the
snake game uses an enum, which is the right instinct:

```c
enum { CP_SNAKE = 1, CP_FOOD, CP_GOLDEN, CP_WALL, CP_OBSTACLE, CP_TEXT };
```

## The eight base colours

`COLOR_BLACK`, `COLOR_RED`, `COLOR_GREEN`, `COLOR_YELLOW`, `COLOR_BLUE`,
`COLOR_MAGENTA`, `COLOR_CYAN`, `COLOR_WHITE` — numbered 0 to 7.

Adding `A_BOLD` to a foreground colour historically produces its *bright*
variant, which is how eight colours become the familiar sixteen. This is a
convention, not a guarantee; on a few terminals bold really is just bold.

## `use_default_colors()` and why it matters

Hard-coding `COLOR_BLACK` as your background means a user with a light theme
gets a black rectangle stamped into their terminal. With
`use_default_colors()` you can pass `-1`:

```c
use_default_colors();
init_pair(1, COLOR_CYAN, -1);   /* cyan on whatever they're using */
```

This one call is the difference between a program that fits the user's
terminal and one that fights it. It's an ncurses extension — check the return
value if you care about portability to other curses implementations.

## More than 8 colours

```c
mvprintw(0, 0, "COLORS=%d COLOR_PAIRS=%d", COLORS, COLOR_PAIRS);
```

`COLORS` is how many colours the terminal advertises; `COLOR_PAIRS` how many
pairs you may define. On a modern `xterm-256color` you get 256 and 65536.

The 256-colour layout is standardised:

| Range | Contents |
| --- | --- |
| 0–7 | the base colours |
| 8–15 | their bright variants |
| 16–231 | a 6x6x6 RGB cube: `16 + 36*r + 6*g + b`, each 0–5 |
| 232–255 | 24 steps of grey |

`colors.c` renders the cube when your terminal supports it.

If `TERM` doesn't advertise 256 colours you'll see 8 — the fix is the
environment, not the code: `TERM=xterm-256color`.

## Redefining colours

```c
if (can_change_color())
    init_color(COLOR_RED, 900, 200, 200);   /* components are 0..1000 */
```

Rarely usable. Most terminal emulators report `can_change_color()` as false,
and where it works it changes the colour globally — including for other
programs sharing the palette. Treat it as a curiosity.

## Degrading gracefully

The snake game's approach is worth copying: a single flag, checked at every
use site.

```c
static int use_color;
...
use_color = has_colors();
if (use_color) { start_color(); init_pair(...); }
...
if (use_color) attron(COLOR_PAIR(CP_SNAKE));
```

Slightly repetitive, but it means the game is fully playable over a serial
console. The alternative — wrapping the attribute calls in helpers that
no-op when colour is absent — is cleaner once you have more than a handful
of call sites:

```c
static void colour_on(int pair)  { if (use_color) attron(COLOR_PAIR(pair)); }
static void colour_off(int pair) { if (use_color) attroff(COLOR_PAIR(pair)); }
```

## Exercises

1. Add `use_default_colors()` to the snake game and change every
   `COLOR_BLACK` background to `-1`. Try it in a light-themed terminal.
2. Print the 24-step greyscale ramp (colours 232–255).
3. Write `rgb6(r, g, b)` returning the cube index, and draw a gradient.
4. Run `colors.c` with `TERM=vt100` and confirm the no-colour path works.

---

Previous: **[Lesson 4](../04-attributes/lesson.md)** ·
Next: **[Lesson 6 — Lines and boxes](../06-lines-and-boxes/lesson.md)**
