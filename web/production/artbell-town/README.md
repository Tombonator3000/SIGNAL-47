# Art Bell som Earl i byen

Denne separate banken gir Earls eksisterende bysamtaler den samme syntetiske Art Bell-parodistemmen som radioverten. Manus er uendret. De 115 radiovertsklippene, 28 innringer-/nyhetsklippene, 1555 Kinski-klippene og de tre tidligere katalogene skal være byteidentiske mot 210b52f.

AST-uttrekket leser bare statiske `EA(...)`-kall under `PEOPLE.host` i `src/story/caliche.ts`. Det kontrollerer at hjelperen fortsatt bruker `who: 'EARL'`. Begge hilsengrenene og alle samtaleemnene inngår:

| Del | Klipp |
| --- | ---: |
| Første møte og gjenbesøk | 2 |
| Calls | 4 |
| Visitors | 2 |
| Tape | 4 |
| Boy, den utvidede oppfølgingen | 4 |
| Sum | 16 |

De 16 unike replikkene er 150 ord. Spillersvar, asider, valgknapper, status og vitnepapirer er utelatt. Ingen av tekstene finnes i radiovertens 115-klippsbank. Identiske tekst-ID-er kan også finnes i Kinski-banken; bankene har hver sin mappe og stemmeidentitet.

Eksisterende lokal VoiceStudio-profil `494881fc` brukes. Referansehash er låst til `1e99f04ee7126f5d6f3e0a23d3cd20fd5a12d6b5ed563bc95e61220653cbea82`. Profil, referanseopptak og modeller endres ikke. Referansens opprinnelse, tidsintervall og tidligere ASR er videreført fra radiovertens proveniens til provenance.json. Den er en bevart Art Bell-hilsen på Artbell.com Commemorative Disc fra 2003. Offentlig tilgang er ikke en ny lisens eller en bekreftelse på godkjenning; originalopptaket følger ikke spillet.

VoiceStudio 0.5.6 og installert OmniVoice gjenbrukes. VoiceStudio er AGPL-3.0-only. Modellkoden er Apache-2.0; det nedlastede modellkortet oppgir CC-BY-NC for de forhåndstrente vektene på grunn av treningsdata. Nøyaktig modellrevisjon, kortets hash og lisensbevis står i provenance.json. Ingen ny tjeneste eller modell er installert.

## Gjenskaping

Med den eksisterende lokale profilen tilgjengelig:

```sh
node tools/artbell-town/extract.mjs
python3 tools/artbell-town/generate.py
python3 tools/artbell-town/verify.py --asr 16
```

Genereringen er seriell og kan gjenopptas. Innstillingene er engelsk, OmniVoice, 32 steg, seed 42, speed 1, denoise og det dokumenterte standardvalget broadcast. Mellomlagrede WAV-filer ligger utenfor repoet. Profilreferansens hash kontrolleres før generering, og cacheidentiteten inkluderer innstillinger og referansehash.

Hver fil får én konstant forsterkning mot -20 dBFS aktiv RMS, begrenset av maksimalt -2 dBFS topp etter MP3-dekoding. Formatet er mono MP3, 24 kHz og 64 kbit/s. Ingen tidsstrekking, tonehøydeendring eller hastighetsskalering brukes. Normalisert tekst beholder originalordlyden; ID er de første 16 heksadesimale tegnene fra SHA-256 av teksten etter vanlig sammenslåing av mellomrom og trimming.

Kjøretidsmappen inneholder bare 16 genererte MP3-filer og catalog.json. Katalogen har tekst, ID, filnavn og positiv varighet. Ingen profil-ID, referanse-WAV, originalkilde, modell, lokal tjenesteadresse eller privat filsti følger med katalogen.

Alle filer dekodes og kontrolleres med ffprobe, filhash, energi, topp, varighet og dropped_chunks. Alle 16 ordkontrolleres med eksisterende faster-whisper-small. ASR er en ufullkommen automatisk tekstkontroll, ikke en lyttetest. Tallformat, egennavn, homofoner og småord kan gi avvik. Stemmelikhet, uttale, innlevelse og fysisk avspilling forblir UNVERIFIED ved menneskelig lytting. Profile-preservation og asset-preservation dokumenterer bevaring før og etter produksjonen.

Sluttkontroll: 16 MP3-filer, 370 496 byte og 44,92 sekunder. Alle har riktig format, positiv dekodet varighet, filhash i samsvar og dropped_chunks = 0. Ingen fullskalaklipping, manglende eller foreldreløse filer. ASR: 14 strengt ordrette; to har bare dokumenterte skrive-/tall-/homofonavvik. Alle 1698 gamle MP3, tre kataloger og 12 profilmetadata er bevart. Ingen ny profil, modell eller referanse ble opprettet.
