# Lesson 14 — Menus and forms

**Programs:** `menu_demo.c`, `form_demo.c`
**Links:** `-lmenu -lncurses` and `-lform -lncurses`

```sh
make 14                                    # menu_demo
./lessons/14-menus-and-forms/form_demo
```

Two more libraries ship with ncurses. Both follow the same shape, so
learning one teaches you the other.

## The common pattern

```
1. create the items/fields
2. create the menu/form from them
3. attach a window and a SUB-window
4. post it            <- this is what draws it
5. loop: read a key -> translate to a REQ_* -> pass to the driver
6. read the results
7. unpost, free the menu/form, then free the items/fields
```

The **sub-window** is the part people miss. The outer window holds your
border and labels; the sub-window is the area the library draws into. If you
draw your own text into the sub-window, the library will overwrite it.

```c
WINDOW *outer = newwin(14, 58, 4, 3);
WINDOW *inner = derwin(outer, 10, 54, 3, 2);
set_menu_win(menu, outer);
set_menu_sub(menu, inner);
```

`scale_menu()` / `scale_form()` ask the library how much room it needs, so
you can size the outer window around it rather than guessing.

## Menus

```c
ITEM **items = calloc(n + 1, sizeof *items);     /* NULL-terminated */
items[i] = new_item("name", "description");
MENU *menu = new_menu(items);
```

**`new_item` does not copy its strings.** They must stay alive for as long
as the menu does. String literals or a static table are fine; a `char[]` on
the stack of a function that returns is not — and this is the single most
common crash with the menu library.

Driving it:

```c
menu_driver(menu, REQ_DOWN_ITEM);
```

| Request | Effect |
| --- | --- |
| `REQ_UP_ITEM` / `REQ_DOWN_ITEM` | move the cursor |
| `REQ_SCR_UPAGE` / `REQ_SCR_DPAGE` | page |
| `REQ_FIRST_ITEM` / `REQ_LAST_ITEM` | jump |
| `REQ_TOGGLE_ITEM` | select/deselect (multi-select only) |
| `REQ_NEXT_MATCH` | jump to the next item matching typed text |

Useful options:

```c
set_menu_format(menu, rows, cols);   /* visible size; the rest scrolls */
menu_opts_off(menu, O_ONEVALUE);     /* enable multi-select */
set_menu_mark(menu, " [x] ");        /* the marker on selected items */
set_menu_fore(menu, COLOR_PAIR(2));  /* the cursor row */
set_menu_back(menu, COLOR_PAIR(1));  /* other rows */
set_menu_grey(menu, A_DIM);          /* disabled items */
```

Reading the result:

```c
ITEM *cur = current_item(menu);         /* single-select */
item_name(cur); item_description(cur);

for (int i = 0; i < n; i++)             /* multi-select */
    if (item_value(items[i])) { /* chosen */ }
```

Read **before** you unpost and free.

## Forms

```c
FIELD *f = new_field(rows, cols, top, left, offscreen, extra_buffers);
```

The last two arguments are almost always `0`. Coordinates are relative to
the form's sub-window.

Validation is the reason to use this library rather than rolling your own:

```c
set_field_type(f, TYPE_INTEGER, padding, min, max);
set_field_type(f, TYPE_ALNUM, min_width);
set_field_type(f, TYPE_ALPHA, min_width);
set_field_type(f, TYPE_ENUM, list, case_sensitive, partial_match);
set_field_type(f, TYPE_REGEXP, "^[0-9]+$");
set_field_type(f, TYPE_IPV4);
```

The form refuses to leave a field whose contents don't validate — you get
`E_INVALID_FIELD` from the driver and the cursor stays put.

Driving it is the same idea, but note that **any key you don't translate
should be passed through** as a literal character:

```c
default:
    form_driver(form, ch);    /* types the character into the field */
    break;
```

### Two traps

**`REQ_VALIDATION` before reading.** While you're typing, the text lives in
an internal edit buffer. `field_buffer()` doesn't see it until the field is
left or validation is forced:

```c
form_driver(form, REQ_VALIDATION);      /* sync the current field */
char *s = field_buffer(field[0], 0);
```

Skip this and the last field the user typed in reads back with its *old*
value. It's an intermittent-looking bug that's entirely deterministic.

**Buffers are space-padded** to the full field width. `field_buffer()` on a
20-column field always returns 20 characters. Trim the trailing whitespace
yourself — `form_demo.c` has a `trimmed()` helper.

## Are they worth using?

Honest assessment:

**Menus** — yes for a long scrolling list where you'd otherwise implement
scrolling, marking and pattern-matching yourself. For a five-item list, a
hand-rolled loop with `chgat` for the highlight (lesson 4) is less code and
far more flexible.

**Forms** — yes when you have several validated fields and want tabbing for
free. The validation types are genuinely good. For a single input, a
hand-rolled field is simpler.

Both have the same drawback: the API is from 1990, the driver-request model
fights against a modern event loop, and customising the appearance beyond
the provided setters means fighting the library. They are worth knowing
because they're always available and they solve real problems, not because
they're pleasant.

## Linking

```sh
gcc -o menu_demo menu_demo.c -lmenu -lncurses
gcc -o form_demo form_demo.c -lform -lncurses
```

The add-on library goes **before** `-lncurses`.

## Exercises

1. Add a "disabled" item with `item_opts_off(item, O_SELECTABLE)` and see
   how `set_menu_grey` renders it.
2. Add `REQ_NEXT_MATCH` so typing letters jumps through the menu.
3. In `form_demo.c`, remove the `REQ_VALIDATION` call and observe the last
   field reading back stale.
4. Add a `TYPE_REGEXP` field for a hex colour and try to submit garbage.
5. Wire `menu_demo.c` up to the snake game as a real options screen.

---

Previous: **[Lesson 13](../13-resize/lesson.md)** ·
Next: **[Lesson 15 — Mini snake](../15-mini-snake/lesson.md)**
