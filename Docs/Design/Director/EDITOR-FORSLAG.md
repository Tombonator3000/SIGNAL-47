# Forslag: visuell editor for oppdrag, saker, radiotekst og hendelser

**Status:** Forslag, ikke vedtatt. **Gjelder:** Freeform først; Story kun lesing. **Sist kontrollert:** 7. november 2026. Lisens og aktivitet er lest fra GitHubs API og prosjektsidene samme dag. Ingen av verktøyene er installert eller prøvd.

## Anbefaling

**En liten lokal nettside: én HTML-fil og et lite Python-skript uten avhengigheter, `Tools/director/editor.py`.** Den leser og skriver de samme JSON-filene som spillet bruker, og kjører samme validering. Tom starter den med én kommando og får den i nettleseren. Ingen tjeneste på nett, ingen konto og ingen motorjobb.

Størrelse: **M** for hendelser, tidslinje og validering. **M** til for saker og radiotekst, fordi disse først må flyttes ut av C++. **L** hvis alt også skal ha full node-redigering av samtaler.

Hvorfor Python-skript og ikke bare HTML: En rå HTML-fil fra disk kan ikke skrive tilbake i alle nettlesere. Skriptet gir trygg lagring (midlertidig fil, så bytte), samme validator som bygget bruker, og simulatoren. `python3` finnes på maskinen, `node` gjør det ikke. Prosjektet har allerede `Tools/`-mapper og Python-validering i `generate.py`/`build_pack.py`, så dette er kjent mønster. Skriptet lytter bare på `127.0.0.1`.

## Hva Tom kan gjøre i den

- **Hendelser:** se alle i én liste og et tidslinjekart over døgnet (19 til 06, og hvilke dager). Dra timevinduer, endre sjanse, nedkjøling og budsjett.
- **Prøve rytmen:** «Simuler 30 netter» som spillertype og se hva som ville skjedd, uten å starte spillet.
- **Prøve i spillet:** «Send til spillet» skriver override-filen, og F10-panelet laster den.
- **Saker:** skrive spørsmål, tre forklaringer, tre prøver, sannhetstabell, spor og svar. Editoren viser om prøvene faktisk skiller forklaringene.
- **Radio og tips:** skrive meldinger og velge hvilket sted eller hvilken hendelse de hører til.
- **Nye steder for lys:** legge til et sted fra en liste over steder spillet kjenner.
- **Samtaler:** se scener som noder med valg, og redigere replikker i kveldspakken.
- **Se hva som er endret:** vise `git diff` for filen. Editoren committer ikke selv.

**Hva den ikke kan:** lage ny oppførsel. En ny type møte, kropp, lyd eller mekanikk må fortsatt programmeres. Editoren kan bruke handlinger spillet allerede har (`compare`, `field`, `service`, `photo`, `feeder`, `escape`, `telex`, `control`, `sample` i `generate.py`). Det gjør det enkelt å lage flere saker og tips, men en virkelig ny hendelse trenger også kode.

## Kilder den redigerer

| Innhold | Fil i dag | Må gjøres først |
| --- | --- | --- |
| Rytme og regler | ny `Content/Director/director.json` | Trinn 1 i [DIRECTOR-FORSLAG.md](DIRECTOR-FORSLAG.md) |
| Hendelsestekster | `unreal/Narrative/Freeform/events.json` | Ingenting. Allerede JSON. |
| Kveldens samtaler, dokumenter og flagg | `unreal/Narrative/EveningShift/modules/*.json` | Ingenting. Validering finnes i `build_pack.py`. |
| Story-oppdrag og scener | `unreal/Narrative/HorrorStory/missions.json`, `scenarios.json` | Kun lesing til Freeform er ferdig (Toms beslutning). |
| Saker | C++-tabell i `S47FreeformCases.cpp` | Flytt til `Narrative/Freeform/cases.json` med `generate.py`. M. Ingen endret oppførsel. |
| Radiotips, feltmeldinger | Tekst i `S47FreeformDirector.cpp`, `S47FarmSite.cpp`, `S47DoorVisit.cpp` og `S47Taken.cpp` | Flytt til `Narrative/Freeform/lines.json`. S–M, etter den andre agentens fletting. |

## Validering mot spillets regler

Samme Python-modul brukes av editoren og bygget, så de kan ikke være uenige.

- **Struktur:** unike ID-er, `day>=1`, `duration>0`, gyldige `family` og `action`. Dette gjør `generate.py` allerede. Dagens `assert len(ids)==19` må fjernes før nye hendelser kan legges til.
- **Lagringer:** En ID kan ikke slettes eller gis nytt navn, bare merkes `retired`. `Begin` slår av hele Director for en lagring hvis aktiv hendelse mangler i katalogen (`S47FreeformDirector.cpp:299`).
- **Rytme:** `danger`-hendelser varsler hvis timevinduet går inn i kvelden, med tekst som viser Toms føring. Advarsel, ikke feil, siden kl. 02 ikke lenger er absolutt.
- **Saker:** hver forklaring må gi et annet mønster over prøvene. Dagens tabeller følger dette; nye må også gjøre det.
- **Språk:** spilltekst på engelsk, `*_nb` på norsk. Feltnotat varsler på ord som «alien» eller påstand om opphav, samme tanke som egenprøven i `S47Aftermath.cpp:179`.
- **Kanon:** 1420.405 MHz, 47 sekunder, 27 antenner og retning 026 varsles hvis de endres. Modemlyd avvises i radio. Dale bare via `phone`, `walkie`, `radio`.
- **Referanser:** steder, flagg, lyder og dokumenter må finnes.
- **Story:** ingen `story`-hendelse med tilfeldig tidshopp.

Regler som krever verdenens tilstand, som sikt og faktisk dør, kan ikke kontrolleres her. De sees i F10-panelet.

## Visningen

- **Tidslinje** som hovedbilde: rader per hendelse, kolonner kl. 18 til 08. Faste tider vises som streker, trekk som felt med prosent, konflikter i rødt.
- **Noder** bare der noe faktisk er en graf: samtaler med valg, sak med prøver og svar. Egen enkel SVG holder for saker. For fri node-redigering av samtaler er Drawflow (MIT, ingen avhengigheter) førstevalg, som én lokal kopi med lisensfil.
- **Skjema** til høyre for det valgte. Lagring gir validering med en gang.

## Sammenligning

| Valg | Bra | Dårlig | Vurdering |
| --- | --- | --- | --- |
| **Lokal nettside + Python** | Virker uten motor. Samme JSON og validator. Tekstfiler kan flettes i Git. Agenter og Tom kan bruke samme filer. | Må lages og vedlikeholdes. Ingen 3D-visning. | **Anbefalt** |
| Unreal-plugin, Editor Utility Widget | Kan se nivå og assets. | Krever at editoren er åpen, en tung jobb som ikke kan gå sammen med prøver. Binær `.uasset` kan ikke flettes eller leses av agenter. Ingen treff på `EditorUtility` i C++-kilden. | Nei |
| Innebygd F10-panel | Justering mens du spiller. | Dårlig til å skrive tekst. | Ja til lesing og last på nytt, nei til forfatting |

## Frie verktøy

| Verktøy | Lisens og status | Hva det kan | Passer? |
| --- | --- | --- | --- |
| [Twine](https://github.com/klembot/twinejs) | Verktøyet GPL-3.0. Aktivt. Lisens for utdata er egen for hvert historieformat og ikke avklart her. | Forgrenet tekst, visuelt kart. | **Nei som del av spillet.** Kjenner ikke tider, sjanser eller sakslogikk. Grei som skisseblokk for Tom, uten kobling til spillet. |
| [Yarn Spinner](https://github.com/YarnSpinnerTool/YarnSpinner) | Kjernen MIT. [Unreal-utgaven](https://www.yarnspinner.dev/docs/unreal/01-installation/) har egen Yarn Spinner Public License, er forhåndsutgave, «We don't recommend you use it to ship a game yet», krever UE 5.7+ og .NET-kompilator. Den gamle Unreal-repoen er arkivert. | Skriptede samtaler med valg og kommandoer. | **Nei nå.** Spillet har allerede eget samtaleformat med opptak og stemmeprofiler. Å bytte er omskriving. |
| [ink](https://github.com/inkle/ink) / [Inky](https://github.com/inkle/inky) / [Inkpot](https://github.com/The-Chinese-Room/Inkpot) | ink MIT, [inkle oppgir MIT](https://inklestudios.com/ink/) for Inky. Inkpot MIT, UE 5.7+ og 5.8, aktivt. Oppsettet er dokumentert for Windows; Linux er bare nevnt i endringsloggen. Kompilatoren krever .NET 5. | Svært godt skriveverktøy for lang, forgrenet tekst. | **Ikke nå.** Løser bare samtaler, og krever ny plugin. Eneste jeg ville vurdert senere, hvis Freeform får mange lange samtaler. |
| [Arrow](https://github.com/mhgolkar/Arrow) | MIT. Linux-utgave. Siste kode september 2025. | Node-basert fortellerdesign, kan spille av i HTML, versjonskontroll-vennlige filer. Prosjektsiden bekrefter ikke egne nodetyper eller validering. | **Nei.** Eget format, må ha konverterer, og kjenner ikke spillets regler. Verdt et blikk som inspirasjon til nodevisning. |
| [Monologue](https://github.com/atomic-junky/Monologue) | MIT. Lite prosjekt, 51 stjerner. | Dialogkart lagret som JSON. | Nei, samme grunn. |
| [Drawflow](https://github.com/jerosoler/Drawflow) | MIT. Ingen avhengigheter. Siste kode oktober 2024, mange åpne saker. | Nodekart i ren JavaScript med eksport og import. | **Ja, som bibliotek** ved behov, kopiert lokalt. Lite nok til å bytte ut. |
| [Rete](https://github.com/retejs/rete), [xyflow](https://github.com/xyflow/xyflow), [LiteGraph](https://github.com/jagenjo/litegraph.js) | MIT. Rete og xyflow aktive, LiteGraph sist 2024. | Nodekart. | Nei foreløpig. Ikke vurdert i dybden: Drawflow dekker behovet uten rammeverk. |

**Prosjektbiblioteket** ga ingen relevante treff, også med `--match any` (generelle RPG- og spillskills uten editor). `FREE_ASSET_CATALOG.md` har bare modeller, lyd og font; ingen verktøy å gjenbruke. Editoren krever ingen endring der før Drawflow eventuelt legges inn; da føres den med lisens i `Docs/THIRD_PARTY_NOTICES.md`.

## Rekkefølge

1. **Første versjon (M):** `director.json` og `events.json`, tidslinje, skjema, validering, simulator, «send til spillet». Krever trinn 1 fra Director-forslaget.
2. **Saker og tekst (M):** flytt saker og radiolinjer til JSON med egenprøve om at innholdet er likt, så skjema og saksvisning.
3. **Samtaler (M til L):** nodevisning av kveldspakken. Story kun lesing.

## Tom må avgjøre

1. Er lokal nettside riktig valg? Anbefaling: ja.
2. Hva er viktigst først: rytme og hendelser, eller nye saker og tips? Anbefaling: rytmen først, fordi den er minst arbeid og gir oversikten du ba om. Hvis målet er flere oppdrag raskt, kan trinn 2 tas samtidig.
3. Skal Story-filer være redigerbare? Anbefaling: bare lesing til Freeform er ferdig.
