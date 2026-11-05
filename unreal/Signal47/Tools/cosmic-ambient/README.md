# Kosmisk musikklag

**Status:** Under arbeid. **Gjelder:** Unreal-prototype for ekstra kosmisk ambient under eksisterende musikk. **Sist kontrollert:** 5. november 2026.

Tom ønsker langsom cosmic ambient med romfølelse, undring og mystikk. Prototypen lager tre originale teksturer: en dyp horisont, langsomme brede plasmasveip og luftige støvskyer. Ulike stereoforsinkelser gir bredde. Ingen faste noter legges over musikkspor uten kjent toneart. Syntesen er C++ og krever ingen modell, GPU, nettverk eller nye pakker.

`S47CosmicDSP.h` er den samme syntesen i Unreal og det lille lytteverktøyet. `S47CosmicAmbient` kobler den til prosjektets lydgraf. `FS47Score` eier én node, koblet til eksisterende musikkdestinasjon og dermed spillerens musikkvolum. Night/calm, eerie, tension, dawn, drive og recovery har forsiktige ulike nivåer og klang. Town, motel og action slipper laget. Ingen skjult hendelses- eller fiendetilstand leses. Stemningsendringer glattes; lyttehold fader til eksakt null innen et halvt sekund. Pause gir stillhet og fryser både syntese og referanseposisjon. Naturlige sangpauser beholder teksturens klokke. Full Stop og sluttmodus frigir noden.

Når den allerede autoriserte lokale lydfilbanken er lastet, kan `nasa_earth_chorus`, `nasa_cassini_jupiter` og `space_nebula` blandes inn ett om gangen. Første mulighet kommer etter 37 aktive sekunder, senere med 61 til 89 sekunders avstand. Opptakene spilles i original hastighet med myke kanter, uten løkke. De er ikke målinger eller radiofunn, følger ikke fire/sju-signaturen og gir ingen progresjon. Mottakeren får ren lytteplass også i versjoner hvor vanlig musikk bare dempes ved PC-en. Uten banken fungerer den originale syntesen alene. NASA-kandidatene er hørbarlagte radiodata; Nebula er en kjøpt designet atmosfære med annet opphav. Originaler, lisensmetadata og eksisterende opptaksreferanser bevares. Ingen opptak eller musikk legges i offentlig Git.

## Prøv uten Unreal

Fra repoets rot:

```sh
python3 unreal/Signal47/Tools/cosmic-ambient/preview.py --output /tmp/cosmic-layer.wav
```

Krever C++17-kompilator; `CXX` kan overstyre valget. Verktøyet finner også prosjektets eksisterende Linux-verktøykjede. Kompilatet er midlertidig. 21 atferdssjekker dekker endelige/bundne stereosamples, determinisme, sampleforskjeller, pause, gjenopptakelse, lyttehold og ugyldige verdier ved 8/44,1/48 kHz. WAV-filen er 72 sekunder: night 0 til 20, eerie 20 til 40, dawn 40 til 60, så lyttehold. Nivået er et svakt ekstralag og ikke normalisert som en ferdig musikkutgivelse.

## Prøv faktisk Unreal-graf

Bygg editor-målet under `production_lease` og bruk isolert UserDir. `-run=S47CosmicPreview -S47CosmicPreview=/absolutt/sti/cosmic-runtime.wav` skriver en tilsvarende 72-sekunders fil gjennom den faktiske lydgrafen, med tilgjengelige lokale referanser, og avslutter med statuskode. Loggen oppgir peak, RMS, slutthale og antall referanser. Den prøver også 8 kHz referanse i 48 kHz graf og pause/gjenopptakelse. `FS47Score::SelfTest` inkluderer grafprøven. En bestått syntesetest er ikke det samme som menneskelig lytting, samlet musikalsk balanse eller en installert spillutgave.

## Vurderte kilder

Prosjektbibliotekets [audio-design ved festet commit](https://github.com/gamedev-skills/awesome-gamedev-agent-skills/blob/19d93f97c9c1ea0f8b306e77aa81e0d3e352d3e9/skills/disciplines/audio-design/SKILL.md) beskriver bussruting, lagdeling og demping. Prinsippene er tilpasset prosjektets egen C++-lydgraf; Godot-eksemplene er ikke kopiert. Bibliotekskilden er en referanse, ikke et dokumentert PASS i dette prosjektet.

Fra [listen Tom sendte](https://github.com/Curated-Awesome-Lists/awesome-ai-music-generation) er [LatentScore](https://github.com/prabal-rje/latentscore) interessant for separate ambient-skisser: Apache 2.0, CPU-syntese, WAV, Python 3.11/3.12. Native Windows SDK støttes ikke ifølge prosjektets README. Det er ikke installert eller lydmessig validert her. [AudioCraft/MusicGen](https://github.com/facebookresearch/audiocraft) har MIT-kode, men CC-BY-NC 4.0 modellvekter; derfor er ingen slik modell brukt i spillet. Første prototype gjenbruker eksisterende Unreal-lydmotor fremfor å innføre en Python-modell i runtime.

Editorbygg og commandlet er kjørt 5. november: 56 musikk-/grafkontroller består, tre lokale referanser brukes, peak 0,020663, RMS 0,002909 og slutthale 0. Prosessen avslutter med status 0. Den lokale 60-sekunders før/etter-skissen gjentar samme musikkutdrag og legger teksturer inn i andre halvdel; peak 0,26581. Menneskelig lytting, endelig balanse og installasjon gjenstår.
