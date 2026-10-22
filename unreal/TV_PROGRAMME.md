# Unarius-klipp, lokal kandidat 22. oktober 2026

Tom ønsker bare videre arbeid i Unreal. Filmklipp skal spilles kontinuerlig på TV-ene, ikke vises som ett stillbilde.

## Filer

- `/home/tom/Desktop/Unarius/Arrival-clip.mp4`: 138 sekunder, 1 753 302 byte.
- `/home/tom/Desktop/Unarius/Arrival-trailer.mp4`: 72,35 sekunder, 760 387 byte.
- H.264, 320 x 240, 12 fps, mono AAC. Begge MP4-ene er dekodet helt med ffmpeg uten feil. Hele kildeklippene er bevart.
- Kilder: https://www.youtube.com/watch?v=HVGVZcq63Io og https://www.youtube.com/watch?v=uo3qM8blzcM.
- Kildekopier: `/home/tom/Videos/Signal47-Unarius/`.

Filmene er lokale og følger ikke det offentlige repoet: rettighetene er ikke avklart. `Scripts/prepare-tv.py VIDEO...` lager to ignorerte filer, som stages som løse filer (DefaultGame.ini):

- `Content/WebData/tv/unarius.s47tv`: 2524 JPEG-rammer ved kildenes egne 12 fps (320 x 240, 4:3), 9 648 811 byte, 210,33 sekunder i løkke, først klippet og så traileren.
- `Content/Audio/Raw/tv/unarius.wav`: filmlyden, mono PCM16 ved 24 kHz (2000 punkter per bilde), hvert klipps lyd forlenget med stillhet eller kuttet til nøyaktig bildenes lengde, slik at lyd og bilde går i løkke sammen. 10 096 044 byte. `Scripts/audio-raw.py` lar `Raw/tv` stå når den lager resten på nytt.

Ingen UObject per bilde og ingen videodekoder fra plattformen: en JPEG dekodes bare når bildet skifter.

## Slik spilles den

`FS47TvProgramme` (én per TV-apparat) har klokka, siste dekodede bilde og apparatets lille høyttaler. Lyden er en løkkende buffer i spillets Web Audio (S47WebAudio): highpass 250 Hz to ganger, lowpass 4 kHz, en boksresonans ved 1,4 kHz, litt metning (tanh), mono, fra apparatets høyttalergitter gjennom en panner med invers avstand (`US47Audio::Spatial`, ref 0,9 m), som apparatets andre lyder. Gjennom vegg (kontoret sett utenfra, gangen utenfor rom 6) er den 0,4 ganger så sterk og lowpass 650 Hz. Den dukker til en firedel under telefon, samtale og stemmer (`AS47Story::VoicesDuck`). Mens lyden spiller, følger bildet lydens klokke, så de går i takt; når lyden er stille, går bildets klokke videre med spilltiden, og lyden starter igjen der bildet er.

- Kontor-TV-en (`FS47MotelTv`): filmen i stedet for testkortet, tegnet når bildet skifter, jevn overscan så bildet holder 4:3 på det 4:3-store røret. Sendingen går videre under hendelsen. 47 sekunder carrier og bildet av plassen tar over bildet og gjør filmlyden stille; etterpå kommer filmen tilbake med lyden der bildet er. Lyset fra TV-en på kontoret følger filmens lysstyrke. Uro i natta (unrest) får filmlyden til å vakle som tonen gjorde.
- Rom 6 (`US47Room6`): filmen i stedet for snøen når apparatet er på, med tynne svarte striper på sidene fordi skjermen er litt bredere enn 4:3 (0,4998 x 0,36 m), og glasset mørkt under filmen. Lyset på rommet følger filmens lysstyrke slik det fulgte snøen. Av/på virker som før; av er stille. Sendingen starter ved traileren og går videre også når apparatet er av eller spilleren er ute. Snøsuset brukes bare uten filmlyd. Bildet sto før opp ned (radene gikk opp i feil rekkefølge, noe snøen aldri viste); det er rettet.

Uten rammepakken: testkort og snø som før. Uten lydfila: stille film, og snøsuset i rom 6.

## Prøving 22. oktober, editorbygg i arbeidskopi

`Scripts/test-tv.sh OUT [BUILD]` kjører uten BUILD editorbinæren med `-game` på prosjektet. Hver kjøring venter på tur (ingen annen UnrealEditor eller Signal47 i gang). Første versjon nådde tidsgrensen i opptak/lagring: loggen viser at spillet sto fast i bilde 4 under oppstart, før TV-prøven i det hele tatt begynte, ikke i opptaket. Den samme stillstanden kom her med full disk (`No space left on device` ved oppstart av Zen). Skriptet stopper nå en kjøring når loggen har stått stille i 90 sekunder, og prøver én gang til; det bruker grep i stedet for rg, som ikke er installert.

Siste kjøring: alle sju bestod (editorbygg, innhold fra hovedkopien via symlenker).

- `programme` og `programme-later`: kontor-TV-en på nært hold ved bilde 85 (7,15 s) og bilde 158 (13,20 s), to ulike filmbilder, rett vei opp; lyden spiller fra 3,01 s og ligger på samme tid som bildet (7,15 mot 7,15 s, 13,20 mot 13,22 s).
- `room6-tv` (`-S47Ch4Drive=tvnear`, en meter foran skjermen): bilde 1721 og 1793, lyd og bilde 143,46 mot 143,46 s og 149,47 mot 149,47 s. Lyden går gjennom vegg på gangen og inne i rommet når spilleren er der.
- `room6talk`: N. Vegas samtale ved bordet som før.
- `moteltv`: carrier, grå skjerm, filmlyden av.
- `moteltvframe`: carrier i 47 sekunder, bildet av plassen fra oven tatt opp og lagret (`save roundtrip OK`, variansen 2907), deretter filmen tilbake med lyden fra 52,62 s der bildet var.
- `selftest`: alle TV-sjekkene bestod (4:3 med striper, lydstart der bildet er, klokka rundt løkka, stillhet og ny start, stående kontekst). Selvtesten som helhet har én feil utenfor TV-en: `Board API: moving a card saves its new layout on release`. Arbeidskopien har skriftene i `Content/Fonts` bare som Git LFS-pekere (derfor også tekstløse HUD-er på skjermbildene), noe som trolig gir brettet andre mål.

Den lokale spillkandidaten `/home/tom/Games/Signal47-Unarius/` har fortsatt Codex' forrige binærfil og 8 fps-pakken; den er ikke oppdatert.

Lyd, gjengitt offline (`-S47AudioTour`): filmlyden ved 2 m har RMS 0,0154 (-36 dBFS), gjennom vegg 0,0086, under stemme 0,0035; telefonstemmene ligger på 0,05 til 0,07, altså 11 til 13 dB over TV-en, og 24 dB over den når den dukker. Konvolutten av det gjengitte signalet passer med programmets lyd ved samme tid med forsinkelse -2 ms (vindu 2 ms), korrelasjon 0,88; 98,7 prosent av energien ligger mellom 200 Hz og 5 kHz.

Ikke gjort: subjektiv lytting, pakket bygg (BuildCookRun) og full gjennomspilling.

## Alien på skrivebordet

`/home/tom/Desktop/Alien/` har BLEND, GLB, FBX, glTF, BIN, teksturer og forhåndsvisning. Navn, bruksbeskrivelse og interne filstier er ryddet; originalene og eksporthistorikken er bevart i `/home/tom/Documents/AssetHistory/Alien/`. Geometri: 3771 punkter, 6206 trekanter, rigg med 103 bein og tre pakkede teksturer. Dette er fortsatt den tidligere eksportmodellen, ikke en ny egenprodusert modell. En selvlaget figur må modelleres separat i Blender før den betegnes slik eller brukes som egen grafikk. Modellen er ikke lagt inn i Unreal av denne endringen.
