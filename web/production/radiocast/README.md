# Fiktive radiostemmer

Den valgfrie banken for frakoblet avspilling inneholder de 22 opprinnelige innringerreplikkene og seks oppdiktede nyhetsreplikker fra 1947: 28 klipp og 406 ord. Banken utfyller den separate Art Bell-banken for verten. Manus, CB, Roswell 1230 og personer som snakker ansikt til ansikt, inngår ikke i denne banken.

| Rolle | Klipp | Lokal profil | Grunnlag |
| --- | ---: | --- | --- |
| woman | 8 | 84bc5b84 | Kyutais CC0-donasjon Selfie.wav |
| man | 8 | 1a160d70 | Kyutais CC0-donasjon Butter.wav |
| old | 3 | d65b2bb6 | Kyutais CC0-donasjon oldNerd.wav |
| boy | 3 | 34043cb8 | Fiktiv stemme fra OmniVoice-stemmedesign |
| news | 6 | 1e28f816 | Fiktiv stemme fra OmniVoice-stemmedesign |

Kyutai oppgir uttrykkelig CC0 for mappen voice-donations. Kilderevisjonen er låst til 323332d33f997de8394f24a193e1a76df720e01a. Individuelle nettadresser, SHA-256 for originalfilene, valgte tidsintervaller, referansetekster og lisensbevis finnes i provenance.json. Originale donoropptak og lokale identitetsprøver oppbevares utenfor Git-repoet.

Kilde: [Kyutais stemmedonasjoner](https://huggingface.co/kyutai/tts-voices/blob/323332d33f997de8394f24a193e1a76df720e01a/README.md#voice-donations). Målt periodisk grunntone støtter valget av ulike registre, men bekrefter ikke donorens kjønn, alder eller aksent. oldNerd er et filnavn, ikke dokumentasjon på alder. Ingen menneskelig lytting eller vurdering av rolleprestasjon er gjennomført.

Guttestemmen bruker de støttede taggene `male, child, moderate pitch, american accent`. Ingen virkelig barnestemme brukes. Tre første designforsøk med den spørsmålsledede første guttereplikken som identitetsprøve ga ingen gjenkjennelige ord i ASR. Dette omfattet high pitch, moderate pitch og teenager. En ny child/moderate pitch-prøve med motorens vanlige, hele prøvesetning ble gjenkjent ordrett. Bare de tre guttefilene ble deretter generert på nytt. Feilforsøkene er dokumentert i rejected-boy-design.json; de er ikke godkjent som tale på grunnlag av PCM-energi alene.

Nyhetsleseren bruker `male, middle-aged, low pitch, american accent`. Han fremstilles som en fiktiv rolle, ikke en virkelig nyhetsoppleser fra 1947. Ingen ABC-sending eller annet uklart arkivopptak følger med spillet.

Eksisterende VoiceStudio 0.5.6 og installert OmniVoice brukes. Koden er Apache-2.0; modellkortet for de nedlastede OmniVoice-vektene oppgir CC-BY-NC på grunn av treningsdata. CC0-kildelyd endrer ikke modellens begrensning. VoiceStudio er AGPL-3.0-only. Modellrevisjon og lisensbevis står i provenance.json.

## Gjenskaping

Med de eksisterende lokale VoiceStudio-profilene tilgjengelige:

```sh
node tools/radiocast/extract.mjs
python3 tools/radiocast/generate.py
python3 tools/radiocast/verify.py --asr 28
```

Genereringen er seriell og kan gjenopptas. Forespørslene bruker engelsk, OmniVoice, 32 steg, seed 42, speed 1 og det dokumenterte standardvalget broadcast. Mellomlagrede WAV-filer ligger utenfor repoet. Cacheidentiteten inkluderer profil, referansehash og innstillinger; endret referanse avvises fremfor å gjenbruke en annen stemme.

Hvert klipp får én konstant forsterkning mot -20 dBFS aktiv RMS, med toppgrense -2 dBFS etter MP3-dekoding. Filene er mono MP3, 24 kHz og 64 kbit/s. Tonehøyde og tidsforløp endres ikke. De seks nyhetsopptakene varer til sammen 25,09 sekunder ved normal hastighet, med plass til pauser i hendelsens 47 sekunder.

Katalogen inneholder rolle i voice, nøyaktig tekst med normaliserte mellomrom, positiv varighet, filnavn og stabil ID: de første 16 heksadesimale tegnene i SHA-256 av rolle + NUL + normalisert UTF-8-tekst. Ingen profil-ID, referanse-WAV, modell, privat filsti eller lokal tjenesteavhengighet inngår i kjøretidskatalogen. Katalogen kontrolleres mot AST-korpuset.

verification.json dokumenterer full fildekoding, format, filhash, energi, topper, varighet, dropped_chunks og manglende eller foreldreløse filer. Alle 28 replikker ordkontrolleres automatisk med eksisterende faster-whisper-small. ASR er en ufullkommen ordkontroll, ikke en lyttetest. Tallformat, egennavn, homofoner og enkelte småord kan gi utslag uten at lyden nødvendigvis er feil. Stemmelighet, uttale, troverdig tidsstil og innlevelse forblir UNVERIFIED ved menneskelig lytting. profile-preservation.json viser uendret metadata for alle tidligere profiler.

Sluttkontroll: 28 filer, 1 024 976 byte og 125,61 sekunder. Alle filer har positiv dekodet varighet, ingen fullskalaklipping, korrekt format, samsvarende hash og dropped_chunks = 0. Katalogen har ingen mangler eller foreldreløse MP3-filer. Alle 28 nåværende filer er ASR-kontrollert: 21 er ordrette etter enkel ordnormalisering, mens 7 har registrerte forskjeller. Alle tre korrigerte gutteklippene er ordrette. Forskjellene og begrensningene står i verification.json. De tre mislykkede midlertidige profilene er fjernet; alle sju profiler som fantes før arbeidet, har uendret metadata.
