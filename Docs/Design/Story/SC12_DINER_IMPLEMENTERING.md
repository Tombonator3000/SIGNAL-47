> **Status: Under arbeid** · **Gjelder:** SC12-kilden for nye Unreal-saker med den reviderte Spine-ruten. **Sist kontrollert:** 30.10.2026. Kilden er forfattet; bygg, fysisk runtime, bilder og lagringskontrakten er ikke kjørt i denne leveransen.

# SC12: kilder, varsel og avreise

[Den godkjente tidslinjen](TIDSLINJE_SC01_SC13_2026-10-29.md) overstyrer de eldre klokkeslettene i [historierevisjonen](HISTORIE_SC02_SC13_REVISJON.md). SC12 starter 05:12, sheriffen kommer 05:15, og valgt avreise skjer 05:20. Dineren er et fristed. Kaffe, avisutklippet, radioen og servitrisen beholdes. Den eksisterende sjåføren er bakgrunn i den nye ruten; sheriffens fysiske patruljeark gir veiopplysningen.

Spilleren legger de medbrakte originalene på booth-bordet, leser dem og kontrollerer opphav, klokker og mulig felles signaltilførsel. Følgende dokumenter må faktisk finnes. SC12 lager ikke manglende kildebevis:

| Dokument | SOURCE ID | Opphav og grense |
|---|---|---|
| `HR_SARO_ORIGINAL` | `SARO07-860414-RX01` | SC11 gir det komplette arket først etter lokal lyskontroll, romobservasjon og faktisk flukt. `HR_SARO_EVENT_STARTED` teller ikke. |
| `HR_HONDO_REMOTE` | `HONDO-860414-LOG01` | Ferdig fjernark etter 05:04. Hondo: 04:56:08, 04:56:55, 04:57:42. |
| `HR_SITE11_REMOTE` | `SITE11-860414-TR01` | Ferdig fjernark etter 05:04. Site 11: 04:56:08, 04:56:56, 04:57:42. |

Sammenligningen beholder forskjellen på ett sekund og begrensningen i klokkekontrollen. Den gir ikke et sikkert ankomstminutt, en global telling eller sikker identitet. Ward leser tilbake alle tre ID-er og første grensetid `04:56:08 MST, 14 APR 1986`. Først spillerens bekreftelse av gjentakelsen setter `warningSent` og gir den daterte kvitteringen. En avbrutt samtale gir ingen kvittering. De nye Ward-linjene er tekst uten kreditering som avspilt opptak. 06:00-serien forblir sperret; bilder og komplette tabeller blir i mappen.

Sheriffen legger et datert patruljeark på disken klokken 05:15. Arket gjelder én tur 04:15 MST, én time før ankomsten. Den gamle kontor-/patruljebiltilstedeværelsen undertrykkes bare mens den nye SC12-ruten er aktiv. SC03-figuren endres ikke; kontrakten avleser dens faktiske komponenter for å oppdage dobbel tilstedeværelse.

Etter varselkvittering og lest patruljeark åpner P13 på den eksisterende kartflaten: E09A på STATION 01, nord opp på E14, C øst til 8.15. Et eget overlegg skiller C fra det foreslåtte budmøtet ved 10.4 og lar det gamle kartet være uendret. Riktig svar krever fortsatt faktisk justert kalkerpapir. Spilleren ringer Ward igjen og bekrefter budet, patruljetiden og begrensningene før servicebilens avreisevalg åpnes. Ingen lover trygg vei. Spilleren kan bli i dineren; originalene skal leveres, ikke brukes til en ny måling ved C.

## Tilstand og kompatibilitet

[Chapter5](../../../unreal/Signal47/Source/Signal47/Public/S47Chapter5.h) lagrer den nye tilstanden i `ch5.sc12.v=1`: lese-/kontrollmasker, konklusjon, varsel, sheriff, patruljeark og separat bud-/rutebekreftelse med stempler. De faktisk mottatte tre dokumentenes komplette payload kopieres til kapitteltilstanden, fordi den eksisterende full-case-lagringen ikke lagrer vilkårlige Story-dokumenter. Continue gjenoppretter disse bytene fremfor å finne opp nye ark fra ID-ene. Ukjente lagringsfelter beholdes.

`spine.sc12Route=1` markerer nye saker. En eldre Spine-sak uten feltet får det ikke ved ny lagring. Eldre `ch5` uten `sc12` beholder den gamle ruten, dialogen og karttegningen. Ingen samtale som er pågående i minnet, gjenopprettes som bekreftet.

Den lille delingen mot felles kode er `Spine::UsesRevisedSC12`, `sc12`-kontraktdispatchen og sheriffens tidsplan i Town. Story og Flow endres ikke. Root må samordne Spine-hunkene med de parallelle SC04-/SC11-endringene.

## Målrettet kontroll som gjenstår

[S47SC12Contract.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47SC12Contract.cpp) kjøres i en bygget arbeidskopi med nødvendige genererte Content-filer, gjennom den felles Unreal-vakten. Eksempel på spillargumenter:

```text
-game -unattended -nosplash -windowed -ResX=1280 -ResY=720 -S47Case=0 -S47EveningTimeline -S47Jump=chapter5 -S47SpineContract=sc12 -S47SelfTest -UserDir=/tmp/S47-sc12-r1 -stdout -FullStdOutLogOutput -UTF8Output
```

Kontrakten bruker en uttrykkelig SC11-utdatafixture og én diner-/fotstart. Den beviser dermed ikke SC11-flukten eller en naturlig ankomst fra SC01. Deretter går vanlig Walker mellom booth, telefon og patruljeark, med faktisk kameraray, fokus og `Player.OnUse`. Menyvalg og kartflytting bruker de samme beskyttede produksjonscallbackene; dette er ikke menneskelig Slate-input. Manglende kilde, for tidlige kvitteringer, avbrutte samtaler og feil kartvalg må ikke gi fremdrift. Flere fullstendige disk/title/Continue-grenser kontrolleres. Avreisevalget er merket produksjons-API, ikke en målt kjøretur.

Forventet sluttmarkør er `S47: spine contract sc12 PASS COMPLETE:`. `Saved/SpineSC12Contract/<id>/result.json` inneholder faktisk antall kontroller, gjenopprettinger, stråler, ganglengde og rå tilstandsavlesning. PASS krever den faktiske kjøringen og normal exit 0. Selvtesttall skal leses fra råloggen. Plassering, materialer, lesbarhet, sheriffens ankomst og fysisk rekkevidde er fortsatt UNVERIFIED til root har bygget og kontrollert dem. Ingen pakke, installasjon eller release er utført.

Prosjektbiblioteket ble søkt før implementering. Den lokale UE5.8-profilen for Quod Solers SaveGame-eksempel ble lest som referanse; den er ikke lokalt testet og innføres ikke her. Eksisterende Chapter5-, Spine-, JSON-, Flow- og SC04-kontraktsmønstre er brukt.
