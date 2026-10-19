# Radiostemmer og 1947-innslaget: QA

`tools/radiocast-radio.py` tester det bygde spillet. `S47_URL` velger vanlig Pages-bygg med `/SIGNAL-47/`; uten variabelen tester den `dist-single/index.html` under samme strenge CSP som `readaloud.py`. Kjør begge gjennom prosjektets serielle GPU-kø etter at alle 28 MP3-er er ferdige. Nettlesertesten bygger ikke og genererer ingen lyd.

Feature-status 19. oktober 2026: **PASS på begge bygg** etter retting av samme-sample-gjenopptak og venting på touch-knappen i den holdte testharnessen.

| Bygg | Resultat | Bevis |
| --- | --- | --- |
| Pages under `/SIGNAL-47/` | 70/70 PASS | [JSON](pages/radiocast-radio.json), [logg](radiocast-pages.log) |
| Enkeltfil under streng CSP | 71/71 PASS | [JSON](single/radiocast-radio.json), [logg](radiocast-single.log) |

Begge rapportene har null nettleserfeil og null mislykkede MP3-forespørsler. Enkeltfilen har null eksterne MP3-forespørsler. Begge beviser 22 individuelle innringerstarter, seks naturlige nyhetssluttpunkter og samme sampleoffset etter hele 47-sekundersinnslaget. Første feilrapporter er bevart utenfor repo i samtalens `outputs/radiocast/failed-first-pages` og `failed-first-single`. Python-syntaks og de tre JavaScript-probenes syntaks er også kontrollert. Dette er feature-resultater; status for øvrig kompatibilitet, regresjoner og kostmåling føres separat av hovedagenten.

Prøven observerer original dekoder, `AudioBufferSourceNode.start/stop/ended`, faktisk bufferidentitet, avspillingsrate og lydgraf. Ingen tale erstattes. Hver av de 22 innringerfilene starter i en kort manus-fixture; disse avbrytes etter den første starten. En blandet Art Bell/innringer-utveksling og alle seks nyhetsfiler får full naturlig avspilling. Dette er ikke en lyttetest av uttale, stemmelikhet eller fysiske høyttalere.

- Korpus og katalog: 8 kvinne-, 8 mann-, 3 eldre-, 3 gutt- og 6 nyhetsklipp; SHA-256 av rolle, nulltegn og normalisert tekst, samt faktisk MP3-fil.
- Ekte R- og touch-Radio-handlere, AM-filter, `radioBus` og master, ikke-null PCM og to mottakere som deler én stasjon.
- Hver teksting følger den ekte dekodede varigheten; blandet vert/innringer køer med naturlig tale og pauser.
- Faktisk `NightEvents.now('bulletin', nightIn())`: ekte, kald dekoding holdes tilbake. 49 simulerte sekunder gir ikke tidlig notat. Avbrudd gir ingen fullføringsmarkør og tillater nytt forsøk.
- Hele 47 sekunder følger den pauserbare lydklokken, med seks komplette MP3-er på rate 1. Etterpå kommer samme avbrutte programplan, buffer og offset tilbake. Bare full lytting gir ett notat og `ne:bulletin1`, også kontrollert i et faktisk lagret case.
- Den ekte velgeren ved 02:26 velger guttens tre filer og oppretter eksisterende notat og kartmarkør.
- Av, tuning, reset, lastet case og tittel stopper både aktive/fremtidige kilder og sene resultater fra genuin dekoding. Ukjent tekst og avvist dekoding bruker hele den opprinnelige prosedyrestemmen.
- Mobil 844 × 390: egne innstillinger for innringere/nyheter, Art Bell og Kinski, ekte touch, lagring og reload. Enkeltfilen tillater ingen eksterne MP3-forespørsler og trenger ikke `unsafe-eval` eller `connect-src`.

Eksisterende kontrakter beholdes: `radio.py` og `tripradio.py` velger eksplisitt begge prosedyrestemmene Av før rask spoling. `artbell-radio.py` velger innringere Av for sine eksisterende vertsspesifikke forventninger. Den nye prøven dekker standard På og alle 28 nye opptak. Runtime-agenten vedlikeholder CPU-prøven `radioarchive.ts`, inkludert opprinnelig 47-sekunders avbrudd/gjenopptak og den nye fremdriftskontrakten. `storyexpansion.py` er uendret; den dekker vitner, telefoner og motellets TV.

## Samlet sluttkontroll

Alle kilde- og bygghasher samsvarer med den testede kandidaten. Se [validation.json](validation.json) og [byggmanifestet](../build-manifest.json).

| Kontroll | Resultat |
| --- | --- |
| Radiostemmer i Pages / streng-CSP enkeltfil | 70/70 / 71/71 PASS |
| Ny motor / gammel Art Bell / arkiv | 77/77 / 48/48 / 51/51 PASS |
| Kinski-motor og tekstkorpus | 55/55 og 26/26 PASS |
| Telefonmotor / lagring / styring | 28/28 / 19/19 / 5/5 PASS |
| Kompatibilitet | Alle 16 kjøringer PASS |
| Spillregresjon | Alle 13 kjøringer PASS |
| Lydkostnad | PASS, egen resultatfil |
| Privat lydbank | Med og uten PASS |
| Tidligere stemmefiler | 1670 MP3 og begge kataloger uendret |

Kompatibilitet omfatter Art Bell, opprinnelig radio og ruter, ny historie, lydutfall, privatbanken, opplesning, telefoner, beskrivelser og CSP. Spillregresjonene dekker walkthrough, kapitlene 1 til 6, utviklermeny, kjøring, atmosfære, diner og musikk i begge bygg. Tripradio går i faktisk lydtid under bulletinen og beholder alle 35 eksisterende krav. Tidligere feilforsøk og store øvrige skjermbilder er bevart utenfor Git i samtalens `outputs/radiocast`; de seks nye funksjonsbildene ligger her i `pages` og `single`.

Dette beviser automatisk dekoding, graf, tidsforløp, lagring og inndata. Menneskelig lytting, stemmenes rolleklang, uttale, tidsstil, fysiske høyttalere og målt enhetsytelse er UNVERIFIED. Faktisk PR/main/Pages-levering og fjernfilhash dokumenteres etter publisering i samtalens leveransebevis.
