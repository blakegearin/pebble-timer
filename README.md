# TimerBetterer

A countdown timer for Pebble smartwatches, forked from Pebble's published
`pebble-timer` app and maintained for the current community firmware.

Timers live in a list on the main screen. Add one with the `+` row, dial in a
duration, and it runs; open it from the list (or from its Timeline pin) to
pause, edit, or delete it. When a timer ends the app wakes, vibrates, and
offers snooze or dismiss. Close the app and it reopens itself when the closest
timer expires, and the launcher shows the last-used timer's frozen time in the
app glance.

## Features

- **Running timers on top, most-recently-used first** within each tier, so the
  list reflects what you actually touched last.
- **Settings** (v1.0.0): sort order, whether confirming a duration starts the
  timer, and whether deleting asks first — as a Settings sub-menu, or as
  inline rows on the original Pebble.
- **Create timers paused** ("Start Timers: Manually") — a timer that has never
  run, handled consistently across list order, Timeline pins, wakeups, and the
  app glance.
- **Armed delete** — DOWN arms the confirmation on the spot; UP confirms,
  DOWN cancels, and it self-clears after 2.5 seconds. Turn it off entirely
  with "Delete: Immediately".
- **Timeline pins** for timers of 15 minutes or more, with the end time shown
  while dialling in a duration.

## On real hardware

The app builds for six platforms, which differ in screen, colour depth, and
how much memory compiled code costs. Basalt is the reference; the others are
shown below it.

### Basalt — Pebble Time / Time Steel (144×168, colour)

<details open>
<summary>screens</summary>

| Detail | List | Settings | Options |
| --- | --- | --- | --- |
| ![detail](assets/screenshots/basalt/01-detail.png) | ![menu](assets/screenshots/basalt/02-menu.png) | ![settings](assets/screenshots/basalt/03-settings.png) | ![options](assets/screenshots/basalt/04-options.png) |

</details>

### Aplite — Pebble / Pebble Steel (144×168, 1-bit, 24 KB)

The original hardware, where compiled code lives in the same 24 KB budget as
data. The Settings sub-menu and the radio option window are too expensive
there, so the three settings become permanent rows in the timer list instead
and flip in place on a single SELECT press. The cog row never appears, and the
cog bitmap is not even loaded.

<details>
<summary>screens</summary>

| Detail | List with inline settings | A row flipped |
| --- | --- | --- |
| ![detail](assets/screenshots/aplite/01-detail.png) | ![menu](assets/screenshots/aplite/02-menu.png) | ![flipped](assets/screenshots/aplite/03-flipped.png) |

</details>

### Chalk — Pebble Time Round (180×180 round)

Round screens show one focused setting at a time: the focused cell is 68 px
tall and holds name over value, unfocused cells are 32 px and drop the value
line — this is what the firmware's own settings do. There is no section
header, because the circular mask would clip it.

<details>
<summary>screens</summary>

| Detail | List | Settings | Options |
| --- | --- | --- | --- |
| ![detail](assets/screenshots/chalk/01-detail.png) | ![menu](assets/screenshots/chalk/02-menu.png) | ![settings](assets/screenshots/chalk/03-settings.png) | ![options](assets/screenshots/chalk/04-options.png) |

</details>

### Diorite — Pebble 2 (144×168, 1-bit)

Same layout as Basalt, black and white. Highlighted rows invert, and the
icon-only settings row inverts with them.

<details>
<summary>screens</summary>

| Detail | List | Settings | Options |
| --- | --- | --- | --- |
| ![detail](assets/screenshots/diorite/01-detail.png) | ![menu](assets/screenshots/diorite/02-menu.png) | ![settings](assets/screenshots/diorite/03-settings.png) | ![options](assets/screenshots/diorite/04-options.png) |

</details>

### Emery — Pebble Time 2 (200×228, Large content size)

Runs at a Large preferred content size: 61 px cells and scaled-up system
fonts. No dimension is hardcoded — fonts resolve from the theme and the radio
circles get a wider inset.

<details>
<summary>screens</summary>

| Detail | List | Settings | Options |
| --- | --- | --- | --- |
| ![detail](assets/screenshots/emery/01-detail.png) | ![menu](assets/screenshots/emery/02-menu.png) | ![settings](assets/screenshots/emery/03-settings.png) | ![options](assets/screenshots/emery/04-options.png) |

</details>

### Gabbro — Core 2 Duo (260×260 round, Large content size)

Round geometry identical to Chalk (68/32 cells, 35 px radio inset) at Emery's
Larger fonts.

<details>
<summary>screens</summary>

| Detail | List | Settings | Options |
| --- | --- | --- | --- |
| ![detail](assets/screenshots/gabbro/01-detail.png) | ![menu](assets/screenshots/gabbro/02-menu.png) | ![settings](assets/screenshots/gabbro/03-settings.png) | ![options](assets/screenshots/gabbro/04-options.png) |

</details>

## Building

Requires the Pebble SDK (v4, SDK 3 API) and the `pebble` tool on PATH.

```sh
pebble build                      # all six platforms, into build/
pebble install --emulator basalt  # or aplite|basalt|chalk|diorite|emery|gabbro
```

### Screenshots and visual regression

`tools/screenshots.sh` drives the emulator through a scripted scene and
captures a screenshot at each named step; it needs ImageMagick for the diffs.
With `--baseline` it exits non-zero on any drift and writes a red-highlighted
`.diff.png` per changed shot.

```sh
tools/screenshots.sh -p basalt -w tools/scenes/readme-tour.scene   # the app's screens
tools/screenshots.sh -p basalt -w tools/scenes/menu-empty.scene    # the deterministic diff scene
tools/screenshots.sh -p aplite -w -b tmp/baselines/upstream-master/aplite \
    -o tmp/shots/check tools/scenes/menu-empty.scene               # drift check vs the fork's base
```

`tools/scenes/readme-tour.scene` and `tools/scenes/readme-aplite.scene` are
the scenes that produced the screenshots above; rerun them against
`assets/screenshots/<platform>/` when the UI changes.

## Design notes

- [`CONTEXT.md`](CONTEXT.md) — the glossary: what a duration, setting, option,
  and armed delete mean here, how windows are named, and the one platform
  fact that constrains everything.
- [`docs/superpowers/specs/2026-09-23-settings-menu-design.md`](docs/superpowers/specs/2026-09-23-settings-menu-design.md)
  — the settings-menu design, including the per-platform cell metrics and the
  rejected alternatives with reasons.
- [`docs/superpowers/specs/2026-07-10-mru-timer-ordering-design.md`](docs/superpowers/specs/2026-07-10-mru-timer-ordering-design.md)
  — the list-ordering contract the sort setting sits on top of.

## Credits

- Fork of [pebble/pebble-timer](https://github.com/pebble/pebble-timer),
  originally by Eric Phillips for Pebble.
- The settings-row icon is `Pebble_25x25_Settings.svg` from
  [pebble-dev/iconography](https://github.com/pebble-dev/iconography)
  (Apache 2.0), as are the 80×80 Timeline-pin drawings.
