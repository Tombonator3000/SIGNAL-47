> **Status: Under arbeid** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../STATUS.md).

# Toms feilrapport og todo, 28. oktober 2026

Bestilling: registrer alle ni punktene med bildebevis, undersøk hva som faktisk er i den lokale utgaven og vurder fargereferansene. Dette er en åpen arbeidsliste. Registrering eller tidligere beståtte automatiske tester betyr ikke at de nye feilene er rettet.

Seks originale spillerbilder og to krasjvedlegg er bevart. [Bevisgalleri](../../../unreal/Evidence/Playtest-2026-10-28/README.md) kobler hvert bilde til oppgavene. [Manifest](../../../unreal/Evidence/Playtest-2026-10-28/manifest.json) har opprinnelse, dimensjoner og SHA256. Bildene inneholder ikke byggnummer; opptakstid og nøyaktig spillversjon er derfor ikke fastslått. De to krasjene er fra før grafikk-R2 ble installert kl. 19:12. Den sist kvitterte installasjonen er R2 fra PR197; rapporten er undersøkt uten å starte Blender, Unreal eller spillet.

## 1. Caliche: sheriff, kino, feed, vanlige hus og byskilt

**Delvis levert som kandidat. Installert utgave og menneskelig førerhuskontroll gjenstår.**

- [ ] Integrer de oppgraderte TB2-modellene for sheriffstasjon, kino og feed-bygning helt frem til den installerte utgaven: native import, materialer, assembly, korrekt erstatning, pakking og lokal levering.
- [x] Oppgrader de fire vanlige bolighusene i egen ENV-02-kandidat. Pakket R2 fra `1665723f` består native og målrettet rute-/fallbackkontroll; [PR219](https://github.com/Tombonator3000/SIGNAL-47/pull/219), opprettet mot main. Dette er en separat leveranse fra TB2s fem spesialbygninger.
- [ ] Finn de eksisterende Caliche-skiltene og sørg for et troverdig, lesbart byskilt ved innkjøringen i installert spill. Rett synlighet, retning eller plassering ved behov; adkomst må stemme med faktisk vei.
- [ ] Sammenlign faktisk installert fasade med godkjent Blender-modell. Før Blender-kilde, native asset, runtime-kandidat og installert pakke som separate statuser.

**Historisk kontroll ved registreringen 28. oktober:** [PR179](https://github.com/Tombonator3000/SIGNAL-47/pull/179) var flettet til main i `c0e35da30ad50f46dbe00d879cef04f3680797a8`. [TownBuildings-kildestatus](../../../unreal/Signal47/Tools/townbuildings/README.md) beskrev da manglende native import, nye materialer, assembly og pakket spill. Den undersøkte [native assemblyen](../../../unreal/Signal47/Content/TownBuildings/assembly.json) het `TB1-original-six-building-production`; SHA256 `f0a52a05203f694e63c17395ac66d6dd5cbc317482ee83ec95b55575a446a11b` stemte med installert grafikk-R2s kildefrys. Den installerte runtime-loggen bekreftet seks tidligere bygningsfamilier, 28 deler og 32 patcher. Sheriff, vaskeri, feed, kino og kirke er TB2s fem spesialbygninger; bolighusene inngikk ikke i denne leveransen. Dette forklarte den tidligere installerte bymodellen og er beholdt som historikk.

Den opprinnelige scenen har allerede «CALICHE / POP. 412»-skilt ved begge ender av byen: road-koordinater x/z `(-8.6,1880)` og `(8.6,2335)`, se [Town.ts](../../../web/src/world/Town.ts), linje 472 og 1080. Den opprinnelige dokumentrunden hadde ingen native synlighetsaksept. ENV-02 beholder nå originalgrafikk, UV og retning, og flytter disse to skiltene til z1895/2315 med fire tilsvarende stolpebokser i både gang- og kjøredata.

**Kontrollert 29. oktober, levert kandidat:** [ENV-02-rapporten](../../Evidence/UnrealDeliveries/CALICHE_HOMES_2026-10-29.md) og [pakket R2-kvittering](../../../unreal/Evidence/ENV02-2026-10-29/packed-r2-receipt.json) binder kilde `1665723f`, 13 459 fryste runtime-inputs og åtte payloadfiler. 1709 selvtester, 634 API-rutekontroller og 481 samlede Off-fallbackkontroller består. Ruten måler 469,75269 m nord, 334,90908 m sør og 10,141795 m gange, med faktisk disk/title/Continue. Alle seks pakkede bilder er gjennomsett; begge vedlagte skiltbilder er åpnet i fullstørrelse. Sørgående start og første isolerte case1-disk er merkede fixtures. Ekte input, subjektiv skiltlesbarhet fra førerhuset, fullmatrise og installasjon er fortsatt UNVERIFIED. Pakket R1 avviste manglende TownSignal-cookavhengighet og beholdt originalbyen; det avviste forsøket er bevart. Ingen offentlig release.

Ingen av de seks nye bildene viser disse byfasadene eller trafikksignalet. Rapporten skal ikke tilordne motelbildene som bildebevis for bymodellene. Registrer nye før/etter-bilder når akkurat denne oppgaven utføres.

## 2. Diner: to krasj i installert spill

**Åpen, kritisk for stabilitet.** Tom rapporterer krasj i diner og foreslår samtidig Unreal/spill som mulig forklaring.

- [x] Undersøk de to faktiske krasjene, med motorlogg, krasjøyeblikk og ressursbudsjett fra riktig kjørbar fil. Komplette UE-krasjmapper, kjernefiler og tilhørende binærer/debugsymboler er sikret og hashbundet 29. oktober.
- [ ] Avklar om Unreal/editor eller en annen tung jobb faktisk kjørte samtidig. Samtidighet er en hypotese, ikke fastslått rotårsak.
- [x] Kontroller én målrettet diner-rute uten andre tunge jobber etter en begrunnet rettelse. Registrer kvalitet, skjermoppløsning, renderer og minnebudsjett. Ikke start en full scenariomatrise for triage.

| Vedlegg | Tid, Europe/Oslo | Bekreftet spor |
| --- | --- | --- |
| [PID1692368](../../../unreal/Evidence/Playtest-2026-10-28/crash-1692368-excerpt.txt) | 28.10 kl. 16:30:37 CET | Fatal Vulkan-OOM ved 128 MiB transient heap, frame 0 etter LoadMap. High/TSR-historikk 100; ingen dinerhandling bevist i denne loggen. |
| [PID1710609](../../../unreal/Evidence/Playtest-2026-10-28/crash-1710609-excerpt.txt) | 28.10 kl. 16:53:16 CET | Fatal Vulkan-OOM ved 256 MiB transient heap etter faktiske dinerhandlinger. Ultra/TSR-historikk 200 fra frame 2. |

**Kontrollert 29. oktober:** De nå gjenfunne komplette motorloggene bekrefter `Out Of Memory` i `VulkanMemory.cpp:3114`. SIGSEGV følger motorens bevisste fatalavslutning; det er ikke bevis på en tilfeldig nullpeker i diner. Begge prøver minnetype 1 og deretter 5 på fysisk device-local heap 0. Vulkan EXT viser henholdsvis usage/budget 1097,62/3734,44 MiB og 8002,56/8119,81 MiB. Første avslag er fortsatt ikke forklart av dette budsjettparet alene. Det andre har bare 117,25 MiB differanse før forespørselen på 256 MiB. Begge logger systemoppløsning 2560×1440; separat faktisk viewport-/renderoppløsning er ikke bevart. Se [maskinlesbar triage](../../../unreal/Evidence/Playtest-2026-10-28/diner-triage-20261029.json).

EXT-budsjettet er en veiledning og bruken et prosessestimat, ikke en garanti for at neste allokering lykkes. Verdiene kan endres med systemlast. Derfor blandes de ikke med motorens egne committed/reserved-tellere eller global `nvidia-smi`-bruk. [Khronos, VkPhysicalDeviceMemoryBudgetPropertiesEXT](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceMemoryBudgetPropertiesEXT.html).

Riktig første binær er SHA256 `88508561fbf585009f5d5501aea7797d5b4cfe06657bad1651c99f7f421b4c70`, Build ID `45018ddc2bd83e66419211947400beccb7783a33`. Riktig andre binær er SHA256 `15037a528be5e6a073440790bab8eba8ef6d349b4b5acc3def9d3a57e1a47a04`, Build ID `979838f826aad2cfcdab584cb17369b260733f1b`. Dumpede ELF-sider matcher hver binær, og tilhørende debugfiler løser de faktiske stakkadressene. Første komplette pakkepayload er ikke funnet; første binær/debug/sym og core er sikret separat. Den komplette andre pakken er bevart i `~/scratch-bm/Signal47-graphics-settings-20261028/previous-installed/Linux` og beskyttet mot pakkeopprydding mens undersøkelsen pågår.

Historiske guardkvitteringer viser ingen overlapp med de undersøkte nærliggende electric-arc-/graphics-jobbene, men mangler en prosessmåling akkurat ved krasjene. En bestemt konkurrerende prosess er derfor **UNVERIFIED**. Kerneljournalen viser NVIDIA-minneallokeringsfeil i samme tidsrom; den identifiserer heller ikke en bestemt konkurrent. Andre krasjdump viser dessuten 14 feilede store device-local-allokeringer igjen som 1875,25 MiB faktiske heap 1-allokeringer. Dette er et fallback-/reservasjonsspor i samme prosess, ikke bevis på lekkasje eller hvilket rendererpass som eier bufferne.

**Nye avgrensede prøver:** Den byteidentiske andre pakken ble prøvd på Ultra/Vulkan ved faktisk 2560×1440, isolert `/tmp`-profil og fysisk `water`-rute uten andre tunge jobber. Originalinnstillingene, TSR100 og to Nanite-kapasitetsprøver nådde kritisk minnemangel; egen overvåker stoppet bare egen prosess. Ingen av disse er akseptert som tiltak, og produksjons-TSR/Nanite er uendret.

R5 endret bare den konservative VSM-reserven til 0,25. Hele diner-/returruten, 2628 meter, telefon og fullworld/diskrestore bestod. Global GPU-topp var 8089 MiB, minst ledig 1767 MiB og Vulkan device-local peak 6385,99 MiB. 300 separate GPU-statusmeldinger gav null synlig-instansoverløp, med seks marking-køflagg. CSV-overløpsfeltet har en kjent payload-forskyvning i denne motoren og brukes ikke som aksept. Ny Linux-kilde `b70bf042` endrer bare denne reserven. Pakket R1 uten diagnoseoverstyring består 1709selvtester og full diner/retur, global GPU 8115/minst ledig 1740 MiB. KaldHigh/natt-R2 ved 01:35 har 3159rå statusmeldinger etter Continue uten synlig-instansoverløp. Dette er en målt minnemitigasjon, ikke bevis på full historisk rotårsak eller FPS. [Full prøveoversikt, mekanisme og akseptgrenser](../../Evidence/UnrealDeliveries/DINER_VULKAN_2026-10-29.md).

Detaljer, kopikontrollsummer, rålogger, kernelutdrag og prøvekvitteringer ligger i `~/scratch-bm/Signal47-diner-crash-20261029`. Originale vedlegg og eldre bevis er bevart uendret. Prosjektbiblioteket ga ingen relevant Vulkan-minneløsning etter det påkrevde bredere søket; ingen plugin, motor eller driver er byttet.

## 3. Caliche: trafikksignal over veien

**Levert i kilde og native editor (ENV-03, [PR217](https://github.com/Tombonator3000/SIGNAL-47/pull/217)).** Pakket spill og ekte input er UNVERIFIED til neste samlede fullmatrise.

- [x] Gi det hengende trafikksignalet mer detaljert hus, visirer, linser, kabler, feste og naturlig værslitasje. [Kilde](../../../unreal/ArtSource/TrafficSignal/README.md).
- [x] La opphenget vaie svakt i vinden rundt et fysisk troverdig festepunkt. Feste og bevegelig lampe må henge sammen.
- [x] Bevar den gode lyseffekten når det er mørkt. Emissive og faktiske lyskilder må følge lampene under bevegelsen.

Ingen nytt bilde av signalet fulgte denne rapporten. Senere kontroll begrenses til ett nærbilde og en kort bevegelses-/nattkontroll.

## 4. Hus/farm med vei bort til seg

**Åpen avklaring.** [E01](../../../unreal/Evidence/Playtest-2026-10-28/01-farm-road.png) viser et lite gårdsanlegg med bygninger og vindmølle på andre siden av landskapet.

- [ ] Identifiser nøyaktig stedet i E01 mot kart/koordinater, og avklar om veien er ment å være kjørbar helt frem.
- [ ] Dokumenter hva spilleren kan gjøre der nå: tilgang, kollisjon, eventuell interaksjon og eventuell historietilknytning.
- [ ] Hvis det er bakgrunnsdekor, gjør vei og avgrensning forståelig. Hvis det er et besøkbart sted, gi det en troverdig ankomst. Ikke finn på en ny historieoppgave bare for å forklare en dekorbygning.

**Kildebasert svar:** E01 stemmer svært sannsynlig med den eksisterende ranchen rundt 400 meter øst for veien: hus, låve, vindmølle, tank og nattlys i [roadPlaces.ts](../../../web/src/drive/roadPlaces.ts), linje 76. Den er faktisk 3D-geometri brukt som statisk omgivelse. Ingen implementert innendørs gårdsdestinasjon eller besøksinteraksjon er funnet. Veien er tilsiktet og avsluttes ved stengt rørport/gjerde i [roadProps.ts](../../../web/src/drive/roadProps.ts), linje 303. Native kode bevarer ranchveien i [S47RoadTerrain.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47RoadTerrain.cpp), linje 276, og port/gjerde i [S47World.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47World.cpp), linje 1243. Identiteten i E01 er en sterk slutning fra motivet, ikke en ny fysisk befaring.

## 5. Sierra Motor Court: terrengkontakt, kollisjon, mottak og basseng

**Målrettet kandidat R1 består i pakket spill; E04-klumpen står åpen.** Bruk [E02](../../../unreal/Evidence/Playtest-2026-10-28/02-motel-walkway.png), [E03](../../../unreal/Evidence/Playtest-2026-10-28/03-reception-tv-lamps.png), [E04](../../../unreal/Evidence/Playtest-2026-10-28/04-pool-approach.png) og [E05](../../../unreal/Evidence/Playtest-2026-10-28/05-payphone-wall.png). [Motellrapporten](../../Evidence/UnrealDeliveries/MOTEL_GROUNDING_2026-10-29.md) og [hashbundet pakket R1-kvittering](../../../unreal/Evidence/MOTEL-GROUNDING-2026-10-29/packed-r1-receipt.json) skiller beståtte prøver fra åpne punkter.

- [x] Kilde/native og editor: synlig ganggulv, terskler og Walker-nivå følger 14 forfattede gulvflater. R6b retter 172 identifiserte lukkede gulvøyers vinding; presist g456-terrengkutt bevarer innkjøringen.
- [x] Kilde/native og editor: stolene settes etter målt fotpunkt, og potter med korrekt base beholdes. Bekreftede gamle stol-, ICE- og aviskollidere pensjoneres med de gamle dublettene; hus, gjerde og gjenværende potter beholdes.
- [x] Editor R11: sammenhengende API-styrt Walker/Resolve går 178,6332 m / 58 punkter, med 14 interaksjoner, sju dørnisjer, fire faktiske blokkeringer og 1 611 gulvstråler. Faktisk rom 6-dørbruk og ordinær isolert disk/title/full Flow Continue består.
- [x] Resepsjon: bare den aktive gamle marmorlampen, node 737, pensjoneres; asset/kreditering beholdes. Åpnet R2-kamerabilde viser resepsjonen, men er fra før siste g456-trim.
- [x] TV R11: faktisk native skjermmateriale er bundet til GPU-teksturen, 48 034 / 49 152 piksler endres, frame 185 blir 258 og lydklokken går 6,0373 s frem. Ingen produksjonsretting av eksisterende TV-avspilling var nødvendig.
- [x] Kilde/editor: terreng kuttes under forfattet bassengsti og dekke; faktisk portadkomst og dekkerunde består. Dette er målrettet rute-/geometribevis, ikke en menneskelig vurdering av flimmer.
- [ ] Identifiser den lille mørke klumpen utenfor gjerdet i E04 før eventuell fjerning. Diagnosen skriver 18 meshkomponenter, 57 instanser og åtte stråler, men ingen matching til originalmotivet er fastslått. Den tilsiktede brune pumpekassen er ikke utpekt som feilen. Ingen ekstra sletting er gjort.
- [x] Kilde/editor: telefonen vender mot fri gangside med faktisk fysisk fronttreff. ICE/avis flyttes samlet, bekreftede restdeler fjernes og keramikk bevares. NoCollision-avisglasset kontrolleres med faktiske GPU-rendertriangler; dette er ikke fysisk treff eller sceneokklusjonsbevis.
- [x] Pakket R1 fra `2d13e231` etter main/ENV02-samordning: 1 709 selvtester, 58/14/7/4, 1 611 gulvstråler, 178,695588 m / 143,029735 s, faktisk rom 6 og ordinær disk/title/Continue. TV har 48 262 / 49 152 endrede GPU-piksler, frame 183 → 256 og lydklokkedelta 6,037333 s. Begge originale tekstur-PNG-er er åpnet. Kandidatens kilde, 13 458 inputfiler og åtte payloads er hashbundet; dette endrer ikke installasjonen eller E04-klumpens åpne identitet.
- [ ] Menneskelig input, subjektiv lytting, FPS og fullmatrise/installasjon/release er fortsatt UNVERIFIED eller ikke utført. R2-kamerafixtures erstatter ikke fysisk rute, og API-ruten er ikke menneskelig input.

Avviste R1–R9 beholdes med konkrete årsaker i rapporten. R10-planen stoppet i bygg på `-Wshadow`; ingen R10-rute ble kjørt. Ni gulvprøver med minst 6/9 gyldige treff dekker en faktisk 2 cm kildefuge uten å endre høyde- eller native-eierkrav. Additiv dekor node 770 bevares visuelt og mister bare sin feilaktige gulvkollisjon. Den tidligere motel-R4-leveransen lukker ikke disse nye restpunktene.

## 6. SARO: musehopp, tekstpanel og øst-/utgangsdør

**Åpen, høy prioritet for fremdrift og bruk.** Ingen bilde av SARO-dialogen fulgte rapporten.

- [ ] Behold museposisjonen når neste tekst/dialoglinje kommer. Tom observerer at den hopper til øvre høyre hjørne hver gang teksten skiftes.
- [ ] Flytt dialog-/tekstpanelet ned mot nedre midt på skjermen med plass til knapper og tekstskalering, slik at det dekker mindre av verdenen.
- [ ] Gjør «try the east door and exit» forståelig: bruk synlig dørnavn og et handlingshint som faktisk passer på stedet.
- [ ] Kontroller fysisk åpning og fremdrift fra korrekt side og riktig historiefase. En lastet åpen dør eller åpning utenfra skal ikke gjøre målet umulig eller gi spilleren et misvisende hint.

**Undersøkelsesspor for mus:** [S47Horror.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Horror.cpp), rundt linje 291, lukker modalvinduet og åpner det igjen ved Continue. [S47HUD.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47HUD.cpp), rundt linje 642, skifter da mellom GameOnly/UIOnly og muse-/fokustilstand. Horror sin `FocusPrimary` setter også fokus neste tick. Dette er et konkret fokus-/capture-spor, ikke en runtimebekreftet årsak. Felles [S47Kit.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Kit.cpp), rundt linje 84, sentrerer Shell; dialogen trenger egen lavere plassering uten å flytte alle andre paneler.

**Hvilke dører:** koden mener service door fra kontrollrommet og visitor entrance i vestenden av korridoren. [S47Horror.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Horror.cpp), linje 197, krever faktiske åpninger fra innsiden etter overleveringen, i kontrollrommets eller korridorens sone. En allerede åpen dør fra lagring registrerer ikke en ny åpning. [PR198](https://github.com/Tombonator3000/SIGNAL-47/pull/198), head `f12ec2542830519079dd5e1dc60ae3dc5ee8301c`, er flettet til main med tydeligere navn. Det er kildeendring, ikke bevis på at den testede/installerte grafikk-R2 har ny tekst eller at en mulig adgangsfeil er rettet. Fremdriftspunktet forblir åpent.

## 7. Trafostasjon: svevende lavdetaljmodeller og rampe

**Åpen, høy prioritet for synlig miljøkvalitet.** [E06](../../../unreal/Evidence/Playtest-2026-10-28/06-substation.png) viser den detaljerte hovedtrafoen sammen med enklere hvite/grå deler, synlige åpninger ved fundamentene og en brå kant mot terrenget.

- [ ] Identifiser og erstatt de gjenværende enkle delene foran gjerdet, inkludert den hvite U-formen og den lyse stolpe-/boksgeometrien. Kontroller om de er interaktive legacy-deler som ikke ble dekket av detaljpasset.
- [ ] Gi skap, fundamenter, kabeltilkoblinger, lampefester og servicepunkter nødvendig nærgeometri og materialer. Ingen deler skal sveve.
- [ ] Integrer vei/rampe og grusovergang naturlig i bakken med sammenhengende helning og korrekt terrengkontakt, uten hulrom eller skarpe pålagte plater.
- [ ] Bevar serviceadgang, strømtilstander, lyd, SC02-lysbue og historietriggere under miljørettelsen.

[PR184](https://github.com/Tombonator3000/SIGNAL-47/pull/184) leverte lampe-/kontaktpasset; det betyr ikke at alle eldre rekvisitter og alle terrengoverganger er erstattet. E06 er fulgt opp med [FIX-07s kilde/native-levering](../../Evidence/UnrealDeliveries/SUBSTATION_FIELD_2026-10-29.md), ny T01 og kontrollerte før-/etterbilder. Pakket og installert oppfølging føres fortsatt separat.

## 8. Færre tester og aktiv arbeidsdeling med Claude

**Gjeldende arbeidsregel fra Tom.**

- Bruk subagenter aktivt til avgrenset kildeundersøkelse, modellarbeid, review og dokumentasjon med tydelig filansvar. Samordne med Claude i `OPPGAVER.md` uten å overskrive hans kilder.
- En dokument-/todo-oppdatering trenger kontroll av lenker, vedlegg og diff, ikke motorbygg eller spilltester.
- En rettelse trenger raske relevante kontroller og én målrettet kontroll av den endrede delen. En modell eller menyfane utløser ikke flere hundre scenarioprøver.
- Samle endringer til en sammenhengende leveranse. Kjør en eventuell nødvendig fullmatrise én gang før samlet installasjon/offentlig release. Gjenta bare ved nye relevante endringer, feil eller konkret uavklart risiko.
- Én tung Blender-/Unreal-/pakkejobb om gangen. Ingen tester samtidig med Toms spilløkt; ingen fremmed prosess stoppes.

Denne registreringen bruker tre avgrensede subagenter og ingen motor-/spilltest.

## 9. Mer 1980-talls New Mexico-farger gjennom dag og natt

**Åpen visuell vurdering, ikke godkjent motor-/shaderbytte.**

Toms referanser: [YouTube](https://www.youtube.com/watch?v=ciCRiQmwTrs) og [Fab](https://www.fab.com/listings/3f72729e-41d1-4ce0-bed7-c7bee21a63dd).

- [ ] Lag et lite sammenlignbart fargeforslag for dag, skumring og natt med varme støv-/adobefarger, dempet filmfarge, lesbare skygger og troverdige varme lamper.
- [ ] Vurder opp mot eksisterende Clean/filmprofiler og bevart dag-/nattehimmel. Ikke legg ny grading dobbelt oppå dagens film-/tonekurver.
- [ ] Skill kunstnerisk ønsket tidskoloritt fra tonemapperens tekniske behandling av høylys og metning.
- [ ] Undersøk lisens, Unreal-versjon, Vulkan/Linux, reverserbar integrasjon og kostnad før eventuell bruk av Fab-produktet. Ingen kjøp eller installasjon er utført.

**Referansevurdering:** Videoen er Dallas Drapeaus *The “Unreal Engine Look” Explained — And How to Fix It*. Offisiell YouTube-metadata og forfatterbeskrivelse omtaler ACES kontra AgX. Videobildene er ikke sett i undersøkelsen. Fab-lenken gjelder Hidden Empires **ShaderShift: Engine Shader Overrides**, som tilbyr blant annet tonemappere, BRDF, bloom, korn og halation. Tilgjengelig oppføring dokumenterer ikke en bekreftet UE5.8/Linux/Vulkan-kombinasjon. Pris og valgt lisens er ikke bekreftet; dokumentasjonslenken lot seg ikke lese (404).

**Viktig prosjektbegrensning:** Spillet erstatter allerede Unreals tonemapper med `M_WebTone`: [import_web.py](../../../unreal/Signal47/Tools/webexport/import_web.py), linje 1498, bruker `BL_REPLACING_TONEMAPPER`, og [S47Web.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Web.cpp), linje 714 til 733, kobler det inn. [film_tone.hlsl](../../../unreal/Signal47/Tools/webexport/film_tone.hlsl), linje 23, har egen ACES-lignende kurve og sRGB-konvertering. Samme fil har allerede Carpenter35mm, Desert16mm og Super8-farger/korn/halation. Et motorplugin kan dermed bli omgått av dagens tonepass og gi liten eller ingen synlig endring.

Anbefalt neste steg er å avklare tonekjeden og prøve én reverserbar AgX-variant mot dagens kurve på identiske dag-, skumrings- og nattbilder. Kunstretning kan så gi varme jord-/adobefarger om dagen, naturlig skumring og kjølige nattskygger mot varme armaturer. AgX alene skaper ikke et bestemt tiår eller sted. [Epics forklaring av filmkurve og Color Grading](https://dev.epicgames.com/documentation/en-us/unreal-engine/color-grading-and-the-filmic-tonemapper-in-unreal-engine) skiller grunnkurven fra kunstneriske fargejusteringer. Dette er en vurdering og et åpent forslag, ikke målt resultat eller en installert effekt.

## Kilder og videre eierskap

Primærbevis er Toms observasjoner, seks bilder og to krasjvedlegg. Kildelesing kan forklare en sannsynlig årsak, men er ikke en ny runtime-reproduksjon.

Prosjektbiblioteket ble søkt først; første søk ga null treff, bredere søk fant [scenario-unreal-lighting-rendering](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/game-engines/unreal/scenario-unreal-lighting-rendering/SKILL.md). Kilden gjelder UE5.8 og er brukt som referanse for å skille eksponering, belysning og grading. Dens egne motorhelpers er merket uprøvd i Unreal og er ikke installert eller kjørt her.
