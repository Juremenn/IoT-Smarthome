# Horizontal Pipeline Flowchart in README Implementation Plan

> **For Antigravity:** REQUIRED SUB-SKILL: Load executing-plans to implement this plan task-by-task.

**Goal:** Mengganti master flowchart vertikal (`flowchart TD`) di `README.md` menjadi horizontal pipeline (`flowchart LR`) yang menyamping, bersih, dan proporsional untuk tampilan desktop GitHub.

**Architecture:** Diagram dibagi menjadi 6 kolom tahapan horizontal (Boot, Network & Web, Sensor Input, Decision Logic, Actuators & OLED, Return Sync) dengan simbol standar ISO-5807 dan teks node yang ringkas dan padat.

**Tech Stack:** Markdown, Mermaid.js (flowchart LR), Git.

---

### Task 1: Generate & Validate Mermaid `flowchart LR` Syntax

**Files:**
- Modify: `README.md:54-211`

**Step 1: Draft the minimal failing/syntax test or validate Mermaid code**
Write a Python validator script in `tools/validate_mermaid.py` to check that the Mermaid block in `README.md` is valid syntax and uses `flowchart LR`.

**Step 2: Run validator to verify it catches the old `flowchart TD`**
Run: `python -c "content = open('README.md', encoding='utf-8').read(); assert 'flowchart LR' in content"`
Expected: AssertionError (currently `flowchart TD`).

**Step 3: Update README.md with the horizontal pipeline Mermaid diagram**
Replace the diagram in `README.md` lines 54-211 with `flowchart LR` and compact nodes.

**Step 4: Run validator to verify it passes**
Run: `python -c "content = open('README.md', encoding='utf-8').read(); assert 'flowchart LR' in content; print('PASS: flowchart LR found')"`
Expected: PASS.

**Step 5: Commit**
```bash
git add README.md
git commit -m "docs(readme): convert master flowchart to horizontal pipeline flowchart LR"
```

---

### Task 2: Push Changes to Remote GitHub Repository

**Files:**
- Repository: branch `main`

**Step 1: Check git status**
Run: `git status`
Expected: Working tree clean, ahead of origin/main by commits.

**Step 2: Push to GitHub**
Run: `git push origin main`
Expected: Successfully pushed to `origin/main`.

**Step 3: Verify GitHub commit head**
Run: `git rev-parse HEAD; git rev-parse origin/main`
Expected: Hashes match.
