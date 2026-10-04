# ART_BRIEF.md: grafikk til SIGNAL / 47 (for ChatGPT)

Du lager 2D-grafikk. Claude bygger all 3D-geometri i kode og legger bildene dine på den. Følg navn, størrelse og format nøyaktig, ellers må filene gjøres om.

## Stil

- Referanse: de fem bildene i prosjektet (motellskiltet ved arrayet, kontrollrommet, skrivebordet med feltkamera, servicegården ved S-03, konsollen med SIGNAL LOCK).
- New Mexico, 1986. Natt. Stilisert, ikke fotorealistisk. Litt slitt, ekte materialer, ingen glanset sci-fi.
- Palett:
  - natthimmel #05070d og #0c1220
  - natriumoransje #ffb052, dempet #a8641f
  - CRT-grønn #8cffa4, mørk #2f8a49
  - papir #ebe3cf
  - neonrød #ff5a45, neoncyan #50f0d8
- Ingen ekte merkenavn, logoer eller personer. Ingen tekst i bildet utenom den som står i bestillingen. Hvis teksten blir feil, lever bildet uten tekst. Claude legger da teksten på i kode.

## Leveranse

- sRGB. PNG for alt med gjennomsiktighet, ellers JPG med kvalitet rundt 85.
- Teksturer som skal gjentas må være sømløse. Test ved å legge fire kopier i et 2×2-rutenett, og sjekk at ingen kanter synes.
- Ett bilde per fil, med nøyaktig filnavn. Tom legger filene i `web/src/assets/art/<kategori>/`.

## Runde 1: prologen (kontrollrommet og utsikten)

| Fil | Størrelse | Hva |
|---|---|---|
| `sky/sky_milkyway_equirect.jpg` | 4096×2048 | Ekvirektangulær natthimmel. Melkeveisbåndet går diagonalt. Horisonten ligger på midten, og nedre halvdel er nesten svart. Ingen bakke, fjell eller tekst. |
| `room/tex_floor_hextile.jpg` | 1024×1024, sømløs | 1980-talls sekskantede vinylfliser i varm brun og oker, slitt i fugene, sett rett ovenfra. |
| `room/tex_ceiling_tile.jpg` | 512×512, sømløs | Lydabsorberende takplate, offwhite med prikkmønster, én hel plate med smal skygge i kanten. |
| `room/tex_wall_paint.jpg` | 1024×1024, sømløs | Malt lettbetongblokk i blågrå, matt og litt skitten. |
| `room/tex_desk_laminate.jpg` | 1024×1024, sømløs | Grå kontorlaminat med svake riper og kaffeflekker. |
| `room/poster_listen.png` | 768×1152 | Plakat med teksten LISTEN, RECORD, ANALYZE, UNDERSTAND på fire linjer og en enkel antennesilhuett. Mørk bakgrunn, sliten papirkant. |
| `room/poster_saro.png` | 768×1152 | Plakat med SARO-emblemet og teksten SOUTHWEST ASTRONOMICAL RESEARCH OBSERVATORY. |
| `room/map_new_mexico.png` | 1536×1024 | Gammelt veikart over New Mexico. Et rødt kryss på slettene øst i staten, og en håndtegnet ring rundt Roswell med et spørsmålstegn. |
| `brand/logo_saro.png` | 1024×1024, gjennomsiktig | SARO-emblem: parabolantenne med en bane rundt, enkel strek. Brukes på kopper, skilt og papirer. |
| `ext/tex_concrete.jpg` | 1024×1024, sømløs | Utendørs betong, slitt og støvete. |
| `ext/tex_desert_ground.jpg` | 1024×1024, sømløs | Ørkengrunn sett ovenfra: sand, grus og spredt tørt kratt. |
| `ext/tex_asphalt_wet.jpg` | 1024×1024, sømløs | Sprukken asfalt, fuktig. |
| `ext/sign_sierra_on.png` | 1024×1536, gjennomsiktig | Neonskiltet til Sierra Motor Court sett rett forfra og tent. Teksten Sierra i rød skriveskrift og MOTOR COURT i cyan blokkbokstaver, med en gul stjerne på toppen. Under henger et lysskilt med CLEAN ROOMS, CABLE TV og VACANCY. |
| `ext/sign_sierra_off.png` | 1024×1536, gjennomsiktig | Samme skilt, slukket. |

## Runde 2: kapittel 1 (servicegården, S-03 og fotolaben)

| Fil | Størrelse | Hva |
|---|---|---|
| `yard/sign_service_yard.png` | 1024×512 | Skilt med SARO-emblemet og teksten SERVICE YARD og AUTHORIZED PERSONNEL ONLY. Slitt emalje. |
| `yard/tex_cabinet_metal.jpg` | 1024×1024, sømløs | Grå lakkert stålskap med riper og rust i kantene. |
| `yard/label_s03_procedure.png` | 512×768 | Prosedyreark for S-03: 1. VERIFY POWER, 2. CHECK ALIGNMENT, 3. LOG ANY ANOMALIES, 4. NOTIFY OPS. Maskinskrevet. |
| `concept/ch1_service_yard.jpg` | 1920×1080 | Konseptbilde av servicegangen ut til S-03 om natten. |
| `concept/ch1_photolab.jpg` | 1920×1080 | Konseptbilde av SAROs lille fotolab med rødt mørkeromslys. |
| `concept/ch4_room6.jpg` | 1920×1080 | Konseptbilde av rom 6, der Nora Vega bor: seng, CRT-TV, to stoler og et bord med papirer og lampe. |

## Runde 3: flater fra kapittel 1 som tegnes i kode i dag

Ingen tekst i disse bildene. Teksten legges på i kode, slik at den alltid blir riktig. Alle seks er levert (PR #31) og i spillet fra 4. oktober.

| Fil | Størrelse | Mål i spillet | Hva |
|---|---|---|---|
| `yard/vane_b12.png` | 256×1024 | 0,34 × 1,36 m | B-12-referansen: matt mørk plate med én loddrett elfenbenshvit stripe og skalastreker langs kanten. |
| `yard/board_r07.png` | 512×512 | 0,42 × 0,42 m | R-07: matt mørk plate med tre vannrette hvite striper. |
| `yard/floor_paint_frame.png` | 1024×512, gjennomsiktig | 2,2 × 1,1 m | Slitt gul ramme malt på betong. Brukes til S-03 APRON og B-12 SIGHT LINE. |
| `lab/map_field_yard.png` | 1024×768 | 1,0 × 0,75 m | Håndtegnet kart sett ovenfra: kontrollrommet nederst, gangveien nordover, S-03 og B-12 langs gangen og fotolaben øst for den. |
| `lab/sign_blank.png` | 1024×384 | 0,2 til 0,8 m brede | Tomt emaljeskilt, kremhvitt med mørk kant og litt rust. Bakgrunn for skiltene i gården og laben. |
| `lab/tex_paper_card.jpg` | 512×512, sømløs | kort og ark | Litt gulnet papir med svake bretter. Bakgrunn for prosesskortene og arkene i laben. |

## Runde 4: arkivfløyen i kapittel 2 (levert i PR #34, i spillet)

Ingen tekst i bildene. Begge tegnes i kode i dag og byttes når filene finnes.

| Fil | Størrelse | Mål i spillet | Hva |
|---|---|---|---|
| `annex/tex_floor_vinyl.jpg` | 1024×1024, sømløs | 1,2 × 1,2 m (fire ganger fire fliser på 30 cm) | Rolige vinylfliser i lys gråbeige fra 1970-tallet, matte og litt slitte, smale fuger. Brukes i korridoren og arkivrommet. |
| `annex/vending_front.png` | 512×1024 | 0,74 × 1,48 m | Front på en brus- og snacksautomat uten merkenavn: glassvindu med rader av bokser og poser, myntinnkast og knapper til høyre, uttaksluke nederst. Feltet under vinduet står tomt, der skriver koden COLD DRINKS. |

## Runde 5: STATION 01 (levert i PR #34, i spillet)

Kapittel 3 foregår på målestasjonen fra 1947, rundt klokka 04:00. Se `src/assets/art/concept/station01_field.jpg`, `maps/station01-plan.png` og den bygde stasjonen i `src/world/Station01.ts`. I dag lages disse flatene i kode. Bildene erstatter kodeteksturene, og alt med tekst blir i kode. Alle fire er 1024×1024, sømløse, JPG med kvalitet rundt 85, sett rett forfra eller ovenfra, uten tekst.

| Fil | Mål per bilde i spillet | Hvor | Hva |
|---|---|---|---|
| `station/tex_stucco_wall.jpg` | 2,0 × 2,0 m | Hyttas yttervegger (4 × 3 m, 2,6 m høy) og generatorbua | Solbleket kremgul puss over murblokk, hårfine sprekker, lappede flekker, rustrenner nederst. Ingen vinduer eller dører i bildet. |
| `station/tex_concrete_old.jpg` | 1,0 × 1,0 m | Transittpilaren, fundamentene til A og B | Gammel betong fra 1940-tallet med grov tilslag, avskallede kanter, lav og grå. |
| `station/tex_wood_weathered.jpg` | 0,5 × 1,0 m | Merkestolpene, stakene til C, portstolpene, hyttedøra | Sølvgrått, sprukket treverk som har stått i ørkensol i førti år, langsgående fiber. |
| `station/tex_floorboards.jpg` | 1,2 × 1,2 m | Gulvet i hytta | Slitte furugulvbord, 12 cm brede, gangsti mot benken, spikerhoder, litt sand i fugene. |

## Runde 6: Roswell-veien og dineren (levert i PR #35; grusen er i spillet, resten kobles inn med dineren)

Historien står i `HISTORIE.md`. Natt mot grålysning, 05:00 til 05:25. Dineren har vært åpen hele natten siden 1940-tallet. Ingen tekst i bildene: navn, overskrifter, priser og skilttekst tegnes i kode oppå. Avisen er oppdiktet, ikke en kopi av en ekte forside.

| Fil | Størrelse | Mål i spillet | Hva |
|---|---|---|---|
| `diner/sign_diner_blank.png` | 1024×512, gjennomsiktig | 4,0 × 2,0 m | Gammelt veiskilt for en diner på to stolper: buet metallramme med neonrør langs kanten (rødt og cyan, påslått), tomt felt i midten der koden skriver navnet. |
| `diner/tex_counter_laminate.jpg` | 512×512, sømløs | 0,6 × 0,6 m | Benkeplate i mintgrønn laminat med små gråstjerner, slitt der tallerkenene settes. |
| `diner/tex_floor_checker.jpg` | 1024×1024, sømløs | 2,4 × 2,4 m | Svart og hvitt rutegulv, 30 cm ruter, matt og gulnet i fugene. |
| `diner/menu_board_blank.png` | 1024×512 | 1,6 × 0,8 m | Svart menytavle med hvite rammelister og tomme linjer. Rettene skrives i kode. |
| `diner/clipping_photo_1947.jpg` | 1024×768 | inne i avisutklippet | Avisfoto fra 1947 i grovt raster: natt over en flat mesa, et rolig lys høyt over den, tre rancher i silhuett i forgrunnen. Ingen tekst. |
| `road/tex_gravel_track.jpg` | 1024×1024, sømløs | 4 × 4 m | Grusvei med to hjulspor og spredt kreosotløv, sett rett ovenfra. |

## Runde 7: Sierra Motor Court og papirene i rom 6 (prioritet 2, etter `MotelFront.ts`)

Kapittel 4 begynner rundt 04:35. Spilleren går fra SARO over veien til motellet, inn på kontoret og videre til rom 6. Se `src/assets/art/concept/sierra_court.jpg`, `maps/motel-plan.png` og oppgaven i `todo.md`. Romnumre, OFFICE, VACANCY og all skrift på papirene tegnes i kode. Ingen tekst og ingen personer i bildene.

| Fil | Størrelse | Mål i spillet | Hva |
|---|---|---|---|
| `motel/door_room_blank.jpg` | 512×1024 | 0,92 × 2,03 m | Motelldør fra 1950-tallet rett forfra, malt tre i oksblodrødt som i konseptet, slitt maling ved håndtaket, kikkhull, messinghåndtak, sparkeplate nederst. Ingen nummer. Dørbladet fyller hele bildet. Brukes på alle dørene. |
| `motel/window_night_lit.jpg` | 512×512 | 1,4 × 1,2 m | Motellvindu sett utenfra om natten: aluminiumskarm, gardinene trukket for, varmt lys bak. Ingen silhuetter. |
| `motel/window_night_dark.jpg` | 512×512 | 1,4 × 1,2 m | Samme vindu og karm med mørkt rom bak gardinene og et svakt rødt skjær fra skiltet i glasset. |
| `motel/tex_motel_wall.jpg` | 1024×1024, sømløs | 2,0 × 2,0 m | Malt puss på motellveggen i blek fersken eller sand som i konseptet, litt skitten nederst, uten vinduer, dører eller kanter. |
| `docs/card_field_1947.jpg` | 1024×640 | i dokumentvisningen | Kartotekkort (5 × 8 tommer) fra 1947: gulnet, trykte blå linjer og rød marglinje, et kaffemerke i ett hjørne, ingen skrift. Rett ovenfra. Bakgrunn for Tomás' feltkort (E11). |
| `docs/letter_paper_1947.jpg` | 1024×1365 | i dokumentvisningen | Brevark fra 1947 som har vært brettet i tre: tydelige bretter, gulnet, ingen skrift. Rett ovenfra. Bakgrunn for brevet fra Tomás (E13). |

## Runde 8: dineren, to flater (prioritet 2, etter `Diner.ts`)

Til dineren i «All Night» (se oppgaven i `todo.md`). Ingen tekst i bildene. Sømløse, rett forfra, uten skygger fra en bestemt lyskilde.

| Fil | Størrelse | Mål i spillet | Hva |
|---|---|---|---|
| `diner/tex_booth_vinyl.jpg` | 512×512, sømløs | 0,5 × 0,5 m | Rødt kunstskinn på båsene med knappede sømmer i rutemønster, litt slitt og blankere der folk har sittet. |
| `diner/tex_wall_panel.jpg` | 1024×1024, sømløs | 2,0 × 2,0 m | Furupanel i honningfarge fra 1950-tallet: loddrette bord med kvister, litt mørknet nederst og ved kjøkkenluka. |

## Runde 9: normal- og ruhetskart til Ultra (PC)

Bestilt 4. oktober 2026 om kvelden, etter at Ultra kom inn (`core/ultra.ts`: ekte lys med skygger fra de nærmeste lampene, GTAO og bloom). Med ekte lys som treffer flatene skrått, ser flate fargebilder platte ut. Disse kartene gir fuger, mørtel, sprekker, fiber og slitasje relieff og glans. De lastes bare i Ultra, på PC. High og Low bruker dem aldri. Ingen nye fargebilder i denne runden, og fargebildene som finnes, skal ikke endres.

Codex lager kartene, manifestet og kontrollen i egen gren. Claude kobler dem inn i `art.ts` og materialene og tar den samlede testen.

### Filnavn og plassering

For hvert fargebilde (albedo) to filer i samme mappe, med samme navn pluss en ending:

- `<navn>_n.png`: normalkart
- `<navn>_r.jpg`: ruhetskart

Eksempel: `room/tex_floor_hextile.jpg` får `room/tex_floor_hextile_n.png` og `room/tex_floor_hextile_r.jpg`.

### Mål og format

| | Normalkart `_n.png` | Ruhetskart `_r.jpg` |
|---|---|---|
| Størrelse | 512×512 for alle (også der fargebildet er 1024) | 512×512 |
| Format | PNG, 8 bit RGB, uten alfa | JPG, kvalitet 90, 8 bit gråtone (én kanal) |
| Fargerom | Lineære data, ikke sRGB. Ingen gammakorreksjon, ingen fargeprofil | Lineære data, ikke sRGB |
| Sømløst | Ja, nøyaktig som fargebildet: samme repetisjon, samme søm | Ja |
| Plassering | Piksel for piksel over fargebildet (skalert til 512): fuga i fargebildet og fuga i kartet ligger på samme sted | Samme |

### Konvensjoner

- **Normalkart:** tangentrom, OpenGL-konvensjon (grønn kanal peker opp, +Y), slik three.js forventer. Ikke DirectX (der grønn er snudd). Flat flate er RGB (128, 128, 255). Vektorene er normaliserte, og blå er aldri under 128 (ingen normal peker inn i flata).
- **Styrke:** stilisert lavpoly, ikke fotorealisme. Rolige flater er nesten flate (helning under rundt 15 grader). Bare fuger, mørtellinjer, sprekker, kanter på fliser og bord og dype hjulspor får tydelig relieff (opp mot rundt 40 grader). Ingen innbakt lys eller skygge i kartene, og ingen fin støy som blir til flimmer på avstand.
- **Ruhetskart:** hvit er ru (1,0), svart er speilblank (0,0). three.js leser den grønne kanalen; en gråtone-JPG gir det samme i alle kanaler. Ingen metallkart: alt metall i spillet er malt.
- **Kilde:** kartene utledes fra fargebildet de hører til (for eksempel en høydeskisse tegnet over fuger og sprekker, så normaler fra den), slik at de stemmer med det spilleren ser. Ikke generiske fliser fra et bibliotek.

### Prioritet A: SARO, det spilleren ser mest

| Fargebilde | Flate i spillet | Normalkart | Ruhet |
|---|---|---|---|
| `room/tex_floor_hextile.jpg` | Gulvet i kontrollrommet (sekskantfliser) | Fugene senket, flisene flate med svakt avrundede kanter | Flisene 0,45 til 0,55, fugene 0,9 |
| `room/tex_wall_paint.jpg` | Malte murblokker i kontrollrommet | Mørtellinjene senket, blokkene svakt ujevne | Malingen 0,7 til 0,8, mørtelen 0,9 |
| `ext/tex_concrete.jpg` | Gangveien, plattformene, bygningene ute | Sprekker og skjøter senket, grov overflate svakt | 0,85 til 0,95 |
| `ext/tex_desert_ground.jpg` | Ørkenbakken | Småstein og tuer litt opp, tørkesprekker ned | 0,95 til 1,0 |
| `ext/tex_asphalt_wet.jpg` | Riksveien og motellplassen | Grov asfalt svakt, sprekker ned | Tørr asfalt 0,75 til 0,85, vannpyttene 0,15 til 0,3 (det er dette som gir speilingen av lampene) |
| `annex/tex_floor_vinyl.jpg` | Gulvet i korridoren og arkivet | Skjøtene mellom vinylflisene senket | 0,45 til 0,6, slitte ganger litt blankere |
| `yard/tex_cabinet_metal.jpg` | Skapfronter og stål | Bulker og kanter svakt, rustflekker litt opp | Malt stål 0,5 til 0,65, rust 0,9 |

### Prioritet B: de andre stedene

| Fargebilde | Flate i spillet | Normalkart | Ruhet |
|---|---|---|---|
| `room/tex_desk_laminate.jpg` | Pultene | Nesten flat, svake riper | 0,35 til 0,5 |
| `room/tex_ceiling_tile.jpg` | Himlingsplatene | Rillene mellom platene, svak porøs overflate | 0,95 |
| `station/tex_stucco_wall.jpg` | Hytta og generatorbua på STATION 01 | Puss med sprekker og lappede flekker | 0,9 |
| `station/tex_concrete_old.jpg` | Transittpilaren og fundamentene | Avskallede kanter, grovt tilslag | 0,9 til 1,0 |
| `station/tex_wood_weathered.jpg` | Stolper, staker, hyttedøra | Langsgående fiber og sprekker tydelig | 0,85 |
| `station/tex_floorboards.jpg` | Gulvet i hytta | Fugene mellom bordene, fiber svakt | 0,6 til 0,75, slitt gangsti litt blankere |
| `road/tex_gravel_track.jpg` | Grusveien | Hjulsporene senket, steiner opp | 0,95 |
| `motel/tex_motel_wall.jpg` | Motellveggen | Puss svakt, sprekker | 0,85 |
| `diner/tex_floor_checker.jpg` | Rutegulvet i dineren | Fugene mellom rutene | 0,35 til 0,5 (bonet gulv) |
| `diner/tex_wall_panel.jpg` | Furupanelet i dineren | Spor mellom bordene, kvister svakt | 0,55 til 0,65 (lakk) |
| `diner/tex_booth_vinyl.jpg` | Båsene | Knappene og sømmene ned, putene opp | 0,3 til 0,45 |
| `diner/tex_counter_laminate.jpg` | Benkeplata | Nesten flat | 0,3 til 0,4 |

Ikke lag kart for skilt, plakater, kart, papir, dokumenter, himmelen, B-12-vingen, R-07-tavla eller det tente motellskiltet.

### Manifest og kontroll

- `production/round9_manifest.json`: for hver fil fargebildet den hører til, metode og verktøy, størrelse, sha256, og eventuelle promter.
- Kontrollbilder i `production/round9_qa/` (kommer aldri med i spillet): hvert normalkart lagt 2×2 for å vise sømløshet, normalkartet med 50 prosent dekning over fargebildet for å vise at fuger og sprekker ligger på samme sted, og en enkel kule- eller flatepreview lyst ovenfra som viser at grønn peker opp (en forhøyning er lys på oversiden).
- Automatisk sjekk: riktig størrelse og kanaler, ingen alfa, blå kanal aldri under 128, normalvektorene har lengde rundt 1 (mellom 0,95 og 1,05 etter dekoding), ruhetskartet er én kanal.
- Leveransenotat `ART_ROUND9_DELIVERY.md` med resultatet av sjekkene (PASS, FAIL, UNVERIFIED), som tidligere runder.

### Hvordan Claude kobler dem inn

Kartene legges i en egen gruppe i `art.ts` (`ULTRA_ART`) som bare lastes når Ultra slås på. Når Ultra er på, får de matte standardmaterialene `normalMap` og `roughnessMap` med samme repetisjon som fargebildet; slås Ultra av, tas de bort igjen. Testen `tools/ultra.py` tar de samme utsnittene før og etter.

## Slik kommer bildene inn i spillet

Legg filene i `web/src/assets/art/<kategori>/` med nøyaktig navn fra tabellene, for eksempel `web/src/assets/art/room/tex_floor_hextile.jpg`. På GitHub går det med Add file, Upload files i riktig mappe. Claude kobler så fila inn i `src/core/art.ts` og på flaten den hører til. Bilder som SARO trenger, lastes før verden bygges, og spillet viser TRY AGAIN hvis et bilde ikke kan hentes. Bilder som bare ett område trenger (STATION 01, veien, senere motellet og dineren), lastes når området bygges. Bare bilder som importeres i `art.ts`, kommer med i spillpakken. Konsepter, kart, `production/` og kontrollbilder blir aldri med. Store PNG-er får en lett WebP-kopi i `runtime/` med `python3 tools/prepare_art.py`.

Slik brukes filene (29 bilder i spillet per 4. oktober kl. 12.40; listen under gjelder de 22 første):

- Rett på flaten: gulv, tak, vegg og bord i kontrollrommet, betong, ørkenbakken, de to plakatene, New Mexico-kartet, servicegårdskiltet, S-03-prosedyren, det tente motellskiltet og `yard/vane_b12.png` (B-12, stripa ligger der stripetesten venter den). Himmelen blandes inn i stjerneshaderen.
- Under tekst fra koden: `yard/board_r07.png` (R-07 skrives under stripene), `lab/map_field_yard.png` (tittel, alle stedsnavn, fence og N), `yard/floor_paint_frame.png` (S-03 APRON og B-12 SIGHT LINE), `lab/sign_blank.png` (alle skilt og merkelapper med kant, også i arkivet), `lab/tex_paper_card.jpg` (kort, ark, arbeidsordren og papirene på pultene), `yard/tex_cabinet_metal.jpg` (skapfrontene med stensiltekst, og stålet på sidene), `ext/tex_asphalt_wet.jpg` (veimerkingen tegnes oppå) og `brand/logo_saro.png` (kaffekoppen).
- Ikke i spillet: `ext/sign_sierra_off.png` (kildevariant), alt i `concept/`, `maps/` og `production/`, og forhåndsvisninger som `lab/paper-repeat-preview.jpg`.

## Ikke lag dette

- 3D-modeller. Claude lager geometrien.
- Dokumenter i spillet (skiftlogg, utskrifter). Disse lages i HTML og CSS, slik at teksten alltid blir riktig.
- Brukergrensesnitt og menyer.

## Konsepter og lokasjonskart levert 4. oktober 2026

De tre konseptbildene i runde 2 er levert, sammen med `concept/saro_archive.jpg`, `concept/station01_field.jpg` og `concept/sierra_court.jpg` i 1920×1080. `ch3_room47.jpg` er bevart som eldre variant. Etter gjennomgang av Claude-grenen `e5706fb` er `ch4_room6.jpg` levert etter den korrigerte briefen. Grenen registrerer rom 6 og K4 som avklart.

Toms bestilling om lokasjonskart er levert som `maps/world-overview.jpg` (2400×1350) og redigerbare `maps/saro-plan.svg`, `maps/station01-plan.svg`, `maps/motel-plan.svg`, alle med PNG-kopier (1800×1280). Nærkartene fastlegger ikke nye spillkonstanter. Illustrasjonen er skjematisk; SARO-nærkartet bruker web-geometrien fra main på `65ad59f` før arkivtilbygget, mens målestasjonen og motellet er planforslag.

De seks opprinnelige miljøkonseptene er skalert fra 1672×941. Rom-6-revisjonens kilde står i `concept/ROOM6_REVISION.json`. Se `CONCEPT_DELIVERY.md`, promptmanifestene og det lokale galleriet `src/assets/art/concept/index.html`.

## Runde 3: tekstfrie flater fra Claudes oppdaterte brief

Lest fra `origin/ccr-30e38858-767d90` på `e5706fb`, 4. oktober. Alle seks filer er levert. Tekst og bevislogikk beholdes i kode.

| Fil | Størrelse | Bruk |
| --- | --- | --- |
| `yard/vane_b12.png` | 256×1024 | Mørk referanseplate, én elfenbensstripe og 12 skalastreker |
| `yard/board_r07.png` | 512×512 | Mørk plate med nøyaktig tre vannrette striper |
| `yard/floor_paint_frame.png` | 1024×512, RGBA | Slitt gul ramme, gjennomsiktig bakgrunn |
| `lab/map_field_yard.png` | 1024×768 | Tekstfritt feltkart for kapittel 1, med plasser for engelske etiketter |
| `lab/sign_blank.png` | 1024×384 | Tomt kremhvitt emaljeskilt |
| `lab/tex_paper_card.jpg` | 512×512, sømløs | Lavkontrast papirunderlag for kort og ark |

Se `CLAUDE_HANDOFF.md` og `src/assets/art/production/index.html`. Mål og etikettposisjoner står i `production/PRECISE_GRAPHICS.json`. Claude koblet filene inn 4. oktober: PR #30s loader er beholdt og utvidet med de seks, den parallelle loaderen fra `0bb9170` er fjernet, og tekstene legges på i kode som beskrevet over.
