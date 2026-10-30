# SC13: kildebygg, native binding og fysisk kontrakt

Status: **Kilde og native består i R5 fra 7d19482b.** Faktisk Blender-lagring/gjenåpning, import65 og separat ferskleser47 består. Runtime-RHI, live shader, bilder, ytelse og kjøretur er fortsatt UNVERIFIED. Kommandoene nedenfor beskriver arbeidsflyten.

Kildene er [source-contract.json](../../../ArtSource/FinaleSky/source-contract.json), [geometry.py](geometry.py) og [cloud_field.hlsl](cloud_field.hlsl). [Kildebeskrivelsen](../../../ArtSource/FinaleSky/README.md) forklarer opphav og bevaringskrav. Ingen Engine-asset eller eksisterende global sky endres.

## Utførelsesrekkefølge

Root eier alle tunge jobber, én om gangen gjennom `Scripts/unreal_guard.py require-idle` og felles reservasjon. En aktiv spilløkt skal ikke stoppes. Arbeidskopien må først inneholde integrert SC12, SC13 og et faktisk bygget editor-modul med `S47.FinaleSkyAssets`. Tynn kildegren alene har ikke disse binær- eller Content-bevisene.

Fra den integrerte `unreal/Signal47` kan root skrive en ny, uutført plan:

```sh
python3 Tools/finalesky/prepare.py --project <absolutt-Signal47-prosjektmappe> --run-root <ny-privat-bevismappe> --departure <faktisk-SC12-PASS-mappe>/departure-checkpoint.disk.json
```

`unexecuted-plan.json` inneholder eksakte argv, egne `/tmp`-profiler, miljøvariabler, forventede markører og inputhash. Skriptet starter ingen prosess. `--blender` og `--editor` kan angi faktisk lokal installasjon. Avreisegrunnlaget må være SC12-kontraktens virkelige fullstendige diskcheckpoint med PASS i søskenfilen `result.json`; ingen ny syntetisk SC12-state lages her.

1. `build_source.py` kjøres i Blender med `--background --factory-startup --python ... -- --output <ny-source-mappe>`. Det skriver original `.blend`, tre FBX-er, saved/reopened native-reference og manifest. Forventet markør er `S47 FINALE SKY SOURCE REOPEN PASS`.
2. `import_assets.py` kjøres som Python-commandlet med `S47_FINALE_JOB=<apply-job.json>`. FBX Interchange deaktiveres med den faktiske ConsoleVariables-ini-opsjonen i planen. Bare syv eide assets under `/Game/FinaleSky` kan endres: ett mesh, to mastere og fire MI-er. Alle andre Content-filer hashes før/etter. Forventet markør er `S47 FINALE SKY APPLY PASS`.
3. Ny PID og ny profil kjører `verify-job.json`. Dette er lesende for hele Content og sammenligner den komplette faktisk lagrede renderpayloaden med importen. Egen compiled konsollkommando leser navngitte saved-grupper og orienterte triangler; PolygonGroupID antas aldri å være global materialslot. Forventet markør er `S47 FINALE SKY VERIFY PASS`.
4. Først etter faktisk outer exit 0, riktige markører og PASS-rapporter kan root kopiere `binding-verified.json` til `Content/WebData/finalesky/binding.json`. Importverktøyet skriver ikke denne runtimebanen automatisk. Native PASS gir ikke bilde-, shader-, GPU- eller reiseaksept.

Importerens saved/source-triangelkontroll er én-til-én med orientering, UV og materialnavn uavhengig av vertex-ID. Toleransen 0,001 cm gjelder float32-runding av denne normaliserte kilden på omtrent 100 cm, før runtime-skala 680; UV-grensen er 1e-6. Faktisk maksimalfeil rapporteres. Det er ikke en generell prosjekttoleranse. Renderhash, indekser og seksjonsruting må deretter være identiske mellom import og fersk leser og ved runtime. En basis- eller windingfeil skal diagnostiseres, ikke godkjennes ved å øke grensene.

## Fysisk SC13-prøve

Planen skriver tre målrettede RHI-kjøringer. Alle bruker `-S47Case=0 -S47EveningTimeline -S47SpineContract=sc13 -S47SelfTest` og isolert profil:

| Modus | Tillegg | Faktisk omfang |
|---|---|---|
| `full` | `-S47Title -S47SC13Mode=full -S47Quality=high -S47SC13Departure=<diskfil>` | Vanlig disk/title/Continue ved 05:20, faktisk Walker til servicebilen, ray/focus/OnUse, sammenhengende kjøring fra diner-lot til C, avslutning og vanlig Continue tilbake til avreise. |
| `line` | `-S47Title -S47SC13Mode=line -S47Quality=low -S47SC13Comfort -S47SC13Departure=<diskfil>` | Samme avreisegrunnlag og interaksjon, deretter én uttrykkelig utviklerfixture ved siste linje. Low, calm-flash og still-shots. Dette er ikke full kjøretur. |
| `legacy` | `-S47Jump=chapter6:line -S47SC13Mode=legacy -S47Quality=high` | Original siste-linje-fixture uten SC12-rute eller nye kunstkrav. Ingen ny-rute-adopsjon. |

Full og line krever ikke-tom faktisk `chain_id`, SC12-scenario og `departure_checkpoint_file` med nøyaktig sti, størrelse og SHA1. Foreldrefilene bytekopieres og skal være uendret ved slutt. Titteloppstarten lager ingen forutgående kapittelfakta. `line` beholder sin uttrykkelige siste-linje-fixture etter vanlig lasting; den teller ikke som full kjøretur.

Forventet slutmarkør er `S47: spine contract sc13 PASS COMPLETE: <modus>`. Resultater ligger i `Saved/SpineSC13Contract/<GUID>/result.json` med utførte kontroller, rå snapshots, fasebilder, faktiske rammetider og SHA1 for originalt 1947-bilde. Selvtesttallet må leses fra den aktuelle råloggen, ikke kopieres fra eldre kjøringer. Root må binde faktisk source-head, bygget modul, argv, profil, guardresultat og råfiler før godkjenning.

Kontrakten krever faktisk SARO 07-førerhytte gjennom hele kjøringen. Avreisevalg endrer bare biltype/pendlerflagg ved den komplette SC12-rutens eksisterende C6-hook. Det flytter ikke bilen, kaller ikke `FinishCommute` og endrer ikke privatbilens lagrede parkering. Manglende legitim serviceavreise avvises tydelig.

Det kjøres ingen poseflytting under fullruten, ingen klokkeendring for å nå C, ingen sluttkort-skip og ingen save midt på veien. Pause bruker den vanlige menyen og skal fryse eksakt fase, klokke, komponenter og materialparametre. Slutten går naturlig gjennom eksisterende tittel, credits og foto. Uavbrutt full kjøredistanse skal være over 13 km før hendelsen. 10.4 nås ikke.

## Bilder og grenser

Kontrakten ber om ekte viewport-PNG ved skyoppbygging, EVENT +4/+14/+23/+43, uttrekk +6/+29, original sluttittel og 1947-bildet. Råfilenes CRC, IEND, minimumsoppløsning, SHA1 og faktisk request-frame lagres. Menneskelig bildeaksept er alltid separat. Root må kontrollere to rullende skybanker, åpen himmelpassasje, opak enorm silhuett, faktisk dekking/avdekking, lys/eksponering, frontrute/pullout, alternativt blikk og Low/High. Rammetidene er målinger; ingen automatisk 60 FPS-påstand eller godkjenning av et 1-2 ms effektbudsjett lages.

Fallback uten verifisert kunst beholder original finale, men ny-rute-kontrakten skal da feile på den konkrete kunstporten. Ingen installasjon, fullmatrise, offentlig release eller faktisk menneskelig input er utført av denne leveransen.

## Lokal primærbasis

Kontrollert mot lokal UE5.8.4: `FbxMainImport.cpp` aksekonvertering, `FbxStaticMeshImport.cpp` UV/hjørnerekkefølge, `FStaticMeshConstAttributes`, `FStaticMeshSection`, målrettet `FStaticMeshCompilingManager::FinishCompilation`, `MaterialExpressions.cpp` SceneDepth/PixelDepth og `UMaterial::GetMaterialResource(EShaderPlatform)`. Prosjektets `Tools/motelv2/native.py` gjenbrukes bare for FBXFactory/import og rapportskriving. Materialsetterens returbool er ikke bevis i denne UE-versjonen; faktiske parent-/parameterverdier avleses.

Prosjektbibliotek: UDS-metodekort og `kevinpbuckley/unreal-engine-skills` sine Niagara-sekvens/VFX-referanser, festet til bibliotekets `b22b11d0a46bac2d55eae8d3273d2ef22c08a95d`. Referansene er metodekunnskap med uavklart gjenbrukslisens og uten prosjekt-PASS; ingen kildekode eller assets er kopiert. UDS er ikke installert.


## Faktisk kilde- og nativekontroll 30. oktober 2026

Originalkilden er faktisk lagret og gjenåpnet i Blender. R5 fra7d19482b består65importkontroller og47i separatferskPID. Alle treLOD-er beholder15268/10004/7076triangler, UV/orientering/materialnavn og opprinnelig graf. Kollisjonskart og faktiske renderseksjoner er kontrollert separat etter bytekontrollert reload av kun eget lagret mesh. Sju nativefiler og verifisert binding er bevart. Tidligere readonly-, maskpinne- og NullRHI-seksjonsavslag er beholdt privat. Dette gir ingen RHI-/bilde-/GPU-/reiseaksept. Se Docs/Evidence/UnrealDeliveries/SC13_FINALE_2026-10-30.md fra reporoten og portabel nativekvittering i unreal/Evidence/SC13-2026-10-30.
