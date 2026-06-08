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

LATIN_FALLBACKS = ("DejaVu", "LiberationSans", "FreeSans")

CJK_INDICATORS = (
    "CJK", "Gothic", "Mincho", "Song", "Hei", "Ming",
    "Source Han", "WenQuanYi", "Nanum", "Malgun", "YaHei",
    "PingFang", "Hiragino", "msyh", "simsun", "msjh",
    "NotoSansCJK", "NotoSerifCJK", "wqy",
)

def fc_match(lang_tag):
    """Ask fontconfig for the best font for a language tag (Linux/macOS)."""
    try:
        result = subprocess.run(
            ["fc-match", "--format=%{file}", f":lang={lang_tag}"],
            capture_output=True, text=True, timeout=5
        )
        path = result.stdout.strip()
        for name in LATIN_FALLBACKS:
            if name in path:
                return None
        return path if path and os.path.exists(path) else None
    except Exception:
        return None

def fc_list_first(lang_tag):
    """Return the first font file that fontconfig declares support for lang_tag.
    Unlike fc-match, fc-list only returns fonts that genuinely cover the language."""
    try:
        result = subprocess.run(
            ["fc-list", f":lang={lang_tag}", "--format=%{file}\n"],
            capture_output=True, text=True, timeout=5
        )
        for line in result.stdout.splitlines():
            path = line.strip()
            if path and os.path.exists(path):
                return path
        return None
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
    CJK_IDEO_FONT   = fc_list_first("zh") or find_first("/System/Library/Fonts/PingFang.ttc")
    CJK_IDEO_FNUM   = 0
    CJK_KOREAN_FONT = fc_list_first("ko") or find_first("/System/Library/Fonts/AppleSDGothicNeo.ttc")
else:  # Linux
    LATIN_FONT_REG  = find_first(
        "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    ) or fc_match("vi") or fc_match("la")
    CJK_IDEO_FONT   = fc_list_first("zh")
    CJK_IDEO_FNUM   = 0
    CJK_KOREAN_FONT = fc_list_first("ko")

# ── pyftsubset wrapper ────────────────────────────────────────────────────────

def run_pyftsubset(source, output, *, unicodes=None, text=None, font_number=None):
    # TTC collections always need a font-number; default to 0 (Regular weight)
    if font_number is None and source.endswith(".ttc"):
        font_number = 0
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
    if not LATIN_FONT_REG:
        print("ERROR: no Latin/Vietnamese font found (needed for viet_name.h)")
        print("  Install a font covering Latin Extended Additional (e.g. 'ttf-liberation' or 'noto-fonts')")
        sys.exit(1)

    missing_cjk = []
    if not CJK_IDEO_FONT:
        missing_cjk.append("CJK ideograph font (zh)")
    if not CJK_KOREAN_FONT:
        missing_cjk.append("Korean font (ko)")
    has_cjk = not missing_cjk
    if missing_cjk:
        print("WARNING: no CJK fonts found — writing stub cjk_names.h (CJK_FONTS_UNAVAILABLE):")
        for m in missing_cjk: print(f"  - {m}")
        print("  Install a CJK font (e.g. 'noto-fonts-cjk') to get real CJK font embedding")

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
        latin_font_number = 0 if LATIN_FONT_REG.endswith(".ttc") else None
        run_pyftsubset(LATIN_FONT_REG, tmp_viet_sa, unicodes=VIET_SA_UNICODES, font_number=latin_font_number)
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

        # ── cjk_names.h — ideographs + Korean ────────────────────────────────
        out = os.path.join(OUT_DIR, "cjk_names.h")
        if has_cjk:
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
        else:
            with open(out, "w", encoding="utf-8") as f:
                f.write("// Auto-generated — do not edit.\n")
                f.write("// CJK fonts not found at generation time; CJK language options will\n")
                f.write("// render without dedicated ideograph/hangul font subsets.\n")
                f.write("// Install noto-fonts-cjk (or equivalent) and re-run:\n")
                f.write("//   python tools/generate_font_headers.py\n")
                f.write("#pragma once\n\n")
                f.write("#define CJK_FONTS_UNAVAILABLE\n\n")
                f.write("static const unsigned char CJKNames_Ideographs[] = {0};\n")
                f.write("static const unsigned char CJKNames_Korean[]     = {0};\n")
            print(f"  ->{os.path.relpath(out, REPO_ROOT)}  (stub — no CJK fonts found)")

    finally:
        shutil.rmtree(tmp)

    print("\nDone. Rebuild the project to pick up the new headers.")

if __name__ == "__main__":
    main()
