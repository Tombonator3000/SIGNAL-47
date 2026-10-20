# Tredjepartsinnhold i web/

## Seks nye syntetiske stemmer, 20. oktober 2026

`src/assets/audio/ensemble/` inneholder nye fremføringer av spillets eksisterende replikker: Harry Dean Stanton som bensinstasjonsbetjenten, Lauren Bacall som N. Vega, Robert Stack som visesheriffen, David Lynch som suvenirmannen og Vincent Price på den eksisterende T. Vega-opptaksfrasen. `src/assets/audio/serling/` inneholder Rod Serling-parodien for fortellerbeskrivelser og mellomkapitteltekst. Navnene beskriver syntetisk rollebesetning, ikke originale fremføringer av spillets manus eller støtte fra personene.

Referanser er korte utsnitt av offentlige intervjuer og bevarte originalopptak. Stanton kommer fra [WNYC The Takeaway](https://www.wnycstudios.org/story/317438-harry-dean-stanton-singer-actor-partly-fiction/), Bacall fra [WHYY Fresh Air](https://freshairarchive.org/segments/actress-lauren-bacall) og Stack fra [Bob Barrys eget intervjuarkiv](https://bobbarry.libsyn.com/robert-stack). Lynch bruker en bevart [Daily Report fra hans egen nettside](https://archive.org/details/david-lynch-daily-report-2005-2010). Serling bruker [Mike Wallace-intervjuet fra 1959](https://archive.org/details/The_Mike_Wallace_Interview___Rod_Serling), med intervjukontekst fra Rod Serling Memorial Foundation. Price bruker [BBC Archives bevarte Parkinson-klipp fra 1976](https://archive.org/details/twitter-1322516170109558784). Offentlig tilgang er ikke dokumentasjon på en lisens for stemmekloning eller gjenbruk, og arkivuploaderes lisensmerker påstås ikke som verifiserte rettigheter.

Originallyd, stemmereferanser, profilfiler og modellvekter følger ikke spillpakken eller det offentlige repoet. Samme eksisterende VoiceStudio 0.5.6 og festede OmniVoice-modell som under er brukt, med uendrede program-/modellvilkår. De nye filene merkes ikke som CC0. Profil- og kildehash, korte referansefraser, faktiske genereringer, filkontroll og ordprøver dokumenteres i `production/ensemble/`.

## Valgfri syntetisk Kinski-opplesning, 19. oktober 2026

Lydfilene i `src/assets/audio/kinski/` er genererte engelske opplesninger av spillets eksisterende tekst, laget lokalt for Toms parodiversjon. Dette er en syntetisk stemmekopi, ikke originale Kinski-replikker. Den korte tyske stemmereferansen er hentet fra Fitzcarraldo-opptaket med Walter Saxer som er gjengitt i Werner Herzogs *Mein liebster Feind* (1999), via https://www.dailymotion.com/video/x2siao8. Kildelyd og referanseopptak følger ikke spillet.

- Generator: VoiceStudio 0.5.6, AGPL-3.0-only, https://github.com/debpalash/VoiceStudio. Programmet distribueres ikke med spillet.
- Modell: k2-fsa/OmniVoice ved revisjon `c5fdb5ccb189668d56333f77ba2629f4cd7535f4`, https://huggingface.co/k2-fsa/OmniVoice. Det nedlastede modellkortet oppgir Apache-2.0 for kode og CC-BY-NC for modellvektene. Modellvekter distribueres ikke med spillet; genererte lydfiler merkes ikke automatisk som CC0 eller Apache.
- Kilde, referansehash, modellkortets hash, forespørsler og etterbehandling er dokumentert i `production/kinski/provenance.json` og `generation.json`. Tom godkjente innfletting i main og publisering 19. oktober 2026.

## Syntetiske innringere og nyhetsleser, 19. oktober 2026

De 28 lydfilene i `src/assets/audio/radiocast/` er nye fremføringer av spillets 22 innringerreplikker og seks oppdiktede nyhetslinjer fra 1947. Tre voksenroller bruker referanser fra Kyutais uttrykkelig CC0-lisensierte `voice-donations`: `Selfie.wav`, `Butter.wav` og `oldNerd.wav`, ved revisjon `323332d33f997de8394f24a193e1a76df720e01a`. Se [Kyutais kilde og donasjonsvilkår](https://huggingface.co/kyutai/tts-voices/blob/323332d33f997de8394f24a193e1a76df720e01a/README.md#voice-donations). Filnavnene bekrefter ikke donorenes alder, kjønn eller aksent.

Gutten og nyhetsleseren er fiktive stemmer laget med OmniVoice sitt stemmedesign. Ingen ekte barneopptak eller arkivopptak fra ABC følger spillpakken. Originalreferanser og modellvekter holdes utenfor repoet og spillpakken.

Samme lokale VoiceStudio 0.5.6 og festede OmniVoice-revisjon som over er brukt. CC0-status for donerte referanser endrer ikke modellens vilkår; de genererte spillfilene påstås ikke å være CC0. Kildehash, referanseintervaller, design, profil-ID-er, generering og faktisk ordkontroll er dokumentert i `production/radiocast/`.

## Syntetisk Art Bell-radiovert, 19. oktober 2026

Lydfilene i `src/assets/audio/artbell/` er nye opplesninger av spillets eksisterende Night Line-manus. Stemmen er merket som en syntetisk Art Bell-parodi for den fiktive verten Earl. Dette er ingen original fremføring av spillmanuset eller påstand om støtte fra Art Bell.

Referansen er en 4,40 sekunders frase fra Art Bells personlige `ArtFareWell.mp3` på *Artbell.com Commemorative Disc*, Rowland Network Communications, januar 2003, [bevart hos Internet Archive](https://archive.org/details/Artbell.com_Commemorative_Disc). Original og referanse følger ikke spillet. Arkivtilgang er ingen påstått lisens eller tillatelse fra rettighetshaverne.

Samme lokale VoiceStudio 0.5.6 og festede OmniVoice-revisjon som over er brukt. Modell- og programvilkårene er de samme; lydfilene merkes ikke som CC0. Kildehash, profil, innstillinger og faktisk generering er dokumentert i `production/artbell/provenance.json` og `generation.json`.

De 16 ekstra filene i `src/assets/audio/artbell-town/` bruker samme profil og referanse til Earls eksisterende replikker i Caliche. De er nye syntetiske opptak av 150 manusord, uten originalkildelyd i spillpakken. Kilde, generering, filkontroll og ASR er dokumentert i `production/artbell-town/`. De tidligere radiovertsfilene er bevart.

## Kjøpte lydeffekter: Ultimate SFX for Games (Function Loops), 19. oktober 2026

Noen lyder kommer fra Ultimate SFX for Games av Function Loops, kjøpt av Tom (Humble Bundle). Lisensen (Function Loops' EULA) tillater bruk og redigering i spill, krever ikke kreditering, men tillater ikke at lydfilene deles videre. Derfor ligger de ikke i dette repoet: de redigerte utsnittene spillet bruker ligger i det private repoet `Tombonator3000/SIGNAL-47-assets`, som byggejobben for Pages henter til `src/assets/licensed/` (se `sfx/MANIFEST.md` der for kilde, utsnitt og bruk av hver fil). Uten mappa bygger og lyder spillet som før. De står i: suset og knitringen i AM-radioene, TV-brummen på motellkontoret, trykket under advarselen før tapt tid og under THE EVENT, én av lydene ute i mørket, den åpne linja i telefonkiosken, dørene på SARO, nøklene i tenningen, hellingen av kaffe og fremkaller, bryterne i stativet og i rom 6, og telefonrøret i prologen. Pakken nevnes i rulleteksten og tittelskjermens info når filene er med (en høflighet, ikke et krav).

## Tidligere innhold

Alt under er hentet fra Unity-prosjektet i samme repo, med samme lisens som der (se `Docs/THIRD_PARTY_NOTICES.md` i roten). Lyd er konvertert til MP3 for nettleseren.

## Musikk

- "Signal to Noise" av Scott Buckley, CC BY 4.0. https://www.scottbuckley.com.au/library/signal-to-noise/ (`title_music.mp3`)

Stykkene under er hentet fra Scott Buckleys bibliotek 5. oktober 2026, der de ligger under CC BY 4.0 (https://www.scottbuckley.com.au/library/using-this-music/; bruk i spill er gratis med kreditering). Hvert er kuttet til et utdrag fra starten, tonet inn 3 s og ut 12 s, lydnivåjustert (loudnorm, -22 LUFS) og kodet som MP3 96 kbps i `public/music/`. De strømmes når spillet spiller dem (`core/score.ts`). Kreditering står i rulleteksten (`ui/Ending.ts`) og på tittelskjermens info (`ui/UI.ts`).

- "Shadows and Dust", de første 210 s. https://www.scottbuckley.com.au/library/shadows-and-dust/ (`shadows_and_dust.mp3`)
- "Decoherence", de første 230 s. https://www.scottbuckley.com.au/library/decoherence/ (`decoherence.mp3`)
- "The Old Ones", de første 190 s. https://www.scottbuckley.com.au/library/the-old-ones/ (`the_old_ones.mp3`)
- "In Search of Solitude", de første 240 s. https://www.scottbuckley.com.au/library/in-search-of-solitude/ (`in_search_of_solitude.mp3`)
- "Neon", de første 200 s. https://www.scottbuckley.com.au/library/neon/ (`neon.mp3`)
- "Hymn to the Dawn", de første 200 s. https://www.scottbuckley.com.au/library/hymn-to-the-dawn/ (`hymn_to_the_dawn.mp3`)

### Musikk i bakgrunnen hele tiden (16. oktober 2026)

Tom ba om stille musikk i bakgrunnen hele tiden, skummel og meditativ, i ånden til Mike Oldfield, Angelo Badalamenti og David Lynch. Deres egen musikk er beskyttet og brukes ikke; stykkene under har åpen lisens, lest på kildenes egne sider 16. oktober 2026. Hele stykker, tonet inn 2 s, lydnivåjustert (loudnorm, -22 LUFS), kodet som MP3 64 kbps i `public/music/` uten omslagsbilde og metadata, strømmet (`core/score.ts`). Kreditert i rulleteksten og tittelskjermens info.

- Chris Zabriskie, «Cylinder One», «Cylinder Two» og «Cylinder Eight» fra albumet Cylinders (2014), CC BY 4.0 (https://chriszabriskie.com/cylinders/: «free to use in creative projects under a Creative Commons Attribution license», lenket til CC BY 4.0). Filene er hentet fra Internet Archive (`Cylinders-15736`). (`cylinder_one.mp3`, `cylinder_two.mp3`, `cylinder_eight.mp3`)
- Kevin MacLeod, CC BY 4.0 (https://incompetech.com/music/royalty-free/licenses/, standardlisensen er Creative Commons med kreditering), hentet fra incompetech.com: «Floating Cities», «Ossuary 1 - A Beginning», «Ossuary 5 - Rest», «Long Note Four», «Lightless Dawn», «Phantom from Space» og «Night on the Docks - Sax». (`floating_cities.mp3`, `ossuary_1.mp3`, `ossuary_5.mp3`, `long_note_four.mp3`, `lightless_dawn.mp3`, `phantom_from_space.mp3`, `night_on_the_docks.mp3`)
- Shane Ivers, «Signs To Nowhere» (2020), CC BY 4.0 (https://www.silvermansound.com/free-music/signs-to-nowhere). (`signs_to_nowhere.mp3`)

## Låtene på radioen (18. oktober 2026)

Tom syntes de få taktene som ble laget i koden og gikk i ring, var for irriterende. De er byttet ut med fire stykker av Kevin MacLeod, CC BY 4.0 (https://incompetech.com/music/royalty-free/licenses/), hentet fra incompetech.com 18. oktober 2026: «Drankin Song», «Bama Country», «Cattails» og «Anamalie». Lydnivåjustert (loudnorm, -20 LUFS), smalnet til AM (90 til 5500 Hz), mikset til mono og kodet som MP3 40 kbps, 22 kHz, uten metadata, i `public/radio/` (`drankin_song.mp3`, `bama_country.mp3`, `cattails.mp3`, `anamalie.mp3`). De strømmes når en radio spiller dem (`core/radio.ts`), en del av en låt om gangen, og er ikke med i enkeltfilbygget, der taktene fra koden står igjen. Kreditert i rulleteksten og tittelskjermens info.

## Motoren i lastebilen (18. oktober 2026)

Tom ba om ny og bedre motorlyd. Motoren var laget bare av oscillatorer; nå er den to opptak fra Freesound, delt i fire filer, begge CC0 1.0 (lest på hver lyds egen side 18. oktober 2026). Fra Freesounds forhåndsvisning (128 kbps), klippet og mikset til mono. Løkkene er kryssfadet i skjøten, nivåjustert til samme styrke, og lagret som WAV 11 kHz så skjøten ikke får stillheten MP3 legger til; start og stopp er MP3 64 kbps. Alle i `src/assets/audio/` og med i begge byggene (`drive/engineSound.ts`).

- 1978 Cadillac Eldorado Biarritz - Engine Start, Exhaust Idle, Revving and Shutdown, inspire153, Freesound 873856: tomgangen 20,5 til 30,0 s (`engine_idle.wav`), starten 5,6 til 10,0 s (`engine_start.mp3`) og stoppen 46,45 til 48,8 s (`engine_off.mp3`)
- G13-19-Car Steady 30 MPH.wav, craigsmith, Freesound 438001: jevn kjøring sett innenfra, 5,0 til 15,0 s (`engine_drive.wav`)

## Nasjonalsangen på radioen, allemannseie

- "The Star Spangled Banner", korversjon, The United States Air Force Band. Hentet 16. oktober 2026 fra Internet Archive, https://archive.org/details/TheStarSpangledBanner (`Star_Spangled_Banner_choral.mp3`). Siden sier at innspillingen er laget av en ansatt i det amerikanske forsvaret som del av tjenesten, og derfor er allemannseie (public domain) som et verk fra den amerikanske staten; melodien og teksten (John Stafford Smith, Francis Scott Key) er for lengst fri. Air Force Band ber om kreditering, så den står i rulleteksten. Stillheten i begynnelsen og slutten er klippet bort, og lyden er nivåjustert (loudnorm, -16 LUFS), mikset til mono og kodet som MP3 48 kbps, 22 kHz, i `public/radio/anthem.mp3`. Den strømmes når en radio i spillet spiller den (`core/radio.ts`), og er ikke med i enkeltfilbygget.

## Lyder ute i mørket (16. oktober 2026)

Seks korte lyder som spilles langt ute i mørket når spilleren går med lommelykta (`core/calls.ts`). De står i rulleteksten til They Are Here, men er hentet fra originalene på Freesound, aldri fra det spillet. Lisensen er lest på hver lyds egen side 16. oktober 2026. Fra Freesounds forhåndsvisning (128 kbps), klippet, tonet inn og ut, nivåjustert (loudnorm, -16 LUFS), mono, 22 kHz, MP3 56 kbps i `src/assets/audio/`.

- Alien Voice Crack, resourcemoon, Freesound 456249, CC BY 3.0, hele (`alien_voice.mp3`)
- Alien?, superhalo, Freesound 145063, CC0 1.0, hele (`alien_radio.mp3`)
- Animals from Mars.wav, InSintesi, Freesound 372218, CC BY 4.0, 121,0 til 126,6 s (`alien_calls.mp3`)
- Singing alien.wav, InSintesi, Freesound 346209, CC BY 4.0, 11,0 til 17,2 s (`alien_song.mp3`)
- Space Temple SFX Soundscape Fantasy 200731_0026_01.wav, szegvari, Freesound 529414, CC0 1.0, hele (`alien_temple.mp3`)
- War of the worlds Horn 1.wav, JarredGibb, Freesound 244796, CC0 1.0, hele (`alien_horn.mp3`)

CC BY-lydene er kreditert i rulleteksten (`ui/Ending.ts`) og i tittelskjermens info (`ui/UI.ts`).

## Lydeffekter, CC0 1.0

- old dot-matrix printer, viertelnachvier, Freesound 181420 (`printer.mp3`)
- telephonering.wav, transitking, Freesound 15826 (`phone_ring.mp3`)
- ceramic cup shatters on tile floor, geraldfiebig, Freesound 524999 (`ceramic.mp3`)
- desert_wind.wav, DarkShroom, Freesound 645305 (`wind.mp3`)
- Kenney Interface Sounds, https://kenney.nl/assets/interface-sounds (`click.mp3`, `switch.mp3`)
- Kenney Impact Sounds, https://kenney.nl/assets/impact-sounds (`thud_soft.mp3`; fottrinnene `step_concrete0` til `4`, `step_wood0` til `4`, `step_carpet0` til `4` og `step_gravel0` til `4`, laget av `footstep_concrete_00x`, `footstep_wood_00x`, `footstep_carpet_00x` og `footstep_snow_00x` i versjon 1.0, lastet ned 16. oktober 2026, mono, utjevnet og kodet som mp3; `License.txt` i pakken sier CC0)

### Natten, dører og papir (4. oktober 2026), CC0 1.0

Hentet fra Toms egne prosjekter: `Tombonator3000/morbidium` (`assets/lyd/`, kildeliste i `KILDER.md` og `lyd.json`) og `Tombonator3000/Loincloth-Legends` (`public/assets/sound/`, `KILDER.md`). Filene der er laget av Freesounds forhåndsvisning (128 kbps), trimmet, tonet inn og ut, mikset til mono og normalisert. Freesound-siden for hver fil ble åpnet 4. oktober 2026 og viser fortsatt CC0 1.0. Navnene under er filnavnene i spillet.

- crickets, FreethinkerAnon, Freesound 129678 (`crickets.mp3`, morbidium `amb_natt`)
- Buhos.wav, Gamba_Studio, Freesound 447211 (`owl.mp3`, morbidium `ugle`)
- Dogs Barking in Distance_Rural.wav, rvandemark, Freesound 581478 (`dog0.mp3`, morbidium `hund`)
- Distant Dog Bark, qubodup, Freesound 813116 (`dog1.mp3`, morbidium `hund_2`)
- wind_gust_short_sqeeeek.wav, sqeeeek, Freesound 381853 (`gust.mp3`, Loincloth-Legends `vindkast`)
- Squeaky door opened quickly.wav, CastIronCarousel, Freesound 216878 (`door_creak0.mp3`, morbidium `door`)
- Door - Creak.wav, JarredGibb, Freesound 219499 (`door_creak1.mp3`, morbidium `door_2`)
- Metal Door Slam_SoundSmith.wav, Lunardrive, Freesound 48980 (`door_metal.mp3`, morbidium `dorslag`)
- door_close, wjtaylor, Freesound 266682 (`door_close.mp3`, morbidium `dorslag_2`)
- PageTurn.wav, yatoimtop, Freesound 346835 (`paper0.mp3`, morbidium `paper`)
- Page Turn 01, LilMati, Freesound 397548 (`paper1.mp3`, morbidium `paper_2`)

Fjern torden over mesaen lages i kode (rullende lavpasset støy og en lav sinus), etter «far»-varianten av `thunderSyn` i Loincloth-Legends.

Brummen, bærebølgen, telefonopptaket, smellet og motorene lages i kode med Web Audio.

## Fonter

- VT323, SIL Open Font License 1.1, The VT323 Project Authors
- Reenie Beanie, SIL Open Font License 1.1, James Grieshaber
- Oswald, SIL Open Font License 1.1, The Oswald Project Authors
- Special Elite, Apache License 2.0, Astigmatic

Fontene er innebygd uendret. Lisenstekstene finnes på https://github.com/google/fonts.

## Grafikk til Roswell-veien og dineren

4. oktober 2026: seks originalbilder laget for SIGNAL / 47 med den innebygde ChatGPT-bildegeneratoren etter runde 6 i Claudes `ART_BRIEF.md` på `1781093`. Eksakt modellversjon er ikke eksponert. Dette er genererte prosjektassets, ikke nedlastede tredjepartsbilder eller en påstått CC0-lisens. Ingen ekstern betalt assettjeneste er brukt.

- `diner/sign_diner_blank.png`, `tex_counter_laminate.jpg`, `tex_floor_checker.jpg`, `menu_board_blank.png` og `clipping_photo_1947.jpg`.
- `road/tex_gravel_track.jpg`.

Stier er relative til `src/assets/art/`. Prompts, originale kildebaner, hash, eksporter og kontrollnotater står i `diner/DINER_SURFACES.json` og `diner/DINER_ROAD_PROPS.json`. Avisfotoet er oppdiktet for fortellingen og er ikke kopiert fra en historisk avis. Se `ART_ROUND6_DELIVERY.md`. Tidligere kreditering gjelder uendret.

## Grafikk

18. oktober 2026, nye konsepter: åtte bilder i `src/assets/art/concept/night_20261018/` laget med innebygd `image_gen.imagegen`, med tre målrettede korrigeringer i samme verktøy. Eksakt modellversjon er ikke eksponert. Prosjektets egne historiefiler og tidligere konsepter er kildegrunnlaget. Ingen nye eksterne fotografier, andre spills grafikk eller påstått CC0-lisens. Originale PNG-er fra alle elleve løp, fullstendige prompter, filhash og eksportoppskrift ligger i `src/assets/art/production/concepts_night_20261018/`. Se `CONCEPT_NIGHT_DELIVERY.md`. Konseptene er ikke importert som spillgrafikk.

Geometrien og instrument-/bevisgrafikken er laget i kode for dette prosjektet.

4. oktober 2026: 17 nye bilder laget for SIGNAL / 47 med den innebygde ChatGPT-bildegeneratoren etter `ART_BRIEF.md`. Eksakt modellversjon er ikke oppgitt av verktøyet. Dette er genererte prosjektassets, ikke nedlastede tredjepartsbilder eller en påstått CC0-lisens. Ingen eksterne betalte assettjenester er brukt.

- `room/`: `tex_floor_hextile.jpg`, `tex_ceiling_tile.jpg`, `tex_wall_paint.jpg`, `tex_desk_laminate.jpg`, `poster_listen.png`, `poster_saro.png`, `map_new_mexico.png`.
- `ext/`: `tex_concrete.jpg`, `tex_desert_ground.jpg`, `tex_asphalt_wet.jpg`, `sign_sierra_on.png`, `sign_sierra_off.png`.
- `yard/`: `tex_cabinet_metal.jpg`, `sign_service_yard.png`, `label_s03_procedure.png`.
- `sky/sky_milkyway_equirect.jpg` og `brand/logo_saro.png`.

Stier er relative til `src/assets/art/`. Prompts, kildebaner, eksportmål og SHA-256 står i `room/ROOM_TEXTURES.json`, `ext/EXTERIOR_TEXTURES.json`, `brand/GRAPHICS.json` og `sky/SKY_SIGN_PROMPTS.json`. `runtime/manifest.json` knytter WebP-kopiene til originalene (sju fra PR #30, og fra 4. oktober også `lab/sign_blank.png` og `yard/floor_paint_frame.png`). Kartet er en stilisert spillrekvisitt, ikke et geografisk navigasjonskart. Tidligere lyd- og fontkreditering gjelder uendret.

Samme dato: seks miljøkonsepter i `concept/` og `maps/world-overview.jpg` er laget med den innebygde ChatGPT-bildegeneratoren. Eksakt modellversjon er ikke eksponert. Referanser er prosjektets egne spillbilder og tidligere konseptillustrasjoner, ikke nye nedlastede tredjepartsbilder. Fullstendige prompts og filhash ligger i `concept/room-concepts.json`, `concept/exterior-concepts.json` og `concept/additional-concepts.json`. `maps/*-plan.svg` er prosjektets egne redigerbare kartdiagrammer, med PNG-eksporter og kildegrunnlag i `maps/map-spec.json`. Ingen ny tredjepartslisens eller CC0-status er påstått. Se `CONCEPT_DELIVERY.md`.

Runde 3 samme dato: `yard/floor_paint_frame.png`, `lab/sign_blank.png`, `lab/tex_paper_card.jpg` og rom-6-revisjonen `concept/ch4_room6.jpg` er laget med innebygd ChatGPT-bildegenerator, med prompts og kildehash i `yard/FLOOR_FRAME.json`, `lab/LAB_SURFACES.json` og `concept/ROOM6_REVISION.json`. Eksakt modellversjon er ikke eksponert. `yard/vane_b12.png`, `yard/board_r07.png` og `lab/map_field_yard.png` er originale kodebaserte SVG-eksporter for dette prosjektet; redigerbare kilder og målkontrakt finnes i `production/`. Ingen nedlastede tredjepartsbilder inngår.

Runde 4 og 5 samme dato: `annex/tex_floor_vinyl.jpg`, `annex/vending_front.png` og de fire `station/tex_*.jpg` er laget med innebygd ChatGPT-bildegenerator etter Claudes brief. Fullstendige prompts, bevarte kildehash, eksportmål og begrensninger følger `annex/VINYL_TEXTURE.json`, `annex/VENDING_FRONT.json`, `station/STATION_SURFACES_A.json` og `station/STATION_SURFACES_B.json`. Eksakt modellversjon er ikke eksponert. Ingen ekstern betalt assettjeneste eller nedlastede tredjepartsbilder er brukt, og ingen CC0-lisens påstås. SARO-kartets oppdatering er original kodebasert SVG med PNG-eksport; det nyere kildegrunnlaget står under `saro` i `maps/map-spec.json`.

Rom 6s selvstendige geometri og enkle kodeteksturer er originale prosjektfiler. Ingen nye eksterne modeller, fotografier, varemerker eller fonter følger `src/world/Room6.ts`.

Runde 7, 4. oktober 2026: `motel/door_room_blank.jpg`, `window_night_lit.jpg`, `window_night_dark.jpg`, `tex_motel_wall.jpg`, `docs/card_field_1947.jpg` og `letter_paper_1947.jpg` er laget for prosjektet med innebygd ChatGPT-bildegenerator etter briefen på `db47e04`. Eksakt modellversjon er ikke eksponert. Det mørke vinduet er en lysvariant av den bevarte genererte kilden til det tente vinduet. Prosjektets eget motellkonsept er inspisert som stilreferanse. Ingen eksterne fotografier, betalte tjenester eller ny CC0-lisens påstås. Prompts, kildehash, eksportmål og begrensninger står i `motel/MOTEL_DOOR_WINDOWS.json` og `docs/MOTEL_PAPERS_WALL.json`. Papirene er tekstfrie spillunderlag, ikke historiske originaldokumenter.

`MotelFront.ts` har original kodebasert geometri og kodeteksturer. Den bruker også allerede krediterte oppstartsassets via den eksisterende delte bildebufferen. Ingen nye eksterne modeller eller fonter følger modulen.

Runde 8, 4. oktober 2026: `diner/tex_booth_vinyl.jpg` og `diner/tex_wall_panel.jpg` er laget for prosjektet med innebygd ChatGPT-bildegenerator etter briefen på `1a6b1f9`. Hver flate fikk én generering og én målrettet rettelse av repetisjonen. Eksakt modellversjon er ikke eksponert. Ingen eksterne fotografier, betalte tjenester eller ny CC0-lisens påstås. Fulle prompts, valgte kildekopier, SHA-256, eksportmål og begrensninger står i `production/round8-2026-10-04/manifest.json` og `ART_ROUND8_DELIVERY.md`.

`Diner.ts` har original kodebasert geometri og kodeteksturer. Den gjenbruker de fem allerede krediterte dinerbildene fra runde 6. Personene, den oppdiktede avisforsiden, skilt- og menytekst er laget i kode for prosjektet. Ingen nye eksterne modeller eller fonter følger modulen.

Runde 9, 4. oktober 2026: 19 supplerende høydeskisser er laget med innebygd ChatGPT-bildegenerator, med prosjektets egne eksisterende albedoer som referanser. Eksakt modellversjon er ikke eksponert. 38 normal-/ruhetskart er deterministisk utledet fra kildematerialenes semantiske masker og svakt, bandbegrenset støttebidrag fra skissene. De opprinnelige albedoene er uendret. Fullstendige prompts, bevarte generatororiginaler, SHA256, format, konverter og kontrollbevis følger `production/round9_manifest.json`, `production/round9_sources/` og `ART_ROUND9_DELIVERY.md`. Ingen bibliotekteksturer, eksterne fotografier eller betalte assettjenester er brukt, og ingen ny CC0-lisens påstås. Privat materialvisning gjenbruker den allerede krediterte three.js-avhengigheten.

Runde 10, 4. oktober 2026: `docs/photo_1947_master.jpg`, `docs/poster_halley_1986_blank.jpg`, `docs/fanfold_1986.jpg` og `docs/fanfold_1947.jpg` er fire originale, oppdiktede dokumentbilder laget for SIGNAL / 47 med innebygd ChatGPT-bildegenerator; tekniske komposisjonsguider og mål-, sRGB- og JPEG-eksport ved Codex. Ingen ekte avis, personer, merkenavn eller eksternt bildearkiv. Promter og originale kilder ligger i `production/round10_qa/sources/`, sjekksummer i `production/round10_manifest.json`. Eksakt modellversjon er ikke eksponert, og bildene er ikke hevdet CC0. Teksten på plakaten og papirene er tegnet i kode for prosjektet. Se `ART_ROUND10_DELIVERY.md`.

Runde 11, 5. oktober 2026: `road/milepost_blank.png`, `road/witness_post.png`, `road/survey_disk_brass.png`, `road/tex_oldroad_asphalt.jpg`, `flash/flash_eye.jpg`, `flash/flash_hand.jpg`, `flash/flash_face.jpg` og `flash/flash_man.jpg` er åtte originale, oppdiktede bilder laget for SIGNAL / 47 med innebygd ChatGPT-bildegenerator; mål-, sRGB-, PNG- og JPEG-eksport og kontroll ved Codex (PR #64). Promter og originale kilder ligger i `production/round11_qa/sources/`, sjekksummer i `production/round11_manifest.json`. Eksakt modellversjon er ikke eksponert, og bildene er ikke hevdet CC0. Ingen ekte personer, merkenavn eller eksterne bildearkiv. Tallene på milestolpene, ordene på vitnestolpen og stempelet på messingskiva er tegnet i kode for prosjektet. De tre PNG-ene brukes i spillet som WebP-kopier i `runtime/` (`tools/prepare_art.py`). Se `ART_ROUND11_DELIVERY.md`.

Runde 12, 5. oktober 2026: `road/tex_oldroad_macro.jpg` er et originalt, oppdiktet ovenfra-bilde av høysletta (2048×2048, 1 m per piksel) laget for SIGNAL / 47 med innebygd ChatGPT-bildegenerator; mål-, sRGB- og JPEG-eksport og kontroll ved Codex (PR #68). Generatorens kilde var 1254×1254 og er tilpasset til 2048×2048. Promter og originalkilder ligger i `production/round12_qa/sources/`, sjekksummer i `production/round12_manifest.json`. Eksakt modellversjon er ikke eksponert, og bildet er ikke hevdet CC0. Se `ART_ROUND12_DELIVERY.md`.

Runde 13 til 15, 18. oktober 2026: 66 originale prosjektbilder etter `ART_BRIEF.md`. 36 motiver er laget med innebygd `image_gen.imagegen`: portrettene, vindusinteriørene, avisa, pappfiguren, bortføringsbildene, kork og albumpapir, utstyrstegningene, motellparkeringen og delingsbildet. Eksakt generatormodell er ikke eksponert. 30 enkle skilt, silhuetter, rammer, små rekvisitter, testbildet og ikonet er originale SVG-tegninger ved Codex. Rastertekst på skilt bruker Liberation Sans fra arbeidsmiljøet; ingen nye fontfiler er distribuert med spillet. Ingen eksterne fotografier eller andre spills grafikk er brukt, og ingen CC0-lisens påstås. Fulle promter, bevarte generatororiginaler, SVG-kilder og eksportoppskrift ligger i `src/assets/art/production/round13_15_sources/`; mål, filhash og kontroller i `round13_15_manifest.json` og `round13_15_qa/`. Se `ART_ROUND13_15_DELIVERY.md`. Portrettene tas med av den eksisterende portrettinnlastingen. De øvrige er bygget inn 18. oktober (Claude, grenen `art-wiring`): byen, silhuettene, bortføringen, Kit, bevisbordet, albumet, motell-tv-en og delingsbildet. De sju skiltene brukes i utgaven med tekst; de blanke utgavene er ikke med i spillet. Store PNG-er brukes som WebP-kopier i `runtime/`.

Runde 16, 19. oktober 2026: `sky/horizon_night.png` og `sky/horizon_dawn.png` (8192×1024, RGBA) er et originalt, oppdiktet 360-graders bånd av høysletta, mesaene og ryggen rundt spillområdet, laget for SIGNAL / 47 med innebygd `image_gen.imagegen` ved Codex. To genererte paneler på 2172×724 (0 til 180 og 180 til 360 grader) er skjøtt, tilpasset briefens retninger og høyder, maskert til felles alfa og lyssatt til natt og grålysning med Pillow, NumPy og SciPy. Eksakt modellversjon er ikke eksponert. Ingen eksterne fotografier eller andre spills grafikk er brukt, og ingen CC0-lisens påstås. Prompter, generatororiginaler og kontroller ligger i `src/assets/art/production/round16/`, se `src/assets/art/sky/MANIFEST.md`. Spillet bruker radene rundt horisonten i halv størrelse som WebP i `runtime/` (`tools/prepare_art.py`), bygget inn av Claude i `world/backdrop.ts`.
