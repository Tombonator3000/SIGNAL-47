# Manuspakke for innkjøring, Dale og det hemmelige kontoret

Ferdig skrevet engelsk spilltekst for den nye Unreal-fortellingen: vanlig innkjøring gjennom Caliche, valgfritt matstopp, telefonoverlevering 18:00, Dales senere samtaler, kontornøkkel, private dokumenter og et bevegelig VHS-feltopptak. Trafostasjonens to besøk og aktive sammenligninger er også skrevet.

**Kinski/Dale finnes bare på telefon, radio eller walkie-talkie.** Ingen fysisk Dale-figur og ingen Kinski-forteller på TV/VHS. Den siste bruker miljølyd og skriftlige merknader. Ward, Nora og Earl beholder sine roller.

## Les og bruk materialet

- [Lesemanus](generated/SCRIPT.md): alle scener, valg, dokumenter, feltopptak og UI-tekster samlet.
- [Opptaksliste](generated/voice_lines.csv): engelske replikker med aktør, kanal, stemmeprofil, regi og eventuell gjenbruk.
- [Opptaksmanifest](generated/recording_manifest.json): lydfiler, format og målt varighet. Alle 109 takes er produsert; se [stemmetakene](voice/README.md).
- [Undertekstgrunnlag](generated/subtitles.json): dialog og stille tekster. Dialogtiming er målt fra opptakene; VHS-timing er planlagt.
- [Integrasjon](INTEGRATION.md): triggerpunkter, kapittelgrenser, tilstand, lyd/film og konkret produksjonsliste.
- [Kontrollscenarier](SCENARIOS.md): ruter som må fungere og hva som kan kontrolleres før runtime.
- [Overordnet plan](../../ANOMALY_PLAN.md): historien, byen, trafostasjonen og gjennomføringsrekkefølgen.

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
| `modules/office.json` | Nøkkel, eksisterende kontordør, sju dokumenter og 72 sekunders VHS-manus med bilde/lyd/tekst i sju sammenhengende deler. |
| `modules/field.json` | N01-dokumentkontroll, vanlig refleks, T01/T02, kontrollresultater og tre nye inspeksjonsdokumenter. |

Pakken har 61 scener, 45 valg, 161 scenelinjer, 10 dokumenter, 39 UI-tekster og ett filmmanus. 110 talecues er fordelt på Dale, servitrisen og vanlig lokalradiovert. Radiofragmentet gjenbruker første overleveringsopptak, så det trengs 109 forskjellige takes. De 51 stille undersøkelseslinjene skal ikke få en diegetisk Kinski-forteller.

## Leveransestatus

Manus, betingelser, stemmeregi, filmregi og integrasjonsgrunnlag er skrevet. JSON-/referansekontroll og deterministisk eksport kan kjøres nå. Denne mappen er ikke koblet til en aktiv Unreal-loader.

De 109 rene stemmetakene er produsert med VoiceStudio/OmniVoice (`voice/`); WAV-filene ligger utenfor Git. Kanalmiks, musikk, VHS-film, kontorrom, ny innkjøringsrute og runtimeadapter er ikke produsert av denne leveransen. Faktisk spillflyt, lyd i mottaker, filmbevegelse, lagring og ytelse står UNVERIFIED frem til implementering og prøving i spillet.
