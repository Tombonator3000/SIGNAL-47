# Tools/recordsroom: arkivrommets realismeløft (RECORDS-ROOM-LIFT-20261113)

**Status:** Gjeldende · **Gjelder:** SARO-arkivrommet i Unreal-utgaven (`S47RecordsRoom.cpp`, `/Game/RecordsRoom`) · **Sist kontrollert:** 13. november 2026

Rommet løftes etter Toms fotoreferanseark (seks vinkler, `~/scratch-bm/refs/records-room-20261113/`, utenfor Git). Alt her er eget arbeid eller CC0. Designnotat med plantegning og før/etter: [Docs/Design/World/RecordsRoom](../../../../Docs/Design/World/RecordsRoom/README.md).

## Løypa

Alle tunge steg går gjennom køen (`python3 ~/.claude/skills/signal47-nattskift/scripts/q.py ...`), ett om gangen.

| Steg | Kommando | Lager |
|---|---|---|
| Overflater | `<venv>/python3 make_surfaces.py <src>/tex` | blokkvegg, vinylflis, himlingsplate, linse, makrokart, kork, kartong (PNG, utenfor Git) |
| Etiketter | `python3 make_labels.py <src>/tex` | `T_Rec_Labels.png` (40 celler: kartongkort, skuffe- og ryggslipper) |
| Modeller | `python3 Tools/recordsroom/run.py blender <nøkkel> ...` | FBX, bakte atlas og Blender-bilder i `<src>/work` |
| Cosmos | `python3 -I cosmos_props.py` | vifte og brannslukker til `Content/Rocket` (git-ignorert) |
| Import | `python3 Tools/recordsroom/run.py import [hva ...]` | `/Game/RecordsRoom` (materialer, teksturer, mesh) |

`<src>` er `S47_REC_SRC` (standard `~/scratch-bm/records-room-20261113`), `<venv>` er en Python med numpy og Pillow. Modellnøklene står i `ASSETS` i `build_records.py` og `MODELS` i `import_assets.py`: `ceiling`, `fittings`, `table`, `lamp`, `typewriter`, `shelving`, `bay2`, `cabinetA`, `cabinetB`, `cabinetC`, `archive`.

## Slik er rommet bygget

- **Vegger og gulv** beholder web-eksportens geometri (geo 342 og 345) og får nye materialer lagt på etter verdensposisjon: `M_Rec_Block` (malt blokk 40 × 20 cm, fuger i normalkartet, 1,6 m flis) og `M_Rec_Floor` (vinylflis 30 cm, 1,2 m flis). Hvert har et makrokart for rommets egen slitasje (skrubb i stol- og trallehøyde, fett ved bryter og telefon, lekkasjen i sørvest, ganglinjer, nyere fliser). Avbildningen står i hodet på `make_surfaces.py`.
- **Himlingen** (`SM_Rec_Ceiling`) henger på 2,70 m: T-profiler, plater, to 0,6 × 1,2 m armaturer med linse. Lysene beholder web-nodenes drivere (strøm, flimmer, bryter); bare kilden flyttes (`AS47Web::RehouseSaroRecordsLights`, `RehouseSaroRecordsLamp`).
- **Rekvisittene** modelleres i `build_records.py` med ekte avfasninger, prosedyrematerialer over fototeksturer og slitasje fra kanter og hjørner, og bakes til ett atlas per gjenstand (farge, normal, ruhet, metall). Kartonger, permer og mapper deler ett 4K-atlas.
- **Etiketter** er egne kort fra `T_Rec_Labels`, ett per kartong, med navn fra spillets egne arkivetiketter.
- Alt lastes hvis det finnes. Mangler en asset, står den delen av rommet som før (`-S47RecordsRoomOff` gir hele rommet som før).

## Kilder og lisens

- Toms ChatGPT-teksturer (`~/scratch-bm/refs/textures-20261112/saro/`: `block_int`, `vct_floor`, `ceiling_tile`, `galvanized`) er utgangspunkt for flatene. Bildene ligger utenfor Git; de avledede teksturene ligger som uasset i `/Game/RecordsRoom`.
- Poly Haven, CC0: `wood_table_001` (bordplate), `green_metal_rust` (lakkert stål), `rusty_metal_02` (rust). Lastet ned til `<src>/dl/polyhaven`; brukt som underlag i bakte atlas.
- Cosmos (Toms kjøp): `RPS_DeskFan` og `RPS_FireExtinguisher02`. Ligger bare i `Content/Rocket` (git-ignorert), valgt på navn og mål, aldri brukt som referanse for bilde- eller modellverktøy. Uten dem står den gamle vifta og den røde sylinderen.
- Skrifter: prosjektets egne (Special Elite, Reenie Beanie, Oswald). Ingen ekte merker; skrivemaskinen heter «CALDERA 210».

## Hjelpeverktøy

`geo_bounds.py` og `geo_tris.py` leser web-eksportens GLB (grenser, materialer og trekanter per boks) og ble brukt til å finne hva som kan klippes bort av gammel dekor. `preview_lit.py` viser et tekstursett belyst uten motor. `plan_drawing.py` tegner plantegningen.
