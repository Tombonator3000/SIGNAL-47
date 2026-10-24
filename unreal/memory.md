# Minne for unreal/

## Highway-kontinuitet, 24. oktober 2026

- PR 153 flettet i acaa99d7. Samme testede H3 installert med 40 hashverifiserte payloadfiler, 46 uendrede Saved-filer og verifisert sikkerhetskopi.

- H3 fra 464078be: Roswell følger eksisterende historieport også i pendlerbilen; lydregisterets camelCase-motornavn brukes med reell lavpassfiltrering og trygg stopp/ny innstigning.
- Natur og én detaljert hovedstrømlinje deler kanonisk road-ramme; SARO-offset (8024, 0, -300), 200 m overlapp, 86 delte komponenter/4816 instanser. RoadFloras fem tomt eksporterte drawbarn måtte aktiveres etter runtimefylling. Ikke aktiver alle skjulte eksportnoder.
- Gamle wiremeshes deler gjerder og kraftledning. Tools/highwaypower/prepare_cpu.py må gjøre g684/g918 CPU-lesbare før cook; runtime fjerner bare høye ledningstriangler. 6 mm gjerdetrådkloner rasteriseres uten egen hardwareRT; nye stolper/kraftledninger beholder RT.
- 1392 selvtester, 7 fokuserte pakkede forløp, fire reelle områdeskifter og 8 vurderte bilder PASS. Natt bekrefter silhuetter, ikke fine bakkedetaljer. Eksisterende D2/H3 RT-varsel 83.107 MiB / 400 MiB ved 20% er urettet. HIGHWAY_FIXES.md/acceptance.json skiller testet kilde, råbevis og levering. Ingen full 110/PSO-runde, menneskelig lytting/input eller helruteytelse.
- Auditfixture må vente på World->bBusy før BeginCommute; ellers beholder åpningsturens ventende StartTrip sin opprinnelige posisjon.

## Mesa Diner, 24. oktober 2026

- Originalt ellevedelt Blenderkit i Tools/mesadiner: fasade/interiør, maskert støvparkering, neonskilt ved fasaden og tractor/reefer. Faktisk root/yaw/floor og originale handlinger/dynamiske objekter beholdes. Dør fysisk åpen; ny fysisk leaf/signfoot, atomisk komplett-kit-fallback.
- D2 fra9ff022a3:320 kilde-,47 native-,1383 selvtester og9 korte pakkede scenarioer PASS, ti sluttbilder vurdert. Kildescene og spillbilder leveres i skrivebordsmappen Signal47 Mesa Diner 24okt. MESA_DINER.md/acceptance.json har faktisk scope; PR152 flettet i61598105 og D2 installert med40 payloadhash/48 uendrede Saved-filer og verifisert backup.
- Ikke legg frontskirting gjennom åpen dør. ClearTimer inne i executing lambda kan destruere callbackens shared-captures; sett ferdigvakt før etterfølgende ticks. Plasser ikke native neonpunkter fåcm fra malt skiltflate; det ga hvite inverse-square-flekker.

## Trafostasjonens omgivelser og nærlys,24. oktober2026

- Tools/substationsite: tre egne kraftmaster på faktisk g873-bakke, eksakte ledere fra gamle busstubber, vedlikeholdsskur, tromler/verktøykasse og kontaktstein. Telefonlinje og eksisterende kulvert er bevart. Original Blend/PBR/3LOD; ingen ny falsk inngang eller oppdragsport.
- Kilde3f3687d5/kandidatP1:222 kilde-,29 native-,1383 selvtester og7 korte pakkede scenarioer PASS. Eksisterende inspeksjon/diskreload, nye modellvinkler og nær/far-lykt er kontrollert. SUBSTATION_SITE.md/acceptance.json har faktisk bevis; PR151 er flettet i c8c4de1f og kandidat P1 installert: 40 payloadhash, 48 uendrede Saved-filer og verifisert backup.
- Lommelyktens native spot kompenserer nå for nært synlig treff under3.5m, med jevn overgang og story/flicker uendret. Faktisk P1:1.472m→74.309cd,5.280m→420cd, begge ThreeIntensity64. Ikke mørklegg teksturene som lokal eksponeringsløsning.

## Caliche Junction Substation, 24. oktober 2026

- Originalt kit i Tools/substation: 10 deler, 3 LOD-er, redigerbar Blend og originale PBR-kart. Native root Three(7890,-1.10,536.20), yaw180, tilgjengelig kontrollfront utenfor stengt yard. Normal fotgjengermodus her er diner, selv om Props og gangdata er road.
- PR150 flettet i8bd33349, kandidatS2 installert: 40 payloadhash, 48 uendrede Saved-filer og verifisert backup.
- Kilde43ab2430 / kandidatS2: 304 kilde-,41 native-,1378 selvtester og seks korte pakkede kontroller PASS. Faktisk Walker-gange, gjerdestopp, øyetrace, journal/service/lampe og diskreload med samme pose/tid er kontrollert. SUBSTATION.md/acceptance.json har råforsøk og begrensninger.
- Walkable tester bare soner; collidere håndheves av Resolve. Test fysisk gange når et gjerde skal verifiseres. STATION01 world-origin er z8000, ikke0. Unngå ugyldige diagnostiske posisjoner. Nær lommelykt kan overeksponere nytt skap; lysbalanse gjenstår.

## Xbox-kontroller, 24. oktober 2026

- Xbox-kilde `3d016a07`, kandidat X4: 1373 selvtester, elleve native kernel/SDL/Slate-opptak i målte scopes og 16 målrettede regresjoner PASS. PR149 er flettet i fda74798 og kandidat X4 installert med 40 verifiserte payloadfiler og 48 uendrede Saved-filer. GAMEPAD.md har kontroller, bevis og uverifiserte handlinger.
- Tom ber om kortere testløp 24.10: velg nødvendige, målrettede kontroller etter endringens risiko. Den komplette 110-runden for Xbox ble stoppet etter 16 beståtte scenarioer; ikke kall den fullført. Bruk parallelle agenter til kode/gjennomgang; én tung jobb på maskinen består.

## Skyoppgaven sluttkontrollert, 24. oktober 2026

- Tom ber Codex fullføre Claudes volumetriske skyer etter limit. Skykode f55b760c/04d98231 er allerede integrert; ved overtakelsen gjenstod cache og levering på samlet natur/film/sky. Gammelt opptak under Signal47-sky har andre shader keys og kan ikke dokumentere ny pakke. SKY.md har faktisk sluttbevis.
- Ny samlet cache og sluttpakke er kontrollert: 84/84 opptaksbolker, 4329 SPC, 620 binære PSO-er, 285 runtime-graphics, 1328 selvtester, 110/110 unike scenarier/naturkontroller og 52 visuelt vurderte bilder. Lokalt flettet og installert med 40 payloadhash, 48 uendrede Saved-filer og backup. Skrivebordsmappen Signal47 skyer og lys har bildene. SKY.md har gjeldende bevis og målegrenser.
- Naturmulighetene Canyons of Utah, New Mexico-geologi, fotografisk fjernprojeksjon og Mesh Terrain er lagret som åpne vurderings-/prøvepunkter i todo.md, uten nedlasting eller migrering.

## Caliche natur, 24. oktober 2026

- Pakke/runtime/kilde/native/installasjonsbevis finnes i Tools/calichenature/acceptance.json og CALICHE_NATURE_PASS.md. screenshots.sh bruker starhole.log/png to ganger; første scenario er derfor tatt om med eget navn, uten å skjule rårapporten med avvist antallsport. Selve 110-scenariorunden bestod på 52,3 minutter.
- Tre originale Blenderkit i Tools/calichenature og calichestreet/backdrop, 35 597 deterministiske HISM-forekomster,36 mineralfliser,12 prototyper med tre LOD-er og 103 550 trekanter geologi. Native/kilde19ed326d, pakke de38da9747d9680c9788414688de9f641f09086bc1c936a38a8685619bad7ea8. Sky 04d98231 og core/film f9e1d4aa integrert. 110/110 full regresjon, 11 funksjonsprøver/1328 selvtester og 33 visuelt vurderte bilder PASS. Lokalt flettet i core 26dbb3df og installert med 40 payloadhash/51 uendrede Saved-filer. Se CALICHE_NATURE_PASS.md.
- Store terreng-UV-er trenger FP32: les faktisk render-FBX UV/RGBA tilbake, ikke bare importflagg. HISM-materialene må bruke used_with_instanced_static_meshes=True før rekompilering/lagring/cook; manglende SMU kan gi grå fallback selv med riktig materiale og teknisk mesh-PASS. Runtime avleser de faktiske kokte LOD-grensene og instanskomponentene. Beskyttet editorbool er fortsatt UNVERIFIED.
- Geologisk UV/Winding/veiklaring er egne porter. OldRoad-beskyttelsen kan lage urealistisk rampesilhuett selv med korrekt kollisjon/klaring; faktisk spillerbilde må avgjøre kunstnerisk form. CN5 beholder dette som kjent stilbegrensning, med lokale bergbrudd/erosjon.

## Filmfiltre, Codex 23. oktober 2026

- Tom bestilte alle filmuttrykkene med særlig vekt på Carpenter: Carpenter 35 mm, Desert 16 mm, Super 8, forbedret VHS og Clean. Implementasjon og faktisk teststatus føres i FILM_LOOKS.md. Nye High/Ultra-profiler får Carpenter; eksisterende eksplisitte valg beholdes.
- MPC_Web `Vhs` må forbli blend 0..1. Den brukes også i untonemapped-materialer, himmel og verdensliv, og SceneCapture setter den midlertidig til0. Nye enum `PictureMode` er separat. Styrke0 setter Vhs0 og tonepasset respekterer capture-gaten. Ikke legg mode2..4 i det gamle feltet.
- Generer bare filteret med S47_ONLY=picture etter editorbygg; film_tone.hlsl og MPC-tillegg må inn i cook. Bruk private Web-filer. Tilgjengelighet fryser korn og stopper flimring/registreringsuro/båndjitter. Ingen fps-lås er del av effektene.
- Sluttkode 8ba39556 inkluderer fellesgren 04517414 (nye antenner/radio). Sluttpakke i ~/scratch-bm/Signal47-film-looks/candidate-core: 1298 selvtester, ni funksjonsprøver, ti kamerakjøringer/36 bilder, MPC-readback, ekte settings-diskomstart og cache-GUID/213 graphics PASS. Samme binær og fem containere hashkontrollert mot alle kjøringer. Carpenter kostet +0,384 til +0,487 ms i korte High/1440p-vinduer; kontrollrom p95 var over 16,667 ms også i Clean. Claude har gjennomgått kode/bilder. Play.sh bruker egen player-profile; ingen installasjon eller publisering. FILM_LOOKS.md har UNVERIFIED for menneskelig temporal komfort, HDR/Deck og ny full-world-PSO-matrise.


Beslutninger og fallgruver. Dagbok hører hjemme i log.md.

## Caliche konseptpass, 24. oktober 2026

- Ni faktiske visuelle iterasjoner er levert og installert. Se CALICHE_CONCEPT_PASS.md og Signal47/Tools/calichestreet/acceptance.json for kilde, pakke, begrensninger og modellprioritet. Sluttpakke 6103dd97769d, native f63aba3a, C++/config 266f6fc2.
- Radio og butikk har slitt mineralpuss, malte skilt, treverk og variert fysisk innredning. Lokal grus/asfalt/jord har myke vertexalpha-kanter og rolige avbrutte spor. Sju fjerne mesaformer og lokale sky-/lysjusteringer gir mer dybde. Originale gulv, markeringer, ankre, klokker og samtlige 27 antenner bevares.
- Legacy FBX-reimport kunne beholde feil mesh-seksjonsbindinger etter materialsporendring. Egne genererte Caliche-/Station-mesher opprettes ferskt på samme stier; kontroller faktisk seksjon mot source used_materials, ikke bare materialspornavn. VertexColor REPLACE og eksplisitt GLB COLOR_0 alpha er verifisert. Dither-fades bruker teksturalpha ganger vertexalpha uten å endre original bakkekollisjon.
- Bakgrunnens gamle innbakte nattfjell skjules bare lokalt ved full Caliche-vekt og faktisk synlig ny bakgrunn. Alle opprinnelige synlighetsflagg gjenopprettes ved områdebytte eller fallback. Radioens punktorb er fjernet; innelys følger fysiske armaturer, fasadelys følger eksisterende døgngrense. R9-printkort og metrisk tre-UV endrer ikke pivoter eller adgang.
- 42 faktiske spillbilder visuelt gjennomgått, 110/110 komplett screenshots.sh, 11 runtimeprøver/1305 selvtestsjekker, fotopilot og tre horisontprøver PASS på samme uendrede pakke. 249 native kontroller, 239 filhasher og fire portable Blender-kilder PASS. 40 installerte payloadhasher og 51 uendrede Saved-filer. Dette er automatiske kamera-/API-prøver; menneskelig input, lytting, fullnatt og helruteytelse er fortsatt UNVERIFIED. Ingen ny ekstern publisering eller push/PR.
- Leveranse: ~/Desktop/Signal47 Caliche konseptpass. Fire originalblends med teksturer/eksporter, offline SPILLBILDER.html, LES_MEG_KONSEPTPASS.md og pakkehashbundne bevis. Tidligere antennekilde SARO_Array.blend og bensinstasjonsoriginal bevares på sine opprinnelige leveringsstier.

## Skumlere anomaly-pilot, Codex 23. oktober 2026

- Tom ba om sterkere uro etter integrert `ed061aca`. Gjeldende pass står i `ANOMALY_SCARIER.md`; førsteleveransens rapport er historikk. Korte observasjoner i verdensbildet, ingen ekstra fotokvittering, ingen «pappen lever»-stråmann. Bevis og plausible uavklarte valg ligger i film/notatbok.
- N02 forklarer kveldslampen, men ny nattfilm har et uavklart vindusomriss. A02 lagrer ankomstretning og en andre usett plassering nærmere veien; ny film har en ekstra form. P01 følger faktisk gange med nærmere printer, tier ved stans og blinker ON AIR i tomt studio. Disse tre designreglene bøyes bevisst for Toms «spenning foran regler»; harde klokker/kanon beholdes.
- `anomaly.DreadVersion=2` i save v3 bevarer film-ID/klokker, fase/retning og P01-slutt. Tidligere hørt P01 avsluttes ved migrering, uten reprise. Nye former er faktiske skjulte eksponeringsmesher med fri sikt og vellykket render, ikke bare metadatamerker. Tekst røper dem først etter fremkalling. Gamle foto beholdes.
- `AS47Web::AdoptNode` bevarer bare posisjon; bruk `FS47Obj::SetScale` for native deler som senere flyttes/roteres, ellers blir de enhetsstore. Nye cue-kilder og ambience-gate må eie egne lydnoder og resettes ved modal, stor scene, telefon, områdebytte, last og tittel. Ikke endre opprinnelige bussnivåer eller stemmer.
- Sluttspillkode `01d92bd4`, core206afd9b, pakke6 og 1293 selvtester PASS. Hele samlingen på siste pakke: åtte pilotscenarioer, High/VHS og Low, ekte gammel pilotmigrering, 7/7 historieregresjoner og to gamle kameramotiv. Tolv nye hovedbilder, før/etter og Low manuelt gjennomgått. Menneskelig input/lytting/frykt og fullnatt UNVERIFIED. Evidens i `~/scratch-bm/Signal47-anomaly-scarier`. Én tung jobb, ingen stopp av Tom og ingen installed-game-grace. Ingen push/PR/installasjon; Claude integrerer.
## Takeaway etter innkjøringen, Codex 24. oktober 2026
- Samlet verifisert pakkekilde `f83f3dc9` med core `92dc7a47`: 1298 selvtester, 33 vanlige bruksforløp/21 ferske matlastinger, begge biler, 48 inngangspasseringer, burgerrute og 3/3 korte regresjoner PASS. Seks pakkehasher uendret, bilder gjennomgått og midlertidig Content/OpenOrder ryddet. Nyere core `6614cd90` er ikke i pakken; Claude integrerer. Ingen push/PR/installasjon.
- `FS47Takeaway`, eid av `FS47Evening`, holder én porsjon pose/kaffe. Faste steder: `diner`, `desk`, `annex`, `hood`, `passenger`, `bench` (dinerens hagebenk, ikke kjemilabbens wetBench). Bilplasser følger bilens kanoniske pose. Ingen fri fysikk eller nye gangkollidere.
- Save får additivt `evening.takeaway` versjon 1 med `mode` og `spotId`. Pakken er sannhetskilde for kjøpt/utlevert/spist, også mellom UI-callback og neste Tick/autosave. Gammel utlevert mat blir båret; ukjent plass gir den tilbake til hånden. Spist mat kommer ikke tilbake.
- `EatTakeaway()` setter konsumering atomisk uten å endre dialogpeker, ventende svar eller dokumenter. Spisevalget finnes etter D01 ved skrivebord/annex, med plass til hele 720 sekunder før historiens neste reserverte grense. Tid går bare fremover. Sted, konsumering og tid committes før save; firesekunders fade er transient og gjentas ikke etter load. Ingen måltid under telefon, kritisk hendelse eller finale.
- Inaktiv matproxy må ha NoCollision, ikke bare tom Label: Inter.Pick stanser ellers ved første usynlige mål. AdoptNode normaliserer også meshskala, så målvolumets skala settes etter adopsjon. Holdt mat er uten kollisjon og skjules under kamera, lykt, modal/notatbok og kjøring.
- Den gamle workTable-queryen omsluttet matmålene ved 0,78 m. Den følger nå bordplaten som et 1 cm plan ved 0,775 m, med opprinnelig fotavtrykk/ID/arkivhandling og uendret gangkollider. Vanlig ray/fokus/bruk må fortsatt åpne arkivet. Dinerens gamle disktallerken skjules mens samme porsjon holdes; ingen dobbel servering.
- `-S47EveningTakeaway` og `-S47TakeawayEatPlaced` tester vanlig interaksjonskjede og ferske disk-/verdenslastinger. Modalskjermbilder må bruke sanntidsticker: Story.After står stille mens paneler pauser spillklokken. Rapport og faktisk sluttstatus: `EVENING_TAKEAWAY.md`; egen kandidat/evidens i `~/scratch-bm/Signal47-takeaway`. Menneskelig input, lytting og fullnatt merkes fortsatt UNVERIFIED.

## Åpningens biler og SARO-inngang, Codex 23. oktober 2026

- Pakket kilde `66d2b5a6`, core `04517414`, retter Toms tre åpningsfeil fra `ed061aca`. Rapport og reproduksjon: `EVENING_VEHICLE_FIXES.md`; kandidat/bevis: `~/scratch-bm/Signal47-opening-vehicles`. Nyere core-endringer er ikke en del av denne testpakken.
- Skjul HELE SARO-kjøreriggens Group under personbilbruk; selvstendige cab-/dørbarn blir ellers stående igjen. Wagonen bruker eget førerøye, komplett interiør og egen hengslet dør. Gjenopprett rigg og serviceførerøye ved ankomst.
- `fairheaven_sw84` er delt med props. Importens opt-in `keep_complete` bevarer standardmodellens 19 461 trekanter; den interaktive bilen bruker `_body` (18 244) og `_door_driver` (1 217). Regenerer/importer alle tre før cook; genererte Free-assets skal ikke committes.
- Kanonisk `SetParkPose` nuller pitch/roll og hjuldropp. Bare nøyaktig gammel originalpad repareres på load; andre serviceposer bevares. Utrygg/manglende personbilparkering bruker besøksplassen `(4.5,-0.62,14)`. Walkerens orienterte bilboks følger samme pose, slås av når skjult og slås opp på ID fordi dørbladene endrer indeksene.
- En lukket østdør kunne fjerne eneste gangsone under spilleren. Avvis manuell terskellukking mens spillerens fotavtrykk overlapper. Gammel ugyldig save må repareres FØR Flow-lasterens Walkable-port, ellers når den aldri reparasjonen. Åpne kun berørt terskel og bevar posisjon/klokke.
- `-S47EveningVehicles` og `-S47EveningEntrance` gir 1286 selvtester og 48 walkerpasseringer før/etter D01, parkert/flyttet, fire ferske disk-/verdenslastinger og gammel ugyldig terskelsave. Hel burgerrute og seks gamle historiehopp PASS på samme pakke. Bilder visuelt kontrollert, seks pakkehasher uendret. Dette er API-styrt normal bevegelse, ikke menneskelig tastaturtest. Content-lenker/OpenOrder ryddet; ingen installasjon, push eller PR.

## Anomaly-pilot, Codex 23. oktober 2026

- `codex/anomaly-pilot` implementerer N01/N02/A02/P01 på den flettede kveldstarten. Førsteleveransen er dokumentert i `ANOMALY_RUNTIME.md`; den senere skumlere runden står i `ANOMALY_SCARIER.md`. Senere kontor/trafo/VHS, bygningsløft og fullnatt er fortsatt egne leveranser.
- Gjenbruk kamera/instantalbum, docs/notatbok og authored N01-manus. Undersøkelser har separate referanser, eksponeringer, reviderbare vurderinger og fysisk oppfølging; `TownSeen` blir aldri automatisk et bevist funn. Tidlige paneler røper ikke P01 eller senere figurplassering.
- Save v3 får `anomaly` versjon1 og rutinekontrollpunkt før survey. Arkivets faktiske 17:10-JPEG lagres med saken som egen ID/url/clock, så senere programversjoner ikke bytter gamle referansebilder. Albumets `unavailable` følger en manglende original gjennom disk og verdenslasting; erstatningskort er aldri fotografisk bevis.
- Ett eksisterende pappobjekt har vindus-/studioposisjon med samme prisetikett og splitt. Vinduets høyde kommer fra faktisk byggproxy, ikke skrånende terreng. Siktkontroll følger albumets regler for synlige opake flater; usynlige hitboxer sperrer ikke foto. Ikke hardkod meshnumre.
- Tidlig kamera fullfører ikke B-12/mørkerom; rutinekjøring utløser ikke ch5. 04:55, Earl 04:00, P13 uten anomalikvote, historisk 1947 og 47 sekunders finale er bevart. Den første leveransen bøyde ingen designregel; den skumlere runden over gjør det uttrykkelig.
- Sluttspillkode 1d3baeb0 og separat testverktøy c6b4f023; core 609a9c6d innflettet. Pakke4, 1278 selvtester, hel pilot i High/VHS og Low/Clean, feilfilm, migrering, 7/7 kortregresjoner, direkte/burger med stemmer og eksisterende TV-opptak PASS. Fire sluttbilder visuelt kontrollert. Statisk Caliche 1440p p95 7,921 → 7,967 ms (+0,58 %), avgrenset av/på-kontroll. Bevis/proveniens i ~/scratch-bm/Signal47-anomaly-pilot og ANOMALY_RUNTIME.md.
- Én tung UE-prosess, 10 GiB og ingen installed-game-grace. Kilde, pakket kandidat og installert standard holdes adskilt. Ingen push/PR/installasjon, Claude integrerer. Menneskelig input, lytting, uro og fullnatt må merkes UNVERIFIED inntil faktisk prøvd.

## Kveldspakkens 109 stemmer, Codex 23. oktober 2026

- Produksjon a4626053 er koblet til runtime i b7faff35/6ade7a94/3345f01f/9d638a0b. Streng import må kjøres med original-WAV-er; den sjekker hash, målte tider, roller og alias før skriving. 109 originaler gir 110 cues, D61 deler første D01-fil. Generert Audio/SoundWave skal aldri committes.
- Host bruker receiverfilter og RadioBus, Dale phone/walkie/radiofilter, servitrisen panner ved sin verdensposisjon og SfxBus. Modal replikk begynner ved målt talestart og blir stående til Continue; radioens caption bruker målt start/slutt og lydklokke. Lagring starter gjeldende avbrutte replikk fra begynnelsen; manuspeker, betaling og svar beholdes.
- Ikke åpne samme aktive samtalepanel om igjen: UI-lukkecallback stopper stemmekøen. ShowDialogue er nå idempotent når panelet er åpent. Hele karakterpanelet er merket Unread for generell beskrivelsesoppleser, også ordre-/valgtekster, så en separat «No recording»-boks ikke vises over ekte karaktertale.
- Sluttpakke 9d638a0b PASS: 1258 selvtestsjekker, hel burgerrute med lyd (281,3 s kontroll, 36 ulike fullførte klipp med faktiske samples/RMS/peak), to disk-/verdenslastinger, radioavbrudd, lydpause/resume og kortregresjon 6/6. Fem rutebilder og D01-transkripsjon visuelt kontrollert. Rapport og reproduksjon i EVENING_AUDIO.md, evidens/kandidat i ~/scratch-bm/Signal47-evening-audio. Content-lenker/OpenOrder ryddet. Ingen push/PR/installasjon eller generert lyd i Git.
- D01_FOOD_002 (innledende «I») og D01_BRIEF_005 («observations») brukes, men krever Toms lytting. Automatisk nivåbevis er ikke menneskelig lytting. Senere kontor/trafo/VHS og fullnatt er fortsatt egne oppgaver. Claude integrerer.

## Kveldstart og tidslinje, Codex 23. oktober 2026

- `codex/evening-runtime` implementerer Toms trinn 1 og 2 i Unreal. Generert manusadapter, egen personbil med delt kjørerigg, valgfri diner/mat, engangsankomst 18:00 og D01 kun i telefon. Tre overleveringspapirer gis én gang; den eksisterende rack-/survey-rutinen fortsetter. Senere kontor/VHS/trafo er datadrevne porter. N01/N02/A02/P01 er siden implementert som egen anomaly-pilot, se statusen ovenfor.
- Nye saker har `v=3`, `timelineVersion=2`, residual 00:13:47 og kapittelminimum 02:20/03:20/04:10. Midnatt krysses på en sammenhengende akse. Gamle saker beholder eksplisitt legacy-versjon og 02:13-referanser. 1947-opptaket og finalen 05:29:00 til 05:29:47 er urørt. Hele gammel/ny-tabellen og komprimeringsreglene står i `EVENING_TIMELINE.md`.
- `printoutAt` er nå faktisk hentetid, separat fra solve/event. Gamle saves lagret aldri den tiden: en gjenopprettet utskrift merkes derfor TIME NOT RECORDED / reconstructed legacy copy. Fotografier omskrives ikke. Ikke oppgi en rekonstruert hentetid som original.
- Kveldsradioen har egen vert og separat mottakerkø. De 109 takesene finnes nå og importeres strengt med hash-/timingskontroll; se `EVENING_AUDIO.md`. Dale går gjennom receiverfilter, host gjennom RadioBus, servitrisen er posisjonell. Eksisterende countrymusikk har musikkrom mellom innslagene. Stemmeimporthjelpen produserer ingen stemmer. Nye rolleopptak virker uavhengig av generell Kinski-opplesning.
- Kveldshimmel og native sol bruker `Daylight()`; historiens `Dawn()` er fortsatt bare morgen. Stjernene holdes borte gjennom tidlig skumring. Destructor skal bare frigjøre egen lyd: `Evening::Stop()` berører levende verdensnoder og skal ikke kalles under sen UObject-purge.
- Bygg/sluttpakke og 1254 selvtester PASS. Fire pakkede åpningsruter, disk-/verdenslasting, native telefonbruk, gamle/nye kapittelhopp og legacy-migrering med bevart bildestempel/bytes er prøvd. Siste kode 3d321da1 inkluderer besøksparkering fri av annexet og ingen gammel 17:20-returtekst ved 18:00. Fem siste bilder og hel burgerrute er PASS. Resultater/proveniens/kandidat i `EVENING_RUNTIME.md`; menneskelig fullnatt/lytting/ytelse er UNVERIFIED. Ingen push/PR/installasjon. Claude integrerer; standardstarteren er bevart.


## Grafikkpass levert, 23. oktober 2026

- Grafikkens WIP 30d407be fortsetter 64d943e7/be7c89e5; core ae95b09a med Props og nyeste dokument-core 9d7571e2 er innflettet. Props-modul/config og installert spill er urørt. Se LIGHTING.md for siste faktisk målte kandidat og historiske avgrensninger.
- Bundlen er levert som S47_Signal47_SF_VULKAN_SM6.spc, 92 902 byte, 4215 stabile PSO-er. Full 38 × 6-matrise med én cook/frosne nøkler er komplett. Sluttcook gir 599 PSO-er: 222 graphics, 310 compute, 67 RT. Pakket runtime åpner samme GUID og behandler 222 bundled graphics; ni cachebeviskontroller PASS. Nye render-/material-/Props-baner krever ny cachevurdering/opptak.
- Alle 9243 cooked render-/asset-/shaderfiler er byteidentiske mot opptaket. Global.shk er byteidentisk; Signal47.shk har ulik serialiseringsrekkefølge/filhash men nøyaktig samme 21 184 logiske tupler/outputhasher. Ikke kall metadatafilene byteidentiske. Frosne nøkler og recordings er urørt; se static/final-cook-equivalence.json.
- S47Theme-lastefasen dekker kompilering/oppvarming, med spill-/testklokker ved READY. Sluttpakke kald READY 11,337 s, varm 5,594 s; sluttregresjon bekrefter tittel/pause/notatbok/kjøring/THE EVENT og cap30 med 285 intervaller på 33,333457 ms i snitt. Første 10 synlige sekunder 1111/1095 rammer, maks 31,585/31,888 ms, ingen over 33,333 ms. Før kald 455 rammer/maks 1296,925 ms. Nullpunkter er verdensstart og HUD/READY-proxy, ikke OS-start/fysisk presentasjon.
- 118 materialprofiler, seks glass og fire CRT-valg må regenereres med S47_ONLY=materials før integrasjon av eldre generert Content. Masters-only reparerer grafene og hopper over MI-parametrene. StaticSwitchParameter krever True/False; streng cookkontroll avviser standardmaterialfallback selv ved UAT exit0. Web/Audio/Licensed/Free-assets committes ikke.
- Seks GLB-spor med én kildematerialoppføring må bindes på rack/diffusere. Native DP-80/CB/3278 bruker EmissiveFactor/Strength, Web-materialer Brightness/EmissivePower. Gate emisjon og bevar PBR/maps. V5/V9 er supplert av sluttpakkens komplette sju indikatorprøver og 36 fasebilder, inklusive selvtest (1220) og faktisk terminalfront av/på. Matrisens gamle dark-bilder er ikke strømbevis; after/final-state-correction og sluttfasetesten er korrekt retest.
- Alle 20 natt + 20 daggrybilder er visuelt kontrollert; 40 par og originalhash/galleri ligger i compare/index.html. Etterbilder har innflettede Props; de er ikke isolert lysforskjell. PreviousNight-kjøring og alle åtte lengre High-prøver er PASS; siste seks regresjoner og begge identisk-geometri fyllpar er også PASS. Korte High-prøver viser 90,1 fps eller mer i gjennomsnitt, men p95 opptil 21,772 ms; lengre natt/daggry viser 93,81–106,73 fps og maks p95 18,142 ms, så det er ikke låst 60 fps for hvert intervall.
- Statisk diagnostisk tittel må ryddes før shutdown. V2s to SIG11 er forkastet; v3s tidligere feilsekvens består også med logPSO. Én tung Unreal-jobb, minst 10 GiB og ingen stopp av installert spill. Tom avsluttet selv før 11:06, deretter kjørte matrisen. Midlertidige Content/Audio, Content/Licensed og Content/Free-lenker er fjernet med kildemappene bevart; OpenOrder-logger er gjenopprettet. Kilde og bundle er levert i 684bc142 på port-gfx.

## Gameplayretning (2026-10-23)

- Tom sluttet seg til aktiv anomaly spotting, mer praktisk bruk av Caliche og gradvis UFO-/alienopptrapping, og ba om en utdypet plan. Se `ANOMALY_PLAN.md`. Detaljene er anbefalt design; N01/N02/A02/P01 er siden implementert på `codex/anomaly-pilot`, se ANOMALY_RUNTIME.md.
- Tom ba senere om tidligere start, ny trafostasjon i ørkenen og bedre bybygninger. Planen fikk 18:00-start 13. april og residual 00:13:47 den 14. april, i stedet for 02:13:47. Tidslinjen er nå implementert på `codex/evening-runtime`; se den nyere statusen ovenfor. Finalen 05:29 beholdes, og historiske 1947-klokkeslett skal ikke masseflyttes.
- Tom valgte deretter spillbar innkjøring før skiftet: lystig radio gjennom Caliche, valgfritt matstopp i dineren, videre til SARO der skiftet begynner 18:00. Innledningen har egen fase uten løpende skiftklokke, slik at matstopp ikke gir tidstilbakesprang. Tidlig dinerbesøk aktiverer ikke ch5/P13/Ward. Reisekjøretøy og servicepickup holdes adskilt.
- N01s grunnmateriale finnes nå i overleveringsmappen på SARO; innkjøring blir ikke en obligatorisk papirjakt. Materialet brukes mot utskriften etter midnatt. Dokumentert byreferanse må finnes også når matstopp hoppes over. Earl har Night Line 00:00–04:00, ikke kveldsvakt; ny datoakse, radiosending, butikker, NPC-er og totimerskravet må tåle midnatt.
- Tom presiserte at Kinski/Dale KUN er på telefon, radio eller walkie-talkie. Overleveringen 18:00 er en telefon etter at Dale har dratt; ingen fysisk Dale-figur eller Kinski-forteller på VHS. Egen rollebasert Dale-dialog må skilles fra generell opplesning. Ward beholder rollen som vakthavende.
- Manuspakken i `Narrative/EveningShift` har 61 scener, 45 valg, 10 dokumenter, 39 UI-tekster og et 72 sekunders VHS-manus. 110 talecues bruker 109 unike takes fordi et fast Dale-fragment gjenbruker første overleveringslinje. Engelske JSON-moduler er sannhetskilde, med generert lesemanus, stemme-CSV, opptaksmanifest og undertekstgrunnlag. Åpningens runtime er integrert på `codex/evening-runtime`; senere faser er porter. De 109 takesene er produsert og koblet til runtime; VHS står pending.
- `build_pack.py --check` validerer referanser, flagg, kanaler, medietidsdekning og opptaksalias. 14 offline forfatterruter/porter er prøvd, inklusive matvalg, kontor uten VHS og faktiske kontrollkrav. Dette er ikke ekte kjøring eller menneskelig spilltest. Ingen ny stemmetjeneste er installert.
- Ny D02 bruker eksisterende låste Operations office-dør: en fysisk, merket reservenøkkel finnes under arkivundersøkelsen. Kontoret viser Dales hemmelige UFO-/alienresearch gjennom utklipp, filer/kontaktark og faktisk bevegelig VHS/TV med lyd. Ett feltopptak gir konkret kontroll ved trafostasjonen. Bevar Noras nøkkel, den brede 1947-avisen og P13. Dale er ikke automatisk skurk, allvitende eller et svarende avtrykk.
- Gatetelefonens to hendelser og dinertelefonens Ward-samtale har forskjellige krav. A gjennom byen er ikke C ved finalen. Nye byankomster må ikke starte ch5 eller den gamle veien.
- Første pilot bruker N01/N02/A02/P01 med kveldens referanser. All Night anbefales fra 04:10, slik at det er mer tid i byen og mulighet for lysbølgen før 04:55. Gamle kapittelminimum og underhendelser må retimes samlet.
- Ny trafostasjon, Caliche Junction Substation, skal ha en vanlig feil/kontroll på første tur (T01) og en annerledes oppfølging senere (T02). Den er ikke STATION 01, og SAROs K3 blir ikke byens strømnettbryter.
- Alle bybygningers utvendige skall skal forbedres, med første sammenhengende gatebit ved bensinstasjon/suvenir/studio. Diner, sheriff og vaskeri følger med nødvendige rom; øvrige bygg får samme utvendige kvalitet. Bevar egne Blender-kilder, ankre og interaksjoner. Bruk eksisterende konseptbilder som visuell retning; material-/lyspasset alene oppfyller ikke bygningsbestillingen.
- Pilotens 20–30 minutter gjelder avgrenset byinnhold med bevis gjennom lagring og tydelig testkutt. Kveldsoverlevering, trafo og mellomkapitler kommer i tillegg og må måles. Se ANOMALY_RUNTIME.md for pilotens senere runtimekontroll. Menneskelig opplevelse og full spilletid er fortsatt UNVERIFIED.


## Native belysning (2026-10-22)

- Videre belysning er bare Unreal. `S47Lighting.cpp` gjør eksportens lampetilstand om til punktlys og spotter. Historienes SetFlood-API er beholdt; materialenes flood, hemisphere, FogExp2, additive lyskjegler og vindusprojeksjoner er av. Ingen Web/Unreal-bryter. Eldre lysnotater nedenfor beskriver porten før denne endringen.
- Lumen bruker Vulkan hardware ray tracing på RTX 3080 når tilgjengelig. Måne, skylight, levende daggry, native volumetrisk tåke og skygger fra lommelykt/billykter erstatter fylltermene. Førerhusets gamle himmelfyll er fjernet; instrumentgløden er svak. Fast eksponering, ACES og VHS/Clean beholdes.
- `S47_ONLY=materials` i import_web.py regenererer Base og Mat uten å skrive Tex, Geo, art eller WebData. Kreves for eldre generert Content. NativeLighting har asset-default 0 for delte historiske checkouter, men denne runtime setter alltid 1. Bruk private materialkopier ved parallelle jobber.
- Low bruker direkte lys, High/Ultra Lumen og volumetri. High/Low bruker 60 % intern oppløsning med TSR; Ultra 100 %. Se `LIGHTING.md` for faktisk ytelse og sammenligningsbilder, ikke trekk fps-konklusjoner fra kvalitetsnavnet.
- Rettelsesrundens High/1440p med TSR fra 60 %: rom 13,619 ms / 73,4 fps, gård 10,466 ms / 95,5 fps, p95 15,330/11,527 ms. Ingen andre tunge prosesser i målevinduet. Low/Clean 90,0 fps i rommet, Ultra 39,2 fps i gården. Faste editorutsnitt; pakket bygg og bevegelse gjenstår. Se LIGHTING.md.
- Før tung kjøring må også prosessnavnet `Signal47` sjekkes. Den installerte Linux-binærfila heter dette, ikke nødvendigvis Signal47-Linux-Shipping. En slik prosess gjorde første ytelsespass ugyldig selv om UnrealEditor-kontrollen var tom.
- Kopiert UBT Intermediate inneholder absolutte stier. Lag ny Intermediate i et worktree, og kontroller kompilatorens response-filer. `Scripts/build-editor.sh -NoUBA` brukes her etter UBA-feil på PCH-rename gjennom symlink. Byggskriptet videresender valgfrie flagg.

- Taklys i SARO ligger i mesh-diffuserne, med rectlys og PBR-emissive fra samme tilstand. Rommets gamle PointLights er tilstandsdrivere uten bAffectsWorld. Fire CRT-er og øvrige registrerte skjermer har egne grønne rectlys; HUD-ens Ops power-kall overstyrer ikke SARO mørkt, fordi faktisk Brightness også leser stasjonsstrømmen.
- GLB-bokser kan ha slot0..5 selv når N.Mats har én oppføring. Lysrørets underside er slot3. FixtureLighting binder alle diffuserflater til samme MID og følger NodeMids sine senere kopier; ikke oppdater bare materiale0 og la gamle emissive-flater stå igjen.
- Annex.flickerTube ligger ved pool3, mens portens FlickerFlood er4. Den fysiske koblingen remapper dette til det faktiske røret. Endre ikke historietidsplanen for å rette lyskilden.
- Arrayets første fem pools ligger ved Y3, men armaturene ved Y6,5. Native spotter står på armaturene og sikter mot skålene. Seks andre eksisterende feltarmaturer bruker sine eksporterte glødepunkter; de deler arrayets strøm/rødpuls. Ingen nye modeller.

## Oppsett (2026-10-21)

- Prosjektet heter Signal47 og ligger i unreal/Signal47. UE 5.8.4 (binærutgaven for Linux) i ~/.local/opt/UnrealEngine-5.8.4, Vulkan SM6, Lumen i programvare, Nanite av for de importerte modellene.
- Pakkede bygg legges utenfor repoet, i ~/scratch-bm/Signal47-build, så Artifacts-regelen i rotens .gitignore ikke berøres.
- Akser: X nord, Y øst, Z opp, cm. Nettversjonens (x, y, z) i meter er (Y, Z, -X) ganger 100. Vinduet i nettversjonen ligger ved z -4.65 (nord), altså X +465 her.
- Rommet er 1200 (Y) x 900 (X) x 300 cm, vindu fra Z 95 til 275 langs hele nordveggen med sprosser hver 180 cm, dør mot sør ved Y -290 til -190 og dør mot øst ved X -210 til -110.
- Modellene importeres med det gamle FBX-importeren (FbxImportUI), uten materialer; hvert materialspor får en MaterialInstanceConstant av M_S47Flat med fargen fra Unity-materialfilen med samme navn (_BaseColor, 1 - _Smoothness, _Metallic, _EmissionColor).
- Fonten VT323 finnes bare som woff2 i web/. Konvertering krever fontTools med brotli, som ikke er installert. Reserve: JetBrains Mono fra THE BLACK MAP (OFL).
- Stjernehimmelen er en tekstur laget ved oppstart (UTexture2D::CreateTransient) på innsiden av en kule med skala 400; fiktiv himmel uten katalog.
- Eksponeringen er manuell (bias 0, fysisk kamera av) så natta blir mørk med vilje. Lamper i candela: taklys 7 til 9, skrivebordslampe 3, gårdslampe 14. Månen er et retningslys på 0,9 lux.

## Prologen (2026-10-21)

- AS47Story er en direkte port av Prologue.ts. Spilltid (GameTime) stopper mens et papir, konsollen, terminalen eller notatboken er åpen (HUD-en setter Story->SetPaused). Tidsplanen (After) går på spilltid.
- Historien når tingene i rommet bare gjennom AS47Saro sine settere og PropLocation(id). Nye ting får Tag(actor, id) og en linje i Label() og Use() i S47Story.
- De fire CRT-ene er render targets på 512 x 384 som HUD-en tegner tolv ganger i sekundet med FWidgetRenderer (SS47Crt). Skjermflaten er en tynn boks med M_S47Screen (Glow 0 når skjermen er av).
- Lyder slås opp ved navn i /Game/Licensed/Sfx, så /Game/Audio/Sfx, så /Game/Audio/Synth. Mangler en lyd, logges «no sound named» én gang og spillet går videre.
- (2026-10-22) Lyden er nå nettversjonens Web Audio portert til C++ (S47WebAudio): én kontekst på 48 kHz, spilt av én USynthComponent, med egen panorering mot lytteren (spillerens øye, satt i US47Audio::Tick). Opptakene dekodes fra WAV i Content/Audio/Raw og Content/Licensed/Raw (index.json). Et filnavn fra prologen (thud_soft) går gjennom grafen når opptaket finnes der, ellers som før til lydassetene.
- Lydmotoren følger Chromium der nettleseren avviker fra spesifikasjonen, fordi det er slik nettversjonen høres: en løkke gjennom en DelayNode er én kvant (128 rammer) lengre enn forsinkelsen, firkant og sagtann har toppen 1 etter Gibbs (nivå 0,848), og kompressoren er Chromiums DynamicsCompressorKernel. Kryssjekket mot Chromiums OfflineAudioContext: 1e-4 eller bedre på konvolutter, biquader, panorering, avspilling, kompressor og kam.
- Lyd-API-et tar låsen per kall; hold FS47AudioContext::FBatch over en hel graf. Bygg lange buffere (amRadio-melodien) før låsen tas, så lydtråden ikke venter.
- De kjøpte glb-modellene importeres av Interchange til /Game/Licensed/Models/<pakke>/StaticMeshes/<mesh> (crt_terminal har crt_terminal og crt_keyboard).
- Et medlem og en konstant kan ikke hete det samme (RelayFirst): clang tar medlemsfunksjonen.

## Tegningene og kapittel 2 (2026-10-22)

- Lerretstegningene (drawings.ts) tegnes med Slate i et render target og leses tilbake: `FWidgetRenderer(false)` (uten gammatrinnet) og et vanlig 8-bits mål, fargene med `FLinearColor::FromSRGBColor`. Da kommer lerretets bytes ut som de er og blandes som lerretet blander. Med `FWidgetRenderer(true)` blir fargene sRGB-kodet to ganger (papiret #efe8d6 ble #f8f5ec).
- Slates kantutjevnede linjer toner ut over to piksler, lerretets over én. Tegningene tegnes derfor i dobbel størrelse og midles ned.
- Nettversjonens Oswald er ett statisk snitt, Medium (500), oppgitt for vektene 200 til 700 (core/fonts.ts), så nettleseren tegner all Oswald i Medium, også titlene som ber om 600. `Content/Fonts/Oswald-Regular.ttf` er en variabel font som Slate tegner i standardsnittet 400. `Oswald-Medium.ttf` er 500-snittet av den, laget med fontTools (`varLib.instancer`, uv-venv i scratch); tegningene bruker den. `S47Style::Display` bruker fortsatt Regular.
- Redigeringsprogrammet kan kjøre spillet uten vindu og uten den genererte verdenen: `UnrealEditor Signal47.uproject -game -RenderOffscreen -nosound -S47SmokeTest -S47SelfTest`. Det holder for selvtesten og for logikk som ikke trenger verdenen.
- Lydnavnene i kapittel 2 er nettversjonens (`phoneRing`, `click`) gjennom `Sfx` og `LoopSfx`; stand-ins i S47AudioWeb gir `PhoneCall` unset, så tekstingen går, og `Ringback` regner ennå ikke med starten sin.
## Lagring og flyt (2026-10-22)

- Lagringene ligger i Saved/SaveGames: save-<sak>-<auto|manual>-<plass>.json med kortets felt og `state` (GameState v2 som i nettversjonen), bildene i photos/<nøkkel>.txt og referert som `idb:<nøkkel>` som i nettversjonen, active.json er saken som ble spilt sist, settings.json er innstillingene (main.ts store).
- Det som ennå ikke er portert (bilen, lommelykten, bortføringen, albumet, kurven, Caliche, ns.ln, ns.array, ns.hondo, kapitler uten C++-port) leses fra lagringen som åpnes og skrives tilbake uendret (`US47Flow::Carry`).
- Pausemenyen stopper verden med SetGamePaused, og lyden pauses med den. Verdenens tidtakere står da stille, så røyktestens tidtakere går på FTSTicker.
- Ethvert -S47-flagg unntatt -S47Picture og -S47Title starter rett i spill som før; uten flagg starter spillet på tittelen.
- I UE 5.8 har FJsonObject::Values nøkler av typen UE::TSharedString: `const auto&` i løkker, `FString(*Key)`.
- AActor har en virtuell Reset(); `AS47Story::Reset` er Prologue.reset og overstyrer den.
- Uten den genererte verdenen kan prosjektet prøves med UnrealEditor <uproject> -game -RenderOffscreen (tom verden): menyene, notatboken og lagringen virker.
## Kjøringen (2026-10-22)

- Kjøringen går i AS47Walker sin tick (CameraOverride → US47World::Frame), så bilen og kameraet flyttes i samme bilde; Unreal har ingen fast rekkefølge mellom gåeren og HUD-en.
- Bilens gruppe roteres i Euler YXZ (stigning, kurs, krengning) som i Drive.ts; FS47Truck holder Position og Rotation selv, så World.ts sin `rotation.y` er kursen.
- Et render target tegnet her trenger SetMap(rt, true) (radene motsatt vei av de eksporterte bildene), som CRT-ene.
- clang godtar ikke en struct med standardverdier inne i en UCLASS som standardargument; FS47TripOpts ligger derfor utenfor US47World.

## Radio og musikk (2026-10-22)

- `S47Radio` / `S47TripRadio` / `S47RadioVoice` / `S47Score` er portene av de tilsvarende TypeScript-filene. Innkoblingen er i Main, World og Flow. Prologens bordradio bruker fortsatt sin egen `amRadio`, som i nettversjonen. Tittellåten bruker lydmodulens `music`-løkke og musikkbussen, utenom Score.
- Lange opptak ligger som PCM16 WAV i `Audio/Raw/media/{music,radio}` og går gjennom FS47MediaSource inn i den samme Web Audio-grafen. Ikke legg dem i Audio.Buf: musikken alene ville brukt omtrent 1,4 GiB som float. Kilden leser 32768 rammer per bit, opptil fire biter, med videre innlesing på bakgrunnstråd. Hold aldri en FBatch rundt Open/Seek eller innlasting av stemmeklipp.
- `audio-raw.py --recordings-only` oppdaterer media, de fire RadioVoice-bankene og katalogene uten å erstatte eksisterende sfx-/telefon-/kjøpt-lydindeks. Kjørt mot hovedkopien: 22 lange opptak og 259 stemmeklipp. Alt under Audio/Raw er allerede staged som UFS.
- `M->TownSeen` inneholder blant annet radioens `ev:boy`, `ln:reel` og `tr:<id>` og lagres i nettversjonens `town`. Øvrige nattsystemer må dele dette settet. `Score->Held` har navngitte funksjoner for deres tilstand inntil de porteres og kobles inn.
- Radioens tekst bruker lydkontekstens klokke i `UI->Toast`, slik at en pause holder både innspilte ord og tekst. Selektiv `Disconnect(destination)` slipper bare den ene mottakeren.
- `-S47Radio=caliche|roswell|far` plasserer en testmottaker ved kameraet gjennom den vanlige grafen og logger stasjon, frekvens, segment og lyttere. `screenshots.sh` har fått en radio-kjøring. Se `RADIO_PORT.md` for tester, native avvik og integrasjonsarbeid.

## Caliche, veien og motellets natt (2026-10-22)

- `US47TownLayer` samler main.ts-koblingene til Town/TownEvents, Roadside, Herd/Pasture, NinthModel/NinthCow, MotelTv, MorningHolds/DinerCooler, Cutouts, Caliche og Diner. Modulene deler `Main.TownSeen`; TV-bildet ligger i `GameState.motelTv`. Trinn og restore er delt inn slik at telefonene og SameThread beholder main.ts-rekkefølgen.
- Caliches stemmer bruker `RecordedDialogue`, `NpcDialogue` og `EarlDialogue` gjennom RadioVoice og den eksisterende Web Audio-konteksten. LostNights må koble `Caliche.Reel`; kapittel fem bruker `TownLayer.Diner`. Kapittel tres `FreeShot` kobles når kapittelheaderen er tilgjengelig.
- Byens bilkollider må oppdateres både i AS47Web og i kjørekontrollerens verdikopi (`World.TownCarMoved`). Direkte områdehopp trenger Player.floor-funksjonen før Place; ellers kan en lav plassering fra SARO bli liggende under Caliches terreng.
- Stopp lydgrafer i native destruktorer uten å gå gjennom Main/Audio/Web. Under editorens avslutning kan Webs nodetabell allerede være frigitt. Roadside og TV har derfor egne nodebaserte stopp i destruktorene.
- Ny eksport/import kreves for `world.town.runtime.*`, `roadside:car`, delte lysplasser og herdets beamLit-materialparametre. Ikke erstatt disse med håndbygd geometri. `pointScreen` er ennå bare eksportmetadata. Se TOWN_PORT.md for nøyaktige grenser og testflagg.
## Verdenens eget liv (2026-10-22)

- `US47WorldLife::Step` kalles sist i `US47Main::Step`, i main.ts sin rekkefølge for steg 13: himmelen, lampene, de delte lysene, tåkefargen, horisonten, så områdenes oppdateringer.
- Himmelens gruppe tegnes ti ganger så langt ut som i three (kuppelen på 30 km); tonepassets tåke stopper ved 25 km (SKY_CUT). Gjennomsiktige ting i himmelen sorteres med prioritet (stjerner -4, komet -3, stjerneskudd -2, bånd -1).
- Lys som legges oppå bildet (kjegler, vinduslys, møll, støv) ganges med DEFOG = 1 / (1 - tåka bak), fordi tonepasset tåkelegger hele bildet etter dybden bak.
- Det three tegner med fog: false og som er ugjennomsiktig, tegnes inn i custom depth (r.CustomDepth=3): stencil 1 for de nære mesaene (horizon.ts hazed: bare daggryets dis, MPC HorizonHazeK), 2 for resten (SAROs dishes sett fra den gamle veien). M_WebTone og DEFOG leser dem.
- VertexColor-uttrykket har ingen navngitte utganger: koble med '' (RGB). Et navn den ikke har (`RGB`, `A`) gir en ukoblet inngang uten feilmelding, altså hvitt. Alfa leses i en Custom-node (`Parameters.VertexColor.a`). En VectorParameter i en Custom-inngang gir float3; bruk en float4-node når HLSL-en venter fire verdier.
- `UInstancedStaticMeshComponent::SetCustomData(Start, End, ...)`: End er den siste indeksen, ikke én forbi.
- ISM med per-instans-data i et materiale: `NumCustomDataFloats` før instansene, og PerInstanceCustomData i pikselshaderen virker.
- Floraens tilfeldighet (mulberry32, hash4, vnoise) må regnes i uint32 med nettversjonens overflyt; selvtesten sammenligner hver rute med `life.tests` i manifestet.
- `-ExecCmds="r.SetRes 1600x900w"` gir -RenderOffscreen-kjøringer i 1600 x 900 (ellers 888 x 500), så de kan legges ved siden av nettversjonens bilder.
- Python-skript i et skrapområde må ikke hete som standardmoduler (`inspect.py` stoppet Playwright).

## Kapittel seks og slutten (2026-10-22)

- `US47Chapter6` er Chapter6.ts: uspart kjøring, siste mila med CB og avstandsklokke, THE EVENT på 47 sekunder, nøkkel/dør, figuren og kamerauttrekket. `ClockAfter` har ch6 sist, og Away-krokene ivaretar main.ts sin E/Q-rekkefølge. `World.OldControl.Camera` går gjennom eksisterende `Walker.CameraOverride`; Walker endres ikke.
- `US47Flow::Finale` stanser 3D-spillet og bruker `SS47Ending` sine sanntids-callbacks til tittel, kreditering, fotografi og `ToTitle`. Ch6 har ingen SaveState eller checkpoint. Siste dinerlagring skal være Fortsett; den virkelige koblingen fra kapittel fem må integreres når dens port finnes.
- `AS47Web` har en merket sluttblokk med faktisk tåkeoverstyring og daggry-/solverdier med hooks. Verdensporten må binde hooks eller lese getters. Et statisk `M_WebSky`-panorama kan ikke tegne soloppgangen eller høydeavhengig daggry; ikke kall den visuelle delen ferdig med bare disse setterene.
- OldRoad-konstruktøren legger figurens sju glødepunkter sist i `world.oldRoad.glow.points`. De gjenbrukes direkte; ingen ny veigeometri eller eksport er nødvendig for kapitlet. `US47Away::ShowFrame` har valgfri gruppeopacity for Ending, mens bortføringen beholder sitt svarte underlag.
- Integrasjonskontrakter og faktisk testgrunnlag står i `CH6_PORT.md`.

## Retning og lokale TV-klipp (2026-10-22)

- Tom har bestemt at videre arbeid kun gjelder Unreal. Web og Unity er referanser.
- Unarius-MP4-er ligger i `/home/tom/Desktop/Unarius/`. Film skal bevege seg på TV-ene. Lokal kandidat, status og videreføring står i `TV_PROGRAMME.md`.

## Egen alien, 22. oktober 2026

- Ny figur med Blender-kilde, 2K PBR, rigg, idle og importprøvde UE 5.8.4-assets ligger i `/home/tom/Desktop/Alien - Egen modell/`. Se README der for import og rapporter. Gammel `/home/tom/Desktop/Alien/` er urørt referanse. Ny modell er klar som ressurs; integrasjon i pakket spill gjenstår.

## KELVORA-rekvisitter, 22. oktober 2026

- Egen CRT-TV og slitt radio i `/home/tom/Desktop/Retro TV og radio/`, med PBR, FBX/GLB, LOD1/2 og importprøvde Unreal-assets i `Unreal/Content/Kelvora/`. TV-skjerm er materialplass 1 med eksisterende filmteksturparametre. Assetene er ferdige; plassering og pakket film-/radioprøve gjenstår hos integrator.

## KELVORA plassert 22. oktober 2026

- `S47RetroProps.cpp` bygger egne TV-er ved `world.motel.tv.group` og `world.room6.objs.tv`, radio ved `room.objs.radio`. `BuildRetroProps` kjøres etter BuildRoom; nye barn følger områdenes synlighet via AdoptNode og gamle bruk-ID-er. Skjermenes egne MID-er registreres på de eksisterende materialhåndtakene, med startverdier fra gamle MID-er.
- FBX-screen UV0 er snudd i V av importøren. Eget unlit skjermmateriale inverterer V før EmU/EmV, fordi de eksisterende canvas-tegnerne setter EmV=(0,-1,1). Ikke fjern dette eller innfør ekstra film-vending. CRT-kildens ringpolygoner hadde motsatt retning av senteret; alle er nå vendt ut og testet i alle LOD-er. Unlit unngår at TV-ens eget punktlys dekker filmen med en hvit refleks. Piloten sjekker RGB, ikke alfa.
- `Content/Kelvora/Office` og `office-patches.json` klipper kun gamle TV-deler fra originalkontoret; g-ID-ene må regenereres ved ny verdenseksport. Kilder og importløype er beskrevet i Tools/kelvora/README.md. Originale Blender-kilder på Desktop/Retro TV og radio er oppdatert.
- Kandidat `scratch-bm/Signal47-kelvora/build` er installert i Games/Signal47, hashverifisert med 27 save/config-filer bevart. 1218 selvtester, ni scenarioer; siste materialversjon er også prøvd på begge TV-er og med av/på. Fullt 47-sekundersopptak/lagring bestod før siste V-only shaderendring. Ingen menneskelig lytting eller hel gjennomspilling i denne økten.
## Utelys og fotolab, 22. oktober 2026

- `port-light` er videreført etter at forrige lys ble integrert/pakket. Ny leveranse tar inn `unreal-port-core` til `3b44956e`, med gratismodellimport, Claudes mørkeromsregel og brettrettelser.
- `S47ExteriorLighting.cpp`: 30 korte, fysiske spotter fra eksisterende armaturer; `S47LabLighting.cpp`: adresserbar `Light("lab.normal"/"lab.safelight"/"lab.enlarger")`, intensitet 0 til 1, og fysisk normal-/rødt-/linse-/varsellys. `US47Chapter1::Darkroom` kopler renderdriverne ved faseskifte; ingen ny film-/dørregel i dette lyspasset.
- Bygg og 1218 selvtester OK, før/etter fra ni kameraer, labmoduser/halvstrøm/kutt/tilbake og faktisk kontaktkopitilstand prøvd. Rapport/API/bilder: `LIGHTING.md`, bevis i `~/scratch-bm/Signal47-lighting-2026-10-22/revision3`. Ny pakke, full fremkalling med ekte input og nye High/1440p-tider gjenstår hos integrator. Ingen endring av installert spill.


## Radiostasjon og array, 2026-10-23 kl. 23:53

Original Blender-radiostasjon 1420 AM med analogt studio og separat lobby følger konseptarket. VLA-lignende antenne har fagverk, feedarmer, serviceplattform og tre bevegelige deler med tre LOD-er. Alle 27 antenner er erstattet. Kildene og native UE-assets er lokalt flettet inn i unreal-port-core fra codex/radio-array-assets. Se RADIO_ARRAY.md og Signal47/Tools/radioarray/README.md.

Ni antenne-FBX-roundtrips, stasjonens 32 eksportkontroller og UE-importens 68 kontroller PASS. Integrasjonspakken fra `66f41e22` bestod 1278 selvtester, antennebevegelse, lukket dør, åpne/gå 6,1 m inn og buzzer. Gangsonens terskelgap ble rettet. Visuell gjennomgang fant gammelt studiobilde som frittstående flate; sluttkode `3aa41334` plasserer det 2 cm foran bakveggen. Kun denne offseten og forklaringen ble endret. Sluttkoden består editor/Linux-pakking, 1278 selvtester, fire High/VHS/1440p-modellvinkler og N01/N02/A02/P01-pilot. Sluttbildene er visuelt kontrollert. Automatiske API-scenarioer er ikke menneskelig gjennomspilling eller lydlytting.

Installert i ~/Games/Signal47/Linux med 40 kontrollerte payloadhash og 51 Saved-filer uendret. Sikkerhetskopi: /home/tom/scratch-bm/Signal47-radioarray/installed-backup-1792792300737671855. Redigerbare leveranser: ~/Desktop/Signal47 radiostasjon og array. Evidens: ~/scratch-bm/Signal47-radioarray. Content-lenker og genererte OpenOrder-endringer er ryddet. Ingen push eller PR.

## Første Caliche-gatebit, 24. oktober 2026

- Caliche-gatebiten er lokalt flettet fra codex/caliche-street og installert i ~/Games/Signal47/Linux. Original suvenirbutikk med butikkrom, sju nye native deler og kontaktjustert bakke rundt eksisterende bensinstasjon og 1420 AM. Steinmaterialer, lokalt lys og faktisk dagslys er rettet etter første visuelle gjennomgang. 212 importkontroller, 84 native kontroller, 1305 selvtester, dør/bevegelse/buzzer, fotopilot og fem korte historieregresjoner PASS. 24 bilder ved 1,62 m i to tidslinjer og dag/skumring/natt er visuelt kontrollert; sekundær kode-/bildegjennomgang uten blokkerende funn. Kjørt sekvensielt med guard, ingen brukerprosess stoppet. 40 payloadfiler hashkontrollert; 51 Saved-filer uendret. Sikkerhetskopi: /home/tom/scratch-bm/Signal47-caliche-street/installed-backup-1792799774134625695. Kilder og bilder: ~/Desktop/Signal47 Caliche gatebit; bevis: ~/scratch-bm/Signal47-caliche-street. Testet C++/config 61120335, native c20beabe, pakke 485f3b19832a. Automatiske scenarioer er ikke menneskelig gjennomspilling, lytting eller fullnatt. Ingen ny ekstern publisering, push eller PR. Midlertidige Content-lenker og OpenOrder-endringer ryddet.
- Blender/Fbx bruker eksplisitte ulike aksekontrakter for butikk og bakke. Ground/glass er NoCollision; behold gamle ganggulv. Uniontrim er atomisk og bevarer fortausmaterialet. Normalstyrke/UV-skala har eksplisitte importparametre; UE-pinnen heter UVs. Midday gjelder bare render fra 05:40, uten å endre Dawn eller historiegater. Se Signal47/Tools/calichestreet/acceptance.json.
