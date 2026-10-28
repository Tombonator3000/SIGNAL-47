# SIGNAL / 47 produksjonskoordinator

Lokal produksjonskø med Claude Code, gjenopptakelse, kildeoppslag og sporbare
testkvitteringer. Python 3.11 eller nyere på Linux; ingen Python-pakker eller ny
server kreves. Claude bruker den eksisterende lokale innloggingen. Systemet
endrer ikke modellvalget eller innloggingen.

## Kom i gang

Fra repoets rot:

```sh
python3 -m Automation.production doctor
python3 -m Automation.production init
python3 -m Automation.production enqueue Automation/production/examples/SELFTEST.json
python3 -m Automation.production run --job PROD-SELFTEST
python3 -m Automation.production status
```

Samme inngang er tilgjengelig fra hvor som helst med
`python3 /sti/til/repo/unreal/Signal47/Scripts/production.py`.
`--repo` og `--state-dir` er globale alternativer og står før underkommandoen.
Standard kødata ligger under `~/.local/state/signal47-production/projects/` og
deles mellom worktrees med samme Git common-dir. Kildesøk får separat indeks
per checkout. Logger, modellresultater og databaser committes ikke.

Kjør første avgrensede Claude-oppgave:

```sh
python3 -m Automation.production enqueue Automation/production/examples/CLAUDE-PILOT.json
python3 -m Automation.production run --job PROD-CLAUDE-PILOT
python3 -m Automation.production show PROD-CLAUDE-PILOT
```

Pilotoppgaven lager en kildebelagt produksjonsplan fra gjeldende status og
arbeidsliste. Den kjører ikke motoren og ferdigmelder ingen spilloppgave.
Resultatet står i `notes` i kvitteringen og venter på vurdering.

## Arbeidsløype

1. En produsent eller kodeagent lager en JSON-kontrakt med oppgave, rolle,
   avhengigheter, eksplisitt filansvar og navngitte kontroller.
2. `enqueue` validerer kontrakten og avviser ID-konflikter og sykluser.
3. Worker velger en klar oppgave og reserverer dens checkout atomisk.
4. Claude får prosjektinstruksene, oppgaven, gjeldende kildeutdrag, filhasher
   og eventuell feil fra forrige forsøk.
5. Claude leser med Read/Glob/Grep og returnerer strukturert JSON. Koordinatoren
   sjekker filansvar og tidligere SHA256 før den skriver foreslått innhold.
6. De avtalte kontrollene kjøres, og kvitteringen binder resultatet til kilde,
   faktisk arbeidskopi, modellbruk, kommandoer og bevis.
7. Teknisk godkjente oppgaver blir `passed` eller `review`, etter kontrakten.
   Et spillforløp med `requires_review` trenger egen menneskelig vurdering.

Roller er `producer`, `graphics`, `gameplay` og `qa`. De bruker samme kontrollerte
arbeidsløype, med forskjellige oppgaver og filansvar. Flere worktrees kan
arbeide samtidig; én checkout har én skrivende jobb. Alle tunge jobber deler
én reservasjon på tvers av kødatabaser og worktrees.

## Oppgavekontrakt

```json
{
  "schema_version": 1,
  "id": "GAMEPLAY-EXAMPLE",
  "title": "Rett den avtalte delsekvensen",
  "description": "Beskriv faktisk ønsket spillerhandling og bevart oppførsel.",
  "kind": "edit",
  "role": "gameplay",
  "depends_on": ["PROD-SELFTEST"],
  "allowed_paths": ["unreal/Signal47/Source/Signal47/Private/S47Example.cpp"],
  "context_query": "gjeldende beslutning oppgave scenario",
  "checks": [
    {"tool": "git_diff", "params": {}, "timeout_seconds": 20}
  ],
  "acceptance": ["Beskriv konkret hva spilleren skal kunne gjøre."],
  "artifacts": [],
  "requires_review": true,
  "max_attempts": 2,
  "budget_usd": 2.0,
  "timeout_seconds": 600
}
```

Dette er en skjemareferanse, ikke en klar spilloppgave. En faktisk kodeoppgave
må ha relevante funksjonskontroller i tillegg til `git_diff`. Typene er
`command` (bare avtalte verktøy), `report` (Claude-rapport uten endringer) og
`edit` (Claude foreslår nye/endrede tekstfiler). Et filområde som slutter med
`/` omfatter underliggende filer; andre verdier er eksakte filstier. Ingen
sletting eller direkte binærskriving gjennom modellforslag. Blender-/asset-
skript lager binærinnhold gjennom de eksisterende produksjonsverktøyene.

Claude velger ikke kontrollkommandoene. Verktøykontraktene må være konkrete
og gjennomgått av produsenten. Dette er arbeidsflytstyring, ikke en OS-sandbox
for vilkårlig kode fra et ukjent repo.

## Verktøy

`python3 -m Automation.production tools` viser hvilke verktøy som krever tungt
vindu. Adapterne bruker argumentlister uten shell-interpolering.

| Verktøy | Parametre | Resultat |
| --- | --- | --- |
| `files` | `paths` | Ikke-tomme materialiserte filer med SHA256. |
| `json_assert` | `path`, `equals` | Eksakte JSON-verdier via punktsti, med typeskille. |
| `task_contract` | `path` | Validerer en ny oppgavekontrakt og alle dens verktøy. |
| `python_compile` | `paths` | Kompilering av Python uten utførelse. |
| `unittest` | `start`, valgfri `pattern` | En ikke-tom bestått unittest-suite. |
| `git_diff` | ingen | `git diff --check`. |
| `build_editor` | ingen | Eksisterende editorbygg. |
| `package_linux` | ingen | Ny pakke under kjøringens `package/`. |
| `graphics_review` | `build`, `views`, valgfri `clock`, `quality`, `picture`, `timeline` | Eksplisitte visninger og gyldig PASS-metadata fra pakket spill. |
| `anomaly_pilot` | `build`, valgfri `scenario`, `quality`, `picture` | Eksisterende scenarioharness og faktisk PASS-rapport. |
| `full_matrix` | `build` | Eksisterende samlede matrise, med ny bevismappe. |
| `blender_script` | `script`, valgfri `args` | Blender i bakgrunnen med feilstatus fra Python. |
| `asset_script` | `script`, valgfri `args` | Eksisterende Python-produksjonsskript. |

`script` ligger under `Automation/` eller `unreal/Signal47/Tools/`.
`build` er en absolutt kandidatsti eller `{run_dir}/package`. `args` kan bruke
`{run_dir}` og `{repo}` som eksplisitte plassholdere. En kodekontrakt kan derfor
bygge, pakke og kontrollere samme kandidat i én reservert serie. Grafikkbevis
med `valid_evidence: false` avvises også om verktøyet returnerer exitkode 0.

## Tungt vindu og prosesser

Tunge jobber starter først med `run --allow-heavy`. Reservasjonen ligger i
`~/.local/state/signal47-production/heavy.lock` og beholdes gjennom hele serien,
også pausene mellom verktøy. Den nye `unreal_guard.py`-hooken observerer samme
reservasjon. Egne verktøybarn gjenkjennes med token og faktisk prosessavstamning.

Ingen fremmed spilløkt eller motorprosess stoppes. Ledig minne og eksterne
tunge prosesser kontrolleres før/under kjøring. Ved timeout stoppes bare
etterkommere av denne kjøringen. Gamle arbeidskopier uten den nye guard-hooken
må fortsatt samordnes gjennom OPPGAVER; de får ikke reservasjonssjekken bare
fordi en annen checkout er oppdatert.

Test etter prosjektets risikoregel: målrettet kontroll under utvikling og én
nødvendig samlet kontroll før installasjon/release. Verktøyet verken installerer
eller publiserer spillutgaver automatisk.

## Gjenopptakelse, budsjett og vurdering

```sh
python3 -m Automation.production run --max-jobs 10 --watch --idle-seconds 60
python3 -m Automation.production recover
python3 -m Automation.production retry GAMEPLAY-EXAMPLE
python3 -m Automation.production review GAMEPLAY-EXAMPLE --accept --note "Beskriv faktisk kontroll og resultat"
```

Køen bruker SQLite-transaksjoner og unike forsøk-ID-er. En gammel worker kan
ikke ferdigmelde et nyere forsøk. En død eier frigis; et avbrutt forsøk sendes
til `review` fremfor å kjøres blindt en gang til. Det kan ikke godkjennes som
testet. Undersøk filer/bevis og bruk `retry` innen gjenværende forsøk/budsjett.

Ferdige oppgaver kjøres ikke på nytt. Venting på avhengigheter eller opptatt
maskin bruker ikke modellbudsjett. `max_attempts` er 1 til 5. Modellens totale
rapporterte bruk trekkes fra oppgavebudsjettet mellom forsøk. Ved manglende
brukskvittering reserveres resten konservativt og merkes som ukjent faktisk
kostnad. Claude Max-bruk er ikke en ny API-faktura, selv om CLI rapporterer et
USD-estimat. Ingen ny API-nøkkel er nødvendig med eksisterende innlogging.

En bekreftet kvotefeil med null rapportert modellbruk setter oppgaven til
`blocked`, bevarer forsøksbudsjettet og venter minst 15 minutter før nytt forsøk.
En worker som avsluttes ved idle-grensen må startes igjen når kvoten er ledig.
`doctor` kontrollerer installasjon/innlogging; ledig Claude-kvote avgjøres først
av den faktiske modellforespørselen.

## Kilder og evaluering

`index`, `search` og `backlog` leser gjeldende dokumentasjon og tekstkilder.
Søket prioriterer Docs/Active og lagrer fil, linjer, SHA256 og kildeversjon.
Endrede/slettede filer og symlinker avvises til neste indeksering. Historiske
arkiver, binærinnhold og kjente hemmeligheter utelates. FTS5 har lokal fallback;
det kreves ingen vector database eller ekstern datakopi.

Bevis ligger i `runs/OPPGAVE-ID/FORSØK-ID/`: `receipt.json`, faktisk Claude-JSON,
stderr, gjeldende kontekst, forslag og kontrolllogger. `status.json` gir en
lett maskinlesbar køoversikt. `passed` gjelder kontraktens automatiske krav;
menneskelig input, lytting, spillerforståelse og skrekkrytme må ha egne bevis.

Raske tester:

```sh
python3 -m unittest discover -s Automation/production/tests -p 'test*.py'
```

Prosjektbibliotek ble søkt før implementering. EngineWire-profilen er merket
UNVERIFIED og er ikke installert eller brukt som kode. Systemet gjenbruker
prosjektets faktiske produksjonsskript. Claude CLI-oppførselen er kontrollert
mot lokalt `claude --help` og [offisiell dokumentasjon](https://code.claude.com/docs/en/headless).
