# Statiske Tripo-figurer

Bruk den aktive bildereviderte listen i Toms private modellmappe. `prepare.py`
leser de originale riggede FBX-ene og baker egne statiske kopier med teksturer.
Originalene endres ikke. Menneskene står med senkede armer; radio-kvinnen,
Nora og diner-patronen får en bakt sittepositur. Ku og Crawler beholder kildeposituren.

Kjør Blender-klargjøringen og deretter `import_assets.py` i Unreal gjennom
prosjektets eksisterende guard. Begge er en samlet jobb for alle 14 figurer.
Ingen løpende skjelett- eller animasjonsberegning kreves av disse modellene.

```sh
blender -b -t 2 --python Tools/triponpcs/prepare.py
```

Kildekatalogen kan settes med `S47_TRIPO_SOURCE_ROOT`; privat resultatkatalog
med `S47_TRIPO_STATIC_DIR`. Standardresultatet ligger ved de originale
modellene i `unreal-static-2026-10-30`. Blender kan avgrenses med
`-- --only dale`; importen kan avgrenses med `S47_TRIPO_ONLY=dale`.

`manifest.json` beskriver hver rolle, FBX, teksturer, mål, positur og
`seat_z_m` for sittende figurer. Lokalt Blender-rom har meter, Z opp og
forover mot minus Y. Den eksisterende FBX-oppskriften gir centimeter,
Z opp og forover mot pluss Y i Unreal, altså pluss Z i spillrommet.
Pivoten ligger ved gulvet under bekkenet. Sittehøyden er bekkenets høyde over
denne pivoten og gjør plassering mot eksisterende benk/stol mulig.

Unreal-modellene lagres som `/Game/Licensed/TripoNPC/SM_Tripo_<role>` med
tre LOD-er, materialer og uten egen kollisjon. Eksisterende spillproxyer
og hendelser eier interaksjonene. Kilder, rigg, teksturer, Blender-filer og
genererte native assets blir utenfor offentlig Git.

Løypa bygger på prosjektets testede `fieldintegration/import_recipe.py` og
`interiors/prepare_dale_prop.py`. Prosjektbibliotekets Blender- og
Tripo-referanser er bakgrunnsstoff og er merket UNVERIFIED; ingen ny addon
eller ekstern MCP-integrasjon installeres.

## Midlertidige plasseringer

`S47TripoNpcs` legger bare til visuelle statiske modeller. Den opprinnelige
spillogikken bestemmer fortsatt tilstedeværelse, dialog og hendelser.

| Modell | Plassering |
| --- | --- |
| Dale | Innenfor kontordøren, dekor uten ny dialog |
| Diner-patron | Den eksisterende sjåførens krakk |
| Servitrise | Reserve i staffgangen; MetaHuman ved disken beholdes |
| Nora | Eksisterende stol i rom 6 |
| Bensinstasjonsmann | Bak nattvinduet |
| Deputy | Sheriffkontoret og eksisterende SC03-figur |
| Earl | Eksisterende studioinngang, følger arbeidstiden |
| Radio-kvinne | Eksisterende benk, med radio |
| Souvenir-mann | Eksisterende røykende mann på trappen |
| Mara Ortiz | Følger SC03-kroppen og dens synlighet |
| Hereford 25 | Den tilsvarende kua i den eksisterende flokken |
| Skyggefiguren | Eksisterende glimtfigur og forsvinning |
| Tall One og Crawler | To eksisterende figurer i SC03 |

Mara, Tall One og Crawler bindes når SC03 oppretter deres vanlige sceneankre.
De legger ikke til nye møter tidligere i spillet. SC02s eksisterende
AI-kropp beholdes fordi dens observerbarhet inngår i spillreglene.

`-S47TripoNpcsOff` gir sammenligningsbilder med de gamle figurene.
`-S47TripoNpcsTest` gjør én kort kontroll av alle 14 faktiske native modeller,
materialer, størrelser og kollisjon, og logger separat antallet sceneankre
som finnes akkurat da. Kameraer og røyktester bruker en isolert UserDir.

## Liten Roswell-alien for Glimpse

`prepare_roswell.py` klargjør en separat, faktisk rigget Tripo-GLB/FBX.
Oppgi originalfil med `--source` eller `S47_ROSWELL_SOURCE`, og privat
resultatmappe med `--output` eller `S47_ROSWELL_DIR`. Standardresultatet er
`modeller/roswell-glimpse-20261031/prepared` i Toms private modellmappe.
Originalkilden bevares. Kopien normaliseres til 1,20 meter, gulvpivot og
samme akser som den eksisterende Pale-importen. `.blend` beholder rigg,
skin, UV-er og pakkede kildebilder; FBX LOD0/1/2 har identisk skjelett og
koordinatramme. PBR-kart eksporteres til Unreal, med DirectX-normaler bare
i UE-kopien. Ukjente bindeledd krever en eksplisitt JSON `--bone-map`;
skriptet lager ikke en erstatningsfigur eller en falsk rigg.

`import_roswell.py` leser samme resultatmappe og oppretter kun
`/Game/Licensed/TripoNPC/Roswell/SK_Tripo_Roswell`, tilhørende materialer,
teksturer, skjelett og tre importerte LOD-er. Den høye Pale-modellen og
de statiske menneskene røres ikke. Kjør begge verktøyene sekvensielt via
prosjektets guard. Skripter og import er ikke i seg selv bevis på sett
bevegelse eller fungerende Glimpse i spillet; dette prøves i native runtime.

Unreal-importen krever `-EnablePlugins=GeometryScripting` og legacy-FBX for
alle LOD-er: `-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0`.
Skriptet setter og leser også det samme flagget i importprosessen. Eksplisitt
enhetskonvertering bevarer meter til centimeter. Hver faktisk render-LOD
kontrolleres mot sine egne Blender-grenser og ni asymmetriske vertexpunkter,
slik at et riktig felles asset-bound ikke skjuler feilvendt/mikroskopisk LOD.
