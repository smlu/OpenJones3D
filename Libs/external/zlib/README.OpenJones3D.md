# OpenJones3D dependency provenance

This directory vendors the unmodified zlib 1.4 release source (263 files).
The additional `README.OpenJones3D.md` records this import, and `.gitignore`
keeps the required upstream `zlib.map` source visible despite the parent ignore rule.

- Upstream release: https://github.com/madler/zlib/releases/tag/v1.4
- Release archive: `zlib-1.4.tar.gz`
- Download: https://zlib.net/zlib-1.4.tar.gz
- SHA-256: `02dd159783677a749c04adbdd02b55b8a2bcc7e054f51b76b549199a916ad584`
- Archive size: 1,507,140 bytes
- License: [LICENSE](LICENSE)

OpenJones3D builds the upstream `zlibstatic` target and links SMUSH through
`ZLIB::ZLIBSTATIC`. Shared-library builds, upstream example/test targets and
installation rules are disabled in the parent CMake file. Upstream contrib
features retain their disabled defaults. The root project owns MSVC CRT selection;
generated configuration headers stay in the build directory.

For MSVC, the static target privately defines `_CRT_DECLARE_NONSTDC_NAMES=1`: zlib
uses POSIX CRT names that UCRT otherwise hides under the project's `/Zc:__STDC__`.
The project keeps its C11 settings, and upstream sources remain unchanged.
