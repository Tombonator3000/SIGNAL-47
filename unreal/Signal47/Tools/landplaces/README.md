# Tools/landplaces: stedene ute på landet som egne modeller (LAND-PLACES-20261114)

**Status:** Under arbeid (vindmølla ferdig; linjehytta, gruva og cairnet gjenstår) · **Gjelder:** Unreal-utgaven, `/Game/LandPlaces`, `S47LandWindmill.cpp` · **Sist kontrollert:** 14. november 2026

De fire stedene Tom bestilte modellert etter referansepakker (`~/scratch-bm/refs/land-places-20261113/`, utenfor Git): vindmølla på Baca-leasen (også gårdens), linjehytta, Esperanza-gruva og cairnet. Regelen for viktige gjenstander gjelder: referansepakke, én gjenstand om gangen, ekte form, bakte kart, kritikk i spillet. Alt her er eget arbeid eller CC0. Designnotat med mål og før/etter: [Docs/Design/World/LandPlaces](../../../../Docs/Design/World/LandPlaces/README.md).

## Løypa

Alle tunge steg går gjennom køen (`python3 ~/.claude/skills/signal47-nattskift/scripts/q.py ...`), ett om gangen, fra `unreal/Signal47`.

| Steg | Kommando | Lager |
|---|---|---|
| Mål og trekanter | `python3 Tools/landplaces/windmill/build_windmill.py --dry` | tall på skjermen, ingen Blender |
| Etiketter | `python3 Tools/landplaces/windmill/make_labels.py <src>/tex` | `T_LP_MillVaneLabel.png`, `T_LP_MillPage.png` |
| Grå leire | `python3 Tools/landplaces/run.py blender windmill clay` | fire rette vinkler og tre skrå i `<work>/Preview` |
| Bake, eksport, bilde | `python3 Tools/landplaces/run.py blender windmill bake export look` | tre atlas (farge, normal, ORM), FBX per del og LOD, `contract.json` |
| Import | `python3 Tools/landplaces/run.py import windmill` | `/Game/LandPlaces/Windmill` (teksturer, materialer, mesh med LOD) |
| Bilder i spillet | `python3 play39/run_pic.py <navn> -S47FreeformTest -S47LandPlaceShotTest=windmill -S47ShotDir=<mappe>` | 6 m, 40 m, 300 m, 1 km og nærbilder, dag, skumring og natt; `-S47LandMillOff` gir stedet som før |

`<src>` er `S47_LP_SRC` (standard `~/scratch-bm/land-places-20261114/windmill`), `<work>` er `S47_LP_WORK` (standard `<src>/work`).

## Slik er settet bygget

- **`lpkit.py`** er ren Python uten Blender: en modell er deler, en del har ett mesh per detaljnivå, og hvert mesh er satt sammen av tre primitiver (`sweep`: et tverrsnitt langs en bane, `revolve`: en profil rundt en akse, `sheet`: en tynn plate). Hver primitiv ber om sine egne UV-felt i delens atlas etter navn. Samme primitiv på lavere nivå havner derfor på de samme tekslene, og alle LOD-er bruker det ene bakte kartsettet. Avfasninger ligger i tverrsnittene (vinkeljern, flattjern, planker), ikke i en modifikator.
- **`lpbake.py`** er Blender-siden: mesh fra settet, flater av noder over fototeksturer (galvanisert, støpejern, tre, betong), baking og eksport. Flatene leser punktets virkelige plass fra attributtet `rest`, så delene kan flyttes fra hverandre under bakingen: hver del får bare skygge fra seg selv, siden den skal bevege seg i spillet. Rust kommer i sprekker og under ting regnet renner av; `stain` og `tone` er egne attributter per punkt.
- **`import_assets.py`** bruker prosjektets målte FBX-oppskrift (`Tools/fieldintegration/import_recipe.py`), setter LOD-ene inn i samme ramme, regner skjermstørrelser fra hver dels egen radius for 40, 120 og 350 m, slår av Nanite og lar spor gå mot et lavt nivås egne trekanter.
- **Alt lastes hvis det finnes.** Mangler en mesh, står stedet som før (`-S47LandMillOff` gir det samme med vilje).

## Vindmølla: deler og hva som driver dem

| Del | Origo | Drives av |
|---|---|---|
| Tower, Pier (fire) | tårnets midte på bakken | står; tårnet settes vannrett på høyeste pirs bakke |
| Head (girkasse, stjert, vinge) | navhøyde på tårnaksen | gir om z: gården mot vinden, Baca der den er surret |
| Wheel | navet | spinn om y: gården med vinden, Baca bare under nattas banking |
| Rod | tårnets midte | slag langs z, ett per omdreining |
| Housing, Lid | bakken / hengsla | lokket åpnes om x av spilleren |
| Tank, Water, Float | tankens midte / ventilen | flottøren vipper så vidt |
| Page, VaneLabel, WireRun, WireKnot | | Reyes-siden i posen, «BACA LEASE NO. 2», den blanke ståltråden |

## Kilder og lisens

- Toms ChatGPT-tekstur `~/scratch-bm/refs/textures-20261112/saro/galvanized.png` (sinkblomst) er underlag for galvanisert stål. Bildet ligger utenfor Git; de bakte kartene ligger som uasset.
- Poly Haven, CC0: `rusty_metal_02` (rust), `rusty_metal_05` (støpejern), `wood_planks_grey` (plattform), `rough_concrete` og `dirty_concrete` (pirer og såle). Lastet ned til `<src>/dl/polyhaven`; brukt som underlag i bakte atlas.
- Skrifter: prosjektets egne (Oswald, Reenie Beanie). Ingen ekte merker; vingen sier «BACA LEASE NO. 2», nesten borte.
- Referansepakken er laget med ChatGPTs bildeverktøy etter Toms brief og er bare brukt som forelegg for form og farge. Ingen Cosmos-deler.
