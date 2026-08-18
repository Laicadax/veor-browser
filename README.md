# VEOR Browser

A premium desktop browser built on Chromium Content API. Engineered for precision, silence, and architectural restraint.

## Philosophy

- Deep black surfaces (#050505)
- Restrained crimson accent (#8B0000)
- Mathematical spacing and invisible grids
- No glassmorphism, no gradients, no decorative noise
- Interfaces feel inevitable, not designed

## Architecture

| Module | Purpose |
|--------|---------|
| `core/` | Base types, logging, result types, IDs |
| `content/` | Chromium integration: BrowserContext, MainParts, ContentBrowserClient |
| `network/` | URLRequestContext, NetworkDelegate, Safe Browsing |
| `safe_browsing/` | Google Safe Browsing API v4 + local hash database |
| `navigation/` | NavigationThrottle for threat interception |
| `ui/` | Views-based interface: shell, tabs, omnibox, palette |
| `workspace/` | Isolated browsing environments |
| `tabs/` | Tab management |
| `history/bookmarks/` | SQLite-backed storage |
| `storage/` | Database engine |
| `devtools/` | DevTools integration |
| `sandbox/` | Platform sandbox (seccomp/Seatbelt/Windows) |

## Building

```bash
# 1. Fetch Chromium
fetch --nohooks chromium
cd src
gclient runhooks

# 2. Copy VEOR
cp -r /path/to/veor ~/chromium/src/veor
echo 'import("//veor/BUILD.gn")' >> BUILD.gn

# 3. Configure
mkdir -p out/Default
cat > out/Default/args.gn << 'EOF'
is_debug = false
symbol_level = 1
enable_nacl = false
treat_warnings_as_errors = false
EOF
gn gen out/Default

# 4. Build
autoninja -C out/Default veor_browser

# 5. Run
./out/Default/veor_browser
```

## Safe Browsing API Key

No key ships with the browser. Without one, Safe Browsing runs in local-only
mode (local hash database and cache, no remote lookups). To enable remote
lookups, provide your own Google Safe Browsing API key:

**Method A: config.json**
```bash
mkdir -p ~/.config/veor/Default
echo '{"safe_browsing_api_key": "your-key"}' > ~/.config/veor/Default/config.json
```

**Method B: environment variable**
```bash
export VEOR_SAFE_BROWSING_API_KEY="your-key"
./out/Default/veor_browser
```

## Features

- **Safe Browsing**: Google API v4 + local SQLite hash database, thread-safe LRU cache
- **Command Palette**: `Ctrl+Shift+P` — unified command/search interface
- **Workspaces**: Isolated browsing environments per profile
- **Blocked Page**: Inline threat warning with VEOR identity
- **DevTools**: `Ctrl+Shift+I` — Chrome DevTools frontend
- **Sandbox**: Platform-native process isolation

## License

BSD-style — see LICENSE file.