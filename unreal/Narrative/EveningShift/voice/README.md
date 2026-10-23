# Stemmetakes for kveldspakken

109 rene engelske takes for alle innspilte cues i `../generated/recording_manifest.json`: 56 Dale, 43 servitrise og 10 lokalradiovert. `D61_FIXED_FRAGMENT_001` gjenbruker `D01_HANDOVER_001` gjennom `recording_aliases` og har ingen egen take. Pakken har ingen Ward- eller ekspeditørreplikker; Ward og Nora får derfor ingen opptak herfra, og aldri Dales stemme.

Takes er laget med eksisterende lokal VoiceStudio 0.5.6 og OmniVoice (`k2-fsa/OmniVoice`, revisjon `c5fdb5cc…`), headless mot det lokale API-et på `127.0.0.1:3900`. Ingen ny motor er installert.

## Stemmer

| Aktør | pack.json-profil | VoiceStudio-profil | Grunnlag |
|---|---|---|---|
| DALE | kinski | `3cf0d3a5` Kinski – sint parodi (Fitzcarraldo) | Toms valg, samme profil som Kinski-korpuset |
| WAITRESS | waitress | `84bc5b84` anonym kvinnelig innringer (CC0) | Ingen eksisterende servitriseprofil; eneste kvinnestemme som ikke er en navngitt rolle |
| RADIO_HOST | evening_host | `d65b2bb6` anonym eldre innringer (CC0) | Ikke Earl og ikke Kinski; lett og munter referanse, brukt i bare tre innringerklipp |

Begrunnelser, forkastede alternativer og referansehasher står i `casting.json`. Servitrisen og verten deler stemme med hver sin anonyme innringer i Earls sending. Det er dokumentert, ikke lyttekontrollert.

## Innstillinger og etterbehandling

Alle forespørsler bruker engelsk, OmniVoice, 32 steg, seed 42, speed 1 og denoise. `effect_preset` er VoiceStudios dokumenterte standard `broadcast`, som i Kinski-korpuset: highpass ved 80 Hz, mild kompresjon og EQ og en limiter. Det er en mastering, ikke et telefon- eller radiofilter. Kanalfilteret legges på i spillet.

Rå modellutgang (24 kHz) blir trimmet til talen med 80 ms luft før og opptil 200 ms etter. Den resamples til 48 kHz med soxr og får én konstant gain mot -20 dBFS aktiv RMS, med topp på høyst -2 dBFS. Det er samme lydnivåregel som i Kinski-korpuset, med median -20,0 dBFS for Dale her og -20,4 dBFS for Kinski. Til slutt kommer 10 ms inntoning og 30 ms uttoning inne i luften. Filene er PCM16 mono WAV uten metadata. Det brukes ingen tidsstrekking, ingen tonehøydeendring og ingen ekstra kompresjon.

## Filer

Ferdige WAV-filer ligger utenfor Git:

- `~/.cache/signal47-eveningshift/takes/<LINE_ID>.wav`: hovedkopien. Rå modellutgang og API-svar ligger i `raw/`.
- `unreal/Signal47/Content/Audio/EveningShift/<LINE_ID>.wav`: kopi i Unreal-sidens ignorerte lydmappe, laget med `--sync`. `audio_file` i manifestet er relativt til `Content/Audio`.

Committet her: `generate.py`, `verify.py`, `casting.json`, `speech-overrides.json`, `retakes.json`, `asr-review.json`, `generation.json` (forespørsel, svar, hasher og målinger per take), `verification.json`, `takes.json` (målt varighet og talestart/-slutt, lest av `build_pack.py`) og `provenance.json`.

```sh
python3 generate.py --limit 25              # neste manglende takes, seriell GPU-bruk
python3 generate.py --until-asr ID...       # ny seed til ASR stemmer, logges i retakes.json
python3 verify.py                           # format, PCM, nivå, kanter og ASR av alle takes
python3 generate.py --sync ../../../Signal47/Content/Audio/EveningShift
python3 ../build_pack.py && python3 ../build_pack.py --check   # fra reporoten
```

## Kontroll

`verification.json` gir teknisk status **PASS** for 109 av 109 takes. Alle er 48 kHz PCM16 mono WAV med riktig hash, uten klipping og med topp på høyst -2 dBFS. Aktiv RMS ligger mellom -22,5 og -19,6 dBFS, integrert loudness mellom -23,7 og -18,8 LUFS. Alle kanter er tonet inn og ut, og talen starter etter 80 ms. Samlet lengde er 309,5 sekunder.

Ordstatus er CHECKED_WITH_UNRESOLVED_ASR. Faster-whisper via VoiceStudio `/transcribe` har kontrollert alle 109. 100 stemmer etter normalisering av tall og stavemåte, og 95 stemmer helt ordrett. Sju avvik er vurdert som stedsnavn eller homofoner: Caliche som «Kalish», SARO som «Sorrow» to ganger, Paper's/Papers, sandwich is/sandwiches, to/too og pole/poll. To avvik står uløst og må lyttes på:

- `D01_FOOD_002` «I tried that once» høres som «Try that once» i alle seeds 42–48 og 14 varianter. Det innledende «I» kan mangle.
- `D01_BRIEF_005` «observations she» høres som «observation she», mulig s/sj-sammensmelting.

Første ASR-runde fant 19 avvik. Tolv cues fikk nye seeds (`retakes.json`). Tre fikk uttalestaving (`speech-overrides.json`). Modellen droppet navnet Reyes i Dales åpning, så det sies som «Ray-yes?». Pakkens tekst og undertekster er uendret. To cues ble satt tilbake til seed 42 fordi senere seeds var verre.

Automatisk transkripsjon er ingen lyttetest. Stemmelikhet, rolletroverdighet, innlevelse og hørbare artefakter er **UNVERIFIED** til noen har lyttet.

## Lisens

- VoiceStudio er AGPL-3.0-only.
- OmniVoice-koden er Apache-2.0. Det nedlastede modellkortet oppgir CC-BY-NC for vektene på grunn av treningsdata, for eksempel Emilia. CC0-kildelyd endrer ikke denne begrensningen.
- Kinski-referansen er et kort utdrag fra en opphavsrettslig beskyttet dokumentar. Stemmen er en syntetisk parodi, og ingen lisens, samtykke eller anbefaling hevdes.
- Kyutais voice-donations (Selfie.wav og oldNerd.wav) er CC0-1.0 ifølge det festede README-et. Donorens kjønn, alder og aksent er ikke verifisert.
- Originalopptak, referanse-WAV, modellvekter og lokale profilstier brukes bare under produksjon og følger ikke spillet. Se `provenance.json`.
