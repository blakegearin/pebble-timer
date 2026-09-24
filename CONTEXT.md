# CONTEXT

A glossary for this repo. Not a manual -- just the words whose meaning is not
obvious from the code, and the one platform fact that constrains everything.

## Window naming

A window is named for **what it shows**, not for what the user does on it:

| file | shows |
| --- | --- |
| `menu_window` | the list of timers |
| `detail_window` | one timer |
| `duration_window` | a duration being dialled in |
| `settings_window` | the list of app settings |
| `option_window` | the options for one setting |
| `popup_window` | a popup |

`duration_window` was called `setting_window` until 2026-09-23. It was named
for the *act* of setting a duration, which broke the rule above and collided
with `settings_window`. Do not reintroduce the collision.

## Terms

- **Duration** -- the length dialled in on the duration picker.
- **Setting** -- one of the app's preferences (`Sort Order`, `Start Timers`,
  `Delete`). Named by `SettingId` in `settings.h`.
- **Option** -- one choice within one setting (`Last Used` / `Duration`).
- **Armed delete** -- the in-place confirm state on the detail window's action
  bar, not a separate confirmation screen.

## Storage index vs. view index

`s_countdown_timers[]` is **always** in recency order: running timers above
paused, most recently used first within each tier. `s_timer_view_indices[]`
lays the selected sort over the top without disturbing it.

A "timer index" in a `menu_window` signature is a **view** index.

## Platforms

Six targets: `aplite` (Pebble/Pebble Steel), `basalt` (Time/Time Steel),
`chalk` (Time Round), `diorite` (Pebble 2), `emery` (Time 2),
`gabbro` (Core 2 Duo).

The one line that matters: **aplite is the 24 KB platform, where compiled code
lives in the same budget as data.** Adding a window there costs RAM even if it
is never pushed. That is why the settings UI is inline rows on aplite and two
sub-windows everywhere else.
