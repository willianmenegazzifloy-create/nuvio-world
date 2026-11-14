#!/usr/bin/env python3
from pathlib import Path

src = Path("src/ajustes_ux_v2.inc").read_text(encoding="utf-8")

def body(signature: str) -> str:
    start = src.index(signature)
    opening = src.index("{", start)
    depth = 0
    in_string = False
    quote = ""
    escaped = False
    i = opening
    while i < len(src):
        c = src[i]
        if in_string:
            if escaped:
                escaped = False
            elif c == "\\":
                escaped = True
            elif c == quote:
                in_string = False
        elif c in ('"', "'"):
            in_string, quote = True, c
        elif c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return src[opening + 1:i]
        i += 1
    raise AssertionError(f"unterminated function: {signature}")

max_rows = body("static int aj2GradeMaxLinhas(void)")
card_height = body("static float aj2GradeCartaoH(void)")
card_draw = body("static void aj2GradeCartaoDesenhar(int s, float f)")

assert "aj2GradeLinha(s) + 1" in max_rows
assert "s < nSecoes" in max_rows
assert "aj2GradeMaxLinhas()" in card_height
assert "(float)(linhas - 1) * AJ2_G_VAO_V" in card_height
assert "/ (float)linhas" in card_height
assert "/ 4.0f" not in card_height
assert "int compacto = r.h < 108.0f" in card_draw
assert "!compacto && vb[0]" in card_draw

for rows in ([4, 5, 3], [4, 4, 3], [3, 3, 3]):
    longest = max(rows)
    usable, gap = 500.0, 20.0
    height = (usable - (longest - 1) * gap) / longest
    for count in rows:
        bottom = (count - 1) * (height + gap) + height
        assert bottom <= usable + 0.001

print("PASS: Settings cards fit the longest column; compact cards keep labels inside")
