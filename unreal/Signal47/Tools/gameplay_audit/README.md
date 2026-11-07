# Gameplay-audit med flere agenter

**Status:** Gjeldende verktøy · **Gjelder:** statisk Unreal-/Freeform-gjennomgang · **Sist kontrollert:** 2. november 2026.

`audit.py` er et lite, skrivebeskyttet analyseharness. Det samler Git-revisjon, dirty-flagg, 24 spillerreiser, rollefordeling og valgfritt et minimalt sammendrag fra en eksisterende lagring. Det kjører ikke Unreal, klikker ikke i spillet og vurderer ikke automatisk om gameplay er godt. Runtime og menneskelig spilltest står alltid som `NOT_RUN` i denne statiske kjøringen. Den separate, eksplisitt aktiverte native-adapteren nedenfor kan senere kjøre en isolert spilløkt.

Kjør fra repoets rot med Python 3, uten ekstra pakker:

```sh
python3 unreal/Signal47/Tools/gameplay_audit/audit.py --repo . --format markdown
python3 unreal/Signal47/Tools/gameplay_audit/audit.py --repo . --save /absolutt/privat/save.json
python3 unreal/Signal47/Tools/gameplay_audit/audit.py --repo . --findings /tmp/workflow.json /tmp/progression.json --format markdown
```

Standard output kan lagres til en privat resultatfil. Original lagring åpnes bare for lesing. Harnesset eksporterer ikke thumbnail, dokumenttekst, fotografier, fritekst eller full lagring. Agentenes egne funntekster må gjennomgås før offentlig deling; verktøyet kan ikke avgjøre om disse inneholder private opplysninger. Det kontrollerer heller ikke at kjørende/installert spill svarer til kilde-revisjonen.

## Samkjøring

1. Koordinator kjører CLI og deler samme revisjon, avgrensning og `scenarios.json` med agentene. Ved dirty kilde må berørte filer fryses eller endringer samordnes under gjennomgangen.
2. Kjør `workflow`, `clarity`, `progression` og `reference` som avgrensede lesere. Med tre ledige agentplasser kan workflow/clarity deles mellom to lesere eller én rolle tas av koordinator. Alle leser samme arbeidskopi. Ingen ny kopi eller motorjobb trengs.
3. Hver agent leverer en JSON-array etter `finding_schema` i `scenarios.json`. Historisk schemaeksempel fra feilen før rettingen (ikke et åpent funn i gjeldende kilde):

```json
[{"id":"WF-01","priority":"P1","owner":"workflow","scenario":"routine","confidence":"source_confirmed","trigger":"Open receiver after collecting the original","consequence":"Receiver sends the player back to an empty printer","fix":"Derive next action from the selected job stage","acceptance":"Complete routine through collection and verify OPS registration is the next action","evidence":["unreal/Signal47/Source/Signal47/Private/S47Freeform.cpp:388"]}]
```

4. `--findings` validerer obligatoriske felt, kjente scenarier, prioritet, bevisnivå og unike ID-er og samler funnene. Dette er sammenstilling, ikke uavhengig bekreftelse. Koordinator må lese kilden, løse uenigheter, slå sammen duplikater og sammenligne eksisterende TODO.
5. Rapporter først spillerkonsekvens og konkret forbedring. Oppdater prosjektets eksisterende hoved-TODO; ikke opprett en ny konkurrerende arbeidsliste. Kjør bare en relevant faktisk spillerprøve dersom den er nødvendig og motorvinduet er ledig.

## Kildevern og begrensninger

`source_contract.json` binder statisk tolkning til de konkret gjennomgåtte funksjonene og enumrekkefølgen. Ved endring blir status `UNVERIFIED_SOURCE_CHANGED`, og neste-handling-rådet undertrykkes. Oppdater kontrakten først etter ny gjennomlesing av funksjonene og tilpasning av Python-tolkningen. Automatisk oppdatering av hash ville skjule endret oppførsel.

Lagringssammendraget beregner jobbens neste stadium, ventende printerkø og blokkert jobbskifte under aktivt opptak/kontroll. Gjennomgått kilde bruker nå stadiumsbestemt receivertekst; den tidligere brede printerbeskjeden rapporteres ikke lenger som en gjeldende motstrid. Originalhenting (`Chart`) og fysisk arkivering (`Packet`) trenger ikke strøm. Ved andre stadier og lagret strømfeil er rådet betinget av om reservegeneratoren faktisk leverer; dette simuleres ikke. `Filed` har en kjent forskjell: receiver peker videre til arbeid, HUD prioriterer uløst strømbrudd.

Servicefokus kan overstyre HUD. Encoder, rekkevidde, dokumentinnhold og hele loaderens normalisering/deduplisering simuleres ikke. Ugyldig valgt indeks avvises. Kontrakten omfatter funksjonene som tolkes, samt hele `S47Freeform.cpp` som konservativt vern for lokale hjelpere; også uvedkommende endringer i denne filen krever ny gjennomgang. Fotoscenariene er testplaner, ikke automatisk analyse av bilder eller bekreftet hendelsesforløp.

Den lille parserkontrollen kjøres slik:

```sh
python3 -B -m unittest discover -s unreal/Signal47/Tools/gameplay_audit -p 'test_*.py'
```

Prosjektbiblioteket ble søkt etter gameplay/workflow og state machine. Treffene gjaldt diagrammer og NPC-AI, ikke denne lese-/koordineringsoppgaven; ingen tredjepartskode er gjenbrukt.

## Native agentbetatest

`native_beta.py` bruker mønsteret fra prosjektets historiske `Automation/gauntlet-user-journey.py`: Linux uinput for taster/mus og X11 for vindu/fokus. Unity-spesifikk telemetry og reiselogikk er ikke gjenbrukt. Adapteren finner kun det selvstartede Unreal-vinduets PID, kontrollerer prosessidentitet, skrivebordslås og fokus, begrenser tastetrykk til fem sekunder, og slipper input ved feil. Den endrer ingen enhetstillatelser og installerer ingenting. Ett slikt inputharness kan styre spillet; øvrige agenter vurderer bilder og logg.

Forutsetninger: ulåst KDE med XWayland, eksisterende tilgang til `/dev/uinput`, Python 3.11+, libX11, qdbus6 og Spectacle. Første native TAB-/mus-/bildeprøve må bekrefte kompatibilitet. Forutsetningene alene beviser ikke fungerende input. `probe` leser bare inventar/lås:

```sh
python3 -B unreal/Signal47/Tools/gameplay_audit/native_beta.py probe
```

Kommandoen under lager bare en plan på standard output. Den oppretter ingen mappe eller spillprosess. `--build` peker til en allerede pakket Linux-mappe, ikke repoets kildeprosjekt:

```sh
python3 -B unreal/Signal47/Tools/gameplay_audit/native_beta.py serve \
  --repo . --build /absolutt/kandidat/Linux --out /tmp/s47-beta-fresh --seconds 2700
```

Når koordinator har gitt et ledig motor-/inputvindu, legges `--execute` til. Prosessen må bli kjørende mens agenten sender enkeltkommandoer fra andre terminalkall. Den bruker prosjektets felles motorreservasjon, nekter grace-unntak for Toms spilløkt og oppretter en ny privat `profile` under resultatmappen. Bare egen spillprosess avsluttes ved stopp, konflikt eller timeout. `--out` må være en ny, kort sti uten symlink. Ingen kildekontrakt oppdateres av native-adapteren.

For to komplette 75-minutters Freeform-døgn brukes minst `--seconds 10800` (tre timer veggtid, standard); maksimalt fire timer tillates per økt. Logg aktiv spilletid separat fra pauser, menybruk, bildeinspeksjon og venting. En prosess som har stått åpen i 150 minutter beviser ikke 150 minutter aktivt gameplay eller to fullførte døgn.

```sh
python3 -B unreal/Signal47/Tools/gameplay_audit/native_beta.py action \
  --session /tmp/s47-beta-fresh --json '{"action":"screenshot","name":"title"}'
python3 -B unreal/Signal47/Tools/gameplay_audit/native_beta.py action \
  --session /tmp/s47-beta-fresh --json '{"action":"tap","key":"Tab"}'
python3 -B unreal/Signal47/Tools/gameplay_audit/native_beta.py action \
  --session /tmp/s47-beta-fresh --json '{"action":"hold","key":"w","seconds":0.3}'
python3 -B unreal/Signal47/Tools/gameplay_audit/native_beta.py action \
  --session /tmp/s47-beta-fresh --json '{"action":"look","dx":30,"dy":0}'
python3 -B unreal/Signal47/Tools/gameplay_audit/native_beta.py action \
  --session /tmp/s47-beta-fresh --json '{"action":"click","x":640,"y":360}'
python3 -B unreal/Signal47/Tools/gameplay_audit/native_beta.py action \
  --session /tmp/s47-beta-fresh --json '{"action":"observe","note":"Synlig mål peker til OPS; originalen finnes under Papers."}'
python3 -B unreal/Signal47/Tools/gameplay_audit/native_beta.py action \
  --session /tmp/s47-beta-fresh --json '{"action":"stop","note":"Avgrenset reise avsluttet; menneskelig vurdering gjenstår."}'
```

Andre handlinger: `drag` med `x/y/to_x/to_y`, `scroll` med `steps` fra -10 til 10 og eksplisitt `activate` etter tapt fokus. Kun de støttede synlige tast-/mushandlingene kan sendes; vilkårlige spillkommandoer, teleport og skjult tilstand finnes ikke i API-et. Inputagenten skal vente på faktisk bilde før neste valg og bruke vanlige menyer for New Freeform, Save, tittel og Continue. Ved fastlåsing registreres det synlige problemet; kildekodefasit skal ikke føres inn som om spilleren forsto veien videre.

`session.json`, `events.jsonl`, `game.log`, `shots/` og `result.json` ligger privat i sesjonsmappen. Resultatet identifiserer binærfilen med SHA256; kilde-revisjon er oppgitt separat og beviser ikke at pakken ble bygget fra den. `INPUT_SENT_REQUIRES_VISUAL_REVIEW` betyr at input ble sendt, ikke at spillet svarte riktig. `CAPTURED_REQUIRES_VISUAL_REVIEW` må følges av bildeinspeksjon. `gameplay_acceptance` er `UNASSESSED` og `human_playtest` er alltid `NOT_RUN`; koordinator fører sin avgrensede agentvurdering separat. `stop` avslutter kun egen prosess; det er ikke bevis for at spillets vanlige Quit-valg fungerer.

Alle native-tester i `test_native_beta.py` er lette enhetstester uten displaytilkobling, enhetsoppretting, input eller motorstart. De prøver planisolering, grenser, låseavslag, frigjøring av taster og kommandovalidering. De er ikke en native spillprøve.

## Midlertidig skjermsparerhemming når Tom har bedt om det

`inhibit_lock.py` holder en privat D-Bus-forbindelse og `org.freedesktop.ScreenSaver.Inhibit` levende gjennom bygg-/betatestvinduet. Bruk bare etter uttrykkelig ønske om å holde økten ulåst. Den låser aldri opp og endrer ingen permanent skrivebordsinnstilling. En allerede låst økt blir avvist. Manuell låsing kan fortsatt skje og native-driverens låsevakt gjelder fortsatt.

```sh
python3 -B unreal/Signal47/Tools/gameplay_audit/inhibit_lock.py \
  --seconds 14400 --state /tmp/s47-gameplay-inhibit.json
```

Prosessen må bli kjørende; et enkelt `qdbus6 Inhibit` som straks avslutter har ikke samme forbindelseslevetid. Privat statusfil inneholder PID, D-Bus-forbindelse, cookie og `INHIBITING`. Når arbeidet er avsluttet, kontroller at PID-en fortsatt tilhører denne helperen, og send SIGTERM til akkurat den prosessen. SIGINT og SIGHUP frigjør også normalt. Ved SIGKILL eller prosessvertens brutale avslutning kan statusfilen bli foreldet; D-Bus-hemmingen forsvinner når forbindelsen brytes, så `INHIBITING` alene beviser ikke en levende hemming. `UnInhibit` kjøres, forbindelsen lukkes og status blir `RELEASED`. Tidsgrensen er maksimalt fire timer. Helperen sender ingen tast/mus og starter aldri spillet. `python3-dbus` må allerede være tilgjengelig; ingen automatisk installasjon følger med.


### Samtidige taster og gjenopptakelse i ny isolert økt

`tap` og `hold` tar enten `key` eller `keys`, aldri begge. `keys` er en liste med én til tre ulike fysiske taster; aliasene `,` og `comma` teller som samme tast. Eksempel: `{"action":"hold","keys":["Shift","w"],"seconds":1}` eller `{"action":"hold","keys":["w","a"],"seconds":0.5}`. Samme tidsgrenser gjelder, og alle taster slippes ved fokusfeil, avbrudd eller delvis skrivefeil. Endret adapterkode blir først aktiv ved neste serverøkt.

Etter vanlig Save/Quit og stopp av gammel beta kan neste `serve` få `--profile-source /tmp/s47-beta-fresh/profile`. Kilden må være en tidligere isolert `/tmp/s47-beta-*/profile` med samsvarende adaptermetadata, bare vanlige egeneide filer/mapper og ingen symlenker. Personlige Saved-mapper godtas ikke. `--out` må fortsatt være ny. Planmodus validerer bare og kopierer ingenting; med `--execute` kopieres profilen før motorstart. Originalprofilen endres ikke, og spillets vanlige Continue må brukes. Metadata merker `profile_mode: resumed`, kilde og om kopien faktisk er utført. Dette verifiserer ikke at Save/Continue fungerer; den synlige prøven må loggføres separat.

## Avgrenset native fotoprøve

Spillbinæren har den eksplisitte kombinasjonen `-S47FreeformTest -S47PhotoEconomyTest` for et faktisk rendret spor. Tillegget `-S47PhotoEconomyEvent=feeder_presence` prøver en native figur ved trafoen om natten med lommelykt av og på. Kjør bare i et ledig, reservert motorvindu, med en egen `-UserDir=/tmp/...`; personlige saker skal ikke brukes. `Saved/FreeformTest/result.json` rapporterer betaling, duplikatavslag og ubetalt/betalt disk-/Flow-retur. Nattprøven beholder de faktiske kamerabildene og måler motivlys og sikt.

Dette er utviklerfixturer med eksplisitt tidspunkt, plassering og `Force`. Av/på-bildene tas mens figuren lever videre, så de er ikke en identisk A/B-positur. Av-bildet kan allerede være tydelig nok til å gi ett krav. Prøven beviser verken naturlig hendelsesfrekvens, menneskelig lesbarhet eller vanlig tastatur-/musreise; native-agentøkten over vurderer den siste delen separat.


### Eksplisitt XTest-mus for X11/XWayland

**Eksperimentell / UNVERIFIED:** En avgrenset prøve 2. november ga ingen synlig musebevegelse med XTest eller ny uinput-enhet; X11-posisjon alene var ikke pålitelig, og klikk i tidligere prøve mistet fokus. Native videreføring er blokkert inntil faktisk bevegelse og treff er visuelt bekreftet. Enhetstester beviser bare adapterlogikk.

`serve --mouse-backend xtest` bruker eksisterende libXtst for all relativ mus, knapper og hjul. Absolutt klikk-/dragmål er fortsatt client-koordinater; bare denne backenden bruker XWarpPointer mot det eide PID-vinduet, og kontrollerer faktisk XQueryPointer-posisjon før klikk. Den blander aldri XWarp-posisjon med uinput-museklikk. Tastatur bruker fortsatt uinput. Standard er fortsatt `uinput`; en vellykket XTest-kallretur er ikke bevis på at gameplay reagerte riktig. PID-, fokus-, låse- og inputfrigjøringsvern gjelder begge.

Bibliotek og nødvendige symboler kontrolleres før motorstart. Ingen pakker installeres. `session.json`/resultat/plan angir `mouse_backend` og `keyboard_backend`. Velg ny resultatmappe og eventuelt `--profile-source` etter ordinær Save/Quit; en kjørende server endrer ikke backend. Før videre beta må faktisk menyklikk, relativ look, scroll og drag observeres i den nye økten.

Lokale UE5.8.4-kilder bruker SDL3 (`SDL_SetWindowRelativeMouseMode`). Det eldre SDL2-hintet `SDL_MOUSE_RELATIVE_MODE_WARP` finnes ikke i de tilgjengelige SDL3-headerne og settes derfor ikke av adapteren. Ingen globale SDL-/skrivebordsinnstillinger endres.


## UFO-scenarioene

Seks scenarioer i `scenarios.json` dekker nærpassering, motorveijakt, forfølgelse av spilleren, vitnebortføring, spillerbortføring og finale. Det delte presentasjonslaget har ingen egen spillklokke. Hendelsenes eksisterende livsløp styrer lys og geometri.

Korte, opt-in Unreal-kontrakter bruker isolert profil og eksisterende spillfunksjoner:

- `-S47UfoVisualTest -S47Case=0 -UserDir=/tmp/s47-ufo-visual`: vanlig Freeform-start, natt-/kamerafixture, nærpassering, faktisk foto, Low/komfort, pause og sceneutgang. Resultat og tre native bilder i profilens `UfoVisualTest`.
- `-S47Freeform -S47Case=0 -S47HighwayUfoTest=chase -UserDir=/tmp/s47-ufo-chase`: naturlig veitrigger etter en eksplisitt startposisjon, kjøring med bilens input, pause, lagring, fysisk disklesing og avgang. Resultat i `Saved/HighwayUfoTest`.
- Samme kommando med `=follow`: eksplisitt forfølgelsesstart og reell flukt ved kjøring. `=chase -S47HighwayKeepDriving` følger sheriffen videre for et bilde av nærpasseringen; standardprøven stanser for å kontrollere at spilleren kan velge hendelsen bort.
- `-S47HorrorContract=SC03Resume -S47HorrorRestore=/absolutt/faktisk-save.json -UserDir=/tmp/s47-ufo-sc03`: hydrert faktisk SC02-escape eller SC03-møtegrense med originalfoto inkludert (`*.loaded.json`); `-S47SC03LookAway` prøver bortvendt kamera og separat savnetrapport. `-S47SC03UfoOnly` avgrenser til reell Ortiz-interaksjon, løft, forsvinning og tre disk-/Flow-returer etter én uttrykkelig posisjonsfixture; dette erstatter ikke den lange Caliche-ruten.

`saro_hold_soak.py` er den isolerte renderprøven for PLAY-31-16: egen `-UserDir` under `/tmp`, reservasjon, GPU-minne utenfra prosessen, og PASS, FAIL eller INVALID. `--focus-cycles N` legger til fokusbytte og ny swapchain, `--video-driver x11` er sammenligningen. Start er opt-in med `--execute`. `test_saro_hold_soak.py` prøver beslutningene uten motor.

Dette er native utviklerkontrakter med navngitte fixtures, ikke menneskelig spillaksept. Selve `audit.py` starter ingen spillprosess og erklærer ikke disse runtime-prøvene bestått.
