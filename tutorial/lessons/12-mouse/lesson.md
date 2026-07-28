# Lesson 12 — Mouse

**Program:** `mouse.c` · **Links:** `-lncurses`

```sh
make 12
```

Drag to paint, right-drag to erase, wheel changes the brush. The right pane
logs the raw events.

> **Needs a real terminal.** This is the one program in the course that
> can't run under `script`, `expect` or a bare pty — those don't implement
> mouse reporting, so `mousemask()` grants nothing and the program exits
> with "no mouse support". That's the graceful-degradation path from below
> doing its job. Inside tmux you also need `set -g mouse on`.

## Turning it on

```c
keypad(stdscr, TRUE);                        /* required */
mmask_t granted;
mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, &granted);
```

Two things to note.

`keypad` must be on. Mouse activity arrives as a `KEY_MOUSE` keypress, and
without `keypad` you get the raw escape sequence instead.

`mousemask` **returns the mask it actually granted**, which may be smaller
than what you asked for — or zero, on a terminal with no mouse support.
Check it. Programs that assume the mouse works are the reason some TUIs are
unusable over a serial console.

## Reading an event

```c
if (ch == KEY_MOUSE) {
    MEVENT ev;
    if (getmouse(&ev) == OK) {
        /* ev.y, ev.x, ev.bstate */
    }
}
```

`getmouse()` must be called immediately after `KEY_MOUSE` — it pops the
event from a separate queue, and another `getch()` in between loses it.

`MEVENT` gives you:

| Field | Meaning |
| --- | --- |
| `ev.y`, `ev.x` | position, in **screen** coordinates |
| `ev.bstate` | bitmask of what happened |
| `ev.id` | device id, for multi-pointer setups |
| `ev.z` | reserved |

## The button mask

`bstate` is a bitmask, so test with `&`, never `==`:

```c
if (ev.bstate & BUTTON1_PRESSED) { ... }
```

| Constant | Meaning |
| --- | --- |
| `BUTTON1_PRESSED` / `_RELEASED` | left button down / up |
| `BUTTON1_CLICKED` | synthesised press+release |
| `BUTTON1_DOUBLE_CLICKED` | two clicks within `mouseinterval` |
| `BUTTON2_*` | middle |
| `BUTTON3_*` | right |
| `BUTTON4_PRESSED` | **wheel up** |
| `BUTTON5_PRESSED` | **wheel down** (ncurses 6+) |
| `REPORT_MOUSE_POSITION` | motion, with no button change |
| `BUTTON_SHIFT` / `_CTRL` / `_ALT` | modifier held |

The wheel being buttons 4 and 5 is an X11 convention that leaked into
terminals. Wheel events are press-only — there's no matching release.

## Clicks vs. press/release

By default curses waits `mouseinterval` milliseconds (default ~200) after a
press to see if a release follows, so it can synthesise `BUTTON1_CLICKED`.

That delay is fatal for dragging: you don't learn the button went down until
200 ms later. Disable it:

```c
mouseinterval(0);
```

Now you get raw `BUTTON1_PRESSED` and `BUTTON1_RELEASED` immediately, and
you track drag state yourself — which is exactly what `mouse.c` does with
its `painting` flag.

Rule of thumb: if you need dragging, use `mouseinterval(0)` and handle
press/release. If you only need clicks, leave the default and use
`BUTTON1_CLICKED`.

## Screen coordinates vs window coordinates

`ev.y` and `ev.x` are always relative to the **screen**, not to whatever
window you care about. Translating by hand means subtracting the window
origin and bounds-checking. Curses provides it:

```c
int y = ev.y, x = ev.x;
if (wmouse_trafo(win, &y, &x, FALSE)) {
    /* y,x are now window-relative and the click was inside */
}
```

The `FALSE` means "screen to window"; `TRUE` converts the other way. The
return value is the inside/outside test, which is the part you'd most
likely get wrong yourself.

## Dragging needs position reports

Without `REPORT_MOUSE_POSITION` in your mask you get presses and releases
but nothing in between, so a drag registers as two points and no line. With
it, the terminal streams motion events — which is a lot of events, so keep
the handler cheap.

## Clean up

```c
mousemask(0, NULL);      /* before endwin() */
```

Mouse reporting is a terminal mode. Leave it on and the user's shell starts
receiving escape sequences whenever they click — visible as garbage like
`^[[<0;34;12M` appearing at their prompt.

## Portability

The mouse is the least portable part of curses:

- Requires `TERM` to advertise it (`xterm`, `screen`, `tmux`, most modern
  emulators do).
- Inside tmux, needs `set -g mouse on` or events go to tmux instead.
- The default protocol breaks past column 223 on old terminals. ncurses 6
  uses SGR mode where available, which fixes it.
- Over ssh it works, since it's all in-band escape sequences.

Always design so the keyboard alone is sufficient. The mouse is an
accelerator, never the only way to do something.

## Exercises

1. Set `mouseinterval(200)` and try to drag. Note the dead time.
2. Drop `REPORT_MOUSE_POSITION` and watch drags become disconnected dots.
3. Add `BUTTON_CTRL` detection to paint in a second colour.
4. Make the snake game's food placeable by clicking, for debugging.

---

Previous: **[Lesson 11](../11-game-loop/lesson.md)** ·
Next: **[Lesson 13 — Resize](../13-resize/lesson.md)**
