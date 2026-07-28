# Lesson 10 — Panels

**Program:** `panels.c` · **Links:** `-lpanel -lncurses` (in that order)

```sh
make 10
```

TAB raises the next panel, arrows move the top one, `h` hides it.

## The problem panels solve

Lesson 7 ended on a limitation: curses has no stacking order. Overlapping
windows corrupt each other, and repairing it by hand means tracking which
window covers which and issuing `touchwin` in the right sequence. With five
windows that's twenty relationships.

The `panel` library adds the missing concept: a **stack**. Each panel wraps a
window and knows its depth. Panels handle the repainting.

## The API

```c
#include <panel.h>

PANEL *p = new_panel(win);    /* attach; goes on top of the stack */
top_panel(p);                 /* raise to front */
bottom_panel(p);              /* send to back */
hide_panel(p);                /* remove from the stack, keep the window */
show_panel(p);                /* put it back */
move_panel(p, y, x);          /* reposition on screen */
del_panel(p);                 /* detach (does NOT free the window) */

update_panels();              /* restack everything into the virtual screen */
doupdate();                   /* push to the terminal */
```

## The one rule

**Once a window has a panel, never call `wrefresh()` on it.**

```c
wrefresh(win);            /* WRONG once win has a panel */
update_panels(); doupdate();   /* right */
```

`wrefresh` pushes that one window straight to the virtual screen with no
regard for what's above it, so a background panel will paint over a
foreground one. The panel library is now the only thing allowed to decide
drawing order.

The frame looks like this:

```c
/* draw into the windows however you like -- mvwprintw, box, ... */
wnoutrefresh(stdscr);    /* if you draw on stdscr at all */
update_panels();         /* stack the panels above it */
doupdate();              /* one terminal update */
```

`update_panels()` does no I/O. It walks the stack bottom to top, doing the
`touchwin`/`wnoutrefresh` bookkeeping you'd otherwise write by hand.
`doupdate()` still does the actual work — the lesson 8 model is unchanged.

## `move_panel`, not `mvwin`

```c
mvwin(win, y, x);          /* moves the window; panel stack now inconsistent */
move_panel(pan, y, x);     /* right */
```

`move_panel` tells the library the geometry changed so it can repair the
region the panel vacated. Using `mvwin` on a panelled window leaves a trail.

## Hiding

`hide_panel` takes the panel out of the stack without destroying anything.
The window and its contents survive; it simply isn't drawn. `show_panel`
returns it — to the **top** of the stack, not to its old depth.

`panel_hidden(p)` reports the current state, which saves tracking it
yourself as `panels.c` does for clarity.

## Walking the stack

```c
PANEL *p = panel_above(NULL);    /* bottom-most */
while (p) { /* ... */ p = panel_above(p); }
```

`panel_above(NULL)` gives the bottom panel and `panel_below(NULL)` the top —
a slightly odd convention worth remembering. `panel_window(p)` recovers the
window.

Since panels give you no way to attach your own data directly, there's a user
pointer:

```c
set_panel_userptr(p, &my_struct);
const void *d = panel_userptr(p);
```

This is how you get from "the panel the user clicked" back to your own model
object.

## `wbkgd` — filling a window

`panels.c` uses this to make the overlap obvious:

```c
wbkgd(win, COLOR_PAIR(n));
```

Sets the window's *background character*, which applies to every cell —
including ones erased later. It's how you get a solid coloured panel rather
than colour only where you happened to draw text.

Careful: `wbkgd(win, ' ' | COLOR_PAIR(n) | A_BOLD)` also sets the character.
Passing just a colour pair keeps the existing characters and restyles them.

## Linking

```sh
gcc -o panels panels.c -lpanel -lncurses
```

`-lpanel` must come **before** `-lncurses`. Static link order matters on GNU
ld: a library only resolves symbols needed by things to its left.

## When to use panels

Use them for dialogs, popups, tabbed layouts, floating help — anything where
windows overlap or change depth.

Don't use them for a fixed, non-overlapping layout. Three panes side by side
need no stacking, and plain windows with `wnoutrefresh`/`doupdate` are
simpler. Panels are the answer to *overlap*, not to *multiple windows*.

## Exercises

1. Replace `update_panels(); doupdate();` with a loop of `wrefresh()` and
   watch the stacking break.
2. Use `panel_hidden()` instead of the `hidden[]` array.
3. Add a panel that shows the current stack order top to bottom, using
   `panel_below(NULL)` and `panel_below()`.
4. Build a modal dialog: raise a panel, take input until Enter, hide it.

---

Previous: **[Lesson 9](../09-pads/lesson.md)** ·
Next: **[Lesson 11 — The game loop](../11-game-loop/lesson.md)**
