"""Check document/asset consistency; does not test gameplay or prove playtime."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, re, subprocess, xml.etree.ElementTree as ET
import pdfplumber
from PIL import Image
import build_visuals

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
BASELINE = 'f156a9f2f656d168356145dabbe0bc8fb4ef0ec9'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    report_path = ROOT / 'Sources/design-review.json'
    report = json.loads(report_path.read_text())
    text = (ROOT / 'design-bible.md').read_text()
    ids = re.findall(r'^\| (P\d\d) \|', text, re.M)
    assert ids == [f'P{i:02d}' for i in range(1, 19)], ids
    minutes = [int(x) for x in re.findall(r'^\| (?:K\d|E) - .*? \| .*? \| (\d+) \|', text, re.M)]
    assert minutes == [50, 45, 60, 45, 65, 55, 10]
    assert sum(minutes) == 330
    for constant in ('1419.900', '1420.110', '1420.405', '4 / 7', '−39 LY', '47 spillsekunder'):
        assert constant in text, constant
    # This is the proposed task model reviewed against the chapter text and map.
    # Its reachability does not imply implemented game states are reachable.
    deps = report['task_dependencies']
    assert set(deps) == set(ids)
    reached = set()
    while len(reached) < len(ids):
        next_ids = {p for p, requirements in deps.items() if set(requirements) <= reached} - reached
        assert next_ids, 'Cycle or unavailable dependency in proposed model'
        reached |= next_ids
    link_count = 0
    for doc in ROOT.glob('*.md'):
        for link in re.findall(r'\]\(([^)]+)\)', doc.read_text()):
            if '://' in link or link.startswith('#'):
                continue
            assert (doc.parent / link.split('#')[0]).is_file(), (doc, link)
            link_count += 1
    visuals = ROOT / 'Visuals'
    assert len(list(visuals.glob('map-*.svg'))) == 4
    assert len(list(visuals.glob('ui-*.svg'))) == 6
    assert len(list(visuals.glob('concept-*.png'))) == 3
    for path in visuals.glob('*.svg'):
        ET.parse(path)
    for path in visuals.glob('*.png'):
        with Image.open(path) as im:
            im.verify()
    for name, board in build_visuals.make_all().items():
        assert all(t['x'] >= 0 and t['right'] <= 1280 and 0 <= t['baseline_y'] <= 800 for t in board.bounds), name
    subprocess.run(['git', 'diff', '--exit-code', BASELINE, '--', 'Unity', 'Spill-SIGNAL47.sh'], cwd=REPO, check=True, stdout=subprocess.PIPE)
    pdf = ROOT / 'output/pdf/SIGNAL47-Designbibel-v0.1.pdf'
    geometry = []
    with pdfplumber.open(pdf) as book:
        for i, page in enumerate(book.pages, 1):
            words = page.extract_words()
            outside = [w for w in words if w['x0'] < 0 or w['x1'] > page.width + .1 or w['top'] < 0 or w['bottom'] > page.height + .1]
            assert not outside, (i, outside)
            geometry.append(dict(page=i, width=round(page.width, 2), height=round(page.height, 2), words=len(words), outside_page=0))
        assert sum(p.width > p.height for p in book.pages) == 13
    report.update(recorded_utc=datetime.now(timezone.utc).isoformat(), baseline_commit=BASELINE,
                  pdf_sha256=sha(pdf), pdf_pages=len(geometry), page_geometry=geometry,
                  task_count=len(ids), chapter_minutes=minutes, total_target_minutes=sum(minutes),
                  local_markdown_links_checked=link_count, vector_files=10, concept_files=3,
                  structural_result='PASS', Unity_changes='none; compared with baseline; launcher also unchanged')
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('structural_result', 'pdf_pages', 'task_count', 'total_target_minutes', 'local_markdown_links_checked')}, indent=2))

if __name__ == '__main__':
    main()
