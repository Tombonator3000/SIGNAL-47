# Lydbibliotek: Ultimate SFX for Games ($25-pakken), Function Loops

**Status: Referanse** · **Gjelder:** Toms kjøpte lydbibliotek målt opp mot SIGNAL / 47; ingenting her er tatt inn i spillet bare fordi det står i en kortliste. **Sist kontrollert:** 10. november 2026. Inngang: [README](README.md).

Kilde: `~/Desktop/SIGNAL 47/bt25_ultimatesfxforgames_softwarebundle/` (skrivebeskyttet, 8,0 GB; samme pakke som `~/Assets/ultimate-sfx/`). Lydfilene er kjøpt og ligger aldri i Git; bare denne oversikten og måletabellen gjør det. Alt under er målt med `ffprobe`/`ffmpeg` + numpy/scipy (skript i `Tools/sfx-catalog/`), unntatt der det står «etter navn» eller «gjetning». Hele tabellen ligger i [sfx_catalog.csv](sfx_catalog.csv) (2335 rader, 0 dekodingsfeil).

## Hva er målt og hva er gjetning

- **Målt:** varighet, kanaler, samplerate, bitdybde, topp, LUFS (BS.1770, gated, egen implementasjon), sentroide og lav/mid/høy-fordeling (gjennomsnittsspekter), tidspunkt for høyeste topp, anslagstid, andel nær stillhet (< −55 dBFS i 5 ms-vinduer), nivåsvingning, start/slutt-forskjell.
- **Gjetning fra måling (heuristikk, ikke hørt):** `kind` og `loopable` i CSV-en.
- **Bare etter navn:** hva en lyd forestiller (f.eks. «Reel», «Cartridge», «Sliding»), tonearter og BPM. BPM stemmer med målt lengde der jeg har sjekket: Darkwave 150 BPM × 8 takter = 12,8 s, Retrowave 100 BPM × 4 takter = 9,6 s, ABYSS 100 BPM × 8 takter = 19,2 s.
- **Måleavvik å kjenne til:** alt er dekodet og resamplet til 48 kHz stereo før måling, så «topp» er topp etter resampling og kan vise opptil +1,5 dBFS (mellom-sample-topper) selv om kildefila er under 0. «Anslag» er tiden fra 20 dB under høyeste topp til toppen, i 5 ms-steg, og maksimalt 1000 ms (1000 betyr «ingen skarp topp»). Ingenting er hørt.

## Totalt

2303 WAV + 32 MP3 = 2335 lydfiler, 7,3 timer. De fleste «SFX»-mappene har bare 30 filer hver; hovedmengden (≈ 1210 filer) er musikk (1984-pakken 601, Modern Music Kits 424, ABYSS 187). Målte `kind`: one_shot 503, stinger_hard 352, long_evolving 338, bed_texture 336, short_sound 181, stinger 159, bed_tonal_drone 105, short_hit 78, whoosh_swell 72, music_stem 72, music_loop 57, riser 26, impact_decay 23, music_track 18, near_silent 15. `loopable`: ja 355, kanskje 220, nei 1730. Betydning av `kind`: bed_texture/bed_tonal_drone = jevn lang tekstur (tonal ved lav spektral flathet), one_shot/short_sound/short_hit = enkeltlyd, stinger/stinger_hard = skarpt anslag (≤ 40 ms / ≤ 15 ms) i starten, riser = stiger ≥ 8 dB mot slutten, whoosh_swell = topp i midten, impact_decay = skarpt støt som dør ut, long_evolving = lang og ujevn, music_track/music_loop/music_stem = musikk (stems er enkeltspor av en låt), near_silent = nesten bare stillhet. Grensene er mine; kolonnen er et sorteringshjelpemiddel, ikke fasit.

## Per mappe

| Mappe | Filer | Min | Lengde (min–maks) | Format (måling) | Hva og bruk her |
|---|---|---|---|---|---|
| 1984 Retro Music Production Bundle | 601 | 210,3 | 0,07 s – 175,62 s | WAV 24-bit 44,1 kHz ×574, MP3 44,1 kHz ×22, WAV 16-bit 44,1 kHz ×5 | Tre musikkpakker (Darkwave/postpunk 150 BPM, Neon Highway 5 kit på 80–124 BPM, Retrowave 100 BPM): loops, stems, one-shots, demo-låter. Eneste periodemusikken. Brukes som jukebox/bilradio og som synth-lag; stems kan bli egne lag. |
| 8-Bit SFX | 30 | 1 | 2 s – 2 s | WAV 24-bit 44,1 kHz ×30 | Chip-lyder (mynt, skade, feil, alarm). Lite å bruke utover enkle datamaskinpip; for glossy/spillaktig til 1986-terminal. |
| ABYSS - Dark Cinematic | 187 | 43,4 | 2,40 s – 350,40 s | WAV 24-bit 44,1 kHz ×187 | Mørk filmmusikk i løkker (100 BPM, tonearter i navnene, 2–8 takter) pluss en 350 s demo. Svært nyttig: bass, pads, teksturer og effektløkker som kan lages til adaptivt lag. |
| Adventure | 30 | 1 | 1 s – 4 s | WAV 24-bit 44,1 kHz ×30 | Engelske replikker (fantasy-NPC: «Greetings…», «Farewell…»). Ubrukelig her. |
| Alarms & Sirens | 30 | 5,8 | 11,52 s – 11,52 s | WAV 24-bit 44,1 kHz ×30 | 30 sirener/alarmer à 11,5 s, alle med utklang. Til varsler, sivilforsvar, SARO-alarmer. |
| Ambience & SFX | 40 | 21,3 | 32 s – 32 s | WAV 24-bit 44,1 kHz ×40 | Ambience SciFi (30, toneart i navn, 32 s) og Noise Ambience City (10, 32 s). Lange, jevne, tonale lag: sci-fi-lag for arrayet, mørke drones. Høyt nivå (rundt −11 LUFS). |
| Ambience Space & Planet | 30 | 16 | 32 s – 32 s | WAV 24-bit 44,1 kHz ×30 | 30 rom-/planetlag à 32 s, mange veldig lave (<300 Hz). Lytting til signaler fra arrayet, trykk under hendelser. Allerede brukt (nr. 04 som «pressure»). |
| Bluster Shots | 30 | 0,7 | 0,50 s – 4 s | WAV 24-bit 44,1 kHz ×30 | Etter navnet skudd/smell (fantasy/arkade). Lite nyttig. |
| Checkpoints, Errors And Riddles SFX | 30 | 1 | 2 s – 2 s | WAV 24-bit 44,1 kHz ×30 | 2 s-lyder: korte toner og klokker for feil/lagring/gåter. Noen lave, tørre pip kan være terminalbekreftelser. |
| Clothing, Fabric & Materials | 30 | 0,7 | 0,50 s – 4 s | WAV 24-bit 44,1 kHz ×30 | Rygsekk, glidelås, spenner, kabler, bandasje. Små kledd-/bagasje-foley; kabel og spenne kan passe utstyr. |
| Cozy Mechanics | 30 | 0,5 | 0,50 s – 2 s | WAV 24-bit 44,1 kHz ×30 | Fiske, grave, kart, inventar (cozy-spill). Lite nyttig; kartlyder kan gjøre nytte på kartbordet. |
| Doors | 30 | 0,9 | 0,50 s – 4 s | WAV 24-bit 44,1 kHz ×30 | Hjemmedører: lås, garasje, smell, småbaner (navn «Sliding»/«Small»/«Soft»). Dører på motell, diner og SARO. Fire er allerede brukt. |
| Dreamy Whooshes | 30 | 1,2 | 1 s – 8 s | WAV 24-bit 44,1 kHz ×30 | Myke whoosher 1–8 s, noen med kraftig stigning. Bra til lysene på himmelen (stigende, hvisking). |
| Foley | 30 | 0,8 | 0,50 s – 8 s | WAV 24-bit 44,1 kHz ×30 | Husholdningsfoley: flaske, glass/vann, lighter, lokk, nøkler, kniv, toalett, søppel. Diner og motell. |
| Glitches | 30 | 1,1 | 1 s – 4 s | WAV 24-bit 44,1 kHz ×30 | Digitale glitcher og støystøt 1–4 s. TV/CRT som reagerer på noe over hodet; flere er rene høyfrekvente støysmell. |
| Granular | 30 | 1,1 | 0,50 s – 4 s | WAV 24-bit 44,1 kHz ×30 | Granulære svisj/tekstur 0,5–4 s, noen stigende. Mørke stigninger og strukturlyd for anomalier. |
| Guns | 30 | 0,8 | 1 s – 2 s | WAV 24-bit 44,1 kHz ×30 | Skudd. Ingen bruk (spillet har ikke våpen). |
| Horror Ambience | 30 | 8 | 16 s – 16 s | WAV 24-bit 44,1 kHz ×30 | 30 lag à 16 s; 26 av 30 er jevne og løkkbare (måling). Hovedkilden til base-lag i det adaptive skrekkmiljøet. |
| Horror Screamers | 30 | 8 | 16 s – 16 s | WAV 24-bit 44,1 kHz ×30 | 30 lyder à 16 s: et skarpt anslag (første 0,1–1 s) og 5–14 s utklang/stille. Brukes som stinger, trimmes i motoren. |
| Household Foley & Devices | 30 | 0,7 | 0,50 s – 8 s | WAV 24-bit 44,1 kHz ×30 | Knapper, brytere, låser, mikrobølge, ovn, kjøleskap, timer, «Reel», «Cartridge». Brytere, bånddekk og diner-apparater. Fem er allerede brukt. |
| Kitchen, Cooking | 30 | 1,9 | 0,50 s – 8 s | WAV 24-bit 44,1 kHz ×30 | Koking, steking, suppe, skjæring, tallerken. Diner-kjøkken (steking er allerede radioens statiske sus). |
| Magic Spells & Attack SFX | 30 | 1,9 | 3,84 s – 3,84 s | WAV 24-bit 44,1 kHz ×30 | Fantasy-magi à 3,8 s. Ingen bruk. |
| Medieval Fighting | 30 | 0,3 | 0,50 s – 1 s | WAV 24-bit 44,1 kHz ×30 | Piler, sverd, rustning (0,5–1 s). Ingen bruk. |
| Messages SFX | 30 | 1 | 2 s – 2 s | WAV 24-bit 44,1 kHz ×30 | 2 s varsler/meldingspip. Enkelte lave toner kan passe en terminal; ellers moderne telefon-varsler. |
| Modern Music Kits | 424 | 80,3 | 0,10 s – 80 s | WAV 24-bit 44,1 kHz ×414, MP3 44,1 kHz ×10 | Trap/R&B, indie, phonk, breakcore, lofi, pop-songstartere. Feil periode og sjanger; ingen bruk bortsett fra eventuelt «Lofi & Dream Hop» som reserve. |
| Monsters, Boss & Zombies | 30 | 1,3 | 0,68 s – 8 s | WAV 24-bit 44,1 kHz ×30 | Engelske bossreplikker («I can smell your fear») og zombielyder (brøl, galle, hvesing). Bossreplikkene passer ikke; zombiebrøl kan klippes som korte organiske stingere. |
| Notification SFX | 30 | 1 | 2 s – 2 s | WAV 24-bit 44,1 kHz ×30 | 2 s varsler. Moderne app-varsler; lite å bruke. |
| Orchestral Braams | 30 | 3,8 | 7,68 s – 7,68 s | WAV 24-bit 44,1 kHz ×30 | 30 braams à 7,7 s, tonearter Bm/Cm/D#m i navn. Store himmelhendelser, avsløring. |
| Orchestral Impacts | 30 | 3,8 | 7,68 s – 7,68 s | WAV 24-bit 44,1 kHz ×30 | 30 dype/kraftige slag à 7,7 s; mange nesten ren sub (50–100 Hz). Himmelhendelser, nedslag. |
| Orchestral Whooshes | 30 | 3,8 | 7,68 s – 7,68 s | WAV 24-bit 44,1 kHz ×30 | 30 orkestersvisj à 7,7 s som bygger og avtar. Stigning før et lys og avtagning etter. |
| Shooter Radio Commands | 30 | 1,1 | 1 s – 3 s | WAV 24-bit 44,1 kHz ×30 | Engelske militære radioreplikker. Ubrukelig (stemmene er ikke Dale-stemmen). |
| Swing & Dash | 30 | 0,6 | 0,50 s – 4 s | WAV 24-bit 44,1 kHz ×30 | Kamp-svisj 0,5–4 s. Lite nyttig. |
| Techno textures and SFX | 123 | 12,5 | 0,12 s – 14,77 s | WAV 24-bit 44,1 kHz ×123 | 130 BPM techno: arps, pads, synth, trommer, Fx (riser, statikk, rattle, gong). Fx-mappen (12) er nyttig til risers og elektronisk støy; resten lite. |
| Ui Clicks | 30 | 0,3 | 0,50 s – 4 s | WAV 24-bit 44,1 kHz ×30 | 30 klikk à 0,5 s (én på 4 s). Tørre, korte; mange mørke. Terminal- og kontrollromsklikk. |
| Ui Games Interface | 30 | 0,5 | 1 s – 1 s | WAV 24-bit 44,1 kHz ×30 | 30 spill-UI-lyder à 1 s, mange stille. Glossy; lite å bruke. |
| Ui Sci-Fi Interface | 30 | 0,2 | 0,50 s – 0,50 s | WAV 24-bit 44,1 kHz ×30 | 30 sci-fi-UI à 0,5 s, noen lave pip. Kan brukes sparsomt til CRT-terminaler. |
| War Strategy | 30 | 0,8 | 1 s – 2 s | WAV 24-bit 44,1 kHz ×30 | Engelske krigsreplikker. Ubrukelig. |

Format-merknad: alt er 44,1 kHz stereo (også de som egentlig er mono-innhold; jeg har ikke målt om kanalene er like). 2298 filer er 24-bit WAV, 5 er 16-bit WAV (NHW-demoene), 32 er MP3. Spillet leser selv 16-bit PCM WAV (se nederst).

## Ikke-lyd

- `.mid` (98): MIDI-filer til Retrowave (58) og Neon Highway (31), Indie Vibes (9). Kan åpnes i en synth for å lage egne varianter; ingen bruk i spillet direkte.
- `.mse` (865): lydbank til pluginen DrumKits (Drum Vault, Drum Vault 2, Future Pop & RNB, Tech House), 173 filer × 5 pluginformater. Format ukjent, ikke avspillbart uten pluginen.
- `DRUMKITS Virtual Instrument Plugin/` (1,7 GB): plugin «DrumKits» v2.10.2 (leverandør Function Loops, bygget på MaizeSoft MSP) som VST3, VST, AU (Mac) og VST3/VST (Windows, `.dll`). Ingen bruk i Unreal; dessuten Windows/Mac-binærfiler.
- `link.txt`: lenke `https://www.functionloops.com/drumkits-vst-au-plugin.html`.
- Presets: `.SerumPreset` (20, Retrowave, Serum 2), `.h2p` (20, u-he Diva), `.spf` (4, Spire), `.opxpreset` (4, OP-X PRO), `.dx7x` (1, Arturia DX7) i Neon Highway-kitene. Bare nyttig om du lager egen musikk i disse synthene.
- Rusk: `.DS_Store` (42), `._desktop.ini` (5), `Info.plist`/`.nib`/`moduleinfo.json`/`CodeResources`/`PkgInfo` inne i plugin-pakkene.

## Lisens

Jeg søkte i hele biblioteket etter lisens-, readme-, EULA-, txt-, pdf-, rtf-, html- og md-filer. **Det finnes ingen lisensfil i biblioteket.** Eneste tekstfil er `DRUMKITS Virtual Instrument Plugin/link.txt` (en lenke til functionloops.com), og `moduleinfo.json` nevner bare «Vendor: Function Loops». Heller ikke zip-filene i `~/Assets/ultimate-sfx/` ($5, $15, $25) har lisensfil. `~/Assets/ultimate-sfx/order.json` bekrefter kjøp via Humble («Ultimate SFX for Games», utsteder FunctionLoops, 19. oktober 2026, 27,18 USD), men sier ikke noe om vilkår.

Det som står om vilkår, står i prosjektets egne notater, ikke i pakken: `web/src/assets/licensed/LICENSES.md` (privat klone `Tombonator3000/SIGNAL-47-assets`) sier: «Vilkår (Function Loops' EULA): fri bruk i kommersielle produksjoner og spill, lov å redigere, ingen krav om kreditering; ikke lov å selge videre, lisensiere videre, laste opp eller dele lydene som lydfiler.» Dette er Toms/Claudes tidligere referat; jeg kan ikke bekrefte det mot noen kilde i pakken. **Må bekreftes av Tom** mot Function Loops' gjeldende EULA / Humble-ordren før noe regnes som avklart: (1) at EULA-en gjelder akkurat denne Humble-pakken, (2) at spillbygg som inneholder redigerte klipp er dekket, men ikke selve råfilene (trolig et skille mellom «innebygd i produksjon» og «videredistribusjon som lydfil»), (3) at pluginen/MIDI/presets har samme vilkår. Inntil da: ikke legg råfiler eller hele mapper i Git (se under).

**Avklart av Tom 10. november 2026:** lydene er kjøpt på Humble Bundle (alle tre nivåene) og klarert for bruk i spill. Pakkesiden (humblealerts.com/software/ultimate-sfx-for-games) bekrefter at alle 38 pakkene ligger på disken, men gjengir ingen vilkår. Regelen om råfiler står: bare redigerte klipp i spillet, ingen lydfiler i Git.

**Musikk mikset av loopene, 13. november 2026 (MUSIC-20261113):** fire stykker er arrangert av løkker fra «1984 Retro Music Production Bundle» og «ABYSS - Dark Cinematic» med [`Tools/music/build_music.py`](../../../unreal/Signal47/Tools/music/build_music.py) (`mixes`): «Night Freight» (Neon Highway kit 5, 80 BPM, F#-E-B-D, 3:36; også en AM-smalnet utgave for den fjerne radiostasjonen), «Cold Dish» (ABYSS, 100 BPM, D#-moll, 3:31, uten trommer), «Carrier Lock» (Retrowave, 100 BPM, C-moll, 2:53) og «County Road» (Neon Highway kit 4, 124 BPM, A-C-D, 3:06). Løkkene er faselåst til ett taktrutenett, lagene kommer og går på taktstreker, og bussen har 9,5 kHz lavpass, myk metning, svak wow og 2:1-kompresjon; nivå -21,5 LUFS som resten av musikken. Skriptet ligger i Git og peker på kildestiene; **råløkkene sendes aldri ut, bare de ferdige miksene**, og de ligger bare i den Git-ignorerte lydmappa (`Content/Audio/Raw/media`). Spillet går uten dem. Ingen har hørt miksene ennå; de er kontrollert ved måling (seksjonsnivå, toneart per takt, ingen klipp eller klikk). **Før offentlig utgave må Tom bekrefte mot Function Loops' gjeldende EULA** at musikk satt sammen av løkkene kan følge med spillet; avklaringen 10. november gjelder lydeffekter i spill og er Toms eget referat, ikke en lisenstekst.

## Kortlister

Tall i tabellene er fra `sfx_catalog.csv`. «Sentroide» = spektral tyngdepunkt (lavt = mørkt). Nivåsvingning = standardavvik i dB over 100 ms-vinduer (lavt = jevnt). Start/slutt-diff = nivåforskjell første/siste del (lavt = lett å løkke).

### a. Skrekkbeds (base-lag i adaptiv encounter-score)

Krav: ≥ 14 s, nivåsvingning ≤ 2,6 dB, ingen stillhet, start/slutt-diff ≤ 2 dB, sentroide ≤ 450 Hz. 21 filer besto; her 12. Horror Ambience er bare 16 s, så bruk kryssfade eller to lag i ulik fase mot gjentakelse. ABYSS-løkkene er eksakte takter (100 BPM) og lar seg stable i samme toneart (D# eller G#).

| Fil | s | LUFS | Sentroide Hz | % under 250 Hz | Nivåsvingning dB | Start/slutt-diff dB | Hvorfor |
|---|---|---|---|---|---|---|---|
| `Horror Ambience/FL_GFX_Horror_Ambience_18.wav` | 16 | -24 | 163 | 86 | 1,6 | 1,6 | Mørkest av horror-lagene som er jevne: 86 % under 250 Hz, løkkbar. Lavt nivå (−24 LUFS) = naturlig grunnlag. |
| `Horror Ambience/FL_GFX_Horror_Ambience_20.wav` | 16 | -28,6 | 177 | 77 | 1,4 | 0,6 | Nesten like mørk (177 Hz), stillest av alle (−28,6 LUFS); minimal variasjon (1,4 dB). Dyp bunn under ting. |
| `Horror Ambience/FL_GFX_Horror_Ambience_26.wav` | 16 | -19,4 | 320 | 49 | 1,7 | 1,2 | Lav-midt (320 Hz), jevn, løkkbar; litt mer «bevegelse» enn 18/20. |
| `Horror Ambience/FL_GFX_Horror_Ambience_07.wav` | 16 | -21,1 | 357 | 45 | 2 | 0,6 | 357 Hz, 45 % lav, start/slutt-forskjell 0,6 dB = nesten sømløs løkke. |
| `Horror Ambience/FL_GFX_Horror_Ambience_11.wav` | 16 | -22,2 | 460 | 28 | 0,9 | 1,6 | Svært jevn (0,9 dB), 460 Hz, −22 LUFS. Rolig mellomlag. |
| `ABYSS - Dark Cinematic/Loops/Bass Loops/FL_ABB_Bass_100_G#_Void_16_8bars.wav` | 19,2 | -22,2 | 53 | 99 | 1,3 | 1,8 | Ren sub (53 Hz, 99 % lav), 19,2 s = 8 takter på 100 BPM (navn); jevn (1,3 dB). Trykk under en hendelse. |
| `ABYSS - Dark Cinematic/Loops/Bass Loops/FL_ABB_Bass_100_D#_Void_01_8bars.wav` | 19,2 | -21 | 225 | 53 | 0,8 | 0,2 | 225 Hz, minst variasjon i hele utvalget (0,8 dB), diff 0,2 dB: eksakt løkke i D#. |
| `ABYSS - Dark Cinematic/Loops/Pad Loops/FL_ABB_Pad_100_D#_Obsidian_01_8bars.wav` | 19,2 | -20,4 | 243 | 64 | 2,3 | 0,1 | Mørk pad (243 Hz, 64 % lav), diff 0,1 dB: sømløs 8-taktsløkke i D#. |
| `ABYSS - Dark Cinematic/Loops/Pad Loops/FL_ABB_Pad_100_F#_Obsidian_29_8bars.wav` | 19,2 | -19,3 | 268 | 69 | 2,4 | 0,4 | 268 Hz; F#-pad, passer ikke tonalt med D# – bruk alene. |
| `Ambience & SFX/Ambience SciFi/FL_GFX_Ambience_SciFi_Gloomy_Silence_Dm.wav` | 32 | -12,2 | 271 | 76 | 2 | 1,4 | 32 s, 271 Hz, 76 % lav, i D-moll (navn). Må dempes (−12 LUFS). |
| `Ambience & SFX/Ambience SciFi/FL_GFX_Ambience_SciFi_Thick_Silence_D#m.wav` | 32 | -11 | 345 | 35 | 1,5 | 1,2 | 345 Hz, svært jevn (1,5 dB); D#-moll passer med ABYSS D#-løkkene. |
| `Ambience & SFX/Ambience SciFi/FL_GFX_Ambience_SciFi_Crimson_Dust_Cm.wav` | 32 | -10,4 | 322 | 61 | 1,3 | 0,4 | 322 Hz, 1,3 dB variasjon, diff 0,4 dB. |


### b. Screamers/stingers

Alle Horror Screamers er 16 s med ett anslag i starten og 5–14 s utklang og stillhet etterpå (målt). De må klippes (f.eks. 1–3 s med fade) for å fungere som «noe ble sett». Høyeste topp ligger i de første ~0,1 s for hard-listen. Hva lydene er (skrik, dyr, syntese) er ikke avgjort ved måling.

**Hard** (anslag ≤ 65 ms, topp i første 1 %):

| Fil | Topp dBFS | LUFS | Høyeste topp ved | Anslag ms (−20 dB→topp) | Nedtrapping s (topp→−30 dB) | Sentroide Hz | Hvorfor |
|---|---|---|---|---|---|---|---|
| `Horror Screamers/FL_GFX_Horror_Screamers_27.wav` | 0 | -13,5 | 0,2 % (0,03 s) | 25 | 11 | 911 | Raskest anslag (25 ms), topp 0 dBFS, høy LUFS. |
| `Horror Screamers/FL_GFX_Horror_Screamers_17.wav` | 1,5 | -19,5 | 0,2 % (0,03 s) | 25 | 10,8 | 1128 | 25 ms anslag; topp +1,5 dBFS (mellom-sample-topp etter resampling): normaliser ned. |
| `Horror Screamers/FL_GFX_Horror_Screamers_30.wav` | -2,9 | -16,2 | 0,3 % (0,05 s) | 40 | 11,7 | 935 | 40 ms, 935 Hz. |
| `Horror Screamers/FL_GFX_Horror_Screamers_06.wav` | -3,9 | -15 | 0,3 % (0,05 s) | 50 | 10,3 | 1278 | 50 ms, lysere (1278 Hz). |
| `Horror Screamers/FL_GFX_Horror_Screamers_29.wav` | -3,9 | -15,5 | 0,7 % (0,11 s) | 50 | 10,7 | 1557 | 50 ms, lysest (1557 Hz): skjærer gjennom bedet. |
| `Horror Screamers/FL_GFX_Horror_Screamers_11.wav` | -0,7 | -13,9 | 0,4 % (0,06 s) | 60 | 12,3 | 1068 | 60 ms, høy LUFS (−13,9). |
| `Horror Screamers/FL_GFX_Horror_Screamers_26.wav` | -0,6 | -19,4 | 0,4 % (0,06 s) | 60 | 7,6 | 747 | 60 ms; kortere hale (7,6 s) og mest stillhet (31 %): lettest å klippe. |
| `Horror Screamers/FL_GFX_Horror_Screamers_15.wav` | 0,2 | -14 | 0,4 % (0,06 s) | 65 | 9,3 | 1407 | 65 ms, 1407 Hz. |
| `Monsters, Boss & Zombies/FL_GFX_Voice_Monster_Zombie_Roar_01.wav` | 0,1 | -15 | 15,7 % (0,24 s) | 45 | 0,8 | 1730 | Kort (1,5 s), 45 ms anslag. Stemme (zombiebrøl): organisk. |


**Mykere** (anslag 145–440 ms, eller topp senere):

| Fil | Topp dBFS | LUFS | Høyeste topp ved | Anslag ms (−20 dB→topp) | Nedtrapping s (topp→−30 dB) | Sentroide Hz | Hvorfor |
|---|---|---|---|---|---|---|---|
| `Horror Screamers/FL_GFX_Horror_Screamers_21.wav` | -0,9 | -18,7 | 1,4 % (0,22 s) | 165 | 10,9 | 580 | 165 ms anslag, mørk (580 Hz), stille (−18,7 LUFS). |
| `Horror Screamers/FL_GFX_Horror_Screamers_02.wav` | -3,9 | -16,9 | 0,9 % (0,14 s) | 145 | 12,9 | 593 | 145 ms, 593 Hz, minst stillhet (4 %): sammenhengende lyd. |
| `Horror Screamers/FL_GFX_Horror_Screamers_04.wav` | -3,9 | -15 | 1 % (0,16 s) | 155 | 10,9 | 679 | 155 ms, 679 Hz. |
| `Horror Screamers/FL_GFX_Horror_Screamers_18.wav` | -3,9 | -12,2 | 1 % (0,16 s) | 165 | 11,2 | 773 | 165 ms, 773 Hz, høy LUFS. |
| `Horror Screamers/FL_GFX_Horror_Screamers_14.wav` | -3,9 | -13,3 | 2,3 % (0,37 s) | 370 | 10,2 | 545 | 370 ms myk start, 545 Hz. |
| `Horror Screamers/FL_GFX_Horror_Screamers_23.wav` | -3,9 | -17,2 | 2,8 % (0,45 s) | 440 | 9,7 | 949 | 440 ms, 949 Hz. |
| `Horror Screamers/FL_GFX_Horror_Screamers_22.wav` | -2,2 | -14,7 | 10,8 % (1,73 s) | 1000 | 9,3 | 295 | Lav (295 Hz, 82 % lav), topp først ved 1,7 s: knurring som vokser, ikke et støt. |
| `Horror Screamers/FL_GFX_Horror_Screamers_28.wav` | -1,6 | -14,8 | 7,6 % (1,22 s) | 1000 | 10,5 | 649 | Topp ved 1,2 s, 649 Hz: sakte inn. |


### c. Risers, whoosher, braams, impacts (store himmelhendelser)

Braams har toneart i navnet (Bm/Cm/D#m). Impacts er mest ren sub. Ekte risers (nivået stiger mot slutten) finnes bare i ABYSS Effect Loops, noen få Dreamy Whooshes/Granular og Techno Fx; orkestersvisjene bygger bare til midten og avtar.

| Fil | s | Topp dBFS | LUFS | Sentroide Hz | % under 250 Hz | Topp ved | Anslag ms | Stigning dB (siste/første tredjedel) | Hvorfor |
|---|---|---|---|---|---|---|---|---|---|
| `Orchestral Braams/FL_GFX_Orchestral_Braams_01_Bm.wav` | 7,7 | -0,1 | -14,7 | 231 | 70 | 0 % | 15 | -40,9 | Skarpeste braam: topp ved 0,2 %, anslag 15 ms, 70 % lav. |
| `Orchestral Braams/FL_GFX_Orchestral_Braams_06_Cm.wav` | 7,7 | -1,3 | -10,9 | 79 | 94 | 2 % | 120 | -44 | Dyp braam (79 Hz, 94 % lav), topp tidlig. |
| `Orchestral Braams/FL_GFX_Orchestral_Braams_09_Cm.wav` | 7,7 | -1 | -12,6 | 93 | 91 | 10 % | 745 | -43,5 | 93 Hz, 91 % lav: tung, uten skarp høy. |
| `Orchestral Braams/FL_GFX_Orchestral_Braams_04_Cm.wav` | 7,7 | -1,8 | -10,8 | 854 | 11 | 43 % | 1000 | -22,2 | Swell: høyeste topp først ved 43 % (3,3 s). Bygger, ikke et støt. |
| `Orchestral Impacts/FL_GFX_Orchestral_Impact_03.wav` | 7,7 | -1,6 | -14,2 | 55 | 99 | 10 % | 315 | -39 | 55 Hz, 99 % lav: ren sub-dunk. |
| `Orchestral Impacts/FL_GFX_Orchestral_Impact_07.wav` | 7,7 | -1,6 | -11,7 | 60 | 99 | 8 % | 410 | -36,7 | 60 Hz, 99 % lav, 1,8 s utklang. |
| `Orchestral Impacts/FL_GFX_Orchestral_Impact_28.wav` | 7,7 | -4,3 | -23,9 | 61 | 98 | 0 % | 20 | -46,9 | Stram: anslag 20 ms, utklang 0,24 s, 61 Hz. Stille (−23,9 LUFS). |
| `Orchestral Impacts/FL_GFX_Orchestral_Impact_01.wav` | 7,7 | -2 | -18 | 158 | 91 | 0 % | 25 | -36,9 | 25 ms anslag, 158 Hz, 1,6 s utklang. |
| `ABYSS - Dark Cinematic/Loops/Effect Loops/FL_ABB_Effect_100_G#_Shadow_12_4bars.wav` | 9,6 | -6,5 | -19,3 | 388 | 63 | 100 % | 1000 | 30,8 | Ekte riser: stiger 30,8 dB, topp i siste 0,2 % (9,6 s), 388 Hz, 63 % lav. G#. |
| `ABYSS - Dark Cinematic/Loops/Effect Loops/FL_ABB_Effect_100_G#_Shadow_17_4bars.wav` | 9,6 | -6,9 | -20,3 | 740 | 5 | 100 % | 1000 | 13,9 | Riser, stiger 13,9 dB, topp rett før slutt, lysere (740 Hz). G#. |
| `ABYSS - Dark Cinematic/Loops/Effect Loops/FL_ABB_Effect_100_D#_Shadow_01_4bars.wav` | 9,6 | -8,2 | -22 | 741 | 29 | 86 % | 1000 | 12,4 | Riser, stiger 12,4 dB, topp ved 85 %. D#. |
| `Dreamy Whooshes/FL_GFX_Motion_Dreamy_Whoosh_21.wav` | 2 | -11 | -25,8 | 225 | 82 | 72 % | 225 | 59,4 | 2 s, stiger 59 dB, 82 % lav (225 Hz): mørk stigende pust. |
| `Granular/FL_GFX_Motion_Granular_23.wav` | 2 | -10,5 | -26,7 | 133 | 98 | 93 % | 355 | 27,8 | 2 s, 133 Hz, 98 % lav, stiger 28 dB. |
| `Orchestral Whooshes/FL_GFX_Orchestral_Whoosh_10.wav` | 7,7 | -2,8 | -14,1 | 676 | 20 | 52 % | 1000 | 4 | 7,7 s, topp ved 51 %, stiger 4 dB: bygger, så avtar (svisj over hodet). |
| `Techno textures and SFX/Fx/FL_VM_130_Fx_Riser_Weld_D#.wav` | 7,4 | -1,2 | -13,4 | 589 | 13 | 80 % | 1000 | 2,9 | 7,4 s, topp ved 80 %, 589 Hz: elektronisk, grovere riser. |


### d. TV-statikk, interferens, glitch (TV og CRT)

Spillet bruker allerede steking (Fry_Meat) som radiosus og mikrobølge som TV-brum (`MANIFEST.md`). Under: støy som kan legges over i reaksjon på noe over hodet.

| Fil | s | Topp dBFS | LUFS | Sentroide Hz | % over 2,5 kHz | Anslag ms | Nedtrapping s | Hvorfor |
|---|---|---|---|---|---|---|---|---|
| `Glitches/FL_GFX_Motion_Glitch_09.wav` | 2 | -10 | -23,4 | 5350 | 100 | 65 | 1,6 | Rent høyfrekvent støystøt: 100 % over 2,5 kHz, 5350 Hz, 2 s. |
| `Glitches/FL_GFX_Motion_Glitch_22.wav` | 4 | -8,7 | -28,5 | 9561 | 97 | 280 | 0,2 | 4 s støy, 97 % høy, 9561 Hz: hvit hvesing for TV-snø. |
| `Glitches/FL_GFX_Motion_Glitch_27.wav` | 2 | -6,4 | -24,7 | 7697 | 98 | 195 | 0,7 | 2 s, 98 % høy, 7697 Hz. |
| `Glitches/FL_GFX_Motion_Glitch_12.wav` | 2 | -10,9 | -21,6 | 8633 | 91 | 20 | 0,4 | 91 % høy, anslag 20 ms: skarp knitring. |
| `Glitches/FL_GFX_Motion_Glitch_10.wav` | 2 | -16,3 | -20,7 | 6836 | 88 | 535 | 1,5 | 88 % høy, 6836 Hz, 2 s. |
| `Glitches/FL_GFX_Motion_Glitch_03.wav` | 2 | -11,6 | -25,2 | 591 | 1 | 65 | 0,7 | Brum/buzz: 591 Hz, 45 % lav, tidlig topp (4 %). Strømbrum som slår inn. |
| `Glitches/FL_GFX_Motion_Glitch_26.wav` | 2 | -7 | -20,8 | 605 | 4 | 175 | 0,2 | 605 Hz, 57 % lav: tørr elektrisk summing. |
| `Glitches/FL_GFX_Motion_Glitch_25.wav` | 1 | -6,9 | -32,3 | 209 | 0 | 10 | 0 | Kort tung dunk: 1 s, 209 Hz, 5 ms anslag. Relé/slag når bildet faller ut. |
| `Techno textures and SFX/Fx/FL_VM_130_Fx_Electronic_Static_C.wav` | 7,4 | -1,4 | -15,2 | 782 | 7 | 1000 | 4,2 | 7,4 s elektronisk statikk, 782 Hz, topp midt i. Lengre statikk. |
| `Techno textures and SFX/Fx/FL_VM_130_Fx_Noise_Abrasive_C.wav` | 3,7 | -0,7 | -15,7 | 6014 | 65 | 30 | 1,8 | 3,7 s slipende støy, 6014 Hz. |
| `Techno textures and SFX/Fx/FL_VM_130_Fx_Electronic_Rattle_D#.wav` | 14,8 | -1,4 | -16,2 | 424 | 2 | 800 | 6,4 | 14,8 s, 424 Hz, 62 % lav: skramling/elektrisk uro. |
| `Household Foley & Devices/FL_GFX_Household_Foley_Devices_Signal_01.wav` | 1 | -18,4 | -22,1 | 3559 | 100 | 45 | 0,1 | 1 s, 3559 Hz: hvin/signal fra et apparat. |
| `Household Foley & Devices/FL_GFX_Household_Foley_Devices_Fridge.wav` | 2 | -9,4 | -24,9 | 259 | 1 | 60 | 0,6 | 2 s kjøleskap, 259 Hz: lavt brum som kan løkkes til CRT-brum. |


### e. Terminal-/UI-klikk, pip, bekreftelser (1986-maskinvare: kort, tørr, ikke glossy)

Utvalg: Ui Clicks med sentroide ≤ 800 Hz og utklang ≤ 0,04 s er de tørreste og mørkeste (høy sentroide betyr glossy). 8-Bit SFX er avvist (2 s-filer, de fleste med 50–90 % stillhet, og tydelig chip-karakter etter navnet). Pipene fra Checkpoints/Sci-Fi er 2 s-filer med mye stillhet; klipp.

| Fil | s | Topp dBFS | LUFS | Sentroide Hz | % over 2,5 kHz | Anslag ms | Nedtrapping s | Hvorfor |
|---|---|---|---|---|---|---|---|---|
| `Ui Clicks/FL_GFX_UI_Click_02.wav` | 0,5 | -7,5 | -28,5 | 578 | 0 | 10 | 0,01 | Tørr: 578 Hz, anslag 10 ms, utklang 0,01 s. |
| `Ui Clicks/FL_GFX_UI_Click_04.wav` | 0,5 | -4,8 | -31 | 630 | 0 | 0 | 0,04 | 630 Hz, 0 ms anslag, topp −4,8 dBFS: tydelig tastetrykk. |
| `Ui Clicks/FL_GFX_UI_Click_05.wav` | 0,5 | -10,3 | -28 | 414 | 0 | 5 | 0,04 | Mørk (414 Hz), 5 ms. |
| `Ui Clicks/FL_GFX_UI_Click_06.wav` | 0,5 | -7,8 | -29,8 | 774 | 0 | 5 | 0,03 | 774 Hz, 5 ms. |
| `Ui Clicks/FL_GFX_UI_Click_10.wav` | 0,5 | -8,7 | -27,5 | 418 | 0 | 5 | 0,01 | 418 Hz, utklang 0,01 s: veldig tørr. |
| `Ui Clicks/FL_GFX_UI_Click_18.wav` | 0,5 | -8,8 | -29,9 | 398 | 0 | 5 | 0,02 | 398 Hz. |
| `Ui Clicks/FL_GFX_UI_Click_20.wav` | 0,5 | -7,4 | -29,2 | 397 | 0 | 5 | 0,03 | 397 Hz. |
| `Ui Clicks/FL_GFX_UI_Click_25.wav` | 0,5 | -4,9 | -26,2 | 516 | 1 | 5 | 0,03 | 516 Hz, topp −4,9 dBFS. |
| `Ui Clicks/FL_GFX_UI_Click_09.wav` | 0,5 | -5,3 | -25,5 | 329 | 1 | 15 | 0,04 | Dumpe tastetrykk: 329 Hz, 15 ms. |
| `Checkpoints, Errors And Riddles SFX/FL_GFX_Checkpoint_Error_Riddle_Sfx_06.wav` | 2 | -5,5 | -15 | 612 | 0 | 15 | 0,17 | Kort pip: 612 Hz, 15 ms, 91 % stillhet i 2 s-fila: klipp til ~0,2 s. Bekreftelse. |
| `Checkpoints, Errors And Riddles SFX/FL_GFX_Checkpoint_Error_Riddle_Sfx_12.wav` | 2 | -8,4 | -18,3 | 466 | 0 | 5 | 0,29 | Kort tone: 466 Hz, 5 ms, utklang 0,29 s. |
| `Checkpoints, Errors And Riddles SFX/FL_GFX_Checkpoint_Error_Riddle_Sfx_03.wav` | 2 | -8,7 | -19,5 | 520 | 0 | 30 | 0,23 | 520 Hz, 87 % stillhet: kort feil-/svarpip. |
| `Ui Sci-Fi Interface/FL_GFX_UI_SciFi_Interface_10.wav` | 0,5 | -9,1 | -30,6 | 402 | 0 | 5 | 0,06 | Lavt blipp (402 Hz), 88 % stillhet, 5 ms. |
| `Ui Sci-Fi Interface/FL_GFX_UI_SciFi_Interface_01.wav` | 0,5 | -12,8 | -26,2 | 307 | 0 | 5 | 0,14 | 307 Hz, 5 ms, utklang 0,14 s: mørk bekreftelse. |


### f. Dører, brytere, husholdningsapparater, kjøkken/diner-foley

Navn er det eneste som sier hva lyden er. Allerede i spillet (fra `MANIFEST.md`): Door_Smack_01 (`door_heavy`), Door_Small_02 (`door_light`), Latch_01, Toggle_Switch_01 og _04, Cartridge_02 (`handset_down`), Microwave (`tv_hum`), Foley Keys, Glass_Water (`pour`), Crunch_01/02, Fry_Meat_02/03 (`radio_static`) og Shred_Vegetables_06.

| Fil | s | Topp dBFS | LUFS | Sentroide Hz | % over 2,5 kHz | Anslag ms | Nedtrapping s | Hvorfor |
|---|---|---|---|---|---|---|---|---|
| `Doors/FL_GFX_Home_Fx_Door_Close_Lock.wav` | 1,5 | -2,9 | -23,8 | 1109 | 16 | 45 | 0,16 | Dør lukkes og låses: 1,5 s, 1109 Hz. |
| `Doors/FL_GFX_Home_Fx_Door_Open_Lock.wav` | 1,5 | -2,1 | -24,8 | 1105 | 14 | 10 | 0,18 | Dør låses opp, 1105 Hz, rask (10 ms). |
| `Doors/FL_GFX_Home_Fx_Door_Lock_03.wav` | 2 | -11,7 | -29,9 | 668 | 5 | 20 | 0,29 | Myk lås, 668 Hz, stille (−29,9 LUFS): rom med gjester. |
| `Doors/FL_GFX_Home_Fx_Door_Garage_01.wav` | 2 | -7,5 | -28,1 | 1011 | 7 | 15 | 0,71 | Garasjeport, 1011 Hz, 0,7 s utklang. Bensinstasjon/skur. |
| `Doors/FL_GFX_Home_Fx_Door_Smack_02.wav` | 1 | -2,2 | -18,1 | 307 | 3 | 35 | 0,17 | Tung smell, 307 Hz. (Smack_01 er allerede brukt som «door_heavy».) |
| `Doors/FL_GFX_Home_Fx_Door_Soft_01.wav` | 0,5 | -4,4 | -23,3 | 795 | 5 | 20 | 0,18 | Mykt lukk, 0,5 s, 795 Hz. |
| `Household Foley & Devices/FL_GFX_Household_Foley_Devices_Toggle_Switch_03.wav` | 0,5 | -6,1 | -24,8 | 1870 | 24 | 20 | 0,04 | Bryter, 1870 Hz, 20 ms, tørr. |
| `Household Foley & Devices/FL_GFX_Household_Foley_Devices_Interrupteur_01.wav` | 1 | -6,2 | -21,8 | 537 | 3 | 25 | 0,12 | Vippebryter/lysbryter, 537 Hz: dempet, kontorlik. |
| `Household Foley & Devices/FL_GFX_Household_Foley_Devices_Reel.wav` | 0,8 | -8,3 | -23,3 | 2410 | 28 | 150 | 0,23 | «Reel», 0,75 s, 2410 Hz, anslag 150 ms: kandidat til bånddekket (må høres). |
| `Household Foley & Devices/FL_GFX_Household_Foley_Devices_Cartridge_01.wav` | 1 | -2,4 | -21,3 | 677 | 5 | 30 | 0,16 | Kassett/patron, 1 s, 677 Hz, anslag 30 ms: kassett-/magasinklikk (må høres). |
| `Household Foley & Devices/FL_GFX_Household_Foley_Devices_Oven_Bell.wav` | 2 | -7 | -20,4 | 4250 | 91 | 5 | 1,10 | Ding, 4250 Hz, 1,1 s utklang: ordrebjelle/kjøkken. |
| `Household Foley & Devices/FL_GFX_Household_Foley_Devices_Microwave_Door_01.wav` | 1,5 | -6,8 | -20,4 | 965 | 16 | 50 | 0,20 | Mikrobølgedør, 965 Hz. |
| `Kitchen, Cooking/FL_GFX_Home_Fx_Kitchen_Cooking_Boiling_01.wav` | 8 | 0,3 | -27,1 | 2225 | 25 | 30 | 6,25 | 8 s koking, 2225 Hz: diner-kjøkken i bakgrunnen. |
| `Kitchen, Cooking/FL_GFX_Home_Fx_Kitchen_Cooking_Stirring_Soup_01.wav` | 8 | 0 | -31,9 | 1201 | 9 | 10 | 0,04 | 8 s røring/suppe, 1201 Hz. |
| `Foley/FL_GFX_Home_Foley_Plates.wav` | 2 | -6,9 | -27,4 | 2895 | 68 | 10 | 0,20 | Tallerkener, 2 s, 2895 Hz, 10 ms anslag. |
| `Foley/FL_GFX_Home_Foley_Spoon.wav` | 0,8 | -8,4 | -25,2 | 4405 | 79 | 25 | 0,08 | Skje, 0,75 s, 4405 Hz. |
| `Foley/FL_GFX_Home_Foley_Lighter_01.wav` | 0,5 | -4,8 | -20,6 | 7601 | 96 | 25 | 0,08 | Lighter, 0,5 s, 7601 Hz. |


### g. Alarmer og sirener

Alle 30 er 11,52 s og ender i utklang (ikke løkker). For en lenge hørt sirene: klipp midtpartiet og lag selv løkka. Topp −0,2 til −5 dBFS og opptil −4,5 LUFS (svært høyt): dempes mye.

| Fil | s | Topp dBFS | LUFS | Sentroide Hz | % over 2,5 kHz | Anslag ms | Nedtrapping s | Hvorfor |
|---|---|---|---|---|---|---|---|---|
| `Alarms & Sirens/FL_GFX_Alarm_Siren_01.wav` | 11,5 | -2,7 | -14,2 | 949 | 0 | 1000 | 9 | 949 Hz, jevn tone uten klikk (anslag ikke skarp), nærmest en vedvarende sirene. |
| `Alarms & Sirens/FL_GFX_Alarm_Siren_28.wav` | 11,5 | -2,5 | -10,2 | 455 | 1 | 635 | 0,3 | Stigende: nivået stiger 28 dB; 455 Hz. Fjern sivilforsvarsvarsel som bygger. |
| `Alarms & Sirens/FL_GFX_Alarm_Siren_20.wav` | 11,5 | -1 | -9,7 | 602 | 0 | 630 | 5,7 | Lav (602 Hz), 5,7 s utklang. |
| `Alarms & Sirens/FL_GFX_Alarm_Siren_29.wav` | 11,5 | -2,5 | -10,7 | 643 | 1 | 1000 | 6 | 643 Hz, 6 s utklang. |
| `Alarms & Sirens/FL_GFX_Alarm_Siren_12.wav` | 11,5 | -4,3 | -15,9 | 859 | 4 | 1000 | 5,6 | 859 Hz, 5,6 s utklang, mørk. |
| `Alarms & Sirens/FL_GFX_Alarm_Siren_07.wav` | 11,5 | -5,1 | -16,2 | 903 | 13 | 150 | 3,2 | 903 Hz, mørk. |
| `Alarms & Sirens/FL_GFX_Alarm_Siren_08.wav` | 11,5 | -3,6 | -15,1 | 927 | 14 | 5 | 1,9 | Skarpt anslag (5 ms), 927 Hz, 45 % lav. |
| `Alarms & Sirens/FL_GFX_Alarm_Siren_27.wav` | 11,5 | -2,5 | -9,8 | 955 | 3 | 210 | 6 | 955 Hz, 6 s utklang. |
| `Alarms & Sirens/FL_GFX_Alarm_Siren_05.wav` | 11,5 | -2,4 | -12,4 | 2891 | 61 | 15 | 0,8 | Skarpt anslag (15 ms), 2891 Hz: lysere buzzer. |
| `Alarms & Sirens/FL_GFX_Alarm_Siren_09.wav` | 11,5 | -2,6 | -14 | 3963 | 50 | 5 | 1,4 | Skarpt anslag (5 ms), 3963 Hz, lys: kontrollrom-alarm. |


### h. Rom/planet for lytting til signaler fra arrayet

Space & Planet er mørkere enn nesten alt annet i biblioteket: 16 av 30 har sentroide under 250 Hz. Jevne lag (diff < 1,5 dB) til bakgrunn, bevegelige (svingning ≥ 4 dB) til «signal som kommer og går».

| Fil | s | LUFS | Sentroide Hz | % under 250 Hz | Nivåsvingning dB | Start/slutt-diff dB | Hvorfor |
|---|---|---|---|---|---|---|---|
| `Ambience Space & Planet/FL_GFX_Ambience_Space_Planet_29.wav` | 32 | -15,2 | 97 | 91 | 1,4 | 0 | 97 Hz, 91 % lav, jevn 1,4 dB, diff 0: dyp, sømløs. |
| `Ambience Space & Planet/FL_GFX_Ambience_Space_Planet_23.wav` | 32 | -14,5 | 114 | 96 | 1,5 | 0,9 | 114 Hz, 96 % lav, 1,5 dB. |
| `Ambience Space & Planet/FL_GFX_Ambience_Space_Planet_12.wav` | 32 | -14,2 | 116 | 92 | 1,5 | 1,3 | 116 Hz, 92 % lav. |
| `Ambience Space & Planet/FL_GFX_Ambience_Space_Planet_17.wav` | 32 | -12,6 | 167 | 82 | 1,2 | 1,5 | 167 Hz, 82 % lav, svært jevn (1,2 dB). |
| `Ambience Space & Planet/FL_GFX_Ambience_Space_Planet_10.wav` | 32 | -14,3 | 241 | 61 | 1,1 | 0,1 | 241 Hz, 1,1 dB, diff 0,1 dB: sømløs midtstemme. |
| `Ambience Space & Planet/FL_GFX_Ambience_Space_Planet_04.wav` | 32 | -15 | 215 | 75 | 1,2 | 0,1 | 215 Hz, diff 0,1 dB. Allerede i spillet som «pressure». |
| `Ambience Space & Planet/FL_GFX_Ambience_Space_Planet_30.wav` | 32 | -18,3 | 570 | 5 | 4,8 | 0,8 | Bevegelig (nivåsvingning 4,8 dB), 570 Hz: pulserer sakte som et signal som kommer og går. |
| `Ambience Space & Planet/FL_GFX_Ambience_Space_Planet_15.wav` | 32 | -14,2 | 173 | 78 | 4,1 | 3,9 | Bevegelig (4,1 dB), 173 Hz. |
| `Ambience Space & Planet/FL_GFX_Ambience_Space_Planet_07.wav` | 32 | -19,8 | 265 | 63 | 4 | 5,3 | Bevegelig (4,0 dB), 265 Hz, stille (−19,8 LUFS). |
| `Ambience Space & Planet/FL_GFX_Ambience_Space_Planet_02.wav` | 32 | -16,8 | 89 | 96 | 3,9 | 2,1 | 89 Hz, 96 % lav, 3,9 dB svingning. |
| `Ambience & SFX/Ambience SciFi/FL_GFX_Ambience_SciFi_Cold_Star_D#m.wav` | 32 | -11,5 | 466 | 5 | 1,3 | 0,7 | 466 Hz, tonal, 1,3 dB – D#-moll, passer ABYSS D#. |
| `Ambience & SFX/Ambience SciFi/FL_GFX_Ambience_SciFi_Space_Nebula_Cm.wav` | 32 | -12 | 516 | 2 | 1 | 0,8 | 516 Hz, 1,0 dB. Navnet «Nebula» finnes i lydverkstedet som `space_nebula`; trolig samme fil (ikke bekreftet). |


### i. Periodemusikk (jukebox/bilradio)

Pakken har ingen amerikansk country/rock/jazz fra 1986. «1984 Retro» er 2010-talls synthwave/darkwave i 80-tallsstil; kan fungere som sen-kveld-bilradio eller en plate på jukeboksen, men ikke som autentisk periodemusikk. Phonk/Trap/Lofi i Modern Music Kits passer ikke. Tempo og toneart under er fra mappe- og filnavn (BPM-ene bekreftet av lengde der mulig), ikke målt av meg.

| Fil | s | LUFS | Sentroide Hz | Nivåsvingning dB | Start/slutt-diff dB | loopable | Hvorfor |
|---|---|---|---|---|---|---|---|
| `1984 Retro Music Production Bundle/Function Loops - Darkwave - Postpunk/Function Loops - Darkwave - Postpunk.mp3` | 126,8 | -10,1 | 450 | 3,4 | 8,4 | no | Full låt, 126,8 s, MP3, 450 Hz sentroide, −10,1 LUFS. Mørk postpunk. Tempo ikke i filnavnet (loopene i samme pakke er 150 BPM). |
| `1984 Retro Music Production Bundle/Function Loops - Retrowave/Music.mp3` | 175,6 | -11,3 | 638 | 5,4 | 32,2 | no | Full retrowave-låt, 175,6 s, MP3, −11,3 LUFS. Tempo/toneart ikke oppgitt (loopene er 100 BPM). |
| `1984 Retro Music Production Bundle/Function Loops - Neon Highway/FL_NHW_KIT05_80BPM_F#-E-B-D/FL_NHW_KIT05_DEMO.wav` | 150,5 | -12,6 | 254 | 5 | 60 | – | 80 BPM, F#-E-B-D (mappenavn), 150,5 s, 254 Hz: tregest og mørkest av Neon Highway-demoene. |
| `1984 Retro Music Production Bundle/Function Loops - Neon Highway/FL_NHW_KIT02_90BPM_D-G-Bb-D#/FL_NHW_KIT02_DEMO.wav` | 93,1 | -12,4 | 388 | 8,4 | 60 | – | 90 BPM, D-G-Bb-D#, 93,1 s, 388 Hz. |
| `1984 Retro Music Production Bundle/Function Loops - Neon Highway/FL_NHW_KIT01_105BPM_G-F-C/FL_NHW_KIT01_DEMO.wav` | 79,2 | -12,3 | 418 | 6,8 | 9,2 | – | 105 BPM, G-F-C, 79,2 s, 418 Hz. |
| `1984 Retro Music Production Bundle/Function Loops - Neon Highway/FL_NHW_KIT03_110BPM_C#-G#-D#-Bb/FL_NHW_KIT03_DEMO.wav` | 76,6 | -12,1 | 528 | 6,5 | 33,8 | – | 110 BPM, C#-G#-D#-Bb, 76,6 s, 528 Hz. |
| `1984 Retro Music Production Bundle/Function Loops - Neon Highway/FL_NHW_KIT04_124BPM_A-C-D/FL_NHW_KIT04_DEMO.wav` | 72,9 | -11,6 | 500 | 9,3 | 19,3 | – | 124 BPM, A-C-D, 72,9 s, 500 Hz: mest drivende, bilradio. |
| `1984 Retro Music Production Bundle/Function Loops - Darkwave - Postpunk/FL_DW_Loops/FL_DW_Music_Loops/FL_DW_150_Music_Memory_Bm.wav` | 12,8 | -13,4 | 603 | 1 | 0,8 | yes | 150 BPM, B-moll, 12,8 s = 8 takter, løkkbar (diff 0,8 dB), 603 Hz. |
| `1984 Retro Music Production Bundle/Function Loops - Darkwave - Postpunk/FL_DW_Loops/FL_DW_Music_Loops/FL_DW_150_Music_Seattle_Am.wav` | 12,8 | -13,9 | 206 | 1 | 0,2 | yes | 150 BPM, A-moll, svært jevn (1,0 dB), diff 0,2 dB. |
| `1984 Retro Music Production Bundle/Function Loops - Darkwave - Postpunk/FL_DW_Loops/FL_DW_Music_Loops/FL_DW_150_Music_Reveria_Dm.wav` | 12,8 | -14,1 | 347 | 1,3 | 0,7 | yes | 150 BPM, D-moll, 347 Hz. |
| `1984 Retro Music Production Bundle/Function Loops - Darkwave - Postpunk/FL_DW_Loops/FL_DW_Music_Loops/FL_DW_150_Music_Youth_Cm.wav` | 12,8 | -12,5 | 456 | 1,4 | 0,3 | yes | 150 BPM, C-moll, lysest (456 Hz), diff 0,3 dB. |
| `1984 Retro Music Production Bundle/Function Loops - Retrowave/Loops/Synth Loops/Chords/FL_SKY_100_Synth_Chord_Nostalgic_Amin.wav` | 9,6 | -12 | 1194 | 1,9 | 0,3 | yes | 100 BPM, A-moll, 9,6 s = 4 takter, diff 0,3 dB, 1194 Hz: akkord-pad for bilradio/jukebox. |


## Slik tar spillet inn lyd i dag (lest i repoet, skrivebeskyttet)

Det er to løp i `US47Audio` (`unreal/Signal47/Source/Signal47/Private/S47Audio.cpp`, `S47AudioWeb.cpp`, `Public/S47Audio.h`):

1. **Web Audio-porten (hovedløpet).** `DecodeAll()` leser `Content/Audio/Raw/index.json` og `Content/Licensed/Raw/index.json`. Hver har `"sources": { "<navn>": "<fil>.wav" }`. WAV-filene dekodes av `S47Wa::DecodeWav` (RIFF PCM/ekstensible) til buffere i `Buf` under navnet. Kjøpte lyder får prefikset `lic:` (`lic:door_heavy`). `ByFile` mapper også filens basenavn til nøkkelen, så `Play("door_heavy")` finner `lic:door_heavy`. `Sfx(Name, FS47Sound{Gain, Rate, At, When, Dest, bLoop, Lp, From, Dur})` spiller en buffer; `LoopSfx(Key, Name, ..)` løkker og `Stop(Key, Fade)` fader. `Or(Name, Old)` gir `lic:`-varianten hvis den finnes, ellers den gamle: slik faller spillet tilbake uten kjøpte filer. `LicGain()` i `S47AudioWeb.cpp` er en hardkodet tabell som hever enkeltlyder (rack_switch 1,6, small_switch 2,2, radio_crackle_long 4,2, radio_crackle_short 3).
2. **Unreal-assets (reserve).** `Find(Name)` leter `USoundBase` i `/Game/Licensed/Sfx`, `/Game/Audio/Sfx`, `/Game/Audio/Synth`, `/Game/Audio/Music`, `/Game/Audio/Radio` (i den rekkefølgen) og spiller med `PlaySound2D/AtLocation`. Loggmeldingen «no sound named X (run Scripts/import-audio.sh)» betyr manglende import.

**Format og navn.** `unreal/Signal47/Scripts/audio-raw.py` kjører ffmpeg til **16-bit PCM WAV** (`-c:a pcm_s16le`) uten å tvinge samplerate. Navnet er filens basename, `lic:<basename>` for kjøpte. Mono for lyder som står et sted i verden (3D-panner), stereo bare for stedløse lag; løkker som WAV (MP3 legger stillhet i skjøten), engangslyder kan være MP3 i den private kilden. `Scripts/import-audio.sh` (krever at Unreal-editoren er lukket) lager både Raw-WAV-ene og uasset-importen. Mapper: `Content/Audio/Raw/` (frie lyder + `sound_design/`, `night/`, `media/music`, `media/radio`, `voice/`), `Content/Licensed/Raw/` (kjøpt, i dag 17 `lic:`-lyder), `Content/Licensed/Sfx` (uasset av de samme).

**Hvordan kjøpt lyd kommer fra pakken til spillet i dag:** `web/src/assets/licensed/tools/build_sfx.py` (i den private klonen) klipper, nivåsetter og konverterer utvalgte filer fra `~/Assets/ultimate-sfx/.../bt25_ultimatesfxforgames_softwarebundle` (`P25` i skriptet) til `web/src/assets/licensed/sfx/*.wav|mp3`. `sfx/MANIFEST.md` dokumenterer kilde, utsnitt og bruk for 17 lyder (tallene i del f ovenfor). `audio-raw.py` konverterer så til `Content/Licensed/Raw`. **En ny lyd tas inn ved å legge den klippet i den private `sfx/`-mappa og kjøre importskriptet; ikke ved å peke på biblioteket.**

**Er det et adaptivt lagsystem (bed + stingers)?** Ikke slik du beskriver det. Det som finnes:

- `FS47Score` (`S47Score.cpp`): musikkdirigent med to dekk og crossfade 2–4 s. Input: `RequestMood(Cue, Severity)` (Severity 0–1; ≥ 0,75 eller cue `action` gir hastverk: overgang innen ≤ 0,35 s i stedet for neste takt/fase), `MusicWant()` fra sted/tid (`night`, `eerie`, `tension`, `motel`, `drive`, `town`, `dawn`, i tillegg `action`/`calm`), `SetDuck`, `Fade`, `Pause`, `MusicHeld` (hold i lytteøyeblikk). Spor er 17 stereo-musikkfiler (`CUES` → `TRACKS`, alle CC BY) i `Content/Audio/Music` og `Raw/media/music/<navn>.wav`. Ingen stems, ingen stingers. Tempo/BPM er bare aktiv hvis `SetTrackTiming(track, bpm, beatsPerBar, phraseBars)` kalles; uten det er overgangene frie crossfades. Å legge til et spor = ny oppføring i `TRACKS` og et cue-basseng i `CUES`.
- `FS47CosmicAmbient`: ikke-diegetisk teksturlag under musikken (syntese + referanseopptak via `AddReference`). Her kan Space & Planet-lag settes inn som referanser.
- `FS47SoundMixer` / lydverkstedet (`Tools/sound-workshop`, `Scripts/sound-workshop.py`, `recipes.json`, `Content/Audio/Raw/sound_design/bank.json`): **dette er det nærmeste et lagsystem.** En oppskrift er en liste lag (kilde, dB, rate, høy-/lavpass, utsnitt, løkke, kontrollkurve `static`/`pulse`/`signal`/`activity`/`constant`). `US47Audio::PlaySoundMix(Key, Recipe, Use, ...)` starter, `SoundMixFrame(Key, Clarity, Interference, Activity, T)` kalles hver oppdatering (tre input 0–1), `StopSoundMix(Key, Fade)`. `Use` er Receiver/Vhs/PcTransfer/Equipment. En ny kilde legges til i `bank.json`; en ny oppskrift i `recipes.json` (kjør `sound-workshop.py --stage-runtime`). Dette kan bære bed-lag styrt av «Activity»/«Interference», men har ingen stinger-kanal.
- `FS47EncounterSound` (`S47EncounterSound.cpp`): `Cue(Audio, EKind{Presence, Discovery, Hunt}, At, Strength)` spiller tre **syntetiserte** korte tekstur-cuer (0,95/1,35/1,9 s, bevisst «ikke et lydt hopp»), `UpdateRadio`/`Crackle`/`Message`/`Talk` for radioen. Valgfri innlasting av `Raw/sound_design/radio_transmission_original.wav`. **Dette er stedet å koble inn stingers** (Presence = svakt, Discovery = hard/mykt tier, Hunt = bed + stinger). Kommentarene i filen (T16-12: «still screams too much and too loud when Roswell turns up») viser at encounter-lyd allerede er skrudd ned; stingers bør derfor være sjeldne og lave, og jeg har ikke hørt noe av dem.
- `FS47Ambience` (`S47Ambience.cpp`): vind, sirisser, ugle, coyote, regn; `BedIn`-bussen for sirissene. Opptak fra `Content/Audio/Raw/night/<prefiks><N>.wav` (CC0). Ingen skrekklag.

Kort sagt: for «base-lag + stingers» finnes ingen ferdig sluse. Minste vei: (1) base-lag = nytt lag i en lydverksted-oppskrift eller en `LoopSfx` på `Music`/`amb`-bussen, regulert av encounter-fasen; (2) stinger = `Sfx("lic:<navn>", {Dest="sfx"})` fra `FS47EncounterSound::Cue`.

**Hvor kjøpt/privat lyd holdes utenfor Git.** `unreal/.gitignore` ignorerer `Signal47/Content/Licensed/` og `Signal47/Content/Audio/`; `unreal/AGENTS.md` gjentar at `Web`, `WebData`, `Audio`, `Licensed` aldri committes. `web/.gitignore` ignorerer `src/assets/licensed/` (den private klonen `Tombonator3000/SIGNAL-47-assets`, hentes av byggejobben med en lese-deploy-nøkkel). Rot-`.gitignore` har dessuten `*.zip` og `Content/Northwood/` ignorert, med ett unntak for Kenneys `interface-sounds.zip`. Det eneste lydmaterialet som er tillatt åpent, er den frie Kenney-pakken; kjøpte pakker er ikke det. Ikke legg noe fra biblioteket i `Content/Audio` hvis det skal i Git; legg det i `Content/Licensed/...` (ignorert) eller den private `SIGNAL-47-assets`.
