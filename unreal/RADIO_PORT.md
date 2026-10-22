# Radio og musikk, 22. oktober 2026

Kildeleveranse på `port-radio`, fra `890fd5c1`. Ingen push, PR eller pakket utgave er laget. Nettversjonen, Unity og de genererte modellene er ikke endret.

## Porterte moduler

| Nettkilde | C++ | Innhold |
| --- | --- | --- |
| `core/radio.ts` | `S47Radio.h/.cpp` | Caliche 1420, Roswell 1230, far 1090, programmer, faste sendetider, guttens samtale, mottakere, AM-graf, replikker, avbrudd og diagnostikken `State()` |
| `core/radioVoice.ts` | `S47RadioVoice.h/.cpp` | Vert, innringere, Earl og ensemble, nøyaktige tekster og tidsmaler, begrenset LRU |
| `story/tripRadio.ts` | `S47TripRadio.h/.cpp` | 23 merker, 51 replikker, 29 ID-er, avstandsvinduer og tilbakelevering av replikker som ikke begynte |
| `core/score.ts` | `S47Score.h/.cpp` | 17 spor, sju cues, valg, pauser, fading og `MusicWant()` / `MusicHeld()` |
| HTML-mediekilder | `S47Media.h/.cpp` | Lange opptak gjennom den samme lydkonteksten og filtrene |

`S47Main.*` lager radioene og musikken, oppdaterer dem i nettversjonens rekkefølge, setter posisjon, strøm, demping og dashbord, tekster hørbare replikker og registrerer gutten i notatboken. `S47World.*` gir turens tilstand og spørsmålene musikkvalget trenger. Eksisterende `OnRadio` kobler R og bruk av radioflaten til samme stasjonsskifte, med de engelske ledetekstene fra main.ts.

`S47Flow.cpp` setter volum og stemmevalg, pauser lyden og musikken, nullstiller radioen ved ny natt og gjenoppretting, og skriver `TownSeen` til `town` i lagringen. Det bevarer blant annet `ev:boy`, `ln:reel` og `tr:<id>`. Områdebytte ved gjenoppretting går gjennom World slik at musikkvalget ser samme område som verdenen. Tittellåten går som i nettversjonen via `audio.loop('music', 'titleMusic')` og musikkbussen, utenom Score. Prologens bordradio beholder sin egen `amRadio`, inklusive forstyrrelsen under hendelsen.

`S47UI.*` og `SS47Widgets.*` har fått det valgfrie klokkefunksjonen til `toast(text, secs, clock)`. Radiotekst følger lydkontekstens klokke også under pause. `S47WebAudio.*` har selektiv frakobling slik at én mottaker kan slippe en stasjon uten å koble fra den andre, og en kø for mediekildenes callbacks til spilltråden. HUD kaller modulenes selvtester med én ekstra linje.

## Opptak og nødvendige native forskjeller

De komprimerte musikkfilene er omtrent 37 MiB og radiofilene 4,4 MiB. Musikken alene ville brukt omtrent 1393,5 MiB som dekodet float. `audio-raw.py` skriver derfor søkbar PCM16 WAV under `Content/Audio/Raw/media/{music,radio}`. Mediekilden leser biter på 32768 rammer, beholder inntil fire biter og leser videre på en bakgrunnstråd. Første biter ved åpning eller søk lastes på spilltråden uten kontekstlåsen. Lydtråden gjør ingen filinnlesing. Kilden mater vanlige noder i FS47AudioContext, ikke en separat Unreal-lydkomponent. Samplingfrekvens og kanaler beholdes; PCM16 gir en kvantisering som nettleserens float-dekoder ikke har.

Stemmebankenes korte, valgte klipp dekodes synkront fra lokale WAV-filer, uten å starte lyd ved innlasting. Det erstatter nettleserens asynkrone fetch/decode. Rolle, ord, fragmentrekkefølge, avbrudd og grensene på 24 buffere / 24 MiB beholdes. Et stoppet musikkopptak slipper filkilden i stedet for å beholde et tomt HTML-element; neste kilde søker til samme sted som originalen.

Importen er kjørt mot hovedkopien mens ingen faktisk UnrealEditor eller UnrealEditor-Cmd kjørte. Den la til 17 musikkopptak, fem radioopptak og 259 stemmeklipp, og bevarte eksisterende sfx-, telefon- og kjøpt-lydindeks. Media bruker omtrent 735 MiB på disk. Eksisterende UFS-staging av `Audio/Raw` inkluderer alt dette. Binærfiler og opptak er ikke del av commiten.

For å gjenta bare denne importen, fra repoets rot og uten en editor som bruker hovedkopien:

```sh
python3 unreal/Signal47/Scripts/audio-raw.py \
  --web "$HOME/SIGNAL-47/web/src" \
  --content "$HOME/SIGNAL-47/unreal/Signal47/Content" \
  --recordings-only
```

Full `Scripts/import-audio.sh` inkluderer nå disse opptakene og avviser kjøring mens en editor er aktiv. `S47_AUDIO_RAW_ONLY=1` hopper over Unreal-assetimporten etter at råopptakene er laget.

## Verifikasjon

- PASS: `Scripts/build-editor.sh`, UE 5.8.4, uten feil eller advarsler.
- PASS: 520 selvtestsjekker i editorens `-game -RenderOffscreen`, med de importerte opptakene og den eksporterte verdenen. Radiotestene dekker blant annet 04:00/05:00, dagslys, køer, avbrudd på 47 sekunder, sampleposisjon ved gjenopptakelse, stemmebytte, opptak og lydklokke under pause. Musikkprøvene kontrollerer seks sekunders innfading, pause ved samme sample og at gammel fading ikke stopper et nytt spor. Mediekildens egne prøver dekker bitgrenser, resampling og callbacks på spilltråden.
- PASS: tre røyktester med Vulkan, 1785 deler og skjermbilder 1280 x 720: Caliche AM 1420, Roswell AM 1230 og Far AM 1090. Logglinjene bekrefter live mottaker og én lytter på hver stasjon. Skjermbildene av verdenen og pausemenyen er visuelt kontrollert.
- PASS: Roswell og Far gjennom SDL3 sin disk-driver, 48 kHz, seks float-kanaler. Roswell-fangsten har 13,035 sekunder, alle prøver endelige, topp 0,20946. Far med pause har 10,859 sekunder, topp 0,25454 og bare nullprøver i de siste to sekundene etter at pausemenyen åpnet. Dette måler faktisk miksutgang, ikke menneskelig vurdering av lyden.
- PASS: statisk sammenligning av radioprogrammenes tekster, rekkefølge og tidsuttrykk mot TypeScript; tilsvarende kontroll av alle musikkspor, cues og turprogrammet.
- PASS: isolert importprøve, faktisk PCM-format i alle opptak, Python-/Bash-syntaks og `git diff --check`.

Lokale kjørebevis ligger i `/tmp/s47-radio-evidence/`: `final-caliche.log`, `roswell.log`, `far-pause.log`, skjermbilder og rå lydfangster. Bygglogg i `/tmp/s47-radio-build3.log`. Editorens kjente Python-feil fra eksperimentelle Toolsets ved oppstart er separate fra spillmodulens tester.

## Integrasjon og åpne prøver

1. Koble de andre nattsystemenes faktiske tilstand til de navngitte funksjonene i `Score->Held`. Feltene følger hele `musicHeld()` fra main.ts: roadside, motelTv, abduction, recWalk, faceQuiet, ninth, roofSteps, reelTape, saroDark, officePhone, archive, arrayBack, truckB og morning. Radioen, prologens AM-radio, cinematic, fase og nattas pust leses allerede direkte. Ingen natthendelser, kurve eller album er portert her.
2. Bruk samme `M->TownSeen` i øvrige nattsystemer. De nye radiofunnene deler nettversjonens `town`-felt; ikke erstatt settet med en egen radiolagring ved sammenslåing.
3. Bevar de små innkoblingene i Main, Flow, HUD og World når parallelle grener flettes. `TripNow()` holder turens eget nummer, mål og tilbakelagte meter; bruk den i stedet for å anslå fra bilens posisjon.
4. Bygg og pakk den samlede kandidaten, og kjør hele `Scripts/screenshots.sh OUT BUILD`. Skriptet har fått en egen `radio`-kjøring. Pakket bygg og hele skjermbildesuiten er UNVERIFIED her fordi arbeidskopien ikke har en egen pakket utgave.
5. Menneskelig lytting til miks, ekte R-/museinput, pausemenyen og lange kjøreturer er UNVERIFIED. Editorens tester og offline-gjengivelse er ikke et slikt bevis.

Alle brukte scenehåndtak finnes i gjeldende manifest, blant annet `world.diner.proxies.counter` og bilenes allerede porterte håndtak. Eksportøren er ikke endret. Prosjektbiblioteket ble søkt; treffene gjaldt andre Unreal-verktøy og ga ingen kompatibel radio-/strømmeløsning som ble brukt.
