# Dales egen stemme

Tom bestemte 9. november ca. 13:45 at Dale får en egen, oppdiktet stemme i stedet for profilen `kinski` (VoiceStudio `3cf0d3a5`, en klone av den ekte Klaus Kinski fra et dokumentarklipp). `kinski` er merket `deprecated_for_new_lines` i begge `casting.json`-filene. De gamle takene står urørt, og profilen slettes ikke uten Tom.

Dale: sint tysker i 50-årene med tung aksent, brå, hes og utålmodig. Temperamentet er inspirasjon. Stemmen skal ikke ligne eller etterligne noen virkelig person (se `Docs/Active/DECISIONS.md`, Dale, 7. november).

## Status

Forberedt, ikke generert. Ingen profil er laget i VoiceStudio og ingen take finnes. Alt som bruker GPU, venter til Tom ikke spiller. Lytting, aksent, alder og rolletroverdighet er **UNVERIFIED**.

## Kandidater

Står i `../EveningShift/voice/casting.json` under `profiles.dale.candidates`, med kilde og lisens i `provenance.json`.

| Kandidat | Grunnlag | Lisens |
|---|---|---|
| `dale_de_design` (anbefalt) | Ren stemmedesign: mann, middelaldrende, lav stemme. Identitetsprøven leses på tysk (egen tekst), og engelsk klones fra den, så aksenten blir tysk uten noe ekte opptak. | Syntetisk, OmniVoice-vektene er CC-BY-NC |
| `dale_donor_de` | CC0-donor fra Kyutai voice-donations: `english_with_german_accent.wav`, anonym frivillig, ikke kjent person og ikke film eller tv. | Referansen CC0-1.0 |
| `dale_ru_design` | Ren stemmedesign med `russian accent`, den nærmeste aksent-taggen. OmniVoice har ingen tysk tag. | Syntetisk |

OmniVoice tar bare faste egenskaper (kjønn, alder, tonehøyde, hvisking og ti aksenter). Hes stemme kan ikke bestilles. Temperamentet kommer fra teksten.

## Filer

- `build_manifest.py` og `manifest.json`: alle Dale-replikker i spillet, hentet fra kildene med fil:linje og kanal. Kjør `--check` etter tekstendringer.
- `create_profiles.py`: lager kandidatene i VoiceStudio (GPU), og `--pin NAVN` fyller inn Toms valg i `profiles.dale`.
- `generate.py` og `verify.py`: kveldspakkens løype (OmniVoice, engelsk, seed 42 og `retakes.json`, -20 dBFS aktiv RMS, topp høyst -2 dBFS, trimming, 48 kHz PCM16 mono, ASR) over manifestet. De skriver til `~/.cache/signal47-dale-voice/<stemme>/`, aldri til Kinski-takene.
- `speech-overrides.json`: uttalestaving overført fra kveldspakken (navnet Reyes).

Alle GPU-steg nekter å starte (`ReservationBusy`, kode 75) mens `Signal47` eller en Unreal-editor kjører. Kjør dem gjennom `~/.claude/skills/signal47-nattskift/scripts/q.py`, som venter på tur.

```sh
python3 build_manifest.py --check
q.py python3 create_profiles.py
q.py python3 generate.py --voice dale_de_design --only D01_HANDOVER_001 FF_BARK_hello FF_PHONE_wagon_1 FF_PHONE_eat_2
python3 create_profiles.py --pin dale_de_design      # etter at Tom har valgt
q.py python3 generate.py                             # alle 139 med profilen dale
q.py python3 verify.py
q.py python3 generate.py --until-asr ID...
python3 generate.py --sync ../../Signal47/Content/Audio/DaleVoice
```

## Avspilling i spillet

Story: kveldspakken og HorrorStory spiller Dale gjennom `FS47RecordedDialogue`, som finner klippet på eksakt tekst i lydbanken og legger på telefon- eller walkiefilteret. De nye takene kan tas i bruk uten C++: bytt Dales klipp i bankene med samme id og tekst (`Signal47/Scripts/evening-voice-catalog.py`, `HorrorStory/voice/catalog.py`), og kjør `build_pack.py` for nye tider i undertekstene.

Freeform: Dale har ingen stemme i dag. Walkien (`FS47FieldRadio::Message`) spiller en syntetisk prat-lyd (`FS47EncounterSound::Message`) og roger-pip, og viser teksten. Tjenestelinja (`FS47Predecessor::ShowTalk("dale")`) er bare tekst. Det som trengs:

1. En egen bank `dale` i `Content/Audio/Raw/voices.json` (id, tekst, fil, varighet, hash), laget fra `takes.json` og `manifest.json` med en katalog som i `HorrorStory/voice/catalog.py`.
2. C++ i walkien: når teksten starter med `DALE: `, slå opp den hele teksten (før `FS47GearFlicker::Broken`) i banken og spill den med `FS47RecordedDialogue` på kanalen `walkie` i stedet for prat-lyden. Knitringen ligger oppå når ord brytes. Dette dekker barks, walkie-linjer, storenatta og regissørskript.
3. C++ i tjenestelinja: spill klippet for hver Dale-replikk som vises, på kanalen `phone`, og stopp når røret legges på.
4. Sammensatte meldinger (`finally` foran en bark, `relay_pre` + rapport + `relay_post`) spilles som flere klipp etter hverandre.
5. De 21 malene med sted eller retning (`status: template`) har ingen take. Tom velger: skriv dem om til faste setninger med detaljen bare i teksten, eller vis dem uten stemme. Variant for variant blir for mange (lysrapportene alene er 4 x 5 innringere x 31 steder).
6. Mangler et klipp, vises teksten som før. Et klipp som ikke ble spilt, skal aldri sette flagget for at replikken er hørt.
