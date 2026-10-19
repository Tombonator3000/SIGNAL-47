# Runde 16: malt horisont

Generert med den innebygde `image_gen.imagegen` etter imagegen-skillen. To generatorløp, to valgte paneler. Eksakt modellversjon er ikke eksponert av verktøyet. Leveransen følger «Runde 16: horisonten som malt bakteppe» i `web/ART_BRIEF.md` og arbeidsmåten i `web/ART_ROUND13_15_DELIVERY.md`.

## Leveranse

| Fil | Piksler | Format og fargerom | Filstørrelse |
| --- | --- | --- | ---: |
| [horizon_night.png](horizon_night.png) | 8192×1024 | PNG RGBA  8 bit per kanal  sRGB ICC | 886 969 byte |
| [horizon_dawn.png](horizon_dawn.png) | 8192×1024 | PNG RGBA  8 bit per kanal  sRGB ICC | 1 760 601 byte |

Begge viser det samme omprøvde maleriet og bruker nøyaktig samme alfamaske. Horisontens øyehøyde er y=768. Nord er x=0, øst x=2048, sør x=4096 og vest x=6144. Skalaen er eksakt 8192/360 = 22,7555556 piksler per grad på begge akser. Sola ved 79° tilsvarer x≈1798; ingen sol er malt inn.

## Kilder og prompter

De fire konseptbildene er inspisert som visuell retning: [Roswell-veien](../concept/night_20261018/roswell_road_empty_truck.jpg), [STATION 01](../concept/night_20261018/station01_cut_cable.jpg), [SARO](../concept/night_20261018/saro_peripheral_glimpse.jpg) og [Caliche](../concept/night_20261018/caliche_main_street.jpg). Bare deres uttrykk for det fjerne landet brukes. Ingen konseptpiksler eller forgrunnsobjekter er klippet inn. Referansehashene er i [input-state.json](../production/round16/input-state.json).

Generatororiginalene er bevart i [round16/sources/](../production/round16/sources/). Begge er 2172×724 RGBA. Sluttfilene er resamplet og geometrisk tilpasset til de bestilte 8192×1024, ikke generert native i 8K. Østpanelet ble laget uten vedlagte bilder. Vestpanelet brukte østpanelet som stilreferanse og ny terrengvariant. Begge kall brukte `transparent_background=true`.

| Generatorkilde | Dekning | Opprinnelig størrelse | Byte |
| --- | --- | --- | ---: |
| [east_0_180.png](../production/round16/sources/east_0_180.png) | 0 til 180° | 2172×724 RGBA | 761804 |
| [west_180_360.png](../production/round16/sources/west_180_360.png) | 180 til 360° | 2172×724 RGBA | 765280 |

Alle eksakte prompter og verktøyparametre finnes også i [prompts.json](../production/round16/prompts.json). De to prompterne er gjengitt uendret nederst i dette manifestet. Opphavet er prosjektets egen bildegenerering og bearbeiding; ingen eksterne bilder er lastet ned, og ingen CC0-lisens påstås. Prosjektbiblioteket ble søkt først; treffene gjaldt Scenario-baserte panoramaverktøy og ble ikke gjenbrukt.

## Skjøting, maske og lys

Østpanelet fyller x=0..4095 og vestpanelet x=4096..8191. Native kontur er hentet fra generert alfa, ryddet med et medianfilter og tilpasset briefens retninger og høyder gjennom koordinatprøvetaking. Den høye østbutten er flyttet til 130°. Små åpninger mot nord, sør og soloppgangen går ned til den flate sletta. De bestilte sektorene er målt på sluttmasken.

Skjøtene ligger i de lave ryggene mot nord og sør. 192 piksler på hver side blandes parvis med en jevn vekt; konturen blandes samtidig, slik at den forblir én silhuett. Etter lyssetting og 8-bits avrunding gjennomsnittsberegnes de to sentrale skjøtkolonnene. Første og siste kolonne har dermed identiske RGBA-verdier, og samme kontroll er gjort ved den interne sørskjøten. Den sammenhengende bakken prøvetas fra hele native y=579..723 over slutt-y=768..1023, uten klampet bunntekstur.

[shared_alpha.png](../production/round16/shared_alpha.png) er den felles masken. Den er alfa 0 i lufta, alfa 255 i landet, med høyst to kantpiksler mellom 0 og 255 per kolonne. Alt fra y=768 og ned er helt dekkende. Ingen delvis transparent himmel, dis, skyer eller stjerner er bevart. Ingen piksler har alfa over 0 over y=654. RGB i helt transparent luft er null.

Natt og grålysning er fargekorrigeringer av [det samme nøytrale maleriet](../production/round16/stitched_neutral.png). Natten har nesten svarte, kalde flater og en svak blågrå klippekant. Åtte svake ranchpunkter er lagt inn som 2×2 eller 3×2 pikslers fargekorreksjoner på sletta, uten bygningsformer eller glød. Grålyset har varmt fiolett motlys i øst, blågrått land med svakt rosa klippeflater i vest og en jevn overgang. Ranchpunktene er slukket. Atmosfærefarger er bare lagt på dekkende land.

Full fremgangsmåte, fargeformler og eksportversjoner står i [PROCESSING.md](../production/round16/PROCESSING.md); kontrollpunkter, høydegrenser og lyskoordinater i [processing.json](../production/round16/processing.json). [silhouette_y.csv](../production/round16/silhouette_y.csv) bevarer konturen. PNG-ene har sRGB ICC-profilen som er bevart i produksjonsmappen. Ingen kodefiler er opprettet eller endret av denne leveransen.

## QA

**PASS: 27 av 27 maskinkontroller og to visuelle gjennomganger, inkludert en uavhengig subagent.** [Maskinrapport](../production/round16/verification.json), [uavhengig kontroll](../production/round16/independent_review.json).

| Kontroll | Resultat | Faktisk måling |
| --- | --- | --- |
| Mål, format, modus, fargerom | PASS | Begge 8192×1024 PNG RGBA8, PNG color type 6, sRGB ICC |
| Samme alfamaske | PASS | 0 ulike alfapiksler |
| Gjennomsiktig himmel | PASS | Alfa 0 utenfor konturen; delalfa bare innenfor den 1 til 2 piksler brede kanten |
| Dekkende under horisonten | PASS | Alle piksler fra y=768 har alfa 255 |
| Ingenting over y=608 | PASS | Øverste landpiksel y=654, 46 pikslers margin |
| Sømløs nordwrap | PASS | Maks RGBA-forskjell mellom x=8191 og x=0 er 0; visuell sømkontroll består |
| Intern sørskjøt | PASS | Maks RGBA-forskjell mellom x=4095 og x=4096 er 0; visuell kontroll består |
| Nord og sør | PASS | Bare lav rygg og åpninger, maks 27 piksler over baseline |
| Øst 65 til 95° | PASS | Maks 10 piksler over baseline, under grensen på 11 |
| Isolert butte ved 130° | PASS | 91 piksler, 4,00° |
| Høyeste vestparti | PASS | 114 piksler, ca. 5° |
| Mindre mesaer 310 til 345° | PASS | Maks 45 piksler, 1,98° |
| Nattlys | PASS | 8 isolerte punkter på 2×2 eller 3×2 piksler, 0 i grålysning |
| Samme land og visuell stil | PASS | Samme klipper og slette; kald natt, fiolett gry og svakt rosa vestklipper |
| Forbudte motiver | PASS | Ingen synlig tekst, folk, dyr, bygg, veier, gjerder, master, kjøretøy, logoer eller himmelmotiver |
| Bunntekstur | PASS | Tidlig strekkfeil korrigert; ingen identiske tilstøtende bunnrader |

Tidlige FAIL og forkastet kontrollbilde er bevart med tydelige `initial`, `before` og `rejected`-navn i produksjonsmappen. Alle gjeldende bilder og deres SHA-256 står i maskinrapporten.

## Kontrollbilder og avgrensning

- [Begge bånd over hverandre](../production/round16/contact_sheet.png), 2048×512. Natt øverst, grålysning nederst. Sjakkbrettet finnes bare i kontrollbildet.
- [Natt med nordskjøten i midten](../production/round16/horizon_night_seam_center.png), 8192×1024 RGBA.
- [Grålysning med nordskjøten i midten](../production/round16/horizon_dawn_seam_center.png), 8192×1024 RGBA.
- [Sømdetaljer i 1:1](../production/round16/seam_detail.png). Nord til venstre, sør til høyre; natt øverst.
- [Detaljkontaktark av landet](../production/round16/land_detail_contact.png). Ensfargede kontrollmatter ligger bak alfa; de finnes ikke i assetene.

Runtimeintegrasjon, WebP-kopier, bygg, spilltest og vurdering på fysisk skjerm er **UNVERIFIED** og inngår ikke i denne bildeleveransen. Ingen commit eller push er kjørt.

Agentens skriving i repoet er begrenset til `web/src/assets/art/sky/` og `web/src/assets/art/production/round16/`. `ART_BRIEF.md` var allerede endret ved oppstart og er bevart med samme hash. Andre samtidige kodeendringer dukket opp under arbeidet; de er dokumentert separat i maskinrapporten og er ikke skrevet eller tilbakeført av denne leveransen. Derfor er samlet Git-status ikke et rent skrivebevis for denne økten.

## Eksakte generatorprompter

### east_0_180

```text
Use case: stylized-concept.
Asset type: hand-painted distant horizon terrain cutout for a 360-degree cylindrical backdrop in SIGNAL / 47.
Create EASTERN HALF panel A, azimuth 0 through 180 degrees, north at the left, east at the middle, south at the right. Very wide landscape 4:1 canvas, ideally 4096x1024. This is a landscape matte painting of ONLY the far New Mexico desert and mesas, on an actually transparent background. Not a landscape poster, not a photograph with a sky.
Style: restrained cinematic painted realism matching the dark distant flat-topped mesas in the inspected SIGNAL / 47 concepts: deep indigo/charcoal blue-grey strata, eroded vertical caprock, stepped ledges and talus fans. A desolate flat high desert plain, subtle fine granular brushwork, distant atmosphere only within the opaque land. Pre-dawn neutral blue ambient lighting with readable but very subdued stone texture, no direct sunlight.
Projection and framing: eye-level panorama without lens perspective, level flat desert/sky baseline at 75 percent of image height. Bottom quarter is quiet flat distant plain all the way to the edges, progressively a little darker and rougher below. All sky above the terrain must be fully transparent. No painted haze above the silhouette. Land is opaque and edge is clean.
Exact terrain left to right across the image: first 0–15 degrees only a very low far ridge, almost level plain, with gaps; 15–65 a long low mesa range, horizontal flat tops with caprock and eroded ledges, mostly 1.5–2.5 degrees tall; 65–95, centered slightly left of the canvas middle, very broad low unobstructed sunrise gap, only open plain and a tiny ridge less than 0.5 degree tall; 95–165 scattered irregular flat-topped buttes and a broken mesa 1–3 degrees tall, one isolated slender 4-degree butte at 130 degrees (72 percent of width); 165–180 very low ridge/open plain again. At this aspect ratio 1 degree is about 23 px if 4096 wide. Most silhouettes should extend only 25–65 pixels above the 768-pixel baseline, tallest butte about90 pixels, absolutely no terrain higher than 608. Keep relief very low in the frame, huge transparent upper area; this is distant land, no foreground rocks, no dramatic mountains.
At both outer edges land returns to the same perfectly level very low ridge, so panels can be joined. Thin far blue-grey ridge behind front mesas. Lots of quiet open space. No ridge taller than the front mesas.
Strictly exclude ALL text, watermarks, logos, people, animals, aliens, buildings, houses, roads, paths, tracks, fences, poles, antennae, towers, vehicles, plants protruding above horizon, sun, moon, stars, clouds, sky, sky gradients. No lights yet. Only natural rock and flat distant desert terrain.
```

### west_180_360

```text
Use case: stylized-concept. Edit/composition variant of the supplied terrain panel, a companion WESTERN HALF (panel B) of the same 360-degree hand-painted New Mexico desert horizon. The supplied panel is the eastern half and is a style/terrain-material continuity reference; generate new western terrain, do not repeat its mesa layout.
Match the supplied image's panoramic 3:1 format (2048x683), same eye-level desert baseline at y=542, same subtle cinematic painterly realistic rock textures, eroded caprock, talus fans, deep blue-grey dawn ambient land, same texture scale and same distant quiet flat plain underneath. ONLY terrain; actual transparent background above it, no opaque black sky. Huge empty transparent upper area; all relief remains in a very thin low horizontal band, not towering peaks. Keep all bottom terrain below y542 as continuous subtle distant flat high plain with no foreground objects, darkening and roughening gradually downwards. Same subtly cool illumination as the supplied panel; final night/dawn color grading will be done later.
The left edge is south (180 degrees), the middle is west (270 degrees), the right edge is north (360 degrees).
Exact composition, map left to right: 180–195 degrees, very low far ridge and open plain; 195–250 degrees a broad long flat-topped mesa in two clear eroded steps with vertical caprock bands and long talus aprons, 2–3 degrees high; 250–310 degrees the highest region, a long irregular flat-topped mesa wall with erosion ledges, smaller isolated flat-topped buttes in front, 3–5 degrees high, visibly taller than the eastern reference but still very low in this enormous transparent frame; 310–345 degrees, diminishing separated mesas 1–2 degrees high stepping down into the far ridge; 345–360 degrees only the very low far blue-grey ridge with gaps to the flat desert baseline. In this2048px-wide180degree panel, 1degree is11.38pixels, so highest western mesa is only57pixels above y542. Most are25–35px. Nothing above y465. Front mesas darker and sharper, the far ridge lighter and bluer. Both outer edges should match the very low level ridge at the edges of the supplied panel. No ornamental giant central peak.
No text, watermarks, logos, people, animals, alien, buildings, roads, tracks, paths, fences, poles, antennae, towers, vehicles, bushes or trees. No lights. No sun, moon, stars, clouds, painted sky, sky gradient or haze in the transparent air. Only natural terrain cutout with genuine alpha.
```
