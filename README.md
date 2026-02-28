# hypr-columns

Equal-width column layout plugin for [Hyprland](https://hyprland.org/) 0.54+.

Windows are arranged in equal-width columns. Within each column, windows stack with equal height. When the column limit is reached, new windows are added to the least populated column.

## Install

### Manual

```bash
make
hyprctl plugin load ./hypr-columns.so
```

### hyprpm

```bash
hyprpm add https://github.com/abjoru/hypr-columns
hyprpm enable hypr-columns
```

## Usage

Set the layout in `hyprland.conf`:

```ini
general {
    layout = columns
}
```

## Configuration

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `plugin:hypr-columns:max_columns` | int | `3` | Maximum number of columns before stacking |
| `plugin:hypr-columns:spawn_direction` | string | `"right"` | `"left"` or `"right"` — where new columns appear |

## Layout Messages

Bind these via `layoutmsg` dispatcher:

| Command | Description |
|---------|-------------|
| `focuscolumn +1` / `-1` | Move focus to adjacent column |
| `movetocolumn +1` / `-1` | Move focused window to adjacent column |
| `swapcolumn +1` / `-1` | Swap entire column with adjacent |

Example keybinds:

```ini
bind = SUPER, bracketright, layoutmsg, focuscolumn +1
bind = SUPER, bracketleft, layoutmsg, focuscolumn -1
bind = SUPER SHIFT, bracketright, layoutmsg, movetocolumn +1
bind = SUPER SHIFT, bracketleft, layoutmsg, movetocolumn -1
bind = SUPER CTRL, bracketright, layoutmsg, swapcolumn +1
bind = SUPER CTRL, bracketleft, layoutmsg, swapcolumn -1
```

## Behavior

- Windows ≤ `max_columns`: each window gets its own column
- Windows > `max_columns`: excess windows stack in existing columns
- `moveTargetInDirection` left/right moves windows between columns (creates new column at edge if under max)
- `moveTargetInDirection` up/down swaps window position within its column
- Resize requests are ignored to maintain equal-width invariant

## License

BSD-3-Clause
