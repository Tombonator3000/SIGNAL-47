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

Se `CLAUDE_HANDOFF.md` og `src/assets/art/production/index.html`. På main må PR30s loader utvides av kodeansvarlig. Claude har siden lagt til automatisk innlasting i `0bb9170`; denne parallelle løsningen må sammenføres med PR30, og tekstflatene trenger fortsatt komposisjon med kodegrafikken. Mål og etikettposisjoner står i `production/PRECISE_GRAPHICS.json`.
