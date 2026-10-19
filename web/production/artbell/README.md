# Art Bell: syntetisk radiovert

Opptakene følger bare spillets eksisterende vert på 1420 Caliche / Night Line, inkludert guttens anrop, avslutningen og kjøreprogrammene. Callere, 1230 Roswell, nyheter, CB og Earl som fysisk samtaleperson er utelatt. Stemmen merkes som syntetisk parodi.

Korpuset har 115 unike tekster og 668 ord: 38 faste replikker, åtte ordrette fragmenter fra seks klokkemaler og 69 hele tidsfraser. Tidsbanken dekker den opprinnelige klokkeformatteringen. Katalogen inneholder `templates` og `timeBank` sammen med de normaliserte tekstene, SHA-256-ID, varighet og filnavn. Ingen nye replikker er skrevet.

Referansen er 4,40 sekunder fra Art Bells personlige CD-hilsen, `ArtFareWell.mp3`, bevart i *Artbell.com Commemorative Disc*, Rowland Network Communications, januar 2003. Kilde: https://archive.org/details/Artbell.com_Commemorative_Disc. Intervallet 29,0 til 33,4 sekunder er én fullstendig frase, kontrollert med eksisterende faster-whisper-small mot lengre kontekst. Original og referanse ligger utenfor repoet og følger ikke spillet. Offentlig tilgang til arkivkopien er ingen påstått tillatelse eller støtte fra Art Bell eller rettighetshaverne.

Lokal VoiceStudio 0.5.6 og den allerede installerte OmniVoice-modellen brukes. Profilen er `494881fc`, merket syntetisk radio-parodi. Ingen samtykkeopptak eller påstand om verifisert egen stemme er opprettet. VoiceStudio er AGPL-3.0-only; OmniVoice-koden er Apache-2.0, mens det nedlastede modellkortet oppgir CC-BY-NC for vektene. Se `provenance.json` for festet revisjon og kildebevis.

Generering skjer sekvensielt med engelsk, 32 steg, seed 42 og hastighet 1. `generate.py` gjenbruker cache utenfor repoet etter profil-, referanse-, tekst- og innstillingshash. Bare tidsfrasenes synteseforepørsler får stor forbokstav og punktum for stabil taleenergi; runtime-tekst og ID er uendret. MP3-filene er mono, 24 kHz og 64 kbit/s, med én konstant nivåjustering per fil og maksimum −2 dBFS etter dekoding.

Kjør `node tools/artbell/extract.mjs`, deretter `python3 tools/artbell/generate.py`. `--limit N` begrenser antall nye opptak. `python3 tools/artbell/verify.py --asr 3` kontrollerer alle filer og en liten spredt tekstprøve. ASR kjøres etter TTS; GPU-nettlesertester venter til begge er ferdige.

Klang, stemmelikhet, innlevelse og hørbar sømløshet mellom tidsbiter er UNVERIFIED fordi menneskelig lytting ikke er gjennomført. Demoens ASR skriver Caliche som Kalish; stedsnavnets uttale må vurderes ved lytting. Automatisk transkripsjon er en ufullkommen ordkontroll.

Sluttkontroll: PASS for alle 115 MP3-filer, 2 065 028 byte og 247,98 sekunder (4 minutter og 8 sekunder). Alle har `dropped_chunks=0`, positiv varighet og taleenergi. Ingen filer mangler eller er foreldreløse, og ingen dekodede samples klipper. Katalogens tidsmaler og tidsbank stemmer med det endelige korpuset.

Tre spredte ASR-prøver er bevart. To lange replikker gjengis ordrett; åpningen har bare avvik rundt tall, Night Line og Caliche. To sammensatte tidslinjer bevarer klokkeverdiene tre oh-seven og twelve o'clock. Rapportene er `verification.json`, `demo.json` og `clock-asr.json`. Ingen ASR-avvik er brukt som bevis på stemmelikhet.

Alle 1555 tidligere Kinski-MP3-filer er faktisk SHA-256-sammenlignet med Gitbase `aa38e2b`, katalogen er identisk, og de fem eksisterende Kinski-profilene er uendret. Filbevis ligger i `kinski-preservation.json`.
