import gzip
import re
from pathlib import Path


def minify_html(text: str) -> str:
    text = re.sub(r"<!--.*?-->", "", text, flags=re.S)
    text = re.sub(r">\s+<", "><", text)
    text = re.sub(r"[ \t]{2,}", " ", text)
    return text.strip()


def minify_css(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"\s+", " ", text)
    text = re.sub(r"\s*([{}:;,>])\s*", r"\1", text)
    text = text.replace(";}", "}")
    return text.strip()


def minify_js(text: str) -> str:
    # Hapus komentar blok & baris penuh secara aman (tidak menyentuh 'http://' dalam string)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    lines = []
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("//"):
            continue
        lines.append(stripped)
    return "\n".join(lines)


def format_byte_array(name: str, data: bytes) -> str:
    hex_items = [f"0x{b:02x}" for b in data]
    rows = [
        "  " + ",".join(hex_items[i : i + 20]) + ","
        for i in range(0, len(hex_items), 20)
    ]
    return (
        f"extern const size_t {name}_LEN = {len(data)};\n"
        f"extern const uint8_t {name}[] PROGMEM = {{\n"
        + "\n".join(rows)
        + "\n};\n"
    )


def write_if_changed(path: Path, content: str) -> bool:
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return True


def sync():
    out_dir = Path("src/web")

    ui_specs = [
        ("web_src/index.html", "INDEX_HTML_GZ", minify_html),
        ("web_src/style.css", "STYLE_CSS_GZ", minify_css),
        ("web_src/app.js", "APP_JS_GZ", minify_js),
    ]

    ui_blocks = [
        '#include "web_assets.h"\n\n'
        "// Aset UI terkompresi gzip level 9 (dihasilkan oleh tools/sync_to_esp.py).\n"
    ]

    total_raw = 0
    total_gz = 0
    for src_path, sym, minifier in ui_specs:
        raw = Path(src_path).read_text(encoding="utf-8")
        minified = minifier(raw).encode("utf-8")
        gz = gzip.compress(minified, compresslevel=9, mtime=0)
        total_raw += len(raw.encode("utf-8"))
        total_gz += len(gz)
        ui_blocks.append(format_byte_array(sym, gz))
        print(f"Gzipped: {src_path} ({len(raw)} B -> {len(minified)} B min -> {len(gz)} B gz)")

    header_content = (
        "#pragma once\n"
        "#include <pgmspace.h>\n"
        "#include <stddef.h>\n"
        "#include <stdint.h>\n\n"
        "// Deklarasi ringan aset web PROGMEM agar web_server.cpp cepat di-compile ulang.\n"
        "extern const uint8_t INDEX_HTML_GZ[] PROGMEM;\n"
        "extern const size_t INDEX_HTML_GZ_LEN;\n\n"
        "extern const uint8_t STYLE_CSS_GZ[] PROGMEM;\n"
        "extern const size_t STYLE_CSS_GZ_LEN;\n\n"
        "extern const uint8_t APP_JS_GZ[] PROGMEM;\n"
        "extern const size_t APP_JS_GZ_LEN;\n\n"
        "extern const uint8_t JOGLO_PNG[] PROGMEM;\n"
        "extern const size_t JOGLO_PNG_LEN;\n\n"
        "extern const uint8_t ALARM_MP3[] PROGMEM;\n"
        "extern const size_t ALARM_MP3_LEN;\n"
    )

    static_blocks = [
        '#include "web_assets.h"\n\n'
        "// Aset biner statis (joglo.png & alarm.mp3) dipisah agar di-compile sekali dan di-cache.\n"
    ]
    for src_path, sym in [("web_src/joglo.png", "JOGLO_PNG"), ("web_src/alarm.mp3", "ALARM_MP3")]:
        p = Path(src_path)
        data = p.read_bytes() if p.exists() else b""
        static_blocks.append(format_byte_array(sym, data))
        print(f"Static:  {src_path} ({len(data)} B)")

    h_changed = write_if_changed(out_dir / "web_assets.h", header_content)
    ui_changed = write_if_changed(out_dir / "web_assets_ui.cpp", "\n".join(ui_blocks))
    st_changed = write_if_changed(out_dir / "web_assets_static.cpp", "\n".join(static_blocks))

    pct = (1.0 - (total_gz / total_raw)) * 100.0 if total_raw else 0.0
    print(f"UI total: {total_raw} B -> {total_gz} B gzip (-{pct:.1f}%)")
    print(f"Written: header={h_changed}, ui={ui_changed}, static={st_changed}")


if __name__ == "__main__":
    sync()
