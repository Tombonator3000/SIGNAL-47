# Filmfiltre for SIGNAL / 47

Toms bestilling 23. oktober 2026: lag filmuttrykkene, forbedre eksisterende VHS og prioriter Carpenter-looken. Arbeidsgren `codex/film-looks`, startet fra `unreal-port-core` ved `ed061aca`. Fellesgrenens nye antenner og radiostasjon ved `04517414` er innflettet i `8ba39556`. Bare Unreal-utgaven endres.

## Uttrykk og bruk

Settings > Picture > Picture har fem valg. Picture strength blander effekten fra 0 til 100 prosent. Clean har ingen ekstra effekt. Menyer, undertekster og dokumentpaneler tegnes etter verdensfilteret og beholder lesbar tekst.

| Valg | Utforming |
|---|---|
| Carpenter 35 mm | Hoveduttrykket. Blåere nøytrale skygger, varme praktiske lyskilder, fint lysavhengig korn, stabilt bilde, svak optisk diffusjon og bredere rød/oransje glød rundt sterke lyskilder. |
| Desert 16 mm | Støvete og dempede farger, svakt olivengrønne skygger, varme høylys, tydeligere korn og svært svak registreringsuro. |
| Super 8 | Varm og mettet reversal-film, grovere korn, mørkere hjørner, svak registreringsuro og forsiktig eksponeringsvariasjon. |
| VHS tape | Separat båndbegrensning av luminans og farge i YIQ, horisontal fargeblødning, linjeavhengig jitter, svak støy/linjestruktur og liten forstyrrelse nederst i bildet. |
| Clean | Spillets eksisterende scene og ACES-kurve. |

Nye High/Ultra-profiler begynner med Carpenter. Low begynner med Clean. Et eksplisitt gammelt VHS/Clean-valg beholdes; `heavy` normaliseres til den forbedrede VHS-profilen. Low kan også bruke filmprofilene. Ingen profil endrer bildefrekvensen eller spillklokken.

Slow fades og Still shots at the end stopper registreringsuro, filmflimring og bevegelige båndforstyrrelser. Kornet fryses i samme modus. Styrke 0 beholder valgt profil, men bruker Clean-bildet.

Visuell referanse er Dean Cundeys beskrivelse av varme interiører, blå natt og avgrensede lyspøler i [Photographing The Thing](https://theasc.com/article/flashback-the-thing/). Dette er et eget stilisert spilluttrykk, ikke en målt emulering av en bestemt historisk filmstock eller linse. Fysiske lamper, strømtilstand og historiesekvenser er bevart.

## Integrasjon

- `Tools/webexport/film_tone.hlsl` er kilden til `M_WebTone`. Den eksisterende scenelesingen, faste eksponeringen og ACES-kurven er felles for alle fem valg.
- `PictureMode` i `MPC_Web` velger off=0, vhs=1, carpenter=2, 16mm=3 og super8=4. `PictureStrength` og `PictureCalm` er separate verdier.
- Det gamle feltet `Vhs` beholdes som blend 0..1 for himmel, stjerner og untonemapped-materialer. Det blir aldri brukt som enum. Clean og styrke 0 setter også denne blenden til 0.
- Feltkamera, TV-kamera og eksisterende SceneCapture-kode setter midlertidig Vhs=0. Tonepasset respekterer dette, slik at bevisbilder ikke får skjermens filmfilter brent inn. Feltkameraets egen film/eksponering er uendret.
- `S47Picture` samler normalisering, etiketter og modeverdier; Flow lagrer `picture` og `pictureStrength`. Kommandolinjeoverstyringer skriver ikke disse preferansene.
- `S47_ONLY=picture` regenererer bare MPC_Web og M_WebTone, i eksisterende materialobjekt. Dette kjøres mot private Web-filer i worktree, aldri mot en annen aktiv jobbs materialer. Genererte Web-assets committes ikke.

```sh
S47_ONLY=picture "$HOME/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd" unreal/Signal47/Signal47.uproject -run=pythonscript -script="$PWD/unreal/Signal47/Tools/webexport/import_web.py" -unattended -nullrhi
```

Editor-målet må være bygget først. Deretter normal Linux-pakking. Eksisterende generert Content uten de nye MPC-feltene gir en tydelig advarsel fra ApplyPicture og må regenereres før levering.

For kjøring og bilder:

```sh
Signal47.sh -S47Picture=carpenter
Signal47.sh -S47Picture=16mm -S47PictureStrength=0.75
Signal47.sh -S47Picture=super8 -S47PictureCalm=1
```

`Scripts/graphics-review.py --picture` godtar også de nye profilene. Scriptets gamle standard VHS beholdes for reproduksjon av historiske nullpunktsturer; oppgi derfor eksplisitt Carpenter ved prøving av hoveduttrykket.

## Bibliotek og avgrensninger

Prosjektbiblioteket ble søkt først. [ntsc-rs-kortet](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/ressurser/x-2022284541965054135.md) er merket UNVERIFIED og beskriver et eksternt verktøy. Ingen kode eller avhengigheter fra det er brukt; løsningens grunnlag er spillets eksisterende Unreal-tonepass og egne shaderendringer.

## Kontroller

Kildekontroll og Python-syntaks er kontrollert. Første kandidat ved `1e17319c` er bygget og pakket uten shaderfeil. Alle fem profiler er kjørt i fire nattkameraer, Carpenter også ved kveld/daggry, styrke0/50%, Low og Ultra. Avbrutte konkurrerende kjøringer er forkastet og beholdt i evidensen. Bilder, kildegjennomgang og fullstørrelsesprøver av alle fem uttrykk er lest av Claude. To lave tekstfunn er rettet. Taklyset øverst til venstre og svak kontrast i enkelte HUD-hint finnes også i Clean og er uendret. Bildene er fra en levende verden og er ikke synkrone piksel-for-piksel-par.

Sluttkandidatens spillkode er `8ba39556`, med `04517414` innflettet. Pakke: `/home/tom/scratch-bm/Signal47-film-looks/candidate-core`. Bevis: `matrix-core` / `controls-core`, oppsummering i `delivery-core.json`. Første kandidat og dens bilder beholdes separat i `candidate` / `matrix`. Samme spillbinær og fem innholdscontainere er hashkontrollert på nytt og matcher alle ti sluttkjøringene.

### Sluttprøver, 24. oktober 2026

| Kontroll | Faktisk resultat |
|---|---|
| Editorbygg og Linux-pakking | PASS, ingen materialkompileringsfeil eller standardmaterialfallback. 61 native RadioArray-assets er reelle filer, ingen LFS-plassholdere. |
| Selvtest | PASS, 1298 sjekker, inklusive de 20 nye filter-/settingsjekkene. |
| Meny, styrke og disk | PASS, faktiske Slate-tastetrykk velger 16 mm og 90 prosent, omstart beholder dem, alle fem profiler gjennomløpes og R gjenoppretter Carpenter. |
| Tilgjengelighet | PASS, Slow fades oppdaterer MPC til calm1 og beholdes etter omstart med valgt profil og styrke. Dette er teknisk funksjonsbevis, ikke menneskelig vurdering av komfort. |
| Kort eksisterende regresjon | PASS, feltkamera/frame01, levende Unarius-TV og kapittel2-brettet. Hele TV-opptakets 47-sekunders lagringsrute er ikke kjørt på nytt i denne leveransen. |
| Bildeprøver i sluttpakken | PASS, ti uavhengige kjøringer / 36 validerte kamera-PNG-er. Alle fem profiler i kontrollrom, gård, diner og Caliche; Carpenter også ved radio/array og kveld/daggry, samt High/1440p nullpunkt. |
| Graderings- og kodegjennomgang | Claude har lest koden og fullstørrelsesprøver av alle fem profiler i første kandidat. Etter modellinnfletting er sju nye Carpenter-/oversiktsbilder godkjent til avgrenset stillbilde-QA. |
| Materialomfang | Bare MPC_Web og M_WebTone er endret i private Web-assets; ingen standardteksturer eller andre Web-materialer er regenerert av picture-importen. |
| Cache | Cookens 588 PSO-er, hvorav 213 graphics, åpnes med samme GUID og antall i runtime. Alle 213 bundled graphics behandles. `cache-and-startup-core.json` har beviset. |

Styrke0/50%, Super8 med bevegelig bilde på Low/720p og Carpenter på Ultra ble i tillegg prøvd i første kandidat, med samme filterkode og materialer. Disse skalar-/quality-prøvene er ikke gjentatt etter at fellesgrenens modeller ble lagt til. Ingen avbrutt eller konkurrerende kjøring brukes som PASS-bevis.

Kort varm High-prøve, 2560 x 1440, samme definerte kameraer og 02:15:

| Sted | Clean snitt / p95 | Carpenter snitt / p95 | Forskjell i snitt |
|---|---|---|---|
| Kontrollrom | 12,876 / 20,640 ms | 13,363 / 21,993 ms | +0,487 ms |
| Caliche | 10,450 / 11,371 ms | 10,834 / 11,862 ms | +0,384 ms |

Det tilsvarer henholdsvis 77,7 til 74,8 fps og 95,7 til 92,3 fps i snitt. Dette er korte offscreen-vinduer i en levende verden, ikke eksakte frosne A/B-bilder eller et løfte om låst 60. Kontrollrommets p95 er over 16,667 ms i begge profiler. Oppstartssporet fra selvtestkjøringen er et diagnostisk spor med verbose RHI-logging og brukes ikke som normal oppstarts-/FPS-måling.

### Start og overlevering

Kjør `/home/tom/scratch-bm/Signal47-film-looks/Play.sh`. Den åpner sluttpakken med en separat `player-profile`, og nye profiler starter med Carpenter på High. Bytt under Settings > Picture > Picture; styrken ligger rett under.

Bildegalleri: `/home/tom/scratch-bm/Signal47-film-looks/index-core.html`. Oversikt over fem profiler: `film-looks-overview-core.jpg`. Original-PNG-er og hasher er i matrise-/leveringsmanifestet. Radiostudioets store bildeflate, speilvendt ON AIR, moire i en veggtekstur og enkelte HUD-kontraster er scene-/modell-/HUD-forhold fra fellesgrenen, ikke innført eller rettet i filterarbeidet.

Kilden er committet på `codex/film-looks` for integrasjon. Ingen standardstarter, installasjon i ~/Games/Signal47, push, PR eller GitHub-release er endret. Kandidaten inneholder ikke installasjonens personlige lagringer eller innstillinger.

Den historiske PSO-bundlen er beholdt og remappingen er kontrollert mot sluttcook og faktisk runtime. En ny full matrise over alle verdens-, strøm- og kvalitetsbaner er UNVERIFIED, og det er også menneskelig opplevelse av bevegelig korn/flimring, HDR, Steam Deck/SteamOS og full gjennomspilling. Stillbilder og automatiserte tastetrykk erstatter ikke disse prøvene.
