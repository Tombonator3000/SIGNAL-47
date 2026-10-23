# Opptakskatalog for EveningShift

`evening-voice-catalog.py` lager en isolert `evening`-oppføring som senere kan flettes inn i `Content/Audio/Raw/voices.json`. Den lager ingen stemmer, konverterer ingen filer og endrer ingen eksisterende assets. Katalogen bruker `voice_profile` og eksakt engelsk undertekst slik `FS47RadioVoice("evening")` slår opp dialog.

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

Standardinnganger er kveldspakkens genererte `recording_manifest.json` og `subtitles.json`. Manifestet inneholder ikke selve replikkteksten, derfor er underteksteksporten også nødvendig. Andre plasseringer kan gis med `--manifest` og `--subtitles`.

Varigheten måles fra faktiske PCM-rammer; SHA-256 og rammetall følger hvert klipp. Kontrollen beviser filformat, varighet og kobling, mens riktig karakterstemme, uttale, ren start/slutt og underteksttiming må gjennomgås ved lytting. Katalogen merker ikke opptak som ferdig godkjent.

Når lydleveransen skal integreres, må integratoren separat kopiere originalfilene uendret til `Content/Audio/Raw/voice/evening/` og flette JSON-oppføringens `evening`-nøkkel inn i eksisterende `voices.json`. Gjør dette etter eventuell kjøring av `audio-raw.py`, som skriver bankindeksen på nytt. Denne hjelpen utfører ikke kopiering, fletting, cook eller pakking. Ingen nye opptak følger kodeleveransen.
