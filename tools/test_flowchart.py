#!/usr/bin/env python3
"""
Automated Quality, Geometry & Anti-Slop Audit for English ISO 5807 Flowcharts.
"""
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DRAWIO_PATH = ROOT / "IotSmarthome.drawio"
MD_GUIDE_PATH = ROOT / "PANDUAN_FLOWCHART_SMART_HOME.md"
TXT_GUIDE_PATH = ROOT / "ALUR_FLOWCHART_SMART_HOME.txt"
README_PATH = ROOT / "README.md"

INDONESIAN_FORBIDDEN = [
    r"\bInisialisasi\b",
    r"\bApakah\b",
    r"\bTidak\b",
    r"\bPintu\b",
    r"\bSuhu\b",
    r"\bKelembaban\b",
    r"\bLayar\b",
    r"\bKirim\b",
    r"\bBaca\b",
    r"\bTombol\b",
    r"\bMulai\b",
    r"\bSelesai\b",
    r"\bPerangkat\b",
    r"\bDarurat\b",
    r"\bTerdeteksi\b",
    r"\bMendeteksi\b",
    r"\bPertahankan\b",
    r"\bSimpan\b",
    r"\bPerbarui\b",
    r"\bTampilkan\b",
    r"\bAktifkan\b",
    r"\bNonaktifkan\b",
]


def check_no_em_dash(path: Path, errors: list[str]) -> None:
    text = path.read_text(encoding="utf-8")
    if "\u2014" in text or "&mdash;" in text:
        errors.append(f"[R-02 FAIL] Em dash found in {path.name}")


def check_no_indonesian(path: Path, errors: list[str]) -> None:
    text = path.read_text(encoding="utf-8")
    for pat in INDONESIAN_FORBIDDEN:
        m = re.search(pat, text, flags=re.IGNORECASE)
        if m:
            errors.append(f"[LANG FAIL] Indonesian token '{m.group(0)}' found in {path.name}")
            break


def check_drawio(errors: list[str]) -> None:
    if not DRAWIO_PATH.exists():
        errors.append("IotSmarthome.drawio does not exist")
        return

    check_no_em_dash(DRAWIO_PATH, errors)
    check_no_indonesian(DRAWIO_PATH, errors)

    tree = ET.parse(DRAWIO_PATH)
    root = tree.getroot()
    diagrams = root.findall("diagram")
    if len(diagrams) != 5:
        errors.append(f"Expected 5 diagrams in IotSmarthome.drawio, found {len(diagrams)}")

    for diag in diagrams:
        dname = diag.get("name", "")
        cells = diag.findall(".//mxCell")
        rhombus_ids = set()
        vertices = {}
        edges = []

        for c in cells:
            cid = c.get("id", "")
            style = c.get("style", "")
            if c.get("vertex") == "1":
                vertices[cid] = c
                if "rhombus" in style:
                    rhombus_ids.add(cid)
            elif c.get("edge") == "1":
                edges.append(c)

        # Check decision diamonds: at most 2 outgoing edges, distinct exit ports, all labeled
        out_by_rhombus: dict[str, list[tuple[str, str, str]]] = {rid: [] for rid in rhombus_ids}
        for e in edges:
            src = e.get("source", "")
            if src in out_by_rhombus:
                style = e.get("style", "")
                val = (e.get("value") or "").strip()
                ex_m = re.search(r"exitX=([0-9.]+)", style)
                ey_m = re.search(r"exitY=([0-9.]+)", style)
                ex = ex_m.group(1) if ex_m else ""
                ey = ey_m.group(1) if ey_m else ""
                out_by_rhombus[src].append((ex, ey, val))

        for rid, outs in out_by_rhombus.items():
            if len(outs) != 2:
                errors.append(
                    f"[ISO-5807 FAIL] Diagram '{dname}' decision '{rid}' has {len(outs)} outgoing edges (expected 2 binary branches)"
                )
            ports = [(ex, ey) for ex, ey, _ in outs]
            if len(set(ports)) != len(ports):
                errors.append(
                    f"[PORT COLLISION] Diagram '{dname}' decision '{rid}' has duplicate outgoing ports: {ports}"
                )
            for _, _, val in outs:
                if not val:
                    errors.append(
                        f"[LABEL FAIL] Diagram '{dname}' decision '{rid}' has an unlabeled outgoing branch"
                    )


def check_readme_mermaid(errors: list[str]) -> None:
    text = README_PATH.read_text(encoding="utf-8")
    check_no_em_dash(README_PATH, errors)
    m = re.search(r"```mermaid(.*?)```", text, flags=re.DOTALL)
    if not m:
        errors.append("No mermaid block found in README.md")
        return
    mermaid = m.group(1)
    for pat in INDONESIAN_FORBIDDEN:
        hit = re.search(pat, mermaid, flags=re.IGNORECASE)
        if hit:
            errors.append(f"[README MERMAID LANG FAIL] Indonesian token '{hit.group(0)}' in Mermaid block")
            break


def main() -> int:
    errors: list[str] = []
    check_drawio(errors)
    for p in (MD_GUIDE_PATH, TXT_GUIDE_PATH):
        check_no_em_dash(p, errors)
        check_no_indonesian(p, errors)
    check_readme_mermaid(errors)

    if errors:
        print("FLOWCHART AUDIT FAILED:")
        for err in errors:
            print("  -", err)
        return 1

    print("ALL FLOWCHART AUDITS PASSED!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
