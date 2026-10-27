# Sierra Motor Court — produksjonskilde for Unreal

Denne pakken inneholder den originale motellgeometrien til SIGNAL / 47 og flyttbare PBR-materialer klargjort fra den lokale V2-modellen i Blender. Den omfatter eksteriør, resepsjon, flyttet basseng, parkering, originalt skilt og et eget lag med mørkt valnøttpanel til det eksisterende interiøret i rom 6. Spillets eksisterende biler, KELVORA-TV-er, bevis, telefonrør, nøkler, dører, seng og figurer beholdes som ressurser i spillet.

Godkjenning i Unreal dokumenteres i rapportene fra import og verifikasjon. En fullført Blender-eksport er ikke alene en godkjenning i Unreal. Statusen i `manifest.json` beholder med hensikt `NATIVE_UNVERIFIED` til de separate kontrollene i Unreal er fullført.

## Pakkens innhold

| Fil | Formål |
| --- | --- |
| `Sierra_Production.blend` | Redigerbar LOD0-geometri, pakkede og flyttbare teksturer, separate scener for uteområdet og rom 6. |
| `exports/SM_Sierra_*.fbx` | 24 logiske deler, hver med filer for LOD0, `_LOD1` og `_LOD2`. |
| `exports/axis_probe.fbx` | Asymmetrisk testgeometri for kontroll av importaksene. |
| `textures/` | Grunnfarge, pakket ORM og DirectX-tangentnormaler; repeterende original asfalttekstur. |
| `manifest.json` | Delenavn, avgrensning og trekanttall per LOD, materialplasser, rottransformasjoner, kildehash og FBX-hasher. |
| `materials.json` | Flyttbare materialdefinisjoner for Unreal, relative teksturstier og fargeromskonvensjoner. |
| `selection.json` | Oversikt over inkluderte og utelatte objekter fra modellarbeidet, samt hash for den opprinnelige lokale kilden. |
| `export_sierra.py` | Uttrekk fra modellkilden, baking av materialer og klargjøring av LOD og FBX. |
| `CREDITS.md`, `provenance.json` | Inkluderte materialkilder og uttrykkelige utelatelser. |

I det lokale arbeidsområdet ligger genererte filer under `output/`. Ved klargjøring til `unreal/ArtSource/MotelV2` plasseres *innholdet* i `output/` i pakkeroten, ved siden av denne README-filen, kreditering, opphavsoversikt og eksportør. Kopier først når den koordinerende oppgaven har godkjent hele eksporten og låst hashene som brukes av verifikasjonen i Unreal. Ikke kopier midlertidige mapper fra forhåndstester, `.blend1`-sikkerhetskopier, den ubehandlede V2-scenen, rendringer med lisensierte biler, hurtigbuffere eller tekstur-/HDRI-mapper fra modellarbeidet.

## Koordinater og eierskap under kjøring

Geometrien er modellert i meter, med Blender Z opp. Eksportinnstillingene bruker fremoverretning `-Y`, opp `Z`, `FBX_SCALE_UNITS` og anvendt enhetsskala. Den utprøvde Unreal-oppskriften bruker `force_front_x_axis=True`, `MATCH_UP_FORWARD_AXES` og 90 graders yaw ved import. Importert geometri følger derfor `UE_cm = 100 * (Bx, -By, Bz)`.

Hver del bruker rotpunktet fra manifestet:

- Deler av uteområdet: `world.motel.group`, Three-rotpunkt `[-45.9, -2.0985683853163195, 674.9]`.
- `SM_Sierra_Room6Lining`: `world.room6.group`, Three-rotpunkt `[0, 0, -8000]`.
- Generell plassering: `worldThree = (rootX + Bx, rootY + Bz, rootZ - By)`.

Panelet til rom 6 må ikke plasseres ved uteområdets rotpunkt. Panelene bevarer de opprinnelige åpningene for inngang, vindu og bad. Eksisterende møbler, gulvlister, interaksjonsankre og kollisjonsgeometri for spillmekanikken i rom 6 beholdes på plass.

Bassenggeometrien roteres 180 grader rundt Blender Z og forskyves med `(11, -29, 0)`. Hoveddekket ligger nå innenfor `x[-19,-8], y[-8,9]`. Nye gangveier i nord og vest forbinder resepsjonssiden med bassengporten. Uteområdet slutter ved `x=15.7`; den gamle kryssingen til bassenget i øst, kantstein, grusbed og feilplassert beplantning er utelatt. Spillet må lage den tilsvarende åpningen i terrenget; modellen erstatter ikke verdens terreng eller vei.

Resepsjonsdiskens topplate passer til de eksisterende dokumentankrene: sentrum `(-0.8,-0.35,0.985)`, dimensjoner `(1.16,3.34,0.07)`, topp `1.020m`. Den opprinnelige statiske nøkkeltavlen og veggtelefonens base gjenskapes ved de etablerte ankrene; nøkkel 6 og telefonrøret forblir interaktive. Den eksisterende TV-støtten heves med `0.053m`. Geometrien til den lukkede ytterdøren for rom 6 og den ugjennomsiktige vindusbakgrunnen fra studiescenen er utelatt.

GlowOffice, GlowExterior, GlowRoom06 og GlowSign er selvstendige deler/materialer som kan knyttes til lysets tilstand i spillet. Dekorativ geometri gir ikke ny adgang og erstatter ikke kollisjonsreglene for spillmekanikken.

## Materialkontrakt

Hver del bruker én materialplass av typen `M_MotelV2_*`. Prosedyrematerialene fra kilden bakes før de opprinnelige shader-grafene fjernes. Materialtilordningene på polygonene bevares når materialene kopieres til baking.

- Grunnfarge: sRGB PNG, fargebaking uten belysning; ingen innbakt scenebelysning.
- ORM: lineær RGB = ambient occlusion 1, roughness, metallic. AO er med hensikt nøytral, ikke beregnet ved baking av okklusjon.
- Normal: lineært DirectX-kart i tangentrom, bakt med negativ grønn/Y. Unreal skal **ikke** invertere grønnkanalen. Den pakkede Blender-grafen for kontrollvisning inverterer grønt for OpenGL-normalnoden sin.
- Asfalt: originalt repeterende bilde med fysisk UV-skala `0.42` repetisjoner per meter og `texture_tint_linear=[0.24,0.25,0.25,1]`.
- Glass og bassengvann: uttrykkelige innstillinger for gjennomskinnelighet i `materials.json`; kildens lystransmisjon overføres ikke automatisk med FBX.

Standardoppløsningen for teksturatlas er 2048, med RoofShell på 4096 og bassengvann på 1024. LOD1 og LOD2 sikter mot henholdsvis 50 % og 22 % av kildens trekanttall. De faktiske tallene og avgrensningene i manifestet er fasiten.

## Eksport fra modellkilden

Den rensede `.blend`-filen, FBX-filene og de relative teksturstiene kan flyttes og brukes til redigering/import. Å gjenskape uttrekket fra den ubehandlede V2-scenen er en separat lokal arbeidsflyt: denne opprinnelige `.blend`-filen er med hensikt utelatt fra pakken fordi den også inneholder tre CC-BY-bilmodeller og en HDRI for presentasjon.

Den nåværende eksportøren godtar en uttrykkelig `--source`; standardverdien er den lokale nabofilen `test-v2/Sierra_Motor_Court_Test_V2_Kelvora.blend`. Asfaltkilden er for øyeblikket den lokale repositoriestien `/home/tom/SIGNAL-47/web/src/assets/art/road/tex_oldroad_asphalt.jpg`. Dette er dokumenterte avhengigheter for modellarbeidet, ikke teksturavhengigheter under kjøring av spillet. Flytting av arbeidsflyten som bygger på nytt fra kilden krever at originalkilden er tilgjengelig og at asfaltstien løses; dette kreves ikke for å åpne den rensede produksjonsmodellen.

Kjør under prosjektets eksklusive DCC-lås, sekvensielt, på CPU med fire tråder:

```bash
blender --background --threads 4 --python-exit-code 1 \
  --python export_sierra.py -- \
  --source /path/to/local/Sierra_Motor_Court_Test_V2_Kelvora.blend \
  --out /path/to/production-output
```

`--inventory` skriver utvalgsdata uten baking/eksport. `--no-bake` er en geometridiagnose og gir et uttrykkelig uverifisert resultat med ufullstendige materialer. Bruk en separat `--out` til diagnostikk. `--select SM_Sierra_Planting,SM_Sierra_CourtDetail` oppdaterer utvalgte deler og beholder uendrede LOD0-objekter fra den forrige fullførte produksjonsfilen i `.blend`-format; den komplette tidligere `.blend`-filen kreves når manifestet inneholder uendrede deler.

En liten forhåndstest av materialene kan bruke en ny utdatamappe med `--select SM_Sierra_Office --resolution 1024`. Kontroller grunnfarge-, ORM- og normalbildene, og åpne/lagre deretter produksjonsmodellen før hele eksporten kjøres. Kilden og den genererte modellen er separate filer. Ved avslutning av eksporten fjernes ubrukte scener fra modellarbeidet, biler, TV, HDRI, materialer og bilder før vanlig lagring av Blender-hovedfilen.

## Grenser for verifikasjon og opphavsoversikt

Den godkjente sluttpakken bør inneholde den koordinerende oppgavens låste filhasher og verifikasjonsrapport for import, akser, materialer og LOD i Unreal. Eksportøren tester ikke selv interaksjon i Unreal, terrengåpninger, strømming, lystilstand, kollisjon eller spillmekanikk i en pakket utgave. En vellykket forhåndstest fastslår bare at materialbaking og lagring av den rensede modellen fungerte for delen som ble testet.

Produksjonsfilen i `.blend`-format inneholder sammenslåtte, triangulerte logiske deler og flyttbare materialer. Geometrien kan redigeres, men filen bevarer ikke hvert opprinnelige modellobjekt, alle modifikatorstabler eller prosedyregrafer fra kilden. Den utelatte lokale V2-scenen forblir modellkilden på et høyere nivå. Det gjøres ingen påstand om eierskap eller CC0-lisens for de utelatte bilmodellene eller referansefotografiene.
