# Opptakskatalog for EveningShift

`evening-voice-catalog.py` lager en isolert `evening`-oppføring for `Content/Audio/Raw/voices.json`. Denne kommandoen lager ingen stemmer, konverterer ingen filer og endrer ingen eksisterende assets. Katalogen bruker `voice_profile` og eksakt engelsk undertekst slik `FS47RadioVoice("evening")` slår opp dialog. Importløypa nedenfor bruker samme validering.

Legg de 109 godkjente originalopptakene i én mappe, med filnavn `<line_id>.wav`. Kravet er ukomprimert PCM16, 48 kHz og mono. `D61_FIXED_FRAGMENT_001` skal ikke ha egen fil: katalogen peker til `D01_HANDOVER_001.wav`. Manglende eller uventede WAV-filer, feil format, avkuttet innhold, identitetskonflikter og feil alias stopper eksporten.

Fra `unreal/Signal47`:

```sh
python3 Scripts/evening-voice-catalog.py \
  --wav-dir /sti/til/godkjente-opptak \
  --output /tmp/s47-evening-voices.entry.json

python3 Scripts/evening-voice-catalog.py \
  --wav-dir /sti/til/godkjente-opptak \
  --output /tmp/s47-evening-voices.entry.json --check
```

Standardinnganger er kveldspakkens genererte `recording_manifest.json` og `subtitles.json`, samt `voice/takes.json`. Manifestet inneholder ikke selve replikkteksten, derfor er underteksteksporten også nødvendig. Andre plasseringer kan gis med `--manifest`, `--subtitles` og `--takes`. `audio_file` kan være `<LINE_ID>.wav` eller `EveningShift/<LINE_ID>.wav`; originalmappen inneholder bare de 109 WAV-filene.

Varigheten måles fra faktiske PCM-rammer; SHA-256 og rammetall følger hvert klipp. Hash og varighet må stemme med `takes.json`; målt talestart og taleslutt må også stemme med underteksteksporten, inkludert aliaset. Disse tidene følger katalogen. Kontrollen beviser filformat, varighet og kobling, mens riktig karakterstemme, uttale, ren start/slutt og opplevd underteksttiming må gjennomgås ved lytting. Katalogen merker ikke opptak som ferdig godkjent.

Importer bare kveldspakken uten Unreal eller endringer av andre banker:

```sh
python3 Scripts/audio-raw.py --evening-only \
  --evening-wav-dir "$HOME/SIGNAL-47/unreal/Signal47/Content/Audio/EveningShift"
```

Importen validerer hele leveransen før skriving, kopierer originalfilene byteidentisk til `Content/Audio/Raw/voice/evening/` og publiserer deretter den nye `evening`-nøkkelen i `voices.json` med atomisk filbytte. Andre bankoppføringer og råfiler bevares. `D61_FIXED_FRAGMENT_001` peker til første D01-take; den importeres ikke som en ekstra WAV. Ingen telefon-/radiofilter bakes inn.

Den vanlige `audio-raw.py`-oppdateringen tar automatisk med pakken når `Content/Audio/EveningShift` finnes. `--recordings-only` bevarer også egne banker som ikke kommer fra web-katalogene. Full rålydimport uten `--recordings-only` bygger som før Raw på nytt. En ugyldig kveldspakke stopper før denne oppryddingen.

Samme native import med valgfri Unreal-assetimport:

```sh
S47_AUDIO_EVENING_ONLY=1 S47_AUDIO_RAW_ONLY=1 Scripts/import-audio.sh
# Uten RAW_ONLY importeres også 109 v_<LINE_ID>-assets til /Game/Audio/Voice/Evening.
S47_AUDIO_EVENING_ONLY=1 Scripts/import-audio.sh
```

`S47_EVENING_WAV_DIR` overstyrer originalmappen; standard er `Content/Audio/EveningShift`. `S47_AUDIO_VOICES=0` hopper over SoundWave-assetimport, men ikke råfilene runtime bruker. Assetimporten kontrollerer maskinens minne og andre tunge prosesser. Generert Content, staging og lyd er fortsatt ignorert og skal aldri committes.

Offline kontrakttester: `python3 Scripts/test-evening-audio.py`. De kontrollerer endrede hasher, foreldet underteksttiming, feil alias, ekstra/manglende take og at importen bevarer andre banker og original PCM.

De to uløste ASR-merknadene brukes som levert og skal lyttes på av Tom: `D01_FOOD_002` (innledende «I») og `D01_BRIEF_005` («observations»). Se [produksjonsrapporten](../../Narrative/EveningShift/voice/README.md).
