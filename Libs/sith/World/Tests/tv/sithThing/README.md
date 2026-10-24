# SithThing binary test vectors

These files contain synthetic `sithThing_WriteThingsListBinary` output. They do
not contain game assets or save data.

The vectors were generated twice with the retail `Indy3D.exe` serializer and
were byte-for-byte deterministic. The reconstructed writer produced identical
files, and the reconstructed reader matched the retail reader after both results
were serialized again by the retail writer.

Retail executable SHA-256:

`3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53`

Vectors:

- `all_fields.bin`: all 15 thing types, resources, sector placement, movement,
  type-specific state, AI state, and path frames.
- `core.bin`: all 15 thing types and their serialized state without resource or
  sector references; also used as the reader golden vector.
- `empty.bin`: an empty thing list.
- `max_names.bin`: the longest null-terminated thing name accepted by the
  fixed-size field.
- `inherited_weapon.bin`: a weapon whose explosion template is inherited from
  its base template.

SHA-256:

```text
74595ecdf90f4449c1efb6969ca5b962a43b26adb64faf0dd42d9ed3fcb333ab  all_fields.bin
4f7696165df1a354e9ecf0256b12d5e8987957b0c3779663fcec863aac3743cc  core.bin
85759b3811ff7dc47b03792ac85317be51431a3f9e01dcafce317ed736a391b0  empty.bin
aadf3051794a902b8ef331732c64514af33a92ef6ce768d6ca756ff31cc6fb51  inherited_weapon.bin
bba543b2521434b6ba5755437f705f3ac1a9e0170ff4f33385051e1140a2ecbc  max_names.bin
```

The retail executable and temporary injected oracle are intentionally not part
of the permanent test suite. Tests consume only these committed vectors.
