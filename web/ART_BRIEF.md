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
| `concept/ch4_room6.jpg` | 1920×1080 | Konseptbilde av motellrom 6, der Nora Vega bor: helt ordinært, med seng, TV, et bord med papirer og en lampe. |

## Runde 3: flater fra kapittel 1 som tegnes i kode i dag

Ingen tekst i disse bildene. Teksten legges på i kode, slik at den alltid blir riktig.

| Fil | Størrelse | Mål i spillet | Hva |
|---|---|---|---|
| `yard/vane_b12.png` | 256×1024 | 0,34 × 1,36 m | B-12-referansen: matt mørk plate med én loddrett elfenbenshvit stripe og skalastreker langs kanten. |
| `yard/board_r07.png` | 512×512 | 0,42 × 0,42 m | R-07: matt mørk plate med tre vannrette hvite striper. |
| `yard/floor_paint_frame.png` | 1024×512, gjennomsiktig | 2,2 × 1,1 m | Slitt gul ramme malt på betong. Brukes til S-03 APRON og B-12 SIGHT LINE. |
| `lab/map_field_yard.png` | 1024×768 | 1,0 × 0,75 m | Håndtegnet kart sett ovenfra: kontrollrommet nederst, gangveien nordover, S-03 og B-12 langs gangen og fotolaben øst for den. |
| `lab/sign_blank.png` | 1024×384 | 0,2 til 0,8 m brede | Tomt emaljeskilt, kremhvitt med mørk kant og litt rust. Bakgrunn for skiltene i gården og laben. |
| `lab/tex_paper_card.jpg` | 512×512, sømløs | kort og ark | Litt gulnet papir med svake bretter. Bakgrunn for prosesskortene og arkene i laben. |

## Slik kommer bildene inn i spillet

Legg filene i `web/src/assets/art/` med nøyaktig navn fra tabellene, for eksempel `web/src/assets/art/room/tex_floor_hextile.jpg`. På GitHub går det med Add file, Upload files i riktig mappe. Spillet bruker et bilde automatisk når fila finnes, og faller tilbake på teksturen fra koden når den mangler. Ingen kodeendring trengs.

## Ikke lag dette

- 3D-modeller. Claude lager geometrien.
- Dokumenter i spillet (skiftlogg, utskrifter). Disse lages i HTML og CSS, slik at teksten alltid blir riktig.
- Brukergrensesnitt og menyer.
