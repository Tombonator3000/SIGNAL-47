# Kveldspakkens stemmer i runtime

Codex, 23. oktober 2026, `codex/evening-runtime` fra `a4626053`. Claude har allerede integrert og installert den tidligere åpningen. Denne leveransen kobler de 109 eksisterende takesene til runtime; Codex endrer ikke den installerte utgaven. Kildeendringer: `b7faff35`, `6ade7a94`, `3345f01f` og `9d638a0b`.

De 109 rene originalfilene dekker 110 cues. `D61_FIXED_FRAGMENT_001` bruker nøyaktig samme fil som `D01_HANDOVER_001`. Rolle og eksakt tekst inngår i oppslaget; Ward, Nora og andre karakterer får aldri Dales stemme. Senere Dale-scener bruker samme adapter når verdensportene deres faktisk åpnes. Dette implementerer ikke kontoret, trafostasjonen eller VHS-en.

| Tale | Runtime-rute |
|---|---|
| Lokal kveldvert | Egen mottakerkø, 320–3200 Hz receiverfilter, RadioBus og bilens av/på. Eksisterende countrymusikk i pausene. |
| Dale | Eksisterende telefon-/walkie-/radiofilter. Telefon/walkie følger SfxBus; radio følger RadioBus. Ingen fysisk Dale. |
| Servitrisen | Panner ved den eksisterende `diner:waitress`-figurens verdensposisjon, 1,8 m referanseavstand og avstandsfall, videre til SfxBus. |

De leverte opptakene får kanalfilteret i spillet. Ingen ny konvertering, ny stemmegenerering eller innbakt telefonlyd brukes. For tilgjengelige takes er pending-tekst og tekniske «recorded dialogue»-etiketter fjernet. Manglende filer og avslåtte karakterstemmer har fortsatt ærlig undertekstfallback.

Den kompilerte pakken leser faktisk klipplengde og talestart/-slutt fra `voice/takes.json`. Radioens undertekst starter ved målt talestart og utløper ved målt taleslutt, på lydkildens planlagte klokke. En manuell dinersamtale/telefon viser replikkteksten ved målt talestart og beholder den som lesbar transkripsjon til Continue. Continue kan avbryte talen. Hele samtalepanelet er unntatt den generelle beskrivelsesoppleseren, slik at ordre-/valgtekster ikke gir en misvisende «No recording»-boks eller Kinski-opplesning ved siden av karakterstemmen. Ingen automatisk hastelesing er lagt til i samtalemenyene.

Global lydpause fryser kildens lydklokke og avspilling. Avsluttet samtalepanel stopper køen. Bilradio av, utstigning og modaler stopper gjeldende host-take uten å flytte manuspekeren; retur starter samme take fra begynnelsen. Ved lasting startes gjeldende replikk fra begynnelsen, med samme valg, kjøp og scenepeker. Lagringsformatet endres ikke, og gamle saker beholder sin tidslinje/stempler.

## Reproduser importen

Fra `unreal/Signal47`, med godkjente originaler i ignorert `Content/Audio/EveningShift`:

```sh
python3 Scripts/test-evening-audio.py
python3 ../Narrative/EveningShift/build_pack.py --check
S47_AUDIO_EVENING_ONLY=1 Scripts/import-audio.sh
```

Kun råfilene som runtime bruker, uten editor:

```sh
python3 Scripts/audio-raw.py --evening-only --evening-wav-dir "$HOME/SIGNAL-47/unreal/Signal47/Content/Audio/EveningShift"
```

Full import gir 109 SoundWave-assets i `/Game/Audio/Voice/Evening`, byteidentiske WAV-er i `Audio/Raw/voice/evening` og streng `evening`-bank i `Audio/Raw/voices.json`. Andre bankoppføringer bevares. Eksisterende packagingregel stager allerede `Audio/Raw` som UFS; ingen ny løsfildistribusjon kreves. All generert lyd og Content forblir utenfor Git. Detaljer og katalogkommando: [kataloghjelpen](Signal47/Scripts/evening-voice-catalog.md).

## Kontrollgrunnlag

Evidens, isolerte profiler, kandidat og logger: `/home/tom/scratch-bm/Signal47-evening-audio/`. Én tung Unreal-prosess om gangen, minst 10 GiB ledig før start; ingen eksisterende prosess stoppes. Midlertidige Content-lenker brukes til hovedkopiens assets. Biblioteksøk ga ingen relevant kompatibel importløsning; Audio2Face-sporet gjaldt ansiktsrigg og var uprøvd. Prosjektets eksisterende lydløype gjenbrukes.

`-S47EveningAudio -S47EveningRoute=burger` er den utvidede rutekontrollen. Den lar hele takes spille, krever faktisk dekodet varighet og positive samples/RMS/peak gjennom korrekt kanal/filter, prøver radioavbrudd, fryser lydkonteksten 0,5 s og lagrer/laster mens D01 spiller. De vanlige raske rutekontrollene er beholdt. `-S47VoiceLevels` kan brukes separat for nivålogg på opptakskøene. Nivåene er målt på samples som faktisk trekkes av destination-grafen, etter filter/panner og før delt buss; bussgain logges separat. Dette er avspillingsbevis, ikke menneskelig lytting eller opptak av høyttalerutgangen.

## Resultater på sluttkilden

Runtimekilden er `9d638a0b`. Kandidaten ligger i `/home/tom/scratch-bm/Signal47-evening-audio/candidate`; ELF og fem containerfiler er hashfestet i `candidate-manifest.json`. Sluttcook tok 84,12 s og avviste materialfallback. Det er en lokal testkandidat; installert utgave er ikke byttet.

| Kontroll | Resultat |
|---|---|
| Streng katalog og import | PASS, 109 original-WAV-er, 110 cues med alias; alle SHA-256 og målte tider stemmer. 109 SoundWave-assets importert. Sju eldre banker semantisk uendret. |
| Offline importkontrakter | PASS, seks tester for endret hash, feil/manglende/ekstra take, alias, foreldet teksttiming og bevaring av andre banker/PCM. |
| Pack og eksport | PASS, `build_pack.py --check` byteidentisk; alle 110 voiced cues har målte speech-vinduer. |
| Editorbygg og selvtest | PASS, editor ved b7faff35 bygget på 25,30 s og bestod 1258. Sluttkilden 9d638a0b er bygget i Linux-pakken og består også 1258. |
| Full sluttkandidat, burgerrute med lyd | PASS COMPLETE, 281,3 s aktiv kontroll / 323,20 s prosessveggtid; 2651,5 m kjørt. Fysisk kjøre-/gåkontroll og API-dialog, radioavbrudd, 0,5 s frosset lydklokke, betalt svar og D01 lagret og lastet fra disk/full verden. Tre dokumenter én gang, ingen ny 18:00-reset. |

Kortregresjonen er **PASS 6/6**, 239.66 s samlet prosessveggtid: D01-transkripsjon, gammel kontortelefon, gammel og ny notatbok ved residualen, gammel Caliche-radio og ny finale. Alle har normal exit, forventet logg, smoke PASS og gyldig bilde, uten fatal/assert-feil eller annen tung Unreal-jobb. `regression-results.json` og `delivery-audit.json` inneholder resultatene.

Den fulle lydloggen har 36 ulike fullførte klipp: ti vertsreplikker, tretten servitrisereplikker og tretten Dale-replikker. Alle hadde positivt PCM-/source-/routed-nivå og positiv bussgain. Disse er et runtimeutvalg; alle 109 er fil-/katalogkontrollert.

| Rute | Fullførte ulike klipp | Målt RMS etter filter/panner, før buss |
|---|---|---|
| car-radio, receiverfilter på | 10 | 0,064553–0,105048 |
| physical, posisjonell, uten receiverfilter | 13 | 0,035723–0,041442 |
| phone, receiverfilter på | 13 | 0,083285–0,111757 |

Logg: `packed-audio-release.log`. Begge ASR-merkede takes finnes blant fullførte telefonklipp. Disse nivåene beviser faktiske samples i spillets graf, ikke opplevd volum eller uttale.

Fem rutebilder er visuelt gjennomgått: [innkjøring](/home/tom/scratch-bm/Signal47-evening-audio/release-shots/burger-commute.png), [betalt bestilling](/home/tom/scratch-bm/Signal47-evening-audio/release-shots/burger-diner-order.png), [parkering](/home/tom/scratch-bm/Signal47-evening-audio/release-shots/burger-parking.png), [18:00-telefon](/home/tom/scratch-bm/Signal47-evening-audio/release-shots/burger-18-phone.png) og [skumring](/home/tom/scratch-bm/Signal47-evening-audio/release-shots/burger-dusk.png). Telefonbildet fanges før målt talestart ved 0,08 s og har derfor tomt tekstfelt; [egen telefontranskripsjon](/home/tom/scratch-bm/Signal47-evening-audio/packed-phone-transcript.png) viser replikken etterpå, som uttrykkelig scenariooppsett. Skumringen har rutens fremoveroppsett til 18:35, ikke målt naturlig venting. Ingen av bildene er menneskelig input- eller lyttebevis.

## Rettelser funnet i kontrollen

Første komplette lydrute feilet etter lagring ved betaling: en ekstra åpning av samme dialogpanel lukket det gamle panelet, og lukkecallbacken stoppet den nye stemmen. `3345f01f` gjør gjentatt åpning idempotent. Hele ruten bestod deretter med faktisk lyd etter begge disk-/verdenslastingene. `9d638a0b` fjerner den separate beskrivelsesopplesningen av ordre-/valgtekster. Den endelige kandidaten har disse rettelsene; første feillogg er bevart som `packed-audio-burger.log`, og brukes ikke som PASS-bevis.

## Toms lytting

Begge merkede takes brukes som levert:

- [D01_FOOD_002.wav](/home/tom/.cache/signal47-eveningshift/takes/D01_FOOD_002.wav): innledende «I» i «I tried that once» kan mangle.
- [D01_BRIEF_005.wav](/home/tom/.cache/signal47-eveningshift/takes/D01_BRIEF_005.wav): «observations» har uløst ASR-avvik.

Stemmeinnlevelse, stemmelikhet, uttale og hørbare artefakter er fortsatt UNVERIFIED ved menneskelig lytting. Se [produksjonsgrunnlaget](Narrative/EveningShift/voice/README.md) for casting, produksjon, lisens og de to merknadene. Ingen takes er erstattet eller merket menneskegodkjent i denne integrasjonen.

## Overlevering

Kilde og importkode er committet på `codex/evening-runtime`; `unreal-port-core` ved a4626053 er kontrollert med ny merge: Already up to date. Props/config, bensinstasjonen og tidslinjelogikken er urørt. Alle midlertidige Content-lenker er fjernet, originalt sporet Kelvora er satt tilbake og egne OpenOrder-endringer er ryddet. Hovedkopiens ignorerte Audio beholder den validerte råbanken og de 109 importerte SoundWave-assetene for Claudes neste pakking. Ingen generert lyd følger commitene. Ingen push, PR, installasjon eller endring av Toms saves.
