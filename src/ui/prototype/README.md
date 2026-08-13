# VEOR UI Prototype v9

Single-file HTML prototype for the VEOR browser interface.

## Location

- **Prototype**: `src/ui/prototype/index.html`
- **Deployed**: v9 (see deploy history)

## What It Contains

| Screen / Component | Status |
|---|---|
| Title Bar (40px) | Brand, Workspace dropdown, Nav, Breadcrumb URL, Command Palette button, Window controls |
| Tab Strip (36px) | Tabs, pinned tabs, tab groups (collapse/expand), audio indicators, close buttons |
| Side Panel (280px) | Bookmarks, History, Downloads — with folder structure, timestamps, progress bars |
| Command Palette | Searchable commands with categories, descriptions, keyboard shortcuts |
| Settings | Sidebar categories, groups, toggles, selects, inputs, sliders with descriptions |
| NTP | Parallax geometric grid (3 layers), shortcuts, logo |
| Error Page | Roman numeral error codes |
| Loading State | 3x3 grid animation |
| Empty Workspace | Keyboard hints |
| Permission Dialog | Origin + type, Block / Allow |
| Toast Notifications | Auto-dismiss 5s, progress bar, success/error states |
| Context Menu | Tab actions with keyboard shortcuts |

## Design System

- **Surfaces**: `#050505`, `#0A0A0A`, `#111111`, `#181818`, `#222222`
- **Accent**: `#8B0000` (crimson), `#A51010` (hover)
- **Text**: `#E8E8E8`, `#A0A0A0`, `#606060`, `#404040`, `#2A2A2A`
- **Spacing**: Base 4px (`--u`), all values multiples of 4
- **Elevation**: 5 shadow layers (`--z1` to `--z5`) + inset press state
- **Typography**: Space Grotesk, architectural proportions
- **Corners**: Square everywhere (no border-radius)
- **Animations**: `cubic-bezier` based, GPU-accelerated transforms only

## Security Color Coding

URL bar bottom border indicates connection state:
- **Green** (`#30A050`) — HTTPS secure
- **Red** (`#CF3030`) — HTTP insecure
- **Amber** (`#B8860B`) — Mixed content
- **Neutral** (`#222222`) — Empty / NTP

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+T` | New Tab |
| `Ctrl+W` | Close Tab |
| `Ctrl+R` | Reload |
| `Ctrl+1/2/3` | Switch Workspace |
| `Ctrl+Shift+P` | Command Palette |
| `Ctrl+,` | Settings |
| `Ctrl+B` | Side Panel (Bookmarks) |
| `F5` | Reload |
| `F11` | Full Screen |
| `F12` | DevTools |
| `Esc` | Close overlays |

## How to Use

Open `index.html` in any modern browser. No build step required.

## Changelog

- **v9**: Side Panel: Bookmarks/History/Downloads only. Workspaces back to Title Bar. Color-coded security on URL bar.
- **v8**: Extension Toolbar removed. `+` button opens Command Palette. Side Panel with 4 tabs.
- **v7**: Depth edition — elevation system, parallax NTP, side panel, breadcrumb URL bar, rich tooltips.
- **v6**: Full rebuild — all screens, responsive modes, settings, command palette, workspace selector.
