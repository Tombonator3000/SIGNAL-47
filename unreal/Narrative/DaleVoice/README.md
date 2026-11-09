# Dales egen stemme

Tom bestemte 9. november ca. 13:45 at Dale får en egen, oppdiktet stemme i stedet for profilen `kinski` (VoiceStudio `3cf0d3a5`, en klone av den ekte Klaus Kinski fra et dokumentarklipp). `kinski` er merket `deprecated_for_new_lines` i begge `casting.json`-filene. De gamle takene står urørt, og profilen slettes ikke uten Tom.

Dale: sint tysker i 50-årene med tung aksent, brå, hes og utålmodig. Temperamentet er inspirasjon. Stemmen skal ikke ligne eller etterligne noen virkelig person (se `Docs/Active/DECISIONS.md`, Dale, 7. november).

## Status

Valgt 9. november: `dale_de_design` (VoiceStudio `e19d9024`), festet som `profiles.dale`. Alle 139 replikker er laget og består teknisk (`verification.json` PASS). ASR treffer 123 av 139 eksakt etter normalisering. 8 avvik er godtatt (stedsnavn, homofoner og tyske ord), og 8 er uløste i `asr-review.json`. Navnet Ward høres ofte som «what» eller «towards» fordi aksenten gjør W til V. Lytting, aksent, alder og rolletroverdighet er **UNVERIFIED**.

GPU-arbeid kjøres bare med `session.py`. Den tar motorreservasjonen, starter VoiceStudio, lager og kontrollerer takes, stopper serveren (ca. 5 GB VRAM) og slipper først da reservasjonen.

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
- `catalog.py` og `catalog.json`: banken `dale` for Freeform (id, tekst, kanal, fil, varighet, hash og talestart/-slutt fra `takes.json`). Bare Freeform-linjer med `status: voice` og en take som stemmer med `takes.json`; resten står i `validation.skipped` og vises som tekst. `--import-raw` kopierer takene til `Content/Audio/Raw/voice/dale/` og legger banken inn i `voices.json` uten å endre de andre bankene. `--self-test` prøver alt i en midlertidig mappe. `audio-raw.py` kjører importen selv etter at den har bygd `voices.json` på nytt.

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
python3 catalog.py --import-raw ../../Signal47/Content/Audio/Raw   # banken dale, så pakking
python3 catalog.py --voice dale_de_design --import-raw ../../Signal47/Content/Audio/Raw   # en kandidat, bare for å lytte
```

## Avspilling i spillet

Story: kveldspakken og HorrorStory spiller Dale gjennom `FS47RecordedDialogue`, som finner klippet på eksakt tekst i lydbanken og legger på telefon- eller walkiefilteret. De nye takene kan tas i bruk uten C++: bytt Dales klipp i bankene med samme id og tekst (`Signal47/Scripts/evening-voice-catalog.py`, `HorrorStory/voice/catalog.py`), og kjør `build_pack.py` for nye tider i undertekstene.

Freeform (DALE-WIRE, 9. november): koblet i C++ gjennom `FS47DaleVoice`, som slår opp i banken `dale` på manifestets id eller eksakt tekst (med eller uten `DALE:` foran) og spiller med `FS47RecordedDialogue` gjennom mottakerfilteret. Uten take gjelder dagens oppførsel uendret.

1. Walkien (`FS47FieldRadio::Message`): hele teksten slås opp før `FS47GearFlicker::Broken`, på kanalen `walkie`. Med take: hans stemme i stedet for den syntetiske prat-lyden, squelch inn og ut rundt ham, roger-pip etter siste take, og knitringen oppå når ord brytes. Dette dekker barks, walkie-svar, anrop, storenatta og regissørskript, siden alle går gjennom `Message`. Vrir spilleren kanalen, stopper stemmen. En rapport som venter, sendes ikke mens han fortsatt snakker.
2. Undertekst på walkien: kommer ved målt talestart og går ved målt taleslutt, på lydens egen klokke. Deler som bare er tekst (en rapport han gir videre), beholder lesetiden. Uten take gjelder dagens tid.
3. Tjenestelinja (`FS47Predecessor::ShowTalk("dale")`): hver Dale-replikk på siden spilles etter hverandre på kanalen `phone`. Stemmen stopper når siden lukkes, røret legges på eller neste side erstatter den. Panelet viser teksten som før med en gang (som transkripsjon).
4. Sammensatte meldinger: `finally` foran en bark blir to klipp, `relay_pre` + rapport + `relay_post` blir to klipp med rapporten som tekst imellom.
5. De 21 malene med sted eller retning (`status: template`) har ingen take og vises bare som tekst, med prat-lyden som før. Tom velger senere om de skal skrives om til faste setninger. Variant for variant blir for mange (lysrapportene alene er 4 x 5 innringere x 31 steder).
6. Karakterstemmer av i innstillingene, ingen lyd eller manglende fil gir samme fallback. Et klipp som spilles eller ikke spilles, setter ingen flagg: hva som er sagt, følger teksten som før.
7. Selvtest: `FS47DaleVoice::SelfTest` i `-S47SelfTest` (uten bank, falsk katalog med én id, `DALE:`-hode, rekkefølge i sammensatte meldinger og på telefonen, kanalskille).

Når takene finnes, må dette prøves i spillet: et bark etter svar på T, et ubesvart anrop og så `finally`, en rapport som gis videre, storenatta om morgenen, tjenestelinja med bytte av tema og lukking midt i en replikk, og at underteksten følger talen.
