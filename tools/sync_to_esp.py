from pathlib import Path

def sync():
    configs = [
        {
            "src": "web_src/index.html",
            "dst": "src/web/index_html.h",
            "var_name": "INDEX_HTML",
            "comment": "// Halaman dashboard. Disajikan di \"/\" oleh web_server.cpp."
        },
        {
            "src": "web_src/style.css",
            "dst": "src/web/style_css.h",
            "var_name": "STYLE_CSS",
            "comment": "// Gaya dashboard. Disajikan di \"/style.css\".\n// Skala tetap: font 12/14/16/32/48, spacing kelipatan 4, radius 12.\n// Pastel per tile (pilihan pemilik): satu warna per jenis perangkat supaya cepat dikenali.\n// Setiap tile punya --tint (latar), --deep (lingkaran ikon), --on (lingkaran saat aktif),\n// --ink (teks status, kontras >= 4.5:1 di atas --tint). Merah tegas hanya untuk bahaya api."
        },
        {
            "src": "web_src/app.js",
            "dst": "src/web/app_js.h",
            "var_name": "APP_JS",
            "comment": "// Logika dashboard. Disajikan di \"/app.js\"."
        }
    ]

    for item in configs:
        src = Path(item["src"])
        dst = Path(item["dst"])
        if not src.exists():
            print(f"[SKIP] {src} not found")
            continue

        raw_content = src.read_text(encoding="utf-8")
        
        header_content = (
            f"#pragma once\n"
            f"#include <pgmspace.h>\n\n"
            f"{item['comment']}\n"
            f"const char {item['var_name']}[] PROGMEM = R\"rawliteral(\n"
            f"{raw_content}\n"
            f")rawliteral\";\n"
        )

        dst.write_text(header_content, encoding="utf-8")
        print(f"Synced: {src} -> {dst} ({len(raw_content)} bytes)")

    # Sync binary joglo.png -> src/web/joglo_png.h
    joglo_src = Path("web_src/joglo.png")
    joglo_dst = Path("src/web/joglo_png.h")
    if joglo_src.exists():
        data = joglo_src.read_bytes()
        hex_items = [f"0x{b:02x}" for b in data]
        lines = []
        for i in range(0, len(hex_items), 20):
            lines.append("  " + ",".join(hex_items[i:i+20]) + ",")
        
        c_code = (
            "#pragma once\n"
            "#include <pgmspace.h>\n"
            "#include <stddef.h>\n\n"
            f"const size_t JOGLO_PNG_LEN = {len(data)};\n"
            "const uint8_t JOGLO_PNG[] PROGMEM = {\n"
            + "\n".join(lines) + "\n};\n"
        )
        joglo_dst.write_text(c_code, encoding="utf-8")
        print(f"Synced: {joglo_src} -> {joglo_dst} ({len(data)} bytes)")

if __name__ == "__main__":
    sync()
