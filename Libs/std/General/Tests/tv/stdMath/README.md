# Math Fixtures

`sin_table.bin` and `tan_table.bin` each contain 4096 IEEE-754 binary32 values,
stored as little-endian bytes. They preserve the engine's lookup constants
independently of the decimal literals compiled into `stdMathTables.h`.

The data was extracted from the unmodified retail v1.0 `Indy3D.exe`, with SHA-256
`3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53`.
These are numeric engine constants, not level, sound, model, or texture assets.
Normal tests read these standalone files and never load or execute the game.

SHA-256 checksums:

```text
ff300f1e4a30a540c3acc98d53d1213a67a88c59c512fb0f4b0f1a73cd26f6a5  sin_table.bin
4f788de911190a0759f93d2906a33ba955055032b8b10cb9490f54f6a9a1cbda  tan_table.bin
```

The inverse-trig cases in `stdMathTest.c` pin single-precision polynomial
evaluation, the fitted coefficients, range-reduction boundaries, and the
original degree scale. Their expected bit patterns were independently evaluated
with binary32 rounding after each operation. They validate the reconstructed C
arithmetic on the supported compiler, not runtime parity with x87 intermediates.
