> **Status: Under arbeid** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../STATUS.md).

# Toms feilrapport og todo, 28. oktober 2026

Bestilling: registrer alle ni punktene med bildebevis, undersøk hva som faktisk er i den lokale utgaven og vurder fargereferansene. Dette er en åpen arbeidsliste. Registrering eller tidligere beståtte automatiske tester betyr ikke at de nye feilene er rettet.

Seks originale spillerbilder og to krasjvedlegg er bevart. [Bevisgalleri](../../../unreal/Evidence/Playtest-2026-10-28/README.md) kobler hvert bilde til oppgavene. [Manifest](../../../unreal/Evidence/Playtest-2026-10-28/manifest.json) har opprinnelse, dimensjoner og SHA256. Bildene inneholder ikke byggnummer; opptakstid og nøyaktig spillversjon er derfor ikke fastslått. De to krasjene er fra før grafikk-R2 ble installert kl. 19:12. Den sist kvitterte installasjonen er R2 fra PR197; rapporten er undersøkt uten å starte Blender, Unreal eller spillet.

## 1. Caliche: sheriff, kino, feed, vanlige hus og byskilt

**Åpen, høy prioritet. Manglende ferdig integrasjon, ikke bare et utseendeønske.**

- [ ] Integrer de oppgraderte TB2-modellene for sheriffstasjon, kino og feed-bygning helt frem til den installerte utgaven: native import, materialer, assembly, korrekt erstatning, pakking og lokal levering.
- [ ] Kontroller og oppgrader de vanlige bolighusene i Caliche. Ikke anta at TB2s fem spesialbygninger inkluderer boligene.
- [ ] Finn de eksisterende Caliche-skiltene og sørg for et troverdig, lesbart byskilt ved innkjøringen i installert spill. Rett synlighet, retning eller plassering ved behov; adkomst må stemme med faktisk vei.
- [ ] Sammenlign faktisk installert fasade med godkjent Blender-modell. Før Blender-kilde, native asset, runtime-kandidat og installert pakke som separate statuser.

**Kontrollert status:** [PR179](https://github.com/Tombonator3000/SIGNAL-47/pull/179) er flettet til main i `c0e35da30ad50f46dbe00d879cef04f3680797a8`. [TownBuildings-kildestatus](../../../unreal/Signal47/Tools/townbuildings/README.md) sier fortsatt at TB2 mangler native import, nye materialer, assembly og pakket spill. [Gjeldende native assembly](../../../unreal/Signal47/Content/TownBuildings/assembly.json) sier `TB1-original-six-building-production`; dens SHA256 `f0a52a05203f694e63c17395ac66d6dd5cbc317482ee83ec95b55575a446a11b` stemmer med installert grafikk-R2s kildefrys. Den installerte runtime-loggen bekrefter seks tidligere bygningsfamilier, 28 deler og 32 patcher. Sheriff, vaskeri, feed, kino og kirke er TB2s fem spesialbygninger; vanlige bolighus inngår ikke i den leveransen. Derfor er det riktig at Tom fortsatt ser tidligere bymodeller, men oppgraderingen er ikke ferdig levert til spill.

Den opprinnelige scenen har allerede «CALICHE / POP. 412»-skilt ved begge ender av byen: road-koordinater x/z `(-8.6,1880)` og `(8.6,2335)`, se [Town.ts](../../../web/src/world/Town.ts), linje 472 og 1080. Finn eksisterende skilt og kontroller plassering, retning og native synlighet før en duplikatmodell legges til. Native synlighet er ikke kontrollert i denne dokumentrunden.

Ingen av de seks nye bildene viser disse byfasadene eller trafikksignalet. Rapporten skal ikke tilordne motelbildene som bildebevis for bymodellene. Registrer nye før/etter-bilder når akkurat denne oppgaven utføres.

## 2. Diner: to krasj i installert spill

**Åpen, kritisk for stabilitet.** Tom rapporterer krasj i diner og foreslår samtidig Unreal/spill som mulig forklaring.

- [x] Undersøk de to faktiske krasjene, med motorlogg, krasjøyeblikk og ressursbudsjett fra riktig kjørbar fil. Komplette UE-krasjmapper, kjernefiler og tilhørende binærer/debugsymboler er sikret og hashbundet 29. oktober.
- [ ] Avklar om Unreal/editor eller en annen tung jobb faktisk kjørte samtidig. Samtidighet er en hypotese, ikke fastslått rotårsak.
- [ ] Kontroller én målrettet diner-rute uten andre tunge jobber etter en begrunnet rettelse. Registrer kvalitet, skjermoppløsning, renderer og minnebudsjett. Ikke start en full scenariomatrise for triage.

| Vedlegg | Tid, Europe/Oslo | Bekreftet spor |
| --- | --- | --- |
| [PID1692368](../../../unreal/Evidence/Playtest-2026-10-28/crash-1692368-excerpt.txt) | 28.10 kl. 16:30:37 CET | Fatal Vulkan-OOM ved 128 MiB transient heap, frame 0 etter LoadMap. High/TSR-historikk 100; ingen dinerhandling bevist i denne loggen. |
| [PID1710609](../../../unreal/Evidence/Playtest-2026-10-28/crash-1710609-excerpt.txt) | 28.10 kl. 16:53:16 CET | Fatal Vulkan-OOM ved 256 MiB transient heap etter faktiske dinerhandlinger. Ultra/TSR-historikk 200 fra frame 2. |

**Kontrollert 29. oktober:** De nå gjenfunne komplette motorloggene bekrefter `Out Of Memory` i `VulkanMemory.cpp:3114`. SIGSEGV følger motorens bevisste fatalavslutning; det er ikke bevis på en tilfeldig nullpeker i diner. Begge prøver minnetype 1 og deretter 5 på fysisk device-local heap 0. Vulkan EXT viser henholdsvis usage/budget 1097,62/3734,44 MiB og 8002,56/8119,81 MiB. Første avslag er fortsatt ikke forklart av dette budsjettparet alene. Det andre har bare 117,25 MiB differanse før forespørselen på 256 MiB. Begge logger systemoppløsning 2560×1440; separat faktisk viewport-/renderoppløsning er ikke bevart. Se [maskinlesbar triage](../../../unreal/Evidence/Playtest-2026-10-28/diner-triage-20261029.json).

EXT-budsjettet er en veiledning og bruken et prosessestimat, ikke en garanti for at neste allokering lykkes. Verdiene kan endres med systemlast. Derfor blandes de ikke med motorens egne committed/reserved-tellere eller global `nvidia-smi`-bruk. [Khronos, VkPhysicalDeviceMemoryBudgetPropertiesEXT](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceMemoryBudgetPropertiesEXT.html).

Riktig første binær er SHA256 `88508561fbf585009f5d5501aea7797d5b4cfe06657bad1651c99f7f421b4c70`, Build ID `45018ddc2bd83e66419211947400beccb7783a33`. Riktig andre binær er SHA256 `15037a528be5e6a073440790bab8eba8ef6d349b4b5acc3def9d3a57e1a47a04`, Build ID `979838f826aad2cfcdab584cb17369b260733f1b`. Dumpede ELF-sider matcher hver binær, og tilhørende debugfiler løser de faktiske stakkadressene. Første komplette pakkepayload er ikke funnet; første binær/debug/sym og core er sikret separat. Den komplette andre pakken er bevart i `~/scratch-bm/Signal47-graphics-settings-20261028/previous-installed/Linux` og beskyttet mot pakkeopprydding mens undersøkelsen pågår.

Historiske guardkvitteringer viser ingen overlapp med de undersøkte nærliggende electric-arc-/graphics-jobbene, men mangler en prosessmåling akkurat ved krasjene. En bestemt konkurrerende prosess er derfor **UNVERIFIED**. Kerneljournalen viser NVIDIA-minneallokeringsfeil i samme tidsrom; den identifiserer heller ikke en bestemt konkurrent. Andre krasjdump viser dessuten 14 feilede store device-local-allokeringer igjen som 1875,25 MiB faktiske heap 1-allokeringer. Dette er et fallback-/reservasjonsspor i samme prosess, ikke bevis på lekkasje eller hvilket rendererpass som eier bufferne.

**Nye avgrensede prøver:** Den byteidentiske andre pakken ble prøvd på Ultra/Vulkan ved faktisk 2560×1440, isolert `/tmp`-profil og fysisk `water`-rute uten andre tunge jobber. Originalinnstillingene, TSR100 og to Nanite-kapasitetsprøver nådde kritisk minnemangel; egen overvåker stoppet bare egen prosess. Ingen av disse er akseptert som tiltak, og produksjons-TSR/Nanite er uendret.

R5 endret bare den konservative VSM-reserven til 0,25. Hele diner-/returruten, 2628 meter, telefon og fullworld/diskrestore bestod. Global GPU-topp var 8089 MiB, minst ledig 1767 MiB og Vulkan device-local peak 6385,99 MiB. 300 separate GPU-statusmeldinger gav null synlig-instansoverløp, med seks marking-køflagg. CSV-overløpsfeltet har en kjent payload-forskyvning i denne motoren og brukes ikke som aksept. Ny Linux-kilde `b70bf042` endrer bare denne reserven; pakket prøve uten diagnoseoverstyring gjenstår. Dette er en målt minnemitigasjon, ikke bevis på full historisk rotårsak eller FPS. [Full prøveoversikt, mekanisme og akseptgrenser](../../Evidence/UnrealDeliveries/DINER_VULKAN_2026-10-29.md).

Detaljer, kopikontrollsummer, rålogger, kernelutdrag og prøvekvitteringer ligger i `~/scratch-bm/Signal47-diner-crash-20261029`. Originale vedlegg og eldre bevis er bevart uendret. Prosjektbiblioteket ga ingen relevant Vulkan-minneløsning etter det påkrevde bredere søket; ingen plugin, motor eller driver er byttet.

## 3. Caliche: trafikksignal over veien

**Åpen.**

- [ ] Gi det hengende trafikksignalet mer detaljert hus, visirer, linser, kabler, feste og naturlig værslitasje.
- [ ] La opphenget vaie svakt i vinden rundt et fysisk troverdig festepunkt. Feste og bevegelig lampe må henge sammen.
- [ ] Bevar den gode lyseffekten når det er mørkt. Emissive og faktiske lyskilder må følge lampene under bevegelsen.

Ingen nytt bilde av signalet fulgte denne rapporten. Senere kontroll begrenses til ett nærbilde og en kort bevegelses-/nattkontroll.

## 4. Hus/farm med vei bort til seg

**Åpen avklaring.** [E01](../../../unreal/Evidence/Playtest-2026-10-28/01-farm-road.png) viser et lite gårdsanlegg med bygninger og vindmølle på andre siden av landskapet.

- [ ] Identifiser nøyaktig stedet i E01 mot kart/koordinater, og avklar om veien er ment å være kjørbar helt frem.
- [ ] Dokumenter hva spilleren kan gjøre der nå: tilgang, kollisjon, eventuell interaksjon og eventuell historietilknytning.
- [ ] Hvis det er bakgrunnsdekor, gjør vei og avgrensning forståelig. Hvis det er et besøkbart sted, gi det en troverdig ankomst. Ikke finn på en ny historieoppgave bare for å forklare en dekorbygning.

**Kildebasert svar:** E01 stemmer svært sannsynlig med den eksisterende ranchen rundt 400 meter øst for veien: hus, låve, vindmølle, tank og nattlys i [roadPlaces.ts](../../../web/src/drive/roadPlaces.ts), linje 76. Den er faktisk 3D-geometri brukt som statisk omgivelse. Ingen implementert innendørs gårdsdestinasjon eller besøksinteraksjon er funnet. Veien er tilsiktet og avsluttes ved stengt rørport/gjerde i [roadProps.ts](../../../web/src/drive/roadProps.ts), linje 303. Native kode bevarer ranchveien i [S47RoadTerrain.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47RoadTerrain.cpp), linje 276, og port/gjerde i [S47World.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47World.cpp), linje 1243. Identiteten i E01 er en sterk slutning fra motivet, ikke en ny fysisk befaring.

## 5. Sierra Motor Court: terrengkontakt, kollisjon, mottak og basseng

**Åpen, høy prioritet.** Bruk [E02](../../../unreal/Evidence/Playtest-2026-10-28/02-motel-walkway.png), [E03](../../../unreal/Evidence/Playtest-2026-10-28/03-reception-tv-lamps.png), [E04](../../../unreal/Evidence/Playtest-2026-10-28/04-pool-approach.png) og [E05](../../../unreal/Evidence/Playtest-2026-10-28/05-payphone-wall.png).

- [ ] Rett nivåene foran romdørene: gangsonen skal møte bakken og tersklene naturlig, uten nedsenkede felt eller feil trinn langs dørene.
- [ ] Sett blomsterpotter og stoler på faktisk overflate. E02 viser tydelig en svevende potte med skygge under.
- [ ] Fjern usynlige kollisjonsvegger i gangsonen og ved dørene. Prøv en sammenhengende spasertur langs fasaden og inn til dørene, ikke bare teleporterte kamerabilder.
- [ ] Fjern eller rett doble/overlappende lamper i resepsjonen. E03 viser to lampebaser/støtter som ser ut til å dele skjerm.
- [ ] Rett resepsjons-TV slik at de eksisterende filmene faktisk spiller med riktig program og lyd. E03 viser et motiv på CRT-en; et stillbilde beviser ikke om filmen går eller står fast. Filmfeilen er Toms observasjon og skal sjekkes i bevegelse.
- [ ] Rett gangfelt/gangvei til bassenget som blafrer av og på. Undersøk overlappende flater, terreng og dybdekonflikt; ikke fastslå z-fighting bare fra E04/E05.
- [ ] Hev/tilpass bassengkant og dekke til riktig terrengnivå. Bevar en logisk gangadkomst og gjerdeport.
- [ ] Identifiser og fjern den rare klumpen utenfor bassengområdet. E04/E05 avgrenser området, men hvilket mesh Tom mener må identifiseres før sletting.
- [ ] Vend telefonkiosken/telefonen mot brukbar gangside og sørg for fri plass foran håndsettet.
- [ ] Rett motellveggen ved telefon, ICE/avis og dekor. E05 viser overlappende deler/skilt ved veggen; sjekk restgeometri fra gammelt motell, veggtykkelse og monteringsdybde. Bevar ønsket retro keramikkmønster.

Den tidligere motel-R4-leveransen opphever ikke disse nye funnene. Modellplassering, terrengkontakt, bevegelse, kollisjon og TV-avspilling trenger egen avgrenset rettelse.

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

[PR184](https://github.com/Tombonator3000/SIGNAL-47/pull/184) leverte lampe-/kontaktpasset; det betyr ikke at alle eldre rekvisitter og alle terrengoverganger er erstattet. E06 er et nytt åpent kvalitetsfunn.

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
