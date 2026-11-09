# Lived: kilden til SARO-romenes og bygningenes bruksspor

Originale bpy-skript for `/Game/Lived` (T16-15, 7. november 2026): kontrollrommets og korridorens møblering (`control_dressing.py`), SARO utvendig (`saro/build_saro.py`), gårdshuset (`house/`) og byens teksturer (`town/`). Laget for SIGNAL / 47; teksturene genereres av `lived_tex.py`, `lived_paper.py`, `vct_floor.py`, `house/house_tex.py` og `town/town_tex.py`. Ingen nedlastede modeller eller bilder.

Kopiert inn fra arbeidsmappa 9. november 2026 (M6, DALE-WINDOW-20261108), der den bare lå i en midlertidig mappe. Mappestrukturen er arbeidsmappas `src/`: `geo.py` og `import_lived.py` ligger her, byggeskriptene i undermapper. `run_blender.py`, `run_import.py` og `runall.sh` forventer `src/` under seg; kjør dem fra en mappe med denne strukturen, med Blender 5.2 og bak prosjektets motorreservasjon (`~/.claude/skills/signal47-nattskift/scripts/q.py`). Bilens skript er ikke med: bilens modell er lisensiert og holdes utenfor Git.

- `saro/build_saro.py`: blokkhuden 6 mm utenpå veggene. 9. november fikk anneksets sørvegg åpning for Dales vindu (x -0,55..1,30, y 1,08..2,40, samme mål som `S47OfficeWindow.cpp` skjærer), og smussbåndene går rundt det.
- Import: `run_import.py saro <src>` skriver bare under `/Game/Lived/Saro` og en rapport i `Saved/lived-saro-import.json`.
