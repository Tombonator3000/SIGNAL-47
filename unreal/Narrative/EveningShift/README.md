# Manuspakke for innkjøring, Dale og det hemmelige kontoret

Runtime for trinn 1 og 2 ligger nå i `S47Evening`/`S47EveningPack`. Se [runtime-rapport og tester](../../../Docs/Evidence/UnrealDeliveries/EVENING_RUNTIME.md). Kontoret og T01/T02 er siden levert; se [T02-rapporten](../../../Docs/Evidence/UnrealDeliveries/T02_RUNTIME.md). De 109 eksisterende opptakene er produsert. Original bevegelig felt-VHS står fortsatt åpen.

Ferdig skrevet engelsk spilltekst for den nye Unreal-fortellingen: vanlig innkjøring gjennom Caliche, valgfritt matstopp, telefonoverlevering 18:00, Dales senere samtaler, kontornøkkel, private dokumenter og et bevegelig VHS-feltopptak. Trafostasjonens to besøk og aktive sammenligninger er også skrevet.

**Kinski/Dale finnes bare på telefon, radio eller walkie-talkie.** Ingen fysisk Dale-figur og ingen Kinski-forteller på TV/VHS. Den siste bruker miljølyd og skriftlige merknader. Ward, Nora og Earl beholder sine roller.

## Les og bruk materialet

- [Lesemanus](generated/SCRIPT.md): alle scener, valg, dokumenter, feltopptak og UI-tekster samlet.
- [Opptaksliste](generated/voice_lines.csv): engelske replikker med aktør, kanal, stemmeprofil, regi og eventuell gjenbruk.
- [Opptaksmanifest](generated/recording_manifest.json): lydfiler, format og målt varighet. Alle 109 takes er produsert; se [stemmetakene](voice/README.md).
- [Undertekstgrunnlag](generated/subtitles.json): dialog og stille tekster. Dialogtiming er målt fra opptakene; VHS-timing er planlagt.
- [Integrasjon](INTEGRATION.md): triggerpunkter, kapittelgrenser, tilstand, lyd/film og konkret produksjonsliste.
- [Kontrollscenarier](SCENARIOS.md): ruter som må fungere og hva som kan kontrolleres før runtime.
- [Overordnet plan](../../../Docs/Archive/Superseded/Unreal/ANOMALY_PLAN.md): historien, byen, trafostasjonen og gjennomføringsrekkefølgen.

JSON-filene i `modules/` og `pack.json` er sannhetskilden. Lesemanus/CSV/undertekster er avledede filer og redigeres ikke for hånd.

```sh
python3 unreal/Narrative/EveningShift/build_pack.py
python3 unreal/Narrative/EveningShift/build_pack.py --check
```

Kjør fra reporoten. Verktøyet trenger bare Python-standardbiblioteket og hverken starter Unreal, bruker en stemmetjeneste eller endrer spillets lagringer. `--check` skriver ingenting og feiler hvis en eksport mangler eller er utdatert.

## Innhold

| Kilde | Innhold |
|---|---|
| `modules/arrival.json` | Lokalradio, valgfri diner, to matvalg, spise/ta med, ankomst og senere gjenkjennelse. |
| `modules/dale.json` | Telefonoverlevering, praktisk hjelp, signalreaksjon, kontorsamtale, feltrespons og ett fast radiofragment. |
| `modules/office.json` | Nøkkel, eksisterende kontordør, 14 dokumenter og 72 sekunders VHS-manus med bilde/lyd/tekst i sju sammenhengende deler. |
| `modules/field.json` | N01-dokumentkontroll, vanlig refleks, T01/T02, kontrollresultater og tre nye inspeksjonsdokumenter. |

Pakken har 101 scener, 45 valg, 161 scenelinjer, 17 dokumenter, 39 UI-tekster og ett filmmanus. De nye private lesetransaksjonene er stille og har ingen nye stemmecues. 110 talecues er fordelt på Dale, servitrisen og vanlig lokalradiovert. Radiofragmentet gjenbruker første overleveringsopptak, så det trengs 109 forskjellige takes. De 51 stille undersøkelseslinjene skal ikke få en diegetisk Kinski-forteller.

## Leveransestatus

Manus, betingelser, stemmeregi, filmregi og integrasjonsgrunnlag er skrevet. JSON-/referansekontroll og deterministisk eksport kan kjøres nå. Pakken er koblet til den aktive C++-loaderen gjennom genererte EveningPackData.inl; ikke rediger denne eksporten for hånd.

De 109 rene stemmetakene er produsert med VoiceStudio/OmniVoice (`voice/`); WAV-filene ligger utenfor Git. Kanalmiks, innkjøring, kontorrom og T01/T02 har egne senere runtimeleveranser. VHS-filmen er fremdeles ikke produsert. Nye kontordokumenter må prøves i rootens pakkede kandidat før de oppgis som levert; denne mappen alene beviser ikke filmbevegelse, menneskelig lesing eller frykt.


## Privatmappen og arrayservicearket

`office.json` beholder de sju eksisterende kontorpapirene og legger til `EV_DALE_SKYLOG`, `EV_DALE_WITNESS`, `EV_DALE_EXPLANATIONS`, `EV_DALE_FRAME18`, `EV_DALE_TAPELOAN`, `EV_DALE_UNSENT` og det separate ordinære `EV_DALE_ARRAY_SERVICE`. Privatmappens seks tekster kan åpnes enkeltvis. Menyen gir ingen leststatus eller dokumentkvote. Den korte T02-ruten krever fortsatt bare feltmappen. `EV_DALE_INDEX` er uendret og beholder Caliche-kulverten.

`FS47Office` oppbevarer faktisk åpnete dokument-ID-er i `office.readDocuments`; pakke-/menystatus alene gir ikke leststatus ved Continue. Gamle kontorsaves uten feltet begynner med uleste nye tekster. Uregistrerte ID-er og feil datatyper gir ingen delvis lesekreditt. Fullførte papirer gjenopprettes i notatboken via vanlig dokumentvisning og dens tekstskalering.

Servicearket beskriver eksisterende gangport, hovedspor og Row One med A syv meter vest og B nitten meter øst for kabelgrøften. Ordinære fysiske serviceboards står på den eksisterende grusrekken; tilgang må kontrolleres i native prøve. Arket gir ingen fullført antenneprøve, strømkontroll, foto eller trusselretrett. Fotokonvolutten er bare konvoluttens tekst; intet fotografi/negativ tildeles. Felt-VHS og eventuelle egne bilde-/skisserekvisitter er separate produksjonsoppgaver.

Root kan kjøre `-S47DaleDocumentsContract -S47Jump=chapter2 -S47Case=0 -UserDir=/tmp/<egen-profil>/` for native innsteg, reelle dokumentpaneler, ulest/lest-skillet og ferske Writer/Reader/full-Flow-gjenopprettinger. Scenestart/Ward-forutsetninger er testfixtures; dette er ikke menneskelig input eller gjennomspilling.
