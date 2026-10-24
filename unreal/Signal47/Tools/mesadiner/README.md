# Mesa Diner: originale redigerbare modeller

Fullt diner-kit for Caliche, New Mexico, 1986. Geometri og PBR er laget fra prosjektets eget Mesa-konsept: slitt lys stucco/rød stripe, teal fliser, dype frontvinduer og persienner, burgunder booths med piping, kromstoler og Formica, faktisk servicehatch/kjøkken, kaffebrygger, dreide opalpendanter, telefonbase og buet jukebox. `exterior_geometry.py` og `make_exterior_textures.py` leverer original lastebil, parkering, benk og neonskilt fra samme modellteam.

Ingen eksterne modell- eller bildefiler. Meshoppskriften gjenbruker bare prosjektets egen tidligere verifiserte `../substation/meshkit.py`, via `geometry.py`/`meshkit.py`. Biblioteksøk etter diner/Blender/Unreal ga ingen testet dinerressurs; den historiske MCP-referansen brukes ikke som modell eller avhengighet.

## Låst koordinat- og interaksjonskontrakt

Blender-meter, Z opp, fasade mot **+X**. Root Three `[7979.95, 4.309006389843889, 1999.95]`, yaw 0. Kartlegging: `worldThree=(rootX+Bx,rootY+Bz,rootZ−By)`. Bygningsfot Bx `[-8,2]`, By `[-9,9]`, gulv Bz 0, takplate topp 3.4 m. Den eksisterende døråpningen By ±0.6 er fysisk fri.

| Del | Site-offset meter | Utførelse |
|---|---|---|
| SM_Mesa_Shell | 0,0,0 | Åpen vindus-/dørgeometri, stucco, fliser, kornis, takventiler/HVAC |
| SM_Mesa_Glass | 0,0,0 | Separate tynne fasaderuter |
| SM_Mesa_Door | 1.9,−0.58,0 | Normalisert hengsel; −105° Z er bakt i vertskoordinatene, runtime yaw 0 |
| SM_Mesa_Interior | 0,0,0 | Checker-gulv, endeverger, wainscot, himlingsraster, lukkede bakdører |
| SM_Mesa_Booths | 0,0,0 | Fire opprinnelige møbelgrupper med avrundet polstring, piping og bordsetting |
| SM_Mesa_Counter | 0,0,0 | Disk, syv kromstoler, fotstang, brygger og pastry display |
| SM_Mesa_Kitchen | 0,0,0 | Opprinnelig lukket kjøkken bak faktisk hatch, range, hood og tallerkenhyller |
| SM_Mesa_Fixtures | 0,0,0 | Persienner, lys, klokke, telefonbase/ledning, jukebox og menyer |

Utendørsdelene `SM_Mesa_Truck`, `SM_Mesa_Sign` og `SM_Mesa_Parking` bruker også site-offset null. Felles prefiks er `SM_Mesa_`/`M_Mesa_`; native namespace `/Game/MesaDiner`.

Eksisterende runtimeobjekter `coffeePot`, `cup`, `regularCup`, `clipping`, `payphoneHandset`, waitress/driver og deres hoder blir ikke duplisert. Den originale foto-/historieteksten beholdes av runtime. Benkens takeawaypunkt B `[2.43,−5,0.46]`, diskpunktet `[-4.5,−1.65,1.0]`, meny-/telefon-/jukeboxproksyer og eksisterende booth-/stolcollidere er autoritative. Diskens reelle topp er 1.000 m; bordtopp er 0.8025 m. Kundeaisle Bx `[-3.12,−0.27]` er fri for nye møbler. Kjøkkenet holder seg bak opprinnelig x−5.7-partisjon.

Døren står permanent åpen; ingen ny mekanikk innføres. Eksakt åpnet mesh-bound står i `model_contract.json` etter bygg. Alle andre statiske modeller er visuelle og følger opprinnelige gangsoner/kollisjoner. Root/runtimeagent eier trim av gamle synlige flater og atomisk aktivering.

## Original PBR og lagret kilde

`make_textures.py` lager åtte egne PBR-sett: Stucco, PaintRed, TealTile, CheckerFloor, Vinyl, Formica, Wood og et lesbart originalt 1986-meny/skiltatlas. Albedo er 2048 px, normal/roughness 1024 px. Normaler følger DirectX; Blender-materialet vender grønn kanal for visning. `materials.py` er grunnlisten; utendørsforfatterens materialrader overstyrer bare egne etiketter i den samlede `materials.json`. Glass har separat translucent materiale med alpha 0.075. Lysmaterialene er `M_Mesa_Fluorescent` og `M_Mesa_PendantGlow`; neonskiltets røde/cyan emisjon styres separat av runtime.

`build_diner.py` skriver `Mesa_Diner.blend`, samlet `Mesa_Diner.glb`, tre FBX-LOD-er per del og en GLB per LOD0-del. Alle bilder pakkes med nøyaktige originalbytes. Hver del har komponentvertexgrupper og metadata; skjult `EDITABLE_ORIGINAL_DINER` inneholder de redigerbare originaldelene. Eksportene normaliseres ved pivot null, med site-offset påført én gang i samlet scene. `axis_probe.fbx` har asymmetrisk senter `(2,4,6)` m og størrelse `(0.2,0.4,0.6)` m. Den tidligere målte native FBX-policyen gir UE `(100x,−100y,100z)`.

## Generering og status

Bare forelderens samordnede tunge slot kan starte teksturer/Blender. Ingen UE eller render startes av disse kildeskriptene:

```bash
# Fra Tools/mesadiner, med prosjektets Pillow/NumPy-runtime:
python3 make_textures.py
blender -b --threads 2 --python-exit-code 1 --python build_diner.py
blender -b --threads 2 --python-exit-code 1 --python verify_source.py
```

Kilde-QA før første Blender-kjøring: åtte bygnings-/interiørdeler har 23 932 / 15 008 / 10 536 trekanter; alle tre LOD-er har ikke-kollapsede UV-trekanter og minste LOD0-triangelareal 7.665e−6 m². Med de tre ferdige utendørsdelene blir samlet kildebudsjett **84 923 / 45 493 / 23 452 trekanter**, 11 deler. Alle kildeflater har definerte materialer og ikke-kollapsede UV-triangler. Disse lette kildetallene erstatter ikke faktisk eksportkontroll. `model_contract.json`, `verification.json` og forelderens native/visuelle rapport er autoritative etter kjøring. Native visuell status er foreløpig **UNVERIFIED**.


## R2: utvendig konseptiterasjon

R2-kilden har et sammenhengende kremfarget mineralbånd opp til 3.58 m, med en 20 cm rød stripe ved 3.00–3.20 m. Dør-/vindusåpninger og den opprinnelige bygningsfoten beholdes. Fasaden har 28 cm dype aluminiumprofiler, et eget transomfelt, slitt terskel og individuelle bevel-fliser på den utvendige brystningen i LOD0/1. Bare den separate fasaderuten bruker varm `M_Mesa_FacadeGlass`; truck-/dørglass er uendret.

Stucco og rød maling får en selvstendig, deterministisk oppskrift med tynne regnspor og sparsom avskalling. De seks beholdte innvendige teksturfamiliene sammenlignes byte for byte i `facade_texture_proof.json` ved faktisk kjøring. `facade_source_proof.json` viser identisk geometri/UV/materialbinding for alle seks beskyttede deler i tre LOD-er (inkludert den åpne døren). Opprinnelige kilder/materialer/UV-bevis er bevart i `evidence/r1-before-facade/`.

Samlet lett kontrollert R2-kildebudsjett med radioagentens nye skilt-/parkeringstillegg er 110 785 / 54 454 / 25 424 trekanter. R2 faktisk eksport er **320 kontroller PASS**, med 110 779 / 54 448 / 25 418 eksporterte trekanter. Alle 18 beholdte innvendige PNG-filer er faktisk byteidentiske. Parkerings-GLB har verifisert `MASK`/cutoff 0.333 for både Asphalt og DryFoliage; planter er tosidige. Native visual er fortsatt **UNVERIFIED** inntil forelderens spillreview. Maskede plantekart og parkeringsalpha følger eksplisitt teksturalpha → clip(.333) → BSDFAlpha i Blender; native bruker sin dithered-mask-støtte for parkeringskanten.
