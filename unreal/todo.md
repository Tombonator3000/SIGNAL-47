## Første mannequin-pass og målrettet Unreal-forbedring, 28. oktober 2026

[Kildegjennomgang, ferdigheter og Nanite-kandidater](UNREAL_SKILLS_2026-10-28.md) er undersøkt. Første personpass er stille MetaHuman-mannekenger på eksisterende steder.

- [ ] Lag egen sittende radio-kvinne fra kvinnelig prototype med passende klær, behold radio/proxy og tilstedeværelse.
- [ ] Lag en mannlig MetaHuman-grunnvariant og bak stående røykende mann samt sittende diner-sjåfør. Ferdig mannlig kilde finnes ikke lokalt ennå.
- [ ] Bruk poserte statiske meshes, delte spillmaterialer og LOD. Behold servitrisen i eksisterende animerte løype og Dale/Kinski som dagens stemmer.
- [ ] Prøv Nanite selektivt på importerte tette solide miljømodeller etter TB2-/plasseringsrettelser. Bevar RT-fallback og gameplay-kollisjon; ikke global konvertering av gress, glass og effekter.
- [ ] Bruk gjennomgåtte lys-, VFX-, ytelses- og lydferdigheter til avgrenset miljøkontakt. Tilpass lyd til dagens egen graf og grading til M_WebTone.

Ingen nye mannekenger er bakt, importert eller installert. [Planfilen](Signal47/Tools/metahuman/mannequins-plan.json) er eksplisitt `planned_not_produced`. Arbeidet utføres med målrettet kontroll og én tung jobb om gangen.

## Toms feilrapport med bildebevis, 28. oktober 2026

Alle punktene er åpne med mindre en avklaring er eksplisitt besvart i [detaljert arbeidsliste](PLAYTEST_TODO_2026-10-28.md). [Seks originalbilder og krasjbevis](Evidence/Playtest-2026-10-28/README.md) er bevart med kontrollsummer.

1. [ ] Caliche: integrer oppdatert sheriff, kino og feed helt frem til installert bygg; oppgrader vanlige hus og rett synlighet/plassering på byskilt ved innkjøringen. TB2-kilder på main betyr foreløpig ikke TB2 i spillet.
2. [ ] Diner-krasj: undersøk begge Vulkan-allokeringssporene og mulig ressurskonkurranse uten å anta rotårsak.
3. [ ] Trafikksignal: mer detaljert oppheng og lampehus, svak vindbevegelse, bevar nattlyset.
4. [ ] Farmen i bilde E01: identifiser stedet og avklar vei, adgang, interaksjon og eventuell historiebruk.
5. [ ] Motor Court: rett terreng/gangnivå, svevende potter/stoler, usynlige vegger, doble mottakslamper, TV-film, blafrende bassengsti, nedgravd bassengkant, uønsket klump, telefonretning og overlappende veggdeler.
6. [ ] SARO: behold museposisjon ved ny tekst, flytt panelet ned og rett forklaring/fremdrift ved øst-/utgangsdør.
7. [ ] Trafo: erstatt svevende lavdetaljdeler og integrer vei, rampe og fundamenter naturlig i bakken.
8. [ ] Følg redusert testregime og bruk avgrensede subagenter aktivt sammen med Claude. Ingen motorprøve for denne dokumentoppdateringen.
9. [ ] Vurder YouTube-/ShaderShift-referansene mot eksisterende filmprofiler for 1986 New Mexico gjennom dag/natt, med kompatibilitet avklart før bruk.

## Grafikkinnstillinger, installert 28. oktober 2026

Grafikkmenyen er flettet via [PR197](https://github.com/Tombonator3000/SIGNAL-47/pull/197), merge `2b26a05459a18213d830dfd2e85ca571d423b485`, og den identiske, aksepterte R2-pakken er installert i `/home/tom/Games/Signal47/Linux`. Settings > Graphics har fem presets, Custom og elleve individuelle kontroller. Pakket meny-/lagringskontroll (47+15) og fullmatrisen (111/111) består. Alle 79 personlige Saved-filer (105 noder) er bevart. Installert skrive-/lesekontroll består (47 + 15), og payload samt Saved er kontrollert etter oppstart. Kanonisk prune er fullført; gjeldende bygg og to eldre beholdes, med eksplisitt vern av andre aktive SC-kandidater. Se [grafikkrapport](GRAPHICS_SETTINGS.md).

## Elektrisk lysbue ved trafo, levert 28. oktober 2026

SC02-lysbuen er flettet via [PR195](https://github.com/Tombonator3000/SIGNAL-47/pull/195), merge `66ae5948e262586e4c26c621dc6d109e6d950552`, og den identiske, aksepterte R4-pakken er installert i `/home/tom/Games/Signal47/Linux`. Hele matrisen består 110/110; den installerte Low-kontrollen består 14 kontroller og to disk-/Flow-restores. Bildet fra installert spill er vurdert. Alle 60 personlige Saved-filer (79 noder) er uendret. Ingen ny offentlig release. Se [lysbuerapporten](FX_ARC_2026-10-28.md).

## Cloud tank-konseptbilder, 28. oktober 2026

- [x] Bevar SARO v1/v2 og Caliche v1 med galleri, prompter og kontrollsummer. [Gjeldende skyretning](Concepts/CloudTankUFO-2026-10-28/README.md): to skybanker velter fremover fra hver sin side. Implementering og spilltest gjenstår i eksisterende finaleoppgave.

## Cloud tank-UFO i siste scenario, 28. oktober 2026

- [x] Lagre Toms finaleønske med ideer, oppskrift og mulig gjennomføring i [FINALE_CLOUD_TANK_UFO.md](FINALE_CLOUD_TANK_UFO.md). Hovedavsløring anbefales i SC13, med kun mulig forvarsel i SC12.
- [ ] Lag original modell og bakte skyklipp, bygg SC13-prototype, prøv timing med hele slutten og mål ytelse. Bevar THE EVENT og 1947-bildet. Ikke implementert.

## SC02/AI-overtakelse, 28. oktober 2026

- [x] Ta vare på Claudes SC02-kandidat, stoppede scenetekster og separate found-footage-/Niagara-grener.
- [x] Implementer omplanlegging ved nye hindringer, faktisk kapselklaring, lavt trinn, bil-/nærveggmål og fysisk fangstkontakt. 1649 selvtester og 28 native kontroller består.
- [x] Samordne framtidige scenetekster/produsentkart og bevar gamle ekte SC01-lagringer.
- [x] Pakket SC02-rute med fangst, løpeflukt og diskrestore: R5 PASS med 159 kontroller, 19 disk-/Flow-restores og 42,48 m løping. Bevis og avgrensninger i [overleveringen](SC02_AI_HANDOFF.md).
- [x] Toms nye bestilling: kontroller separat PR190-fletting. R5 består full 110-matrise og sju gamle Continue-lagringer; ingen scenariofeil. PR190 håndterer denne SC02-flettingen.
- [ ] Produser SC03 som faktisk verdenshandling med Ortiz/vogn23/termose, deretter SC04/SC05. Resten av akt 1 og senere installasjon gjenstår.

## Niagara ved stjernehullet, 28. oktober 2026

- [x] Registrer Toms plassering av tettere meteorregn ved stjernehull-hendelsen og vurder de fire kildene fra bildene. Se [design og kildevurdering](NIAGARA_VFX_2026-10-28.md).
- [ ] Produser en egen, kort Niagara-passasje ved eksisterende hendelsestid, med bevart ro ellers. Prøv tetthet/lesbarhet, High/Low, pause, faktiske foto, lagring/lasting og engangsavspilling; mål ytelse. Dette er senere implementering, ikke levert effekt.

## Trafostasjonens detaljer levert, 28. oktober 2026 kl.08:45 CET

Caliche-trafoens lampehus, kabel-/fundamentdetaljer og målte grusskråning er flettet via [PR184](https://github.com/Tombonator3000/SIGNAL-47/pull/184), merge `168a4ff8a48b928e4d729882ea678535107169fd`, og samme testede R1 er installert i `/home/tom/Games/Signal47/Linux`. Fem originale Blender/native deler, tre LOD. Hele matrisen består 110/110 etter én kjøring; T01 og T02 består. Den installerte Low-kontrollen består fysisk service/gangadgang og diskreload; bildet er faktisk vurdert. Alle 50 personlige Saved-filer er uendret. Fotolab, motel R4, SC01, MetaHuman og godkjent terreng/himmel er bevart.

Canonical prune pensjonerte to gamle motelpayloads, beholdt gjeldende kandidat og to tidligere komplette fotolabpakker, samt særskilt beskyttede SC01/PR174-kandidater. Kilder, assets, Saved og bevis bevares. Claudes TB2-kilder er på main, men native integrasjon gjenstår der. Nattautomasjonen er PAUSED. Denne leveransen publiserer ingen ny release; trafodetaljene finnes i main og den lokale installasjonen. Se [rapport](SUBSTATION_DETAIL.md).

## Fotolab nattpass, 28. oktober 2026

- [x] Original Blender-kilde, PBR og ti native deler med tre LOD.
- [x] Målrettet lys/strøm/fremkalling og faktisk bildevurdering.
- [x] Én fullmatrise, 110/110. PR181 flettet og identisk R1 installert; 50 Saved-filer bevart.
- [x] Installert Low-fremkalling og faktisk diskrestore via Continue; to bilder vurdert. To gamle pakkepayloads pensjonert med kanonisk verktøy.

# Todo for unreal/

## Greyhill dørstudie med kode, 28. oktober 2026

- [x] Følg dørpassering, lyd, kollisjon og lagring med 77 kontrollerte adressehenvisninger. Se [kodeanalysen](GREYHILL_DOOR_CODE_ANALYSIS.md).
- [x] Lever selvstendig C++17-referansemodell og 16 scenarioer utenfor spillets Source. Kompilering/linking PASS.

Scenarioene er ikke kjørt: Windows nektet native start med «Ingen tilgang». Ingen Unreal-integrasjon, spillmatrise eller gameplay-funksjon er levert.

## Greyhill-referanse, 28. oktober 2026

- [x] Bevar egen jakt-/gameplayanalyse og offentlig kontrollsammendrag. 111 pakker dekodet og bytekontrollert; fullt uttrekk beholdes privat utenfor repoet.
- [x] Kartlegg videre analyseområder med konkrete funksjonsankre og klar grense mot implementert gameplay. Se [oversikt](GREYHILL_NEXT_ANALYSES.md).

Dørpassering under jakt med lyd og lasting er anbefalt neste analyse. Ingen nye spilloppgaver er godkjent eller ferdigmeldt gjennom dette referansenotatet.

## Motel V2 levert, 27. oktober 2026

- [x] Redigerbar Blender-modell, keramisk retrofasade, mørkt treinteriør og logisk mottak/basseng. Eksisterende TV og tre biler bevart.
- [x] Native Unreal-import, målrettede tester og hele `screenshots.sh`, 110/110.
- [x] PR178 og PR172 flettet, samlet R4 installert med 50 uendrede Saved-filer. Installert Low og bildekontroll består.
- [x] Kanonisk opprydding utført; øvrige aktive kandidater beskyttet. Se [rapport](MOTEL_V2.md).

## Grenene, 27. oktober 2026

- [x] Regel i `AGENTS.md`: alle PR-er går mot main, og grunnlagsgrenen sjekkes før fletting.
- [x] Hele `screenshots.sh` mot main etter PR #170: 110 av 110 OK på main pluss SC01 (27.10 kl. 19:15).
- [x] Rett beskrivelsen: `unreal-port-core` var avtalt integrasjonsgren, ikke feil gren (27.10 kl. 17:25).
- [x] Flett `unreal-port-core` inn i main på `ccr-30e38858-767d90`: 17 konflikter løst, HorrorStory-generatoren kjørt, 24 tester OK, PR mot main (skysesjon 27.10).
- [x] Bygg editor og pakke, kjør SC01-kontrollene og hele `screenshots.sh` på Kubuntu (alt PASS 27.10, se loggen kl. 19:20). PR #174 flettes med full head-SHA, og `unreal-port-core` slettes når den ikke har commits utenfor main.

## Samle lokale leveranser til repo, 27. oktober 2026

- [x] Samle designbibel, portable redigerbare kilder, konsepter og manglende kvitteringer med SHA-/lenkekontroll. Se [indeks](../Docs/LOCAL_DELIVERIES_2026-10-27.md).
- [x] Bevar nyere MetaHuman-/atmosfærekilder og lenk separat SC01-kilde uten automatisk runtimeintegrasjon.
- [x] Push og opprett [PR 172](https://github.com/Tombonator3000/SIGNAL-47/pull/172); indeks/PDF kontrollert anonymt på GitHub.
- [x] Full 110-scenario-matrise før main-merge av samlet runtime: R4 består 110/110, PR178 inkluderer PR172. Ingen ny spillutgave inngår i arkiveringen.

## MetaHuman-servitrisen, levert lokalt 27. oktober 2026

- [x] Overta fra Claude, sikre kilder og avslutte gamle task-økter/ventere uten å stoppe andre motorjobber.
- [x] Rett faktisk aktivt lydklipp, Body/Face-uttrykkskurver, tomgangsløkketid og stabil blikkfjær.
- [x] Lag/importer original uniform V5 på samme skjelett; erstatt usynlig pakket groom-frisyre med authored hårkort og vanlig coverage-materiale.
- [x] Kontroller 43 ansiktstakes; mål tre faktiske velkomstreplikker, pause og diskrestore i identisk pakket R3. Fem High-bilder vurdert, 1 586 selvtester PASS.
- [x] Installer R3, prøv standardstarter på Low og bevar alle 50 personlige Saved-filer. Canonical prune anvendt med SC01-R4 beskyttet.
- [ ] Senere visuell polering: finere stoff, portrettets alder/utseende, blyant og egne servicegester. Dette hindrer ikke levert figurpilot. Se [rapport](METAHUMAN_WAITRESS.md).
- [x] Avtalt punkt 5 atmosfære/ytelsesrom er levert lokalt i R1. Se [rapport](ATMOSPHERE_2026-10-27.md). Oppfølgingen avsluttes; ingen ny spillutgave publisert.


## Linux-nedlasting, 26. oktober 2026

- [x] Publiser samme testede terreng-R2 som nedlastbar Linux-testutgave med to ZIP-er, portable startskript, kontrollsummer og bruksanvisning. 43 arkivfiler og fem opplastinger er verifisert; alle lenker virker anonymt. [Last ned](https://github.com/Tombonator3000/SIGNAL-47/releases/tag/playtest-linux-2026-10-26).
- [ ] Prøv kompatibilitet og ytelse på mottakermaskinene. Den nye leveringen utførte ingen ny Unreal-kjøring eller Steam Deck-prøve.

## Terreng og natur langs ruten, 26. oktober 2026

- [x] Bryt opp de dominerende fjelltrappene og lag sammenhengende erosjon/skråningsfot i original Blender-geologi, med bevart vei/gangflate og områdetransform.
- [x] Bruk åtte utvalgte eide Fab-modeller i sju grupper langs SARO-innkjøring, hovedvei og Caliche sør. Bind lavere stein til klippefot, gjør grus flekkvis og bevar åpne passasjer. Behold originalpakker og offentlig/private kildegrenser.
- [x] Rett faktisk påviste R1-problemer og prøv R2: seks vurderte bilder, eksisterende selvtest, avgrenset RTCSV og én sammenhengende 2,62 km kjøre-/gangrute med ankomst, telefon og diskrestore. Ingen ny fullmatrise.
- [x] Installer identisk R2 med 68 verifiserte payloadnoder og alle 70 Saved-noder/50 filer bevart. Se [TERRAIN_ROUTE_2026-10-26.md](TERRAIN_ROUTE_2026-10-26.md).
- [x] Fullfør installert standardstarterkontroll: tolv kontroller PASS. Canonical prune har pensjonert tre gamle nærdetalj-payloadsett/21 filer, med gjeldende R2, to eldre pakker og særskilt beskyttet SC01 beholdt. Kilder/assets/Saved/bevis er bevart.

## Detaljer tett på spilleren, 26. oktober 2026

- [x] Modellér tre originale Blender-armaturer med tre LOD-er og bruk dem på tolv steder ved SARO. Rett innfesting, diffusorer, fysiske åpninger og kildeplassering; bevar inngang og kontor.
- [x] Importer native assets, ta dem med i faktisk cook og fjern bare de erstattede gamle husene. Dagbilde ved inngangen og fløyskilt om natten er vurdert. Alle seks skyassets er bevart.
- [x] Fullfør avgrenset R4-kontroll etter lokal justering av monumentlys: fem sluttbilder i tre kjøringer, faktisk strøm av/på og elleve kontroller i standardstarteren. Samme kandidat er installert med 68 verifiserte payloadnoder og alle 67 Saved-noder fra brukerens siste økt bevart. Se [NEAR_DETAILS_2026-10-26.md](NEAR_DETAILS_2026-10-26.md).
- [x] Kjør canonical prune etter levert R4. Fire gamle payloadsett/28 genererte filer er pensjonert; R4 og eldre R3/R2 beholdes, med aktiv SC01-R4 særskilt beskyttet. Kilder, assets, lagringer og bevis er bevart.

## Pure Sky på dagtid, 26. oktober 2026

- [x] Integrer separat fotografisk daghimmel med CC0-kilde, repeterbar import og kreditering. Bevar originale nattassets, stjerner og hendelser.
- [x] Samordne synlig himmel, solretning og skylight-capture. Bland ut fra 18:00 til 18:53 og bruk originalmaterialet direkte om natten.
- [x] Bygg/pakk og sammenlign faktisk dag 17:20 og natt 03:00 mot R5. Kontroller mellomblanding 18:20, original himmel 18:53 og Low dag. Fem kandidatbilder er faktisk vurdert; ingen ny fullmatrise.
- [x] Installer samme R1 og prøv faktisk standardstarter med isolert profil. Åtte kontroller består; 68 payloadnoder stemmer og 65 personlige Saved-noder er uendret. Se [SKY.md](SKY.md) for nøyaktig avgrensning og bevis.
- [x] Kjør canonical prune etter levering. Gamle R4/R3-payloads er pensjonert; Pure Sky R1, to eldre R5-kopier og aktiv beskyttet SC01-R4 beholdes. Kilder, assets, lagringer og bevis er bevart.

## Grafikkfunn til senere, 26. oktober 2026

- [x] Vurder synlig kvalitetsløft per innsats og Poly Haven-videoen mot dagens sky-/lysrigg og RTX 3080 10 GB. Lagre anbefalt rekkefølge og første sammenhengende strekning i grafikknotatet. Ingen nye assets eller funksjoner implementert.
- [x] Terrengformer og natur langs innkjøringen til SARO er levert i terreng-R2 med bevart dag-/nattehimmel og spillbare høyder. Se nyeste terrengstatus øverst.
- [x] Lagre undersøkte Lumen-/FSSS-/skyggefunn og sammenhengen med terrengarbeidet i [RENDERING_RESEARCH_2026-10-26.md](RENDERING_RESEARCH_2026-10-26.md). Skill faktisk konfigurert bruk, Epic-påstander og uprøvde forslag. Ingen grafikkendring eller ny ytelsestest.
- [ ] Ved videre optimalisering etter den leverte sub-12cm-småsteinsrunden: vurder romlig oppdelte skyggegrenser for Caliche-sage/yucca og småstein, med fortsatt synlige modeller. Behold allerede shadowless gress og store lysblokkere; kontroller også distance-field-fallback.
- [ ] Vurder eget Medium/Lumen Lite-nivå mellom dagens Low uten GI og High. Ikke anta at sg.GI=1 alene aktiverer GI, eller at Epics 2x-påstand betyr dobbelt FPS her.
- [x] Vurder svak innebygd FSSS ved SARO. Faktisk High-natt A/B og GPU-profil er dokumentert i atmosfærerapporten. Ingen tydelig bildegevinst for ekstrakostnaden, så standard forblir av; Low er eksplisitt av.

## Parkering, rampeinngang og kontor, R5, 26. oktober 2026

- [x] Bytt tre store besøkstavler med to originale skilt på 46 × 60 cm, og lag faktisk Blender-inngangsparti ved vestre rampe med kort gangforbindelse.
- [x] Registrer stopp hvor som helst i besøksområdet i enhver retning. Bevar faktisk bilstilling, vanlig stoppkontroll og reparasjon av gamle ventende ankomstlagringer.
- [x] Åpne fysisk kontorrute allerede ved ankomst. Modellér rom, innredning, skap/skuff, nøkkel, dør, pult og stol ordentlig, og behold faktisk dokument-/lagrings- og strømtilstand.
- [x] Bevar allerede installert NP1. Tilbakefør midlertidig R4 til verifisert NP1 før integrasjon; kombiner deretter NP1-tekster/lesestatus, skalering, himmel og LineLights med nye modeller i R5. SC01-kandidaten holdes separat.
- [x] Gjennomfør seks fokuserte R5-kjøringer, 1 581 selvtester i samlet prøve, ni utvalgte bildevurderinger og målt RT maksimum 79,811 MiB. Ikke gjenta fullmatrisen uten konkret behov. Se [rapport](ARRIVAL_OFFICE_2026-10-26.md).
- [x] Installer samme R5 med hash-/Saved-kvittering og prøv faktisk standardstarter: åtte kontroller PASS, 61 Saved-noder uendret. Canonical prune er anvendt på ti gamle pakker/70 genererte payloadfiler; R5, to eldre pakker og aktiv SC01-kandidat er bevart. R4-kvitteringen brukes ikke som R5-bevis.
- [x] Modellér utvendige lampehus med feste, optikk og diffusor; rett monteringsgap bak fløylampene og parkeringsarmatur/emitter inne i veggen. Baldakinen har nå egen modell og én lyskilde. Modellarbeidet er ferdig; gjeldende leveringsstatus står i nærdetaljpunktet øverst.
- [x] Installer offisiell Fab 0.0.13 i lokal UE og bruk faktisk plugin/innlogging til å importere ørkenpakken i separat FabStaging. 518 pakkefiler og 381 registrerte leverandørassets er kontrollert og bevart etter editorstopp.
- [x] Last ned Old West Learning med offisiell Linux Epic Games Launcher Preview etter brukerens innlogging. Launcher viser Completed; prosjektet i FabLibrary har 3 227 filer og ingen eglpart. UE-pluginens tidligere «Install from launcher» er dermed fulgt gjennom riktig offisiell løype. Prosjektet er ikke åpnet/konvertert i UE.
- [x] Velg og migrer nødvendige naturassets fra ørkenstaging og Old West Learning med opphavs-/avhengighetsbevis. Åtte modeller er plassert og kontrollert i terreng-R2. MetaHuman er separat. Den eldre Fab-editorens avslutningsfeil er historisk dokumentert i `fab-plugin/final-fab-report.json`.

- [x] Vurder Epics Unreal Mesh Terrain mot gjeldende terreng. Lesende vurdering i `terrain-assessment.json` anbefaler eksisterende grunn pluss målrettede stein-/klippemesher; ingen ny implementering eller test er gjort.
- [ ] Ved eventuell terrengproduksjon: sammenlign én avgrenset klippeprøve med innebygd Experimental Mesh Terrain. Bevar spillbare høydefunksjoner, historiegulv og delt SARO-/veitransform. Ingen full migrering eller kjøp er besluttet.

NP1 har allerede levert de frivillige private dokumentene, separat arrayserviceark, tekstskalering og dagens femtrinns himmeladapter samt LineLights-lagring. Eldre åpne punkter nedenfor omfatter fortsatt den nye SC-koordinatoren, fysiske møter, VHS og fullnatt; de skal ikke brukes til å fjerne NP1-funksjoner eller utløse en dobbel implementering.

## Fullført lokalt: levende/død ku, 26. oktober 2026

- [x] Konsepter, redigerbar Blender-kilde, pakkede teksturer og FBX/glTF med tre LOD-er.
- [x] Native import og kobling til eksisterende flokker og dødku, med beiting/frost/funn-/lagringslogikk beholdt.
- [x] Bygg, 1 437 selvtester, ti pakkede scenarioer og ekstra Low-nærbilde. Se [avgrenset bildekontroll](COW_MODELS.md).
- [x] R2 installert, standardstarter prøvd, 40 Saved-filer bevart og gjeldende pluss to eldre testpakker beholdt. Ingen ny PR/merge/push.

## Plakater, reklame og hverdagsting, 26. oktober 2026

- [x] Lag en samlet JPG-pakke med seks originale kinoplakater, kinobillett/avskjedsark, butikkreklame, menyer, emballasjetrykk, kvitteringer, postkort, arbeidsplasskilt/-blanketter og hverdagslapper. [48 motiver med galleri og kildekatalog](ArtSource/EnvironmentGraphics/README.md) er produsert med plasseringstips og fullstendige bildeprompter.
- [ ] Ved senere miljøintegrasjon: bind teksturene til riktige flater og tilpass løse rekvisitter, UV-er og materialer. Kontroller sideforhold, lesbarhet, mipnivåer, lys og slitasje ved normal spillerhøyde. Behold eksisterende stedsnavn, priser, rom 6/Nora, aktive oppdrag og adgangsregler. Denne produksjonspakken oppfyller ikke runtime- eller spilltestdelen.

## Dokumentrekvisitter, 26. oktober 2026

- [x] Logg Toms to nye obduksjonsbilder som [separate bildeforslag til Dales filer](ArtSource/Documents/References/DaleAutopsy/README.md), med kompakte JPG-kopier og kildehasher. Senere innpassing og eventuell bildetekst er fortsatt forfatter-/integrasjonsarbeid.
- [x] Lag hele Dales dokumentsamling og gamle aviser/arkivark som komprimerte JPG. [ArtSource/Documents](ArtSource/Documents/README.md) har 31 bilder, engelsk kildetekst, promptsett, oversikt og offlinegalleri. Manuell bilde-/kanonkontroll og filkontroll er utført.
- [x] S.7s private tekst-ID-er, tekstvisning/skalering og faktisk lesestatus er levert gjennom NP1 og bevart i installert R5/terreng-R2. Dette ble levert separat fra JPG-produksjonen.
- [ ] Importer og bind de nye JPG-rekvisittene til faktiske flater eller dokumentvisning. Vurder E15 mot eksisterende foto før utskifting; bruk opprinnelig geometrisk grunnlag for målsatte sammenligninger. Ingen JPG-binding eller nytt fotobevis er fullført av tekstintegrasjonen.
## Godkjent gjennomføring: SC01 til SC13

Følg [HISTORIE_IMPLEMENTERINGSPLAN.md](HISTORIE_IMPLEMENTERINGSPLAN.md), med scenariorekkefølge, H-koblinger, full produksjon og aksept-/leveringskrav. SC01s lokale pakkede tekniske aksept består; koden er bygget, pakket og prøvd mot main på `ccr-30e38858-767d90` (PR #174, 27.10). Installasjon som standard gjenstår og avgjøres av Tom. Alle senere scenarioer er planlagt og alle H-kort er åpne. Tidligere S-/I-/R2-/H-punkter er avhengigheter og kontrollgrunnlag for samme gjennomføring, ikke parallelle implementeringer.

- [ ] Lever SC01. Kilde `dbae3177` har normalrunde, H01-dialog, vindushendelse, rapport og lagring; 21 pakkekjøringer og 32 vurderte originalbilder består. Koden er flettet mot main på `ccr-30e38858-767d90` og bygget, pakket og prøvd der 27.10: alle SC01-kontroller, HorrorStory-testene og hele `screenshots.sh` består. Installasjon som standard gjenstår og avgjøres av Tom. Se [SC01_RUNTIME.md](SC01_RUNTIME.md).
- [ ] Lever deretter SC02 til SC13 i rekkefølge, med egen testet PR/fletting og installasjon per SC.
- [ ] SC02-kandidat på `ccr-20021313-mfplx9` (skysesjon 27.10, ukompilert): kjør `ArtSource/HorrorSC02/build_scene.py` i Blender 4.5 og `import_assets.py`, bygg editor og pakke, rett kompileringsfeil, kjør selvtest og prøvene SC02-T01 til SC02-T10 i `Narrative/HorrorStory/world_producers.json`, ta opp H02-stemmene med `voice/generate.py --scenario SC02`, og kjør hele `screenshots.sh` før PR mot main. Se [SC02_TIL_SC13_FORBEREDELSE.md](SC02_TIL_SC13_FORBEREDELSE.md).
- [ ] Ta stilling til de 16 avklaringene i SC02_TIL_SC13_FORBEREDELSE.md (Tom), særlig T01-arkivering mot passiv residual, 06:00-serien, Earls spole og stemmeprofiler for Ward, Ortiz, Earl, Nora og sjåføren.
- [ ] Bruk arbeidspakken og testsettet for hvert SC i `world_producers.json` når SC03 til SC13 startes. Oppdater produsentstatus (existing/partial/new) når koden faktisk er koblet inn.
- [ ] Før separate H-kvitteringer ved SC02, SC05, SC06, SC08, SC09, SC10, SC11, SC12 og SC13 som angitt i planen.
- [ ] Fullfør samlet helnatt-/kompatibilitetsaksept og før menneskelige UNVERIFIED-punkter ærlig.

## NP1 levert, 26. oktober 2026

PR164 er flettet, og kontorfunn/tekstskalering/lesestatus, valgfri himmel og korrekt LineLights-lagring er installert med ku-R2 og personlige lagringer bevart.17 pakkede kjøringer/1 493 selvtester består. S.1/R2.1 gjelder produsert manus, ikke hele nyaktens runtime. [Leveringskvittering og gjenværende grenser](NIGHT_PRODUCTION_RUNTIME.md).

## Sammenflettet hovedscenario, randombevaring, himmel og Dale

[MISSION_FLOW_1800_ROSWELL.md](MISSION_FLOW_1800_ROSWELL.md) er nå én hovedrekkefølge med 13 sammenhengende scenarioer. Tidligere revisjoner og alle 32 løsningskort ligger urørt i [MISSION_DETAILS_REFERENCE.md](MISSION_DETAILS_REFERENCE.md) som sekundært oppslagsverk. Oppgavene her presiserer eksisterende I-/R2-/H-arbeid; de er ikke en ekstra parallell implementering.

- [x] S.0: Flett kveld, tre Calichebesøk, arkiv/kontor, ekstra arrayoppdrag, STATION01, Nora, beleiring og fast finale til én årsakskjede. Bevar tidligere detaljkort og skriv seks frivillige engelske UFO-/humanoiddokumenter i [DALE_OFFICE_DOCUMENTS.md](DALE_OFFICE_DOCUMENTS.md). Inventer eksisterende randomfamilier og deres fase-/tidsvilkår. Dette er levert design, ikke runtime.
- [x] S.1, forfatterkilder: Engelske JSON-kilder/personkunnskap for SC01 til SC13 er levert som 79 scener og 16 dokumenter i HorrorStory, med eksisterende H-/P-/E-/T-ID-er og EveningShifts eierskap bevart. Sju lesefiler er byteidentiske ved generatorens --check; 20 kildekontraktprøver består. Dette fullfører kildearbeidet, ikke de nye verdenshandlingene, runtimekoordinatoren eller opptakene.
- [ ] S.2: Implementer versjonert kveld-/tidskoordinator etter R2.2/H1.7. Gammel Survey/K3/dip/TimeSkip må vente til SC01 til04 og T01-returfiling er fullført. La residualen starte i passiv mottaking uten å oppheve SC04s plansperre. Lag fremoverbudsjett med faktiske transport-/felt-/mørkeromsbehov og vern av 00:13:47/04:04/05:29. Gamle saker beholder gammel mapping; nødvendig innhold hoppes ikke over ved tidsbortfall.
- [ ] S.3: Produser SKY01 til05: ett tidlig kryssende lys, to i Caliche, flere sent på kvelden, ordnet feltbevegelse og større spredning før morgen. Verdensbaner, okklusjon, naturlige siktmuligheter og separat opptrådt/observert/fullført-status. Lagre global fase og faktisk sett; ingen kameratvang, fotoquota eller gratis journalbevis. Behold LineLights, komet, meteorer og StarHole.
- [x] S.3a: Fem faktiske himmelbaner med versjonert opptrådt/observert/fullført, siktsjekk og én avgrenset ny tidlig mulighet. High/Low-kontrakter og disk-/Flow-restore består. Dagens kapitteladapter er foreløpig; eksakt ny SC-/kveldsmapping i S.3/S.2 står åpen.
- [ ] S.4: Bevar hele eksisterende randominventar, odds som utgangspunkt, gjentakelsesgrenser, vinduer og saveflagg. Utvid eksisterende opptattvern til de nye møtene. Utsett ustartet kandidat uten å bruke opp engangsflagg; ingen hendelseskø etter dialog. Gi påbegynt hendelse en trygg avslutning før hovedtopp. Gammel ch1 til5-pool og bAnomalyBaseline krever eksplisitt kveldsmapping, ikke flytting av sene byvinduer til20:00. Samordnes med H5.3.
- [ ] S.5: Behold gammel tilfeldig bortføring med varsel, maks3, minst40min avstand,17 til47min tap og etterspor. Utvid NextFixed-budsjettet for ny scenariorekkefølge uten å endre legacy. Skill dette fra lokal retry i de fem nye møtene og fra fast finale. Samordne betinget gammel STATION01-bortføring med kort ny lyspassasje; ingen lovet lykkeflukt som straks overstyres av Force.
- [x] S.6: Rett LineLights-observasjonskontrakten: start/avbrudd/manglende truck skal ikke skrive full observert lyssekvens eller permanent bruke opp en uopplevd sjanse. Behold legitim faktisk opplevd engangshendelse. Levert i NP1: faktisk kamera/tegnede lys/opaquesikt, presis delnote, pause og versjonert fase. Pakket kontrakt12 kontroller/3 Writer/fresh-reader/full-Flow-restores og23 lavnivåkontrakter består. Gamle konsumertflagg beholdes. Se NIGHT_PRODUCTION_RUNTIME.md.
- [ ] S.7, gjenstående bilde/film: NP1 har levert de seks private tekst-ID-ene, tekstvisning/skalering, lesestatus og ordinært arrayservicevedlegg. De sju eksisterende dokumentene og kort feltmapperute er bevart; R5 bevarer også faktisk lest innhold ved tidlig disk-/verdensrestore. JPG-rekvisittene er produsert, men import, UV-/materialbinding og eventuell bildebevisbruk gjenstår. VHS_DALE_FIELD_01 er fortsatt et72sek manus, ikke produsert film; egen video-/transport-/avbrudds-/saveoppgave. Ingen lesekvote eller tidlige sluttspoilere.
- [x] S.7a: Seks loretekster, eget arrayserviceark og fysiske merker er integrert i kontoret. Faktisk lesing, uavhengig lesestatus, legacy og stor tekst/rulling består på High/Low med191 kontroller og5 full-Flow-diskrestores per prøve. Original film/foto/skisse og selve arraymøtet inngår ikke.
- [ ] S.8: Produser SC04s begrensede sikkerhetsmål og SC12s konkrete rutegrunn. Lokal fysisk auto-/surveyplansperre stanser automatisk kjøring, ikke all passiv mottaking/avgrenset feltkontroll. Vis fungerende utvei og fysisk rute fra bilflukt til SAROs låsbare kontrollrom, med Reyes' avtalte loggsikring og Ward på linjen før SC05. Produser fersk sted-/tidsfestet utfartsobservasjon og bud ved10.4. Test at begge valgene om SARO og senere avreise fra trygg diner gir mening.
- [ ] S.9: Prøv én hel reise uten tilfeldige funn og én med kvalifisert gammel bortføring/tidsbortfall. Ta med oversett tidlig lys, lest/ulest Dale-samling, avbrutt VHS, uavklart T02, alle fem møters lykkeflukt/lokal retry, faktisk kildekontroll, sen Earl-kilde, feltfilm før Nora, fjernark tidlig/sent, Ward-readback og gammel save. Mål faktisk transport, frykt og ro. Ingen PASS på menneskelig opplevelse fra manus alene.

## Integrasjonsforslag: historie, ARRAY-01 og Caliche

[Hovedscenarioets SC03/08/12](MISSION_FLOW_1800_ROSWELL.md) samler anbefalingen etter brainstorming, lokal game-production og kritisk Gauntlet-gjennomgang. De tidligere kortene bevares i oppslagsverket; hovedscenarioet bestemmer rekkefølgen. Detaljene skal prøves før integrasjon.

- [x] Utvikle og sammenligne tre arraymekanikker og tre byroller. Konkretiser «De peker på deg», tre meningsfulle bybesøk, årsakskjede til1947/Nora/Tomás og prøvbare løsninger på kritikk. Skil konseptgjennomgang fra menneskelig spilltest.
- [ ] I.1: Skriv nye engelske delscener for inntrengermelding/utkalling, avtalt hjelpestopp og bortføring i Caliche. Flytt kveldens svarte skikkelse til det samme bybesøket. Produser reell kvittering og savnetmelding; behold tidlig valgfritt diner/mat som separat flyt.
- [ ] I.2: Produser BY-02 med datert Earl/1981-kildekontroll før ARRAY-01, tydelig isolert bånd/live-valg og et begrenset feltregistreringsopplegg. Bånd måler lyd/tid, ikke peiling. Gi legitimt alternativ ved sen ankomst.1420 AM og1420.405 MHz forblir forskjellige.
- [ ] I.3: Produser ARRAY-01 på selve antennefeltet: fast referanse, to navngitte og fysisk tilgjengelige ståsteder, fritt valg A→B/B→A og kort retur, ekte antennerespons, geometrisk observasjon og separat kommando-/drivkontroll. Ingen tre-bokser-sikringsjakt eller forhåndsanimert fasit.
- [ ] I.4: Lag egen versjonert arrayfase og lokal gjenopptakelse, med nattkurvereservasjon og retrett. Gjenbruk per-antenne-bevegelse fra DishesFollow uten gammel Warning/Force-bortføring. Samordne ArrayBack/ArchiveDishes. Gamle saker får ingen retroaktiv hovedgate.
- [ ] I.5: Behold fem aktive møter ved å legge møte4 til arrayet. STATION01 får undersøkelse og kort avskjæring fremfor ny stor jakt. Byen skal gi hjelp, uavhengig kontroll og sen sammenstilling; kutt øvrige pliktbesøk. Produksjonsrekkefølge: hjelpekontakt→arkiv/dato→BY02→array→STATION01→Nora→samordning/varsling→P13/rute.
- [ ] I.6: Gjør avgrenset papir-/leseprøve av reisemotivasjon, kildeuavhengighet og rutevalg, deretter native arrayprototype med faktisk sikt/bevegelse/lagring. Kontroller at samme klokke og forskjellig ståsted gir forskjellig riktig respons. Frykt, varighet og troverdig retur til SARO er UNVERIFIED til prøvd.

## Skrekkrevisjon R2: aktiv kveld og flere alienmøter

Tom vurderte førsteutgaven som anemisk, med én stor aliensekvens og for lite18-24. [SC01 til04 i hovedscenarioet](MISSION_FLOW_1800_ROSWELL.md#sc01-den-første-overflygningen) samler nå kveldsakten. R2 her er designrevisjon, ikke levert T01 R2. Fem nødvendige trusselmøter, tre før midnatt, samt synlig bortføring og fast finale anbefales. Eksisterende randomhendelser beholdes i tillegg. Ingen nye spillfunksjoner er levert av planendringen.

- [x] Skriv seks konkrete førmidnatt-beats: fysisk nærvær ved vinduet18:15, flukt ved trafo19:00, svart figur inne20:15, tilkalt hjelper bortført21:00, annex-inntrenging22:15 og nært bilmøte23:20. Tider er designvinduer, ikke ventefrister.
- [x] R2.1: Lag nye engelske delscener for KVELD-01 til06 og endret personkunnskap. Etter første hendelse går arbeidsmålet til sikkerhet, hjelp og dokumentasjon. Engelske delscener og personkunnskap er produsert i S.1; konkrete hjelpe-/avreiseforsøk, lokal hold og senere nettverksvarsel er atskilt. Verdenshandlingene og nye opptak er fortsatt egne åpne oppgaver.
- [ ] R2.2: Implementer lagret kveldskoordinator med fullføringsporter og korte fremoveroverganger. Dagens Survey/K3/dip/tidshopp må ikke slette akten. Bevar00:13:47-residualen,1947-tider og05:29-finalen. Ingen seks timers virkelig venting.
- [ ] R2.3: Produser trusselmøte1 ved trafostasjonen, etter normal T01-prøve: opak dekning, avsøkt retur, alternativ passasje til bilen og faktisk fangst/gjenopptakelse. Bevar utført servicearbeid.
- [ ] R2.4: Produser særskilt regional kveldshjelper, ordinær ankomst/identifikasjon, synlig bortføring, kjøretøy/etterspor og savnetmelding. Ikke fjern nødvendige eksisterende vitner. Lyd eller tekst alene teller ikke som levert bortføring.
- [ ] R2.5: Produser annex-inntrenging med fast, tidligere hørt Dale-fragment og to virkelige fluktveier. Reaktiv enhet og usett-flyttende skyggemann skal ha klart ulike regler/utseende.
- [ ] R2.6: Produser aktivt bilmøte med lesbar sikt/lyd, mulig vellykket skjul og fysisk gyldige handlinger. Ikke kopier finalens motorstopp eller erstatt gameplay med lang venting/QTE.
- [ ] R2.7: Legg aktivt trusselmøte4 til ARRAY-01 og gjør H07 til koordinert beleiring. STATION01 beholder undersøkelsen og kort avskjæring; unngå enda en lik lang jakt. Se integrasjonsforslaget og I.1-I.6.
- [ ] R2.8: Revider tidlig lokal hold av06:00-serien og H08s fortsatte/nettverksbrede suspensjon. B-12/P07 undersøker mekanisme og kontrollmuligheter etter at aktører er sett; de skal ikke bortforklare observasjonene.
- [ ] R2.9: Prøv pacing med vanlig input: første tydelige brudd omtrent10-15min, første unnslippelse omtrent25-40min, handling/ettervirkning mellom toppene og reell ro etterpå. Tallene er uprøvde mål. Fem møter må være ulike og gjennomførbare, ikke bare plasserte figurer.

## Hele nattens oppdragsdetaljer, utvidet 25. oktober 2026

[MISSION_DETAILS_REFERENCE.md](MISSION_DETAILS_REFERENCE.md) bevarer de32 arbeidskortene og seks valgfrie undersøkelsene med løsninger, kontrollbevis, farer, hint og recovery. [Hovedscenarioet](MISSION_FLOW_1800_ROSWELL.md) bestemmer nå sammenhengen. T01 R2 og dagens kapitler er grunnlag; nye skrekkdeler er ikke implementert.

- [x] Utvid hele reisen fra 18:00 via service, residual, B-12, arkiv, STATION01, Nora, SARO-beleiring, fjernbevis og All Night til Roswell-veien og den eksisterende finalen. Dokumenter begge B-12-løsninger, normal P07, tonefølgen, 1947-tider, Nora/P13 og separate 8.15-/10.4-punkter.
- [x] Rett forfatteravvik i HorrorStory: ufullført T01 sperrer Survey i nye R2-saker, E15 er gammelt dinerutklipp uten 04:45-port, romnotat følger faktisk hendelse og ekstern sikring har konkret måleavskrift/readback. Regenerer lesefilene fra JSON og kontroller manuspakken. Ingen spillkode eller nye takes inngår.
- [ ] H1.5: Bind NATT-01 til NATT-32 til eksisterende engelske H-scener og nødvendige nye delscener/verdenshandlinger. Unngå doble overleveringer og 32 separate kapittelskjermer; hovedspørsmålet skal fortsatt være enkelt å følge.
- [ ] H1.6: Produser nødvendige, konkrete rekvisitter, motivutsnitt, kart-/måleoverlegg og tre nivåer med valgfrie hint for detaljkortene. Bevar ekte fotopiksler, valgte kontrollveier og begrensede slutninger. Ingen dokumentknapp erstatter en fysisk kontroll.
- [ ] H1.7: Innpass hele tids-/transportkjeden: aktive kveldsoppdrag før siste notch/Survey, ingen fremtidsark før 04:04, nødvendig dokumentbunt sikret før beleiring og romnotat først etter hendelsen. Rolig varsling etter flukt og ingen 04:45-sperre for E15. Ikke flytt historiske TAPE A/E10-tider eller finalen.
- [ ] H6.5: Prøv detaljvedleggets sammenhengende rute og feilsituasjoner med vanlig input: direkte ankomst uten diner, begge B-12-kontroller, usett skikkelse, avbrutt VHS, uavklart T02, tåkete kopi/manglende foto, begge fluktruter/fangst, tidlig/sen ankomst, avbrutt varsling, feil P13 og gammel sak. Mål oppdragsmengde og transport før flere pliktærend legges til.

## Prioritet etter spilltest, 25. oktober 2026 kl. 15:03 CET

Før-funn og presise bevis: [SPILLTEST_2026-10-25.md](SPILLTEST_2026-10-25.md). Retting og ny validering: [QA_FIXES_2026-10-25.md](QA_FIXES_2026-10-25.md).

- [x] Kjør hele 110-matrisen og avgrensede tillegg på installert PR157. 108/110 og 12/13 PASS; separat Low-finale FAIL. 1 405 selvtester PASS, fire unike GPU-krasj. Rapport, bilde-/kodegjennomgang og før/etter-bevaring fullført.
- [x] P1 QA00: Rett den reproduserte finalefeilen med støttet Vulkan descriptor heap på denne maskinen. Q3 består hele THE EVENT, uttrekk, sluttittel, credits, sluttfoto og meny på High og Low, samt hele 110-matrisen. Bortføring består matrisen og to ekstra forløp; samme rotårsak som den tidligere enkeltfeilen er ikke bevist.
- [x] P2 QA01/QA02-materialer/QA10: 78,1538 MiB AlwaysResident under uendret 80 MiB-grense, null aktiv/inaktiv materialfallback og null mappingfeil i fersk runtimeaudit. Ekte FRAME03/04 er visuelt leselige på High/Low. Solide ledere, CPU-wiredata og gjerdetriangler er bevart; dette er ingen FPS-måling.
- [ ] P2 QA02-VSM: Profilér oppstartskøen i normal rute/dag/natt. Kort aktivt utsnitt har ingen vedvarende overflow, men lokal motor har en forskyvning i CSV-statnavn. Verken rå nullverdi eller materialrettingen lukker denne ytelsesgjelden.
- [x] P2 QA03/QA04/QA05/QA09: Varsle om siste sikre sjekkpunkt, også ved ekte skrivefeil; bruk gjeldende bindingsnavn; lagre hodebevegelsesvalg; gjør bretttekst større uten automatisk kortoverdekking; krev faktisk P13-justering. Faktisk Slate-pan, Esc, default Enter og Xbox-knapphendelser består. Fysisk Xbox og kamerakomfort er fortsatt menneskelige kontroller.
- [x] P3 QA06/QA07/QA08: Tekstklipp og desktop-/Xbox-hjelp rettet, overgangsmål beskriver neste kapittel, og credits oppgir Unreal Engine.
- [x] Lever samme verifiserte Q3 etter PR158-merge, med 40 payloadhash, 40 uendrede personlige Saved-filer og installert smoke. Prune-plan anvendt; gjeldende og to eldre komplette testpakker beholdes, kilder/Saved/bevis bevart.
- [ ] Kjør en faktisk menneskelig ny sak til slutt uten hopp, med lyd, normal kapittel 2 til 3, fysisk Xbox og forståelse uten scriptkunnskap. Mål pacing/transport/venting og kontroller den svake anomaly-formen ved vaskerivinduet. Rapportens åtte forbedringspunkter er forslag, ikke nye implementasjoner.

## Historie, invasjon og stealth horror, planlagt 25. oktober 2026

Samlet plan: [HORROR_STORY_PLAN.md](HORROR_STORY_PLAN.md). Tom ba om at planen legges her for senere innføring. H0 er dokumentleveransen; H1 til H6 er åpne produksjonsetapper. Eldre skyggemann-, anomaly-, kontor- og trafooppgaver nedenfor inngår i disse etappene, ikke som parallelle kopier av samme arbeid. Dette er ingen melding om at spillmekanikkene er implementert.

### H0: Plan og overlevering

- [x] Gjennomgå eksisterende historie, oppdrag, kanon og faktiske Unreal-systemer, med adskilte lesende gjennomganger av historie, loop og referansespill.
- [x] Undersøk P.T., Alien: Isolation, Amnesia: The Bunker, SOMA og The Exit 8 i primærkilder; dokumenter hva som kan tilpasses uten survival-ressursøkonomi.
- [x] Lag detaljert samlet plan med invasjonens beviskjede, H01 til H10, SARO-beleiring, trusselregler, fangst, tekniske avhengigheter og ferdigkriterier. Koble fra README, ANOMALY_PLAN, memory og OPPGAVER.

### H1: Historie og målt oppdragsgrunnlag

- [ ] H1.1: Kontroller gjeldende kode og spillrute på nytt. Kartlegg tid, transport, panelbruk, venting, bevis og alle eksisterende skrekktopper. Mål faktisk spillerreise; eldre API-hopp er ikke total spilletid.
- [x] H1.2: Skriv om hovedmotivasjonen og H01 til H10 til engelske produksjonsscener. Hvert oppdrag skal ha spørsmål, fysisk handling, kontroll, følge og vei videre ved utilstrekkelig bevis.
- [x] H1.3: Skriv nødvendige, uavhengige hovedrutespor for kartlegging, reaktiv tilstedeværelse og samordnet kommende ankomst. Bevar Nora/Tomás/1947, avtrykksregelen og én slutt. Skill invasjonens nye forfatterfasit fra hva hver person faktisk vet.
- [x] H1.4: Avklar troverdig varsling og formål/rute for siste kjøretur. Kartlegg C med dagens avis/kart; unngå en åpenbart selvdestruktiv frivillig kontroll etter beleiringen. Varslingen skal ha verdi før finalen.

Produksjonsgrunnlaget for H1.2 til H1.4 er levert og senere utvidet i [Narrative/HorrorStory](Narrative/HorrorStory/README.md): ti oppdrag, 13 scenarioer, 79 scener, 16 dokumenter og kontrollert generator. Dette er forfatterkilder, ikke runtimeintegrasjon eller nye opptak. H1.1 er kildekartlagt; menneskelig måling står åpen.

### H2: Etterforskning og skyggemannintroduksjon

- [ ] H2.1: Fullfør nødvendige T01/T02 som faktisk service-/kontrollreise ved trafostasjonen. Modeller/inspeksjoner er allerede levert; den nye fortellersekvensen krever egen integrasjon. Hold STATION01, SAROs K3 og byforsyning atskilt.
- [x] H2.1a: Integrer T01 fra D01 via fysisk Caliche-kontroll til én SARO-arkivering. R2 består native ut-/returreise, 1 424 selvtester, 53 API/19 diskrestores, sju ferske Continue-prosesser, mat/D01 og elleve faktisk vurderte bilder. Gamle saker får ingen ny gate; faktisk manglende HUDmål etter filing/Continue er rettet. PR160 er flettet og samme R2 installert med 40 payloadhash og 40 uendrede personlige Saved; installert smoke og trygg pruning består. Manuell Roam/Xbox/lyd/fullnatt står fortsatt som menneskelig prøve. Se [T01_RUNTIME.md](T01_RUNTIME.md).
- [x] H2.1b: T02 levert og installert med fysisk kontor/nøkkel/feltmappe, fire faktiske kontrollprodusenter, originalfoto og én historisk returarkivering. Pakket High/Low, uavklart utfall, legacy/negative restores, hel native ut-/retur, fem ferske Continue og bevart STATION 01/Nora består innen dokumentert omfang. Se [T02_RUNTIME.md](T02_RUNTIME.md). Original VHS og menneskelig helnatt står åpne.
- [ ] H2.2: Fysisk kontor/nøkkel/feltmappe og eksisterende D02-papirer er levert i T02. Fullfør nye forfatterfunn og original 72-sekunders bevegelig VHS med avspilling, pause/strøm og lagring/Continue. Dale forblir bare telefon/radio/walkie.
- [ ] H2.3: Samordne Glimpses/SaroDark med én regel for svarte skyggemenn. Original 3D-silhuett, usett flytting, perifer observasjon, zoom/lykt/fotosøker og panelvern etter ANOMALY_PLAN del 13.
- [ ] H2.4: Fullfør én hel SARO-vindu/trafo/retur-pilot uten skade, med ekte bilder, kontrollerbar sammenligning, fysisk oppfølging og disk-Continue. Valgfri skikkelse må kunne overses.
- [ ] H2.5: Reduser dobbeltkvitteringer i paneler. Bruk saksbrettet til større slutninger; et fysisk kontrollresultat skal ikke kreve flere like dokumentknapper. Bevar eksisterende bevis-ID-er og bilder.

### H3: Avgrenset stealth-prototype på SARO

- [ ] H3.1: Mål faktisk rom-/dør-/sikttopologi fra spillerhøyde. Lag en liten håndtegnet forbindelsesgraf med to gyldige ruter og skjul som ikke er låsbare blindveier. Eventuell ny serviceforbindelse skal være fysisk og avgrenset.
- [ ] H3.2: Implementer huking og reell kropp-/øyeeksponering bak dekning i dagens Walker/kollisjonsløype, med tastatur og Xbox. En senket kamerahøyde alene er ikke ferdig dekning.
- [ ] H3.3: Lag én inntrenger med faktisk syn/lyd, siste kjente sted, søk, kontakt, forfølgelse, brutt kontakt og tilbaketrekning. Egen gjenkjennelig visuell kategori, adskilt fra skyggemannen. Vurder eksisterende alienressurs før ny modellproduksjon.
- [ ] H3.4: Implementer ekstern avsøkning gjennom vinduer, fungerende opak avskjerming og lesbare rutevinduer. Ikke bruk K3 som global bryter eller blend skjermen slik at handling blir umulig.
- [ ] H3.5: Legg inn én ordinær lydavledning og to gjennomførbare fremgangsmåter uten forbruksinventar. Fangst skal ha forståelig årsak og første oppdagelse en prøvbar reaksjonsmargin.
- [ ] H3.6: Samordne pause, notatbok, dokumenter, dialog, fotosøker og møtevern. Kontroller særskilt SAROs innendørs-/dekningsregler; World::Indoors er ikke et ferdig SARO-vern.

### H4: Hele SARO-beleiringen, The Inspection

- [ ] H4.1: Koble normal befaring, lyskontroll, sperret front, varslet ankomst, sikre måleoriginal, skjul/flukt til servicebilen og rolig etterarbeid i én sak etter Room6 og før All Night.
- [ ] H4.2: La tidligere undersøkelse gi praktisk hjelp med lys, skjerming og rutevalg. Spilleren får ingen ny sikringsjakt eller tilfeldig inventarsperre under angrepet.
- [ ] H4.3: Fangst gir kort særskilt overgang og tydelig lokal gjenopptakelse fra trygt sjekkpunkt. En vellykket flukt respekteres. Alternativ bortføringsgren med tapt tid/oppvåkning utsettes og må ikke snike seg inn gjennom gammel Force-logikk.
- [ ] H4.4: Lagre versjonert møtefase, node, siste sansede sted, søketid, lys/dører/skjerming, variant, bevis og engangseffekter. Prøv ny sak, gammel sak, før møte, støttet midtfase, etter flukt og etter fangst med faktisk disk-Continue.
- [ ] H4.5: Mål hele prototypeforløpet mot prøveanslaget 25 til 35 minutter. Behold korte jaktpartier og handling under press; fjern passiv skapventing og gjentatt tvungen fangst.

### H5: Gradvis invasjon og hel natt

- [ ] H5.1: Integrer uavhengige fjernlogger og samtidige faser i H08. Spilleren skal kunne etterprøve at dette gjelder flere steder og en kommende ankomst; ett duplisert radiosignal er utilstrekkelig.
- [ ] H5.2: Koble faktisk varsling, rolig diner/kartarbeid og begrunnet siste kjøretur. Bevar A/C-skillet, ufrivillig motorstopp, THE EVENT 05:29:00 til 05:29:47 og hele 1947-bildet med Reyes.
- [ ] H5.3: Kurater skrekktopper, lyd og jumpscares gjennom hele reisen. Nye sekvenser bruker felles nattkurve og utsetter konkurrerende kandidater uten å slette gamle hendelser eller bruke opp ustartede engangsflagg. Vern Nora, brev, kartarbeid og etterarbeid. Se S.4/S.5.
- [ ] H5.4: Innpass i eksisterende tidslinje 2 uten tilbakesprang eller straffende global nedtelling. Gamle saker beholder tider/foto og får ingen retroaktiv beleiring. Spilltekst og generert manus går gjennom korrekte kilder.

### H6: Ferdigkriterier og senere integrasjon

- [ ] H6.1: Kjør bygg/selvtest og kort relevant pakket kontroll når kode endres: loop, faktisk dekning/syn/lyd, begge ruter, pause, foto, disk-Continue, gammel kapittelrute og finale. Ingen full testmatrise bare for å krysse av designarbeid.
- [ ] H6.2: Kontroller spillerhøyde, skjul og nødvendige lyssignaler i High/Low og Clean/filmprofil, med redusert blinking/bevegelse og lydtekster. Lytt på romlig trussellyd og kritisk tale.
- [ ] H6.3: Menneskelig prøve med vanlig input, deretter fysisk Xbox. Bruk gjerne fem ukjente spillere; registrer forstått regel, hvorfor oppdagelse skjedde, reell flukt, venting, uro og irritasjon. Juster før flere møter bygges.
- [ ] H6.4: Spill hele historien uten fixturehopp og kontroller at hovedruten alene bærer invasjonens avsløring og reisemotivasjonen. Før faktisk kilde/pakke/teststatus i overleveringen før senere integrasjon/installasjon.

## Neste originalmodeller, feltreise 25. oktober 2026

- [x] STATION01-felthytte med faktiske åpninger, redigerbart interiør og separat dør.
- [x] Transit, feltlampe/deksel, feltreceiver, felttelefon/rør, timingbenk/logg/diagram og bevisrekvisitter.
- [x] SARO-arkiv med skap, kartotek, reoler, feltboks, bevegelig skuff, bryterbord og samtaleopptaker, med levende papir/indikatorer bevart.
- [x] Faktisk Blender-eksportkontroll, native import, kort pakket feltreise og levering av originaler/spillbilder.

- [ ] Senere menneskelig feltreise med fysisk Xbox og lytting; vurder motivlesbarhet i nattfoto. Ingen fullnatt-/helruteytelse verifisert i modellpasset.

## Skyggemenn, designforslag 25. oktober 2026

- [x] Parallell analyse av psykologi, eksisterende skikkelseskode og møter; forslag og primærkilder i [ANOMALY_PLAN.md, del 13](ANOMALY_PLAN.md#13-skyggemenn-og-oppbygging-av-ubehag-25-oktober-2026). Ingen ny runtime eller modell produsert.
- [ ] P0/P1: én konsekvent observasjonsregel, original helt svart 3D-menneskesilhuett og håndplasserte ståsteder. Avklar gamle glimpse-/SARO-forsvinninger før samme visuelle kategori tas i bruk.
- [ ] P2/P3: konservativ synstest, panel-/dialogvern, lagret tilstand og én hel SARO-vindu/trafo/retur-pilot med ekte foto og eksisterende spenningskurve. Ingen skade eller død i introduksjonen.
- [ ] P4: kort menneskelig prøve av forstått regel, uro og irritasjon før senere array-/motell-/dinermøter og en avgrenset sen panikktopp. Antall, avstand og varighet stemmes etter prøven.

## SARO-ankomst, 24. oktober 2026

- [x] Flytt skiltet utenfor kjørekorridoren, lag ryddig besøksparkering og gangsti, forbedre skilt/vinduer og behold egen personbil atskilt fra stabil servicepickup.
- [x] B/Q-matmeny med D-pad/A, nedsetting, bortstuing, fremtaking og kasting, med varig lagring og original måltidsflyt bevart.
- [x] A3: editor/pakking, 1400 selvtester, fem fokuserte pakkede scenarioer og19 faktisk vurderte bilder. PR154 flettet og samme A3 installert,40 payloadhash og46 uendrede Saved-filer. Se SARO_ARRIVAL.md.
- [ ] Menneskelig prøve av SARO-parkering og B-matmeny med fysisk Xbox-kontroller. Syntetisk Slate-ruting og vanlig native kjøre-/gangkontroller er prøvd.

## Highway og åpningstur, 24. oktober 2026

- [ ] Senere RT-minnepolering: eksisterende rød AlwaysResident-advarsel 83.107 MiB / 400 MiB ved 20%-terskelen finnes allerede i D2 og er uendret i H3. Optimaliser relevante mesh-LOD-er etter måling; ikke skjul varslingen eller endre budsjettet for å få rene testbilder.

- [x] Roswell-sperre, faktisk motorlyd, Motor Court-naturkontinuitet og én detaljert strømlinje. H3/464078be, 1392 selvtester, 7 korte pakkede scenarioer og 8 vurderte overgangsbilder PASS; PR 153 flettet og samme H3 installert med 40 payloadhash og 46 uendrede Saved-filer. Se HIGHWAY_FIXES.md.

- [x] Mesa Diner: original Blender-fasade/interiør, parkering/skilt/tractor+reefer, original interaksjon bevart. D2 målrettet pakket kontroll9/9; merge/installasjonsstatus i MESA_DINER.md.

## Trafostasjon, 24. oktober 2026

- [x] Bygg original Blender-transformator og serviceanlegg, plasser tilgjengelig langs eksisterende rute og koble undersøkelser til faktisk scene.
- [x] Kort målrettet modell-/plassering-/interaksjonskontroll og commit/push. Seks sluttkontroller, ingen full verdensrunde.
- [x] PR150 flettet og samme testede kandidat S2 installert, 40 payloadhash og 48 uendrede Saved-filer.
- [x] Rett nattens utvaskede nærlys på skapet: native nærkompensasjon, uendret ordinær rekkevidde, kontrollert på kandidatP1.
- [x] Originale avgangsstolper/ledere, vedlikeholdsskur med kabeltromler/verktøykasse og kontaktstein. Kulverten ved avkjøringen beholdes; ingen ny pipe i flat bakke.
- [x] PR151 flettet og testet kandidat P1 installert: 40 payloadhash og 48 uendrede Saved-filer.
- [ ] Senere terrengpolering: mer naturlige eroderte grusoverganger rundt eksisterende fundament. Ingen terrengendring er gjort i dette modellpasset.


## Xbox-kontroller, 24. oktober 2026

- [x] Implementer analog gange/blikk, biltriggere, fast Xbox-oppsett, menyer/panelpeker og separate lagrede kontrollerinnstillinger.
- [x] Bygg og kontroller faktisk kernel/SDL/Slate/gameplay, 1373 selvtester, elleve native opptak og 16 relevante regresjoner. Full verdensrunde stoppet etter Toms beskjed om mindre testomfang.
- [x] Commit/push, PR149 flettet til unreal-port-core og testet kandidat X4 installert: 40 payloadhash, 48 uendrede Saved-filer og verifisert sikkerhetskopi.
- [ ] Menneskelig prøve med Toms Xbox-kontroller. Se GAMEPAD.md for faktisk teststatus.


## Filmisk New Mexico-natur, muligheter lagret 24. oktober 2026

- [ ] Undersøk Quixels Canyons of Utah / Desert Western på Fab som kilde til skannede klipper, steiner og bakke. Samlingen er ikke funnet lokalt; Epic-kontoens eierskap og faktisk nedlastingslisens må kontrolleres. Hele Megascans-biblioteket er ikke automatisk gratis. Bevar lisensbevis og hold tredjeparts råassets utenfor offentlig Git.
- [ ] Start utvalget med gratisoppføringer som Desert Western Cliff Layered XL 14, Canyon Sandstone Rocky Ground og Desert Western Cluster Rock Medium 05. Kontroller pris, format og lisens på selve nedlastingen, og importer bare relevante deler.
- [ ] Form mesaer etter New Mexico-referanser, eksempelvis Chaco/San Juan Basin: lagdelt sandstein, separate klippevegger, eroderte skråninger/rasmark og naturlige overganger mot dalbunnen. Skannede Utah-overflater erstatter ikke korrekt silhuett og skala.
- [ ] Prøv tre avstandslag i gatebiten: detaljert skannet grus/stein, grøfter og vegetasjon nært; belyst 3D-geometri med PBR i mellomgrunnen; enkel fjellgeometri med fotobaserte detaljer og atmosfærisk dis langt unna.
- [ ] Lag en avgrenset prøve med fotografisk kameraprojeksjon / matte painting på fjerngeometri. Kontroller parallax, sideveis spillerbevegelse, strekking/skjulte flater og innbakt sol/skygge ved dagslys, skumring og natt. Bruk vanlig belyst PBR nær spilleren; skill kameraprojeksjon fra verdensjustert/triplanar materialprojeksjon.
- [ ] Vurder Mesh Terrain i en separat prøve dersom terrengverktøyene trengs. UE 5.8.4 har MeshTerrainMode, merket Experimental og ikke aktivert her. Ingen automatisk migrering av fungerende verden; mål faktisk spillerutsikt, kostnad og regresjoner først.
- [ ] Sammenlign samme gateutsikt ved bensinstasjon, suvenirbutikk og radiostasjon fra spillerhøyde i dagslys, skumring og natt. Kvalitetsmål er konseptbildets natur, troverdig dybde, materialer og lys; eksisterende CN5 er fortsatt stilisert.

Kilder fra vurderingen: [Quixels Moab-opptak](https://www.youtube.com/watch?v=ND53AjjPA8Q), [Quixel på Fab](https://www.fab.com/sellers/Quixel%20Megascans?q=arid), [tidligere Fab-/Quixel-rettigheter](https://dev.epicgames.com/documentation/fab/purchasing-and-downloading-assets-in-fab), [Chacos geologi](https://home.nps.gov/chcu/learn/nature/geology.htm), [Mesh Terrain](https://dev.epicgames.com/documentation/unreal-engine/mesh-terrain-in-unreal-engine), [materialprojeksjon](https://dev.epicgames.com/documentation/unreal-engine/texturing-material-functions-in-unreal-engine). Muligheter, ikke implementerte eller visuelt godkjente løsninger.

## Volumetriske skyer, overtatt fra Claude 24. oktober 2026

- [x] Kode f55b760c/04d98231 finnes allerede i integrert natur-/filmversjon; Claudes SKY.md og dokumentlenker er bevart.
- [x] Fullfør cache og sluttdokumentasjon på integrert natur/film/sky-kilde. Claudes ufullstendige opptak bygger på eldre sky-pakke og andre shader keys, og brukes ikke som cachebevis for dagens pakke.
- [x] Kontroller nye pakkede himmelbilder ved dag, kveld, natt, midnatt og daggry, Low-fallback, komet/stjernehull, selvtester og full screenshots.sh før fletting/installasjon.
- [x] Lokalt flettet og installert med 40 kontrollerte payloadfiler, 48 uendrede Saved-filer og backup. Bildene ligger i Signal47 skyer og lys på skrivebordet.
- [x] Dokumenter faktisk cache-/cook-/runtime-binding og målte begrensninger; behold menneskelig gjennomspilling/lytting og lang tids skybevegelse som UNVERIFIED til prøvd.

- [ ] Reduser ordnet rutedithering i enkelte månelyse High/Clean-skyer; sluttbildene dokumenterer dette som åpen kvalitetsgjeld.
- [ ] Undersøk gjenværende oppstartshakking: det ene varme driver-cache-forsøket har 372,56 ms synlig hakk og 57,9 FPS første synlige 10 s. Cachebevis er ikke garanti for jevn oppstart.

## Caliche natur, 24. oktober 2026

- [x] Tre originale Blenderkit, 36 terrengfliser, 12 plante-/steinprototyper med tre LOD-er og revidert lagdelt geologi.
- [x] 33 faktiske spillerhøye bilder ved dag/skumring/natt gjennomgått. Elleve funksjonstester og 1328 selvtester, kilde og ferske native kontroller PASS på fryst CN5.
- [x] Hele 110-scenario screenshots.sh, lokal fletting og installasjon med backup, 40 payloadhash og 51 uendrede Saved-filer. Alle 110 natur-auditer er bevart, inkludert én omkjøring etter et duplisert loggfilnavn. Redigerbare kilder og bildevelgere i Signal47 Caliche natur; se CALICHE_NATURE_PASS.md.
- [ ] Videre fotolikhet: mer naturlig talus/fotrelieff og adskilte berglag, mindre gjentatte busksilhuetter og rikere terrengdrenering. CN5 er et stilisert produksjonsinkrement, ikke nær-foto-godkjenning.

## Caliche konseptpass, 24. oktober 2026

- [x] Ni visuelle runder av den bestilte gatebiten, inkludert fire originale Blender-kilder, PBR/eksporter, kontaktbakke, planter/stein og fjern horisont.
- [x] 42 faktiske spillerhøye bilder ved dag/skumring/natt gjennomgått. Full 110/110 screenshots.sh, elleve runtimeprøver, fotopilot, horisontfallback og native/kildeportabilitet består på samme pakke.
- [x] Lokalt flettet og installert med backup, hashkontroll og uendrede Saved-data. Skrivebordsleveranse med originaler, bildevelger, prioritert modelliste og bevis. Se CALICHE_CONCEPT_PASS.md.
- [x] Modellpasset etter gateprøven er senere levert: Mesa Diner, sheriffkontor, motell, vaskeri/fôrbutikk, kino og kirke. Se PRODUCTION_MODELS.md for M6 og faktisk leveringsstatus.

## Skumlere pilotrunde, codex/anomaly-pilot, 23. oktober 2026

- [x] Sparsomme verdenshint, plausible uavklarte A02-valg, nattefilm ved vaskeriet, rettet/andre flyttet pappfigur og P01 som kommer nærmere ved gange med ett ON AIR-blink.
- [x] Save v3-migrering, ekte bevart film og tekniske selvtester; egne avbruddsrutiner for lys/lyd. Ingen Evening-/voice-endringer eller endring av harde storygrenser.
- [x] Sluttpakke 01d92bd4 med core206afd9b: 1293 selvtester, åtte pilotscenarioer, High/VHS og Low, ekte gammel migrering, 7/7 historieregresjoner og to kameramotiv. Bilder/før/etter visuelt kontrollert. Se ANOMALY_SCARIER.md for bevis og UNVERIFIED-punkter. Content-lenker/OpenOrder ryddet; ingen push/PR/installasjon.
- [ ] Tom prøver lydmiksen, figurens direkte nærvær, fotosporets lesbarhet og frykt med ekte input. Automatikk avgjør ikke dette.
## Takeaway og samlet åpning, codex/evening-runtime, 24. oktober 2026
- [x] Én pose/kaffe kan settes ned og plukkes opp på seks faste steder; plassering, kjøp og ventende svar lagres uten duplikat. Bilplassene følger bilen.
- [x] Valgfri kveldsmat etter D01 ved skrivebord/annex, fra hånd eller bord. Firesekunders overgang, tolv minutter fremover innen tidsbudsjettet, papir/tom kopp etterpå. Lasting under overgangen gjentar ingenting.
- [x] Holdt mat skjules ved kamera/lykt/notatbok og har ingen kollisjon. Gamle arkivbordhandlingen beholdes med korrekt query-høyde; bare én diner-servering tegnes.
- [x] Sluttkilde f83f3dc9/core92dc7a47: bygg/pakking, 1298 selvtester, 33 vanlige bruksforløp, 21 ferske matlastinger, bilbilder, 48 inngangspasseringer, hel burgerrute og tre gamle regresjoner PASS. Bilder gjennomgått, seks pakkehasher uendret. Se EVENING_TAKEAWAY.md. Content-lenker/OpenOrder ryddet; lokale committer, ingen push/PR/installasjon.
- [ ] Menneskelig input og lytting etter Claudes integrasjon med nyere core6614cd90. Denne pakken fryser core92dc7a47; ingen sammenhengende fullnatt eller ny ytelsesmåling påstås.

## Åpningens biler og inngang, codex/evening-runtime, 23. oktober 2026

- [x] Privat wagon med eget karosseri, førermiljø og åpen dør; alle SARO-riggdeler skjult og gjenopprettet på riktig bil. Komplett delt standardmodell bevart.
- [x] Plan SARO 07 på originalpad, samsvarende kollisjon og klar besøksplass; gamle utrygge åpningstilstander normaliseres.
- [x] Reprodusert terskellåsing ved lukking under spilleren, sperret lukking og reparert gammel save før lasterens Walkable-port. Samme posisjon og klokke beholdes.
- [x] Flettet bygg/pakking, 1286 selvtester, 48 vanlige gangpasseringer, fire ferske lastinger pluss gammel terskelsave, hel burgerrute, seks regresjonshopp og alle påkrevde bil-/inngangsbilder PASS. Rapport EVENING_VEHICLE_FIXES.md, pakkekilde 66d2b5a6/core04517414; hasher uendret, lenker/OpenOrder ryddet. Ingen push/PR/installasjon, Claude integrerer.
- [ ] Menneskelig gjentakelse etter Claudes integrasjon. Denne leveransen beviser automatiske bevegelses-/lastescenarioer, ikke Toms neste fullnatt med ekte input.
## Filmfiltre, codex/film-looks, 23. oktober 2026
- [x] Carpenter 35 mm, Desert 16 mm, Super 8, forbedret VHS og Clean; styrke, lagring og tilgjengelighetskobling.
- [x] Separat PictureMode med bevart Vhs-blend og capture-kontrakt; privat materialregenerering.
- [x] Sluttpakke 8ba39556 med core 04517414, 1298 selvtester, ni funksjons-/UI-/diskprøver, 36 kamerabilder og varm High/1440p-kostnad. Claude-gjennomgang, cache-remapping og hasher PASS. Se FILM_LOOKS.md.
- [ ] Menneskelig vurdering av levende korn/flimring og komfort; HDR og SteamOS/Deck. Ny full PSO-matrise for hele den endrede verdenen gjenstår før en slik dekningspåstand.

## Anomaly-pilot, codex/anomaly-pilot, 23. oktober 2026

- [x] N01 med faktisk utskrift mot kveldens service-/sendeplan, korrigert feilvalg og fysisk forespørsel.
- [x] N02 med to faktiske kameravinkler, lampens kontroll og reviderbart resultat.
- [x] A02 med én figur, ekte bevart normalfilm/arkiv, flyttet posisjon og aktiv bildesammenligning; P01 med kort printeretterspill og fysisk fasadekontroll.
- [x] Tidlig kamera/rutinekjøring og save v3-rutinekontrollpunkt; ingen senere kapitteloppgaver løses. Migrering uten oppdiktede eksponeringer; manglende film er eksplisitt utilgjengelig også etter verdenslasting.
- [x] Sluttpakke/1278 selvtester, High/VHS og Low/Clean, feilfilm/migrering, 7/7 kortregresjoner, begge åpningsruter og TV-opptak. Statisk Caliche p95 +0,58 %, ingen konkurrerende tung jobb; fire sluttbilder visuelt kontrollert. Se ANOMALY_RUNTIME.md for avgrensninger. Content-lenker/OpenOrder er ryddet; Claude integrerer, ingen installasjon/push/PR.
- [ ] Menneskelig pilot/Gauntlet: ekte input, lydlytting, forstått kontroll og stigende uro. Ikke erstatt dette med automatiske API-scenarioer. Fullnatt uten testkutt og faktisk spilletid gjenstår.
- [ ] Senere plansteg: Dales kontor/VHS, trafostasjon, bygningsløft og større UFO-/aliensekvenser. Ingen ny anomalikvote for P13.

## Kveldspakkens lyd, codex/evening-runtime, 23. oktober 2026

- [x] 109 takes importert strengt som byteidentisk PCM og SoundWave, med 110 cues og eksakt D61-alias. Andre banker bevart.
- [x] Receiverfilter/radiobuss for host og Dale; posisjonell servitrise, målte captions og uavhengig karakterlyd. Rettet gjentatt panelåpning som stoppet ny stemme, og fjernet misvisende generell opplesningsboks.
- [x] Sluttpakke 9d638a0b: 1258 selvtester, hel burgerrute med 36 fullførte klipp og faktiske nivåer, pause/avbrudd og disk-/verdenslasting ved kjøp/D01, fem bilder pluss telefontranskripsjon, kortregresjon 6/6 PASS. Rapport EVENING_AUDIO.md; Content-lenker/OpenOrder ryddet, ingen generert lyd i Git eller installasjon.
- [ ] Tom lytter på D01_FOOD_002 og D01_BRIEF_005, deretter stemmelikhet/innlevelse og miks i spillet. Fullnatt og VHS er fortsatt ikke levert her.

## Toms trinn 1 og 2, codex/evening-runtime, 23. oktober 2026

- [x] Manusadapter og datadrevne porter for senere faser, knyttet til eksisterende dialog, telefon, radio, notatbok, mål og lagring.
- [x] Spillbar personbil fra sør gjennom Caliche, valgfri diner med mat i hånden, én betalt ordre og atomisk lagret svar, SARO 18:00 én gang og D01 bare på telefon. Den gamle rutinen fortsetter etter overleveringen.
- [x] Kveld/natt/gry på sammenhengende akse, radio-/NPC-tider og kapitler, uttrykkelig legacy-lagring, faktisk utskriftsstempel og bevarte foto-/1947-referanser. Tidskomprimering og gammel/ny-tabell i EVENING_TIMELINE.md.
- [x] Kveldssol, native lys og skumringskurver; ingen minutt-HUD under innkjøringen. Stillbilder gjennomgås i EVENING_SKY.md.
- [x] Pakket kandidat og målrettet regresjon: 1254 selvtester, fire åpningsruter, legacy-migrering, gamle/nye hopp, native telefonbruk og fem siste bilder PASS. Sluttkilde 3d321da1; kildeoverlevering til Claude, ingen installasjon. Se EVENING_RUNTIME.md.
- [ ] Menneskelig gjennomspilling med ekte input, lytting og målt total spilletid. De 109 takesene er senere produsert av Claude-hjelper og integrert av Codex; VHS og verdensinnholdet i senere trinn gjenstår.


## Grafikkpass på port-gfx, 23. oktober 2026

- [x] Viderefør WIP og flett siste core/Props/dokumentasjon uten egne Props-endringer.
- [x] Materialcook uten fallback, 118 profiler/CRT-glass og integrasjonskommando for generert innhold. V5-selvtest (1220) og native fem emitting-slots av/på; V9 terminalfront visuelt PASS.
- [x] Korrigerte 20 + 20 referanser og 40 gjennomgåtte før/etter-par med originalhash/galleri. Props-forskjeller er oppgitt.
- [x] Full 38 × 6 samme-cook PSO-opptak, 4215 stabile PSO-er, sluttcook 599 og runtime-GUID/222 bundled graphics: cachebevis 9/9 PASS.
- [x] Kald/varm sluttpakke, komplette første 10 verdens-/synlige sekunder. READY 11,337/5,594 s, synlige maks 31,585/31,888 ms og ingen rammer over 33,333 ms.
- [x] Sluttpakkens sju tilfeller, 1220-selvtest og 36 fasebilder visuelt kontrollert, samt korrigerte seks grafikkstrømtilstander. Gamle matrix-dark-bilder brukes ikke som av-bevis.
- [x] PreviousNight-kjøring og to identisk-geometri fyllpar visuelt PASS, compare/contrast-control.png og contrast-yard.png, med proveniens i contrast-manifest.json.
- [x] Åtte lengre High/1440p-prøver ved natt/daggry PASS 93,81–106,73 fps, maks p95 18,142 ms. Korte matriseprøver og Low/Ultra er dokumentert, ikke bruk dem som stabilt 60 fps-bevis.
- [x] Sluttregresjon PASS 151,3 s: tittel, pause, notatbok, kjøring, THE EVENT og valgt 30 fps, 285 synlige cap-intervaller med 33,333457 ms i snitt.
- [x] Sluttall og integrasjonskommando i LIGHTING/egen OPP-rad/minne/logg. Content/Audio/Licensed/Free-lenker fjernet med mål bevart, OpenOrder gjenopprettet og syntaks/diffkontroll PASS.
- [x] Kilde og bundlet SPC levert i vanlig commit på port-gfx, med alle grafikkprøver og bildepar dokumentert. Ingen installasjon, push eller PR bestilt.

## Anomaly spotting og byens funksjon (2026-10-23)

- [x] Utdypet design- og gjennomføringsplan etter Toms tilslutning til retningen: `ANOMALY_PLAN.md`. Bare planarbeid, ingen ny gameplay eller installasjon.
- [x] Revidert planen med tidligere kveld, romsligere kapitteltider, trafostasjon og bedre bybygninger etter Toms oppfølging. Start 18:00 og residual 00:13:47 er senere implementert på codex/evening-runtime; se nyere leveransestatus ovenfor.
- [x] Innarbeidet Toms innkjøring med lystig radio, valgfritt matstopp og skiftstart ved SARO 18:00, samt Dale med Kinski-stemme og hemmelig kontor. Bare plan, ingen nye opptak eller modeller.
- [x] Skrevet og strukturert hele manuspakken i `Narrative/EveningShift`: innkjøring/diner, fjernoverlevering og samtaler, kontornøkkel/filer/VHS, trafo og sammenligninger. JSON-/eksportkontroll, opptaksalias og 14 offline forfatterruter er kontrollert.
- [x] Produser 109 unike stemmetakes, importer gjennom streng katalog og koble målt lyd/undertekst til telefon, bilradio og posisjonell servitrise. Produksjon `a4626053`, runtime og testgrunnlag i EVENING_AUDIO.md.
- [ ] Produser det 72 sekunders bevegelige VHS-opptaket fra pakken. Lytt på alle takes, særlig D01_FOOD_002 og D01_BRIEF_005.
- [ ] Trinn 0: mål dagens reise med ekte input, klokke, transport, triggere og passiv venting; behold baseline. Prøv også lang All Night og avreise etter 05:20: dagens ch6 setter avreisen til 05:20, så mulig tilbakesprang må dokumenteres og løses før ny fullnatt godkjennes.
- [x] Trinn 0: tidsreferanseoversikt for 1986-hendelsen og kapittelporter, datoakse gjennom midnatt, kveldsradio/NPC-er og eldre lagringer. 1947-tider og 05:29-finalen beholdt; se EVENING_TIMELINE.md.
- [x] Trinn 1: kveldsstart, tidlig kit/truck og N01/N02/A02/P01 med normalreferanser, aktiv sammenligning, fysisk oppfølging og lagring. Pilot og nøyaktig testgrunnlag i ANOMALY_RUNTIME.md.
- [x] Første Caliche-gatebit med bensinstasjon, suvenirbutikk/interiør, studio og lokal bakke. Installert; 24 spillerhøydebilder og funksjons-/fotoregresjoner PASS. Se Signal47/Tools/calichestreet/acceptance.json. Menneskelig gjennomspilling gjenstår.
- [x] Trinn 1: I01/D01 med innkjøringsbil, radio, valgfri matbestilling, egne tidlige diner-tilstander, lagret ankomst og Dales telefonoverlevering 18:00. Rollen er KUN telefon/radio/walkie; ingen fysisk Dale. Egen opptaksbank er koblet til uavhengig av generell opplesning; nye takes er nå importert og koblet til kanalene, se EVENING_AUDIO.md.
- [ ] Trinn 1: prøv begge delstrekninger, migrering, sen ankomst og fem observerbare Gauntlet-mål. Merk testkuttet; ikke kall prøven hel gjennomspilling.
- [ ] Trinn 2: ny trafostasjon i ørkenen med avstikker, T01/T02, kontrollresultat, lys/lyd og lagring. Skill vanlig feil fra senere avvik; bevar STATION 01 og K3s rolle.
- [ ] Trinn 2: D02, fysisk reservenøkkel til Dales eksisterende kontordør, private utklipp/filer og faktisk bevegelig VHS/TV med lyd, pause/strøm/lagring. Ett funn gir etterprøvbar feltoppgave; ikke erstatte Nora, avisavsløringen eller P13.
- [ ] Trinn 2: gjør kvittering, kart, telefon og vitner til etterprøving; prøv All Night fra 04:10 og lysbølgen før 04:55. Ferdigstill bedre diner, sheriffkontor og vaskeri med nødvendige rom.
- [ ] Trinn 3: fjern himmelform, sen egen alien og større UFO-observasjon; samordne eksisterende glimt/bortføringer. Resten av byens utvendige bygg får samme kvalitetsløft, med bevart funksjon og nye kveld/natt/gry-bilder.
- [ ] Trinn 4: hele natten med ekte input, riktig klokke og kapittelflyt, lagring, nødvendig regresjon og målt ytelse. Fullnatt og opplevelsesmål er foreløpig UNVERIFIED.


## Native lys og skygger (2026-10-22)

- [x] Erstatt materialenes flood/hemi og gamle projeksjoner med ekte lamper; behold historienes tilstand og delte lyskilder.
- [x] Lommelykt og billykter med skygger, volumetrisk tåke, Lumen, svak måne/skylight og levende daggry. Fast eksponering og VHS/Clean, uten lysbryter mellom Web og Unreal.
- [x] Lokalt editorbygg og målrettede native scenarier fra SARO til den gamle veien. Side-ved-side-bilder og detaljer i LIGHTING.md.
- [x] Taklys fra faktiske armaturer i rom/korridor/annex/lab, synkron emissive, skrivebordslamper fra skjermåpningene og egen grønn glød foran hver registrerte skjerm.
- [x] Varme, skyggekastende array-spotter fra de elleve eksisterende feltarmaturene. Grå diffuserflater og Ops-ghost under strømbrudd rettet.
- [x] High/1440p med TSR fra 60 %: rom 73,4 fps, gård 95,5 fps, p95 15,330/11,527 ms uten andre tunge prosesser i målevinduet. Low/Clean 90,0 fps i rommet, Ultra 39,2 fps i gården. Detaljer i LIGHTING.md.
- [ ] Pakket bygg og menneskelig gjennomspilling med de nye lysene. Standardutgaven er ikke oppdatert av dette arbeidet.
- [ ] Kjør det isolerte materialsteget ved integrasjon med eldre generert Content, før kandidat eller pakket utgave brukes.

## Prøven av prologen

- [x] Prosjektet bygger, modellene er importert, og den pakkede builden starter med kontrollrommet, gården og himmelen (2026-10-21).
- [x] Alle fjorten testkjøringene gjennom med verdenen fra nettversjonen (2026-10-21).
- [ ] Tom går rundt i rommet og ut på gården og sier om prøven holder: lys, stemning, modellenes skala og retning.
- [x] VT323 og Special Elite som i nettversjonen (2026-10-21).
- [x] Verdenen hentes fra nettversjonen (Tools/webexport), så modellene står som der (2026-10-21).
- [ ] Bildet sammenlignet med nettversjonen fra de samme stedene: rommet, gården, antennene, vinduet, racket.

## Hvis prøven holder (hele konverteringen)

- [ ] Prologen spilt av et menneske fra 23:41 til sluttkortet.
- [ ] Historien som JSON felles for web og Unreal, slik at tekst og valg ikke må skrives to steder.
- [ ] Feltkameraet og fotolaben.
- [x] Lyd og musikk fra web/public (2026-10-21).
- [x] `core/Audio.ts`, `signalVoice.ts` og `ambience.ts` portert på en Web Audio i C++, kryssjekket mot Chromium (2026-10-22).
- [ ] Natta ute i gang som i main.ts: `StartNight()` etter `StartRoomTone()`, `NightLife(dt, spillerens posisjon, klokka)` hver ramme, `SetSpace(room|yard|lab)` ved områdebytte, `Suspend` i pausen, volumene fra innstillingene.
- [ ] Prologen spiller tonene med `Beep(...)` og nettversjonens tall i stedet for filene fra make-synth.py, og radioen i høyde 1,45 som i Prologue.ts.
- [ ] ReadAloud og RecordedDialogue (stemmebankene under /Game/Audio/Voice, katalogene i Content/Audio/Raw/voices.json); `decoder.ts` trenger en ConvolverNode.
- [x] RadioVoice, radio.ts, tripRadio.ts og score.ts med kilde for lange opptak, volum, innkobling og selvtester (2026-10-22).
- [ ] Radio-/musikkporten prøvd i samlet pakket bygg med hele screenshots.sh og menneskelig lytting/input. Koble øvrige nattsystemer til Score.Held og samme Main.TownSeen; se RADIO_PORT.md.
- [ ] Lagring ved kontrollpunkter.
- [ ] Kapittel 1 til 6 (web/src/story) og områdene. Verdenene er med i eksporten (AS47Web::Enter).
- [x] Kjøringen (web/src/drive) og turene i World.ts (2026-10-22). Gjenstår: eksporten kjøres på nytt for `drive` i manifestet, lydene laget med Web Audio (dekk, vind, startmotor, ferist), lyktenes mønster (headSlots) i flomlysmaterialet, lyktene på dineren, byen og motellet (shareLights), radioen.
- [x] Glødepunktene blinker som i glow.ts (2026-10-22, `S47Glow`).

## Kapittel 2 (2026-10-22)

- [x] Kapittel 2 portert (S47Chapter2, S47Annex, S47PhoneScripts, S47Drawings2, S47Panels2), selvtest og gjennomkjøring uten verden.
- [ ] Dørene (Doors.onChange) videre til kapitlenes `DoorChanged`, som Prologue.doorChanged.
- [ ] Lagringen kobles til `US47Chapter2::OnSave` og `Host->LoadCase`.
- [ ] Kapittelkortets ring (`bHoldRing`, `RingOut`, `RingAgain`, `RingShown`) når kortet er portert; kapittel 1 sin slutt kaller `US47Chapter2::AfterChapterOne(method)`.
- [x] Bryterbordets linjelamper og opptakerens papirstrimmel (archive047.ts): i export.js (2026-10-22); kjør eksporten på nytt for å få håndtakene (C++ finner dem uten).
- [ ] S47Drawings og S47Drawings2, S47Panels og S47Panels2 slås sammen.
- [x] Lagring ved kontrollpunkter, tittel, pause og notatbok (2026-10-22, `S47Saves`, `S47Flow`).
- [ ] Kapittel 1 til 6 (web/src/story), kjøringen (web/src/drive) og områdene. Verdenene er med i eksporten (AS47Web::Enter).
- [x] Glødepunktene blinker som i glow.ts (2026-10-22, `S47Glow`).

## Kapittel 4 (2026-10-22)

- [x] Kapittel 4 portert (S47Chapter4, S47Room6, S47Motel, S47Walkie, S47CB, S47NpcDialogue, samtalepanelet i S47Panels), selvtest og kjøringer i -game uten pakket build.
- [ ] Med kapittel 3: P11 og P12 trenger nålene P08, e10.carrier og hut.rx på saksbrettet (kapittel 3 sin regelbok). Kapittel 3 sin slutt kaller `US47Chapter4::BeginChapter4(nullptr)`, og kapittel 3 sin `truckTrip` går foran i `US47World::TruckChoice`.
- [ ] Kjør `Scripts/audio-raw.py` (eller `import-audio.sh`) på nytt i hovedkopien, så replikkene i rom 6 har stemme (rollestemmene er nå 50).
- [ ] Hjelperne fra main.ts: `Taken` (abduction.ts), `OfficePhone` (officePhone.ts), og at CB-anropet venter på bortføringen og truckTaken.
- [ ] Lyset fra vinduene på gangveien (windowLight.ts) og papirbildene (`cardField`, `letterPaper`) i dokumentvisningen.
- [ ] Continue i rom 6 prøvd med en ekte lagring.

## Etter lagring og flyt

- [ ] Kjør screenshots.sh med den pakkede builden (title, pause) og se tittelkameraet over gården og miniatyrbildene i lagringene med verdenen på plass.
- [ ] Saksbrettets oppsett i lagringen: sett `M->Flow->SaveBoard` og `LoadBoard` til CaseBoard sin Save()/Load().
- [x] Feltkameraet og rullen i Kit-fanen: `M->Flow->HaveCamera` og `RollText` (2026-10-22).
- [ ] Musikk- og radionivået (Music, Radio) inn i lydmodulen når den har egne busser; volumet virker allerede.
- [ ] Feltkameraet og rullen i Kit-fanen: sett `M->Flow->HaveCamera` og `RollText` når kapittel 1 og albumet er portert.
- [x] Musikk- og radionivået (Music, Radio) inn i lydmodulens busser, inklusive tittellåten (2026-10-22).
- [x] Bildevalg erstattet av filmprofilene i FILM_LOOKS.md; gamle `heavy`-valg normaliseres til forbedret VHS.
- [ ] Fortelleren (Serling) til åpningskortet og kapittelkortet når stemmene er portert.

## Nattens lag (2026-10-22)

- [x] Loggen, kurven, nattens hendelser, ropene, glimtene, lommelykta og albumet (`US47Night`), med lagringen.
- [ ] Krokene i `FS47NightHooks` settes når systemene porteres: bortføringen, veikanten (stillheten), byens hendelser, skrittene på taket, motellets fjernsyn, lysene på B, Earls spole, SARO mørk, kontortelefonen, arkivtelefonen, matrisen tilbake, papfiguren, bilen på B, tatt i bilen, opptaket, morgenen, den niende kua, stjernehullet og bilradioen.
- [x] Det nærmere blikket (zoomK, `S47Phones`, 2026-10-22). Fotskrittene (`player.onStep`) gjenstår.
- [ ] Glimtets og albumfigurens malte omriss (runde 14) og albumets bilder (runde 15) er ikke i eksporten; figuren fra koden brukes.
- [ ] Lommelyktas mønster (core/cookies.ts) og fall-off: nå Unreals invers kvadrat med intensiteten avstemt 4 m ut.

## Telefonene, arkivet og opptegnelsene (2026-10-22)

- [x] Arkivtelefonen og matrisen den vipper, kontortelefonen, skrittene på taket, feltboka og konvolutten, blikkene og det nærmere blikket, tråden (`US47Phones`).
- [ ] Krokene fra andre porter i `US47Phones`: `Reel047` (reel047.ts på dekket), `DecoderPlaying`/`DecoderStop` (Decoder.ts), `Dark` (saroDark), `CbBusy`, `AbductionCount`, `DishRogue`/`DishBack` (dishesFollow, arrayBack), `Frames`, `FbGet`/`FbSet` og `StationDrawer` (kapittel 3), `Foretell` (beitet og lysene på B), `StarHoleExamine`.
- [ ] Kapittel 4: `OfficePhone` på kontortelefonens proxy (`court:officePhone`) spør `M->Phones->OfficePhone` først.
- [ ] Kapittel 2s slutt venter på skrittene: settes kapittel 3s begynnelse som ny `OnEnd`, går den gjennom `M->Phones->Ch2End`.
- [ ] Opplesningen av et nærmere blikk etter 650 ms (Serling) når fortelleren er portert.
- [ ] Dishenes røde lys blinker i takt mens de er vippet (DishArray.beaconPhase) når glødepunktene blinker.
## SARO sine egne hendelser (2026-10-22)

- [x] SARO mørk og telexen fra Hondo Valley, matrisen tilbake, antennene som følger, lysene på B, spole 047, Earls spole og opptaket (`US47SaroNight`), med lagringen.
- [ ] Bortføringens port setter `US47SaroNight::Abduct` (stage, warn, backAt, data.last, overPhase, recordings, holdRecord, data.dishes) og bruker `FS47Walker` fra S47Recording.h for den som står der spilleren ble tatt; til da spilles varselet for antennene av testflagget.
- [ ] Radioens port setter `BoyCallTimes`, `CallerVoice` og `BoyVoices` (Earls spole), og musikken holdes av `ReelTapeBusy`, `SaroDarkBusy`, `ArrayBackBusy` og `RecWalkBusy` (musicHeld).
- [ ] Arkivtelefonens port spiller `Reel047` (update, play, reset) og setter `TipMoving`/`TipHolds` (archiveDishes.ts); feltboka setter `LineLights->bAnyWay`; det nærmere blikket setter `Zoom`.
- [ ] Hjørneklokka i sekunder: én kjede i main.ts sin rekkefølge (abduction.warnClock, saroDark, archive, ch6) i stedet for hver sin `ClockSeconds`.
- [ ] Telexrullen på maskinen (ControlRoom.setTelexRoll) tegnes ikke; papiret og loggen har meldingen.
- [ ] ConvolverNode i S47WebAudio: reverbet i decoder.ts er en kambank som står i for den.
- [ ] Glødepunktene utenom antennenes røde lamper blinker ikke ennå.

## Caliche, veien og motellets natt (2026-10-22)

- [x] C++-port av Town/TownEvents, Roadside, Herd/Pasture, NinthModel/NinthCow, MotelTv, MorningHolds/DinerCooler, Cutouts, Caliche og Diner med main.ts-koblinger, lagringsfelt og testflagg.
- [x] Caliches RecordedDialogue/NpcDialogue/EarlDialogue, innspilte replikkøer, replay og stopp. Generell ReadAloud/Serling gjenstår hos fortellerporten.
- [ ] Regenerer webeksport og import for byens delte lamper/glødepunkter, fjernbilens lys, townSlots/neonSlots og herdets beamLit. Implementer pointScreen i native billboards.
- [ ] Koble LostNights til Caliche.Reel og kapittel fem til TownLayer.Diner. Bygg og prøv kapittel tres FreeShot etter integrasjon; kapittel 3/5 finnes ikke i grenens utgangspunkt.
- [ ] Samlet pakket bygg, hele screenshots.sh, Continue fra disk, ekte input, menneskelig lytting og sammenligning av de regenererte bildene mot nettversjonen. Se TOWN_PORT.md.

## Verdenens eget liv (2026-10-22)

- [x] Glødepunktene, floraen og vinden, den levende himmelen og horisonten, lyset i lufta, vinduslyset, nattelivet, Ultra-lysene og de delte lysene (`US47WorldLife`).
- [x] Toppunktfargene i importen og de nære mesaene uten tåke (hvite kanter ved STATION 01).
- [ ] Eksporten og importen kjøres på nytt i hovedkopien (`Tools/webexport/import.sh`).
- [ ] Krokene i `US47WorldLife::Hooks` settes av kapitlene: `SkyLift` og `SunElev` (kapittel 6, soloppgangen), `SaroDarkAdapt` (SARO mørk), `MorningBusy` (morgenen), `LifeQuiet` (stillheten).
- [ ] `S47ArchiveDishes` bruker `FS47GlowPoints::SetPhase` og `HoldAt` for de vippede dishenes røde lys (DishArray.beaconPhase).
- [ ] `US47Night::Dawn` bruker sin egen formel; bør lese `Sky->Dawn`.
- [ ] Resten av World.update: neonflimmeret på motellet og dineren, byen, beitet og flokkene.
- [ ] Bildevalget Off: stjernene, båndet og stjerneskuddet tegnes uten tonekurve i nettversjonen (ShaderMaterial uten tonemapping_fragment); her går de gjennom M_WebTone som med VHS.
- [ ] Floraen: månelyset skaleres ikke med fSky (bare halvkulelyset), og spotlysenes fFace er ikke portert. Low-nivåets kortere tegneavstand for støv og hvilende tumbleweed er tilnærmet.

## Integrasjon etter bølge 4 (2026-10-22)

- [ ] Kapittel 5 (hjelper) og kapittel 6 (Codex, `port-ch6`) flettes inn; kapittel 5 sin slutt inn i `BeginChapter6`, `ch6.clockShow` sist i hjørneklokka, `US47WorldLife::Hooks.SkyLift`/`SunElev` og `US47Away` sine `Ch6*`-kroker.
- [ ] Opptaksvandringen (`S47Recording`) og bortføringens `FS47JacketWalker`/`Recordings()`: to figurtyper; vandringen bruker sin testopptakelse til de deler én.
- [ ] `US47Away::AfterAbduction` er ikke satt: stationTaken, «station passed», opptaksvandringen og parabolene steppes hver for seg (kapittel 3 og `US47SaroNight`), ikke i bortføringens blokk.
- [ ] Arkivets tippede paraboler: `FS47GlowPoints::SetPhase`/`HoldAt` for de røde lampene (verdenens liv).
- [ ] Byens fjernbil: fast pikselstørrelse (`pointScreen`) for billboards.
- [ ] Kapittel 4: papirkunst `cardField` og `letterPaper` mangler i WebData/art; WindowLights på gangen, readAloud og Earls stemme.
- [ ] CRT-gamma (dobbel koding), Serling-fortelleren, jakka «Cold out», kameraet senket på kortet (fra kapittel 1-hjelperen).
- [ ] `US47Night::Dawn` bør lese `Sky->Dawn` i stedet for egen formel.
- [ ] Pakket bygg: full `Scripts/screenshots.sh` etter ny eksport og import, og en gjennomspilling med ekte input og lyd.

## Kapittel seks og avslutningen (2026-10-22)

- [x] Chapter6.ts og Ending.ts i C++, med main.ts sin finale og testhoppene chapter6, chapter6:lastmile og chapter6:line.
- [x] Siste mila, CB-klokka, key/door og kamerastillingene med selvtester; native scenarier ved avreise og på linja, THE EVENT og kamerauttrekket fram til kontrollert tittelbilde.
- [ ] Integrer kapittel fem sin OnEnd og dinerlagring med BeginChapter6; prøv Fortsett fra denne lagringen etter avslutningen.
- [ ] Bind daggry-/soloverstyringene til verdensportens dynamiske himmel og kontroller mesa, soloppgang og stjerner visuelt mot nettversjonen.
- [ ] Pakket bygg, hele screenshots.sh, menneskelig input og lytting; øvrige uferdige motorlydgrafer hører til motorporten.

## Toms ideer (2026-10-22)

- [ ] Kassettspilleren: bytte den egenlagde tape deck-modellen med en bedre modell. NB: opptakeren i «Utstyr lyd UFO» på skrivebordet er pakket ut av «They Are Here»-demoen (UModel); den kan ikke brukes uten å kjøpe originalpakken (Horror Engine). Den kjøpte Vintage Liminal-pakken har kassetter og en Broadcast VCR, ingen kassettspiller.
- [ ] Tonene: en kombinasjon gir en annen låt og låser opp noe (hemmelighet, hint eller spor). Poenget må bestemmes.
- [ ] Ut av bilen overalt (feil) og hopp: under arbeid.
- [x] Egen alien laget i Blender med nye teksturer, 50-beins rigg og idle: `/home/tom/Desktop/Alien - Egen modell/`. FBX/GLB og UE 5.8.4-import består. Gammel modell er urørt referanse. Integrasjon og prøving i pakket spill gjenstår.
- [ ] Terminalene i verden: kameraet zoomer inn på den fysiske skjermen, og skjermbildet vises på skjermmodellen (som i Black Map). Under arbeid for mottakerkonsollen og driftsterminalen.
- [ ] Grafikk: mer detaljerte modeller, bedre lys og skygger (neste store steg).


## Unarius, overlevert til Claude 22. oktober

- [ ] Fortsett codex/unarius-tv fra TV_PROGRAMME.md: bruk siste bygde binærfil med tidsrettelsen, verifiser faktisk bevegelig film på begge TV-er over tid, koble filmlyd om ønsket, prøv fallback og 47-sekundershendelse/opptak/lagring på nytt, integrer og pakk før standardutgaven erstattes.
- [x] Separat egenprodusert Blender-alien er levert i `Desktop/Alien - Egen modell`, med importprøvde Unreal-assets. `Desktop/Alien` er urørt referanse; den nye figuren er ikke integrert i spillet ennå.

- [x] KELVORA retro-TV og slitt radio plassert i Unreal, 22.10: motellkontoret, rom 6 og SAROs arkivskap. Levende Unarius-film og lyd, av/på, radio og TV-hendelsens lagring verifisert. 1218 selvtester og ni målrettede scenarioer; installasjonen bak skrivebordssnarveien oppdatert med lagringer bevart. Se `Signal47/Tools/kelvora/README.md`.
## Etter Toms utelys-/fotolabprøve, 22. oktober 2026

- [x] 30 utvendige armaturspotter ved SARO og motellet, navngitte labdrivere og kopling til innflettet fremkallingsregel; bygg, 1218 selvtester og målrettede bilder. Se `LIGHTING.md`.
- [ ] Claude gjennomgår `revision3/compare`, pakker i avtalt kø, prøver lukket dør, rød fremkalling, forstørrer, tåket kopi/gjentakelse og normal lysretur med ekte input; SARO-kutt/tilbake og Room 6s lys.
- [ ] Ny High/1440p `stat unit` med én tung jobb om gangen etter dette utelyspasset. Forrige lysleveranses tall er bevart som historikk, ikke målt på nytt her.


## Radiostasjon og array, 2026-10-23 kl. 23:53

- [x] Blender-kilder, realistisk antenne på alle 27 plasser og radiostasjon med interiør integrert, verifisert og installert. Se RADIO_ARRAY.md for bevis og kildekontrakt.

## Modellprioritet SARO og Caliche, 25. oktober 2026

- [x] Modellprioriteten er implementert og avgrenset kontrollert i M6 fra aefd18157be7502ac98d3e1695b4c4fcdd7d26a0: SARO-fasade/tre master, SARO07/to ansattbiler og sheriff, motell/resepsjon/Rom 6/bad, vaskeri, fôrbutikk, kinofoajé og kirke. 10 fokuserte forløp; GPU-status og øvrige begrensninger står i PRODUCTION_MODELS.md. PR155 er flettet i da4e2394 og samme testede M6 er installert: 40 identiske payloadfiler og 46 byteidentiske Saved-filer.
- [ ] Ved senere innføring av av/på for motellets Room6-utelys: splitt den nye diffuserens delte MotelOfficeGlow-materiale og bind off-tilstanden. Dagens historiesti kaller bare SetRoom6Light(true); opprinnelig lysdriver beholdes.

## Fullført: SARO-ankomst og forplass, 2026-10-25 kl. 13:22

- [x] Valgfritt dinerstopp, tydelig besøksparkering, alle tre felt begge veier og direkte overlevering uten mat.
- [x] Asfalt foran østfløyen, personalbil på egen plass, samsvarende collider og presis fjerning av gamle planter/hjulrester. Bygget beholdes for senere utvidelse.
- [x] Avstandsvisning av flora og færre stolper; runtime-instans-/geometrikontroll.
- [x] Bekreftet faktisk motell/Rom6-modellintegrasjon i pakket spill.
- [x] Varig grense: gjeldende+to eldre testbygg; vedlikehold utført med kilder/Saved/bevis bevart.
- [x] Editor/pakke, fokuserte runtimeforløp, bilde-QA og installasjon med 40 uendrede Saved-filer. VISITOR_PARKING.md skiller P1/P2 og uprøvde forhold.


## Windows-test på jobb-PC, 27. oktober 2026

- [x] Privat byggpakke fra utgitt terreng-R2, alle nødvendige kilde-/innholdsfiler og Windows-startfiler levert på egen Drive. ZIP og tilgang kontrollert. Se log.md.
- [ ] Brukeren installerer UE 5.8.4 og C++-verktøy på Windows og kjører Build-Windows.cmd; innhent faktisk byggelogg ved feil.
- [ ] Kontroller Win64-kompilering, cook, oppstart, lagring/lasting og normal/Low-spilling før eventuell ferdig Windows-utgivelse. Ingen Windows-binær er hevdet levert nå.
