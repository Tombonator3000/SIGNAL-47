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

## Runde 4: arkivfløyen i kapittel 2 (ikke levert)

Ingen tekst i bildene. Begge tegnes i kode i dag og byttes når filene finnes.

| Fil | Størrelse | Mål i spillet | Hva |
|---|---|---|---|
| `annex/tex_floor_vinyl.jpg` | 1024×1024, sømløs | 1,2 × 1,2 m (fire ganger fire fliser på 30 cm) | Rolige vinylfliser i lys gråbeige fra 1970-tallet, matte og litt slitte, smale fuger. Brukes i korridoren og arkivrommet. |
| `annex/vending_front.png` | 512×1024 | 0,74 × 1,48 m | Front på en brus- og snacksautomat uten merkenavn: glassvindu med rader av bokser og poser, myntinnkast og knapper til høyre, uttaksluke nederst. Feltet under vinduet står tomt, der skriver koden COLD DRINKS. |

## Runde 5: Roswell-veien og dineren (kan lages nå, brukes etter rom 6)

Historien står i `HISTORIE.md`. Natt mot grålysning, 05:00 til 05:25. Dineren har vært åpen hele natten siden 1940-tallet. Ingen tekst i bildene: navn, overskrifter, priser og skilttekst tegnes i kode oppå. Avisen er oppdiktet, ikke en kopi av en ekte forside.

| Fil | Størrelse | Mål i spillet | Hva |
|---|---|---|---|
| `diner/sign_diner_blank.png` | 1024×512, gjennomsiktig | 4,0 × 2,0 m | Gammelt veiskilt for en diner på to stolper: buet metallramme med neonrør langs kanten (rødt og cyan, påslått), tomt felt i midten der koden skriver navnet. |
| `diner/tex_counter_laminate.jpg` | 512×512, sømløs | 0,6 × 0,6 m | Benkeplate i mintgrønn laminat med små gråstjerner, slitt der tallerkenene settes. |
| `diner/tex_floor_checker.jpg` | 1024×1024, sømløs | 2,4 × 2,4 m | Svart og hvitt rutegulv, 30 cm ruter, matt og gulnet i fugene. |
| `diner/menu_board_blank.png` | 1024×512 | 1,6 × 0,8 m | Svart menytavle med hvite rammelister og tomme linjer. Rettene skrives i kode. |
| `diner/clipping_photo_1947.jpg` | 1024×768 | inne i avisutklippet | Avisfoto fra 1947 i grovt raster: natt over en flat mesa, et rolig lys høyt over den, tre rancher i silhuett i forgrunnen. Ingen tekst. |
| `road/tex_gravel_track.jpg` | 1024×1024, sømløs | 4 × 4 m | Grusvei med to hjulspor og spredt kreosotløv, sett rett ovenfra. |

## Slik kommer bildene inn i spillet

Legg filene i `web/src/assets/art/<kategori>/` med nøyaktig navn fra tabellene, for eksempel `web/src/assets/art/room/tex_floor_hextile.jpg`. På GitHub går det med Add file, Upload files i riktig mappe. Claude kobler så fila inn i `src/core/art.ts` og på flaten den hører til. Alle bilder i spillet lastes før verden bygges, og spillet viser TRY AGAIN hvis et bilde ikke kan hentes. Bare bilder som importeres i `art.ts`, kommer med i spillpakken. Konsepter, kart, `production/` og kontrollbilder blir aldri med. Store PNG-er får en lett WebP-kopi i `runtime/` med `python3 tools/prepare_art.py`.

Slik brukes filene (22 bilder per 4. oktober):

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
