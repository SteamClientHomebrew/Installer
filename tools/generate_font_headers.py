#!/usr/bin/env python3
"""
Generate embedded font headers for the Installer.

Produces two C++ headers in src/include/:

  viet_name.h
      VietName_Standalone – ALL glyphs of "Tieng Viet" from arial.ttf,
                            loaded as Fonts[2] and pushed via PushFont in the
                            language-selector dropdown so the Vietnamese item
                            renders from a single typeface with no Geist mix.
      When Vietnamese is the active language the full system Arial/Bold is
      loaded as the PRIMARY font so every glyph in the UI comes from one
      consistent typeface.

  cjk_names.h
      Minimal CJK subsets covering only the characters used in the
      language-selector display names:  简体中文  繁體中文  日本語  한국어

Original commands (Windows, session 2025-05):
  pyftsubset arial.ttf   --unicodes=U+0020,U+0054,U+0056,U+0067,U+0069,U+006E,U+0074,U+1EBF,U+1EC7 --no-layout-closure
  pyftsubset msyh.ttc    --text="简体中文繁體日本語" --font-number=0 --no-layout-closure
  pyftsubset malgun.ttf  --text="한국어" --no-layout-closure

Requirements:
  pip install fonttools brotli

Usage:
  python tools/generate_font_headers.py
"""

import sys
import os
import tempfile
import shutil
import subprocess
import platform

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR   = os.path.join(REPO_ROOT, "src", "include")

# ── Font candidates per platform ─────────────────────────────────────────────

def find_first(*paths):
    for p in paths:
        if p and os.path.exists(p):
            return p
    return None

def fc_match(lang_tag):
    """Ask fontconfig for the best font for a language tag (Linux/macOS)."""
    try:
        result = subprocess.run(
            ["fc-match", "--format=%{file}", f":lang={lang_tag}"],
            capture_output=True, text=True, timeout=5
        )
        path = result.stdout.strip()
        for fallback in ("DejaVu", "LiberationSans", "FreeSans"):
            if fallback in path:
                return None
        return path if path and os.path.exists(path) else None
    except Exception:
        return None

system = platform.system()

if system == "Windows":
    FONTS = os.environ.get("WINDIR", "C:/Windows") + "/Fonts"
    LATIN_FONT_REG  = find_first(f"{FONTS}/arial.ttf")
    CJK_IDEO_FONT   = find_first(f"{FONTS}/msyh.ttc", f"{FONTS}/simsun.ttc")
    CJK_IDEO_FNUM   = 0   # font-number inside the TTC collection
    CJK_KOREAN_FONT = find_first(f"{FONTS}/malgun.ttf")
elif system == "Darwin":
    LATIN_FONT_REG  = find_first(
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
    )
    CJK_IDEO_FONT   = fc_match("zh") or find_first("/System/Library/Fonts/PingFang.ttc")
    CJK_IDEO_FNUM   = 0
    CJK_KOREAN_FONT = fc_match("ko") or find_first("/System/Library/Fonts/AppleSDGothicNeo.ttc")
else:  # Linux
    LATIN_FONT_REG  = find_first(
        "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    )
    CJK_IDEO_FONT   = fc_match("zh")
    CJK_IDEO_FNUM   = 0
    CJK_KOREAN_FONT = fc_match("ko")

# ── pyftsubset wrapper ────────────────────────────────────────────────────────

def run_pyftsubset(source, output, *, unicodes=None, text=None, font_number=None):
    cmd = [sys.executable, "-m", "fontTools.subset", source,
           f"--output-file={output}", "--no-layout-closure",
           "--ignore-missing-unicodes"]
    if unicodes:
        cmd.append(f"--unicodes={unicodes}")
    if text:
        cmd.append(f"--text={text}")
    if font_number is not None:
        cmd.append(f"--font-number={font_number}")
    subprocess.run(cmd, check=True)

# ── C++ header writer ─────────────────────────────────────────────────────────

def bytes_to_cpp_array(name, data):
    lines = [f"static const unsigned char {name}[] = {{"]
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        sep = "," if i + 16 < len(data) else ""
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + sep)
    lines.append("};")
    return "\n".join(lines)

# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    errors = []
    if not LATIN_FONT_REG:
        errors.append("Arial not found (needed for viet_name.h)")
    if not CJK_IDEO_FONT:
        errors.append("CJK ideograph font not found (needed for cjk_names.h)")
    if not CJK_KOREAN_FONT:
        errors.append("Korean font not found (needed for cjk_names.h)")
    if errors:
        print("ERROR: missing fonts:")
        for e in errors: print(f"  - {e}")
        sys.exit(1)

    tmp = tempfile.mkdtemp()
    try:
        os.makedirs(OUT_DIR, exist_ok=True)

        # ── viet_name.h ───────────────────────────────────────────────────────
        # All glyphs of "Tieng Viet" from arial.ttf.
        # Loaded as Fonts[2] and pushed per-item in the dropdown so the
        # Vietnamese name renders from one typeface with no Geist mixing.
        # Codepoints: space T V g i n t ef(U+1EBF) e-comb(U+1EC7)
        VIET_SA_UNICODES = "U+0020,U+0054,U+0056,U+0067,U+0069,U+006E,U+0074,U+1EBF,U+1EC7"
        print(f"viet_name.h <-{LATIN_FONT_REG}")
        tmp_viet_sa = os.path.join(tmp, "viet_name_standalone.ttf")
        run_pyftsubset(LATIN_FONT_REG, tmp_viet_sa, unicodes=VIET_SA_UNICODES)
        viet_sa_data = open(tmp_viet_sa, "rb").read()

        out = os.path.join(OUT_DIR, "viet_name.h")
        with open(out, "w", encoding="utf-8") as f:
            f.write("// Auto-generated — do not edit.\n")
            f.write("// Arial subset covering all glyphs of \"Tieng Viet\" (U+1EBF, U+1EC7 + ASCII).\n")
            f.write("// Loaded as Fonts[2]; pushed via PushFont in the dropdown so the\n")
            f.write("// Vietnamese item renders from a single typeface with no Geist mixing.\n")
            f.write("// Regenerate: python tools/generate_font_headers.py\n")
            f.write("#pragma once\n\n")
            f.write(bytes_to_cpp_array("VietName_Standalone", viet_sa_data))
            f.write("\n")
        print(f"  ->{os.path.relpath(out, REPO_ROOT)}  ({len(viet_sa_data):,} bytes)")

        # ── cjk_names.h — ideographs (msyh.ttc, font 0) ──────────────────────
        IDEO_CHARS   = "简体中文繁體日本語"
        KOREAN_CHARS = "한국어"

        print(f"cjk_names.h (ideographs) <-{CJK_IDEO_FONT}")
        tmp_ideo = os.path.join(tmp, "cjk_ideographs.ttf")
        run_pyftsubset(CJK_IDEO_FONT, tmp_ideo,
                       text=IDEO_CHARS, font_number=CJK_IDEO_FNUM)
        ideo_data = open(tmp_ideo, "rb").read()

        print(f"cjk_names.h (Korean)      <-{CJK_KOREAN_FONT}")
        tmp_ko = os.path.join(tmp, "cjk_korean.ttf")
        run_pyftsubset(CJK_KOREAN_FONT, tmp_ko, text=KOREAN_CHARS)
        korean_data = open(tmp_ko, "rb").read()

        out = os.path.join(OUT_DIR, "cjk_names.h")
        with open(out, "w", encoding="utf-8") as f:
            f.write("// Auto-generated — do not edit.\n")
            f.write("// Minimal CJK font subsets for language-selector display names.\n")
            f.write(f"// Ideograph source: {os.path.basename(CJK_IDEO_FONT)}"
                    f"  chars: {IDEO_CHARS}\n")
            f.write(f"// Korean source:    {os.path.basename(CJK_KOREAN_FONT)}"
                    f"  chars: {KOREAN_CHARS}\n")
            f.write("// Regenerate: python tools/generate_font_headers.py\n")
            f.write("#pragma once\n\n")
            f.write(bytes_to_cpp_array("CJKNames_Ideographs", ideo_data))
            f.write("\n\n")
            f.write(bytes_to_cpp_array("CJKNames_Korean", korean_data))
            f.write("\n")
        print(f"  ->{os.path.relpath(out, REPO_ROOT)}"
              f"  (ideographs {len(ideo_data):,} B + Korean {len(korean_data):,} B)")

    finally:
        shutil.rmtree(tmp)

    print("\nDone. Rebuild the project to pick up the new headers.")

if __name__ == "__main__":
    main()
