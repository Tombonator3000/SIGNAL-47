# Filmfiltre for SIGNAL / 47

Toms bestilling 23. oktober 2026: lag filmuttrykkene, forbedre eksisterende VHS og prioriter Carpenter-looken. Arbeidsgren `codex/film-looks`, fra `unreal-port-core` ved `ed061aca`. Bare Unreal-utgaven endres.

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

Kildekontroll og Python-syntaks er kontrollert. Pakket bygg, selvtester og visuell kontroll føres her når de faktisk er kjørt. Evidens og kandidat ligger under `/home/tom/scratch-bm/Signal47-film-looks`.

PSO-/shadercache må vurderes etter cook fordi tonepasset og MPC-layouten er endret. Den historiske cachematrisen er ikke bevis på den nye shaderen. Menneskelig opplevelse, lytting og full gjennomspilling er ikke dekket av stillbilder eller automatiserte tester.
