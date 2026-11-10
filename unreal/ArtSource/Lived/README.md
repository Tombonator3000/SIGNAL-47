# Lived: kilden til SARO-romenes og bygningenes bruksspor

Originale bpy-skript for `/Game/Lived` (T16-15, 7. november 2026): kontrollrommets og korridorens møblering (`control_dressing.py`), SARO utvendig (`saro/build_saro.py`), gårdshuset (`house/`) og byens teksturer (`town/`). Laget for SIGNAL / 47; teksturene genereres av `lived_tex.py`, `lived_paper.py`, `vct_floor.py`, `house/house_tex.py` og `town/town_tex.py`. Ingen nedlastede modeller eller bilder.

Kopiert inn fra arbeidsmappa 9. november 2026 (M6, DALE-WINDOW-20261108), der den bare lå i en midlertidig mappe. Mappestrukturen er arbeidsmappas `src/`: `geo.py` og `import_lived.py` ligger her, byggeskriptene i undermapper. `run_blender.py`, `run_import.py` og `runall.sh` forventer `src/` under seg; kjør dem fra en mappe med denne strukturen, med Blender 5.2 og bak prosjektets motorreservasjon (`~/.claude/skills/signal47-nattskift/scripts/q.py`). Bilens skript er ikke med: bilens modell er lisensiert og holdes utenfor Git.

- `saro/build_saro.py`: blokkhuden 6 mm utenpå veggene. 9. november fikk anneksets sørvegg åpning for Dales vindu (x -0,55..1,30, y 1,08..2,40, samme mål som `S47OfficeWindow.cpp` skjærer), og smussbåndene går rundt det.
- Import: `run_import.py saro <src>` skriver bare under `/Game/Lived/Saro` og en rapport i `Saved/lived-saro-import.json`.

## Bilen (D46-7, JEEP-FORM-20261108, 10. november 2026)

Spillerens Jeep er Toms egen Tripo-modell med Codex' kabin- og hjuldeling; kilden er lisensiert og ligger utenfor Git. Den 10. november ble fronten og hekken bygget om som en generisk kompakt 4x4 fra tidlig 1980-tall uten varemerker: avrundede forskjermer med buet hjulåpning og rullet kant, flatt panser med svak hvelving og midtrygg, rektangulær grillåpning med netting bak en malt ramme (ingen spalter), runde lykter i kromringer på spillets målte lyktepunkter, stålkanal foran og bak, og reservehjul stående på bakluka. Hjulnav, radier, lyktepunkter, førerøye, kabindeler og målestokk/akser er uendret.

- Byggskript, blend, glb, før/etter-bilder og importrot: `~/Documents/ChatGPT/Signal 47/modeller/jeep-20261110/` (`build_jeep.py`, `render_views.py`, `import-root/`).
- Import: `unreal/ArtSource/Lived/import_truck.sh [importrot]` kjører `run_import.py truck <rot>` bak motorreservasjonen (Nanite av, tre LOD-er, uten kollisjon) og leser `Saved/lived-truck-import.json`. Kontrakten her (`lived-truck-contract.json`) er den samme som i importroten.
- Selvtesten `FS47Fleet::Audit` kontrollerer at begge lyktepunktene ligger på den importerte kroppen.
