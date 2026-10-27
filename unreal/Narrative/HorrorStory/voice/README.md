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

Fem av fem opptak består teknisk PCM-, fullstendighets-, SHA256-, nivå-, kant- og taletidskontroll. Samlet lengde er35,07s. To tidlige innholdsavvik ble rettet med nye seeds: manglende And i H01_BRIEF_003 og secure i stedet for secured i SC01_FIRST_REPORT_002.

Fire av fem cues har eksakt ASR-ordtekst. SC01_FIRST_REPORT_001 transkriberes med Sorrow i stedet for stedsnavnet SARO; resten stemmer. Dette er samme navnevariant som er dokumentert i kveldspakkens ASR, men opptaket er ikke subjektivt lyttet på her. Avviket står eksplisitt uløst i verification.json; den samlede strenge ordkontrollen er derfor FAIL mens technical_status er PASS. Retakes med seeds42 til46 er dokumentert. Ingen ASR-tekst har erstattet spillteksten.

Menneskelig lytting, stemmelikhet og rolleprestasjon er UNVERIFIED. Manglende tale, mutet avspilling eller skippet linje må aldri gi verdensflagget som sier at en tidligere Dale-replikk faktisk er hørt. Denne lydleveransen alene verifiserer ikke runtime-avspilling eller slike hørthetsflagg.

Kilde- og lisensgrunnlaget følger den eksisterende prosjektløypa og er gjengitt i provenance.json, inkludert modellrevisjon og CC0-referansens festede opphav. Kildeopptak, profilreferanser og modellvekter følger ikke spillpakken.
