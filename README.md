# Aitum Stream Suite — Streamlabs TikTok Fork

OBS Studio plugin providing per-output stream management, extended from [Aitum Stream Suite](https://github.com/aitum-io/obs-aitum-stream-suite).

## Added: Streamlabs TikTok Output

This fork adds a **Streamlabs TikTok** output type that:

- Authenticates with Streamlabs via OAuth PKCE flow
- Fetches a dynamic TikTok RTMP stream key from Streamlabs' internal API at stream start
- Starts/ends the TikTok stream through the Streamlabs API
- Provides category search with debounced auto-complete
- Includes a manual "End TikTok Stream" button on the config page as a fallback

### ⚠️ Disclaimer

**This integration uses undocumented Streamlabs internal API endpoints** (`/api/v5/slobs/tiktok/*`). These endpoints are consumed by the official Streamlabs Desktop application and are **not a public API**. There is **no guarantee** they will remain stable. If Streamlabs changes or removes these endpoints, this plugin will stop working and there is little that can be done beyond finding a new approach.

Use at your own risk.

### Credits

The OAuth PKCE flow and API interaction pattern are based on the work of **casuallyawaiting**'s [`StreamlabsTikTokStreamKeyGenerator.py`](https://github.com/casuallyawaiting/StreamlabsTikTokStreamKeyGenerator) Python script — thank you for reverse-engineering the flow.

## Building

Requires:
- CMake 3.16+
- Visual Studio 2022 Build Tools (or full VS)
- Windows SDK 10.0.20348+

```powershell
cmake --preset windows-x64
cmake --build build_x64 --config RelWithDebInfo
```

The built plugin (`aitum-stream-suite.dll`) and locale file will be under `build_x64/RelWithDebInfo/`.

## Installation

Copy to your OBS installation:

| File | Destination |
|------|-------------|
| `build_x64/RelWithDebInfo/aitum-stream-suite.dll` | `C:\Program Files\obs-studio\obs-plugins\64bit\` |
| `data/locale/en-US.ini` | `C:\Program Files\obs-studio\data\obs-plugins\aitum-stream-suite\locale\` |

## License

Same as upstream — [GNU General Public License v2.0](LICENSE).
