# Third-party code in Babylon

Every dependency of the Babylon Engine is recorded here: vendored code lives in
a directory below with its license text, fetched code is not committed and gets
a row with its source and version (ADR-0017).

| Name | Version | License | Source | Files |
|------|---------|---------|--------|-------|
| doctest | 2.5.3 | MIT | <https://github.com/doctest/doctest/releases/tag/v2.5.3> | `doctest/doctest.h`, `doctest/LICENSE.txt` |
| SDL3 | 3.4.18 | zlib | <https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18> | fetched at configure time, not vendored |

Every new dependency needs a row here and must have a permissive license: zlib,
MIT, BSD, Apache-2.0 or public domain. No GPL or AGPL code (see ADR-0008).
