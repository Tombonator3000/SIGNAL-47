# SC01: fem produserte stemmetakes

Denne leveransen dekker bare H01_BRIEF_001/002/003 og SC01_FIRST_REPORT_001/002. Alle fem er faktiske syntetiske opptak fra eksisterende lokal VoiceStudio0.5.6/OmniVoice. Andre HorrorStory-cues er fortsatt uprodusert. Ingen ny motor, modellnedlasting eller betalt tjeneste er brukt.

Dale bruker samme festede Kinski-profil som kveldspakken. DISPATCH får egen caliche_dispatch-identitet gjennom den eksisterende anonyme CC0-innringerprofilen1a160d70 fra Butter.wav. Ingen navngitt NPC omcastes. Referansehasher og kildegrunnlag står i casting.json/provenance.json.

## Produksjon og filer

`generate.py` gjenbruker kveldspakkens eksisterende lokale genererings- og etterbehandlingskode. Det velger eksplisitt de fem SC01-cue-ID-ene og bevarer kildeteksten. Rå opptak, API-svar og ferdige WAV-filer ligger under `~/.cache/signal47-horror/sc01`, utenfor Git.

Rene masters er WAV48kHz PCM16 mono, med konstant nivåtilpasning mot minus20dBFS aktiv RMS, topp høyst minus2dBFS, målt talestart/-slutt og korte inn-/uttoninger. Telefonfilteret legges på i spillet. Ingen tidsstrekking eller tonehøydeendring er brukt.

- `generation.json`: eksakte forespørsler, svar, referanse-/lydhasher og målte nivåer.
- `retakes.json`: seedhistorikk og årsak til nye forsøk.
- `takes.json`: delvis HorrorStory-timingtabell for de fem faktiske SC01-opptakene.
- `verification.json`: PCM-/hash-/kant-/nivåprøver og ASR per cue.
- `catalog.json`: målt horror-bank for runtime.
- `catalog.py`: etterkontroll og atomisk import som bevarer alle øvrige stemmebanker.

```sh
python3 unreal/Narrative/HorrorStory/voice/generate.py --limit 5
python3 unreal/Narrative/HorrorStory/voice/verify.py
python3 unreal/Narrative/HorrorStory/voice/generate.py --sync unreal/Signal47/Content/Audio/HorrorStory
python3 unreal/Narrative/HorrorStory/voice/catalog.py --import-raw unreal/Signal47/Content/Audio/Raw
python3 unreal/Narrative/HorrorStory/voice/catalog.py --check
```

Syntese og ASR deler GPU med Unreal og kjøres sekvensielt. Tjenesten som ble startet for denne leveransen er stoppet etter produksjon. Alle audiofiler under Content/Audio er ignorert av Git, som i eksisterende lydløype. Kun kode, kildegrunnlag og målemetadata versjoneres.

## Faktisk kontrollstatus

Fem av fem opptak består teknisk PCM-, fullstendighets-, SHA256-, nivå-, kant- og taletidskontroll. Samlet lengde er 35.08 s. Den 29. oktober ble bare SC01_FIRST_REPORT_001/002 produsert på nytt for det gjeldende manuskriptet om slag mot glasset. De tre Dale-masterne og deres genereringsmetadata er byteidentiske med den tidligere leveransen. Andre RAW-banker og alle tidligere råopptak er bevart.

Alle fem har nå ASR-ordtekst uten avvik, og både samlet `status` og `technical_status` er PASS i verification.json. Første nye dispatch-take krevde seeds46 til49: de første tre ga «Sorrow», mens49 ga «Saro», med korrekt normalisert ordtekst og uendret spilltekst. Andre take består med seed44. De eldre masterne, avviste forsøkene og metadataene er sikret i den lokale nattkvitteringen. Ingen ASR-tekst erstatter manuskriptet.

Den isolerte VoiceStudio-tjenesten brukte eksisterende festede modeller og en privat kopi av databasen og bare de to nødvendige profilreferansene. Original database og referansefiler ble ikke skrevet. Etter RAW-import var øvrige banker, de tre Dale-masterne, referansene og alle gamle råfiler hashkontrollert uendret. Tjenesten og egne etterkommere er avsluttet, uten lytter på port3900. Målrettet faktisk runtimekontroll av begge nye takes følger separat.

Menneskelig lytting, stemmelikhet og rolleprestasjon er UNVERIFIED. Manglende tale, mutet avspilling eller skippet linje må aldri gi verdensflagget som sier at en tidligere Dale-replikk faktisk er hørt. Denne lydleveransen alene verifiserer ikke runtime-avspilling eller slike hørthetsflagg.

Kilde- og lisensgrunnlaget følger den eksisterende prosjektløypa og er gjengitt i provenance.json, inkludert modellrevisjon og CC0-referansens festede opphav. Kildeopptak, profilreferanser og modellvekter følger ikke spillpakken.
