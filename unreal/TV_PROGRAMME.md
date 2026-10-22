# Unarius-klipp, lokal kandidat 22. oktober 2026

Tom ønsker bare videre arbeid i Unreal. Filmklipp skal spilles kontinuerlig på TV-ene, ikke vises som ett stillbilde.

## Filer

- `/home/tom/Desktop/Unarius/Arrival-clip.mp4`: 138 sekunder, 1 753 302 byte.
- `/home/tom/Desktop/Unarius/Arrival-trailer.mp4`: 72,35 sekunder, 760 387 byte.
- H.264, 320 x 240, 12 fps, mono AAC. Begge MP4-ene er dekodet helt med ffmpeg uten feil. Hele kildeklippene er bevart.
- Kilder: https://www.youtube.com/watch?v=HVGVZcq63Io og https://www.youtube.com/watch?v=uo3qM8blzcM.
- Kildekopier: `/home/tom/Videos/Signal47-Unarius/`.

Filmene er lokale og følger ikke det offentlige repoet. `Scripts/prepare-tv.py` lager `Content/WebData/tv/unarius.s47tv` fra lokale MP4-filer. Den ligger allerede i den ignorerte WebData-mappa og følger UFS-stagingen. 1683 JPEG-rammer ved 8 fps, 6 436 140 byte, 210,4 sekunder i løkke. Ingen tusenvis av UObjects og ingen Linux-avhengighet til en ekstern videodekoder. En ny JPEG dekodes bare når rammeindeksen endres. Dette er bevegelig film, ikke ett stillbilde. Bildet bruker eksisterende CRT-flater og støy. Filmlyden er bevart i MP4-ene, men runtime-kandidaten bruker foreløpig TV-apparatenes eksisterende sus, ikke filmlyd.

`FS47TvProgramme` er koblet til kontor-TV-en og TV-en i rom 6. Den vanlige 47-sekundershendelsen, kameraopptaket, lagringen og TV-av/på har fortsatt prioritet. Uten framepakken brukes fargekort/snø. Ingen endringer i web eller Unity.

## Kandidat og prøving

Gren `codex/unarius-tv`, arbeidskopi `/home/tom/SIGNAL-47-wt/unarius-tv`. Lokal spillkandidat `/home/tom/Games/Signal47-Unarius/Linux/Signal47.sh` bruker nytt kompilert spillprogram og eksisterende pakket innhold, med den nye framepakken som løs fil. Vanlig spillstarter er uendret. Kandidaten har egen Saved-mappe.

Spillmålet er kompilert med UE 5.8.4 Linux Development, uten kodeadvarsler eller feil. Første UBA-kjøring feilet med ufullstendige filavlesninger; ny kjøring med `-NoUBA` bestod. Ingen editorbygg kjørt.

`Scripts/test-tv.sh OUT BUILD` prøver selvtesten, film på kontor-TV-en ved to tidspunkter, film i rom 6, samt eksisterende samtale, carrier og opptak/lagring. Logg og skjermbilder: `/tmp/s47-unarius-evidence`. Første kjøring bestod 6 av 7 scenarioer; opptak/lagring nådde tidsgrensen og er FAIL/UNVERIFIED. Visuell sammenligning avdekket at kontorfilmen ble holdt igjen av hendelsens tidsvindu. Kilden er rettet slik at vanlig film går uavhengig av hendelsesvinduet. Siste tidsrettelse er også kompilert uten feil (build3, -NoUBA), etter venting på en annen agents byggelås. Den er ikke kjørt og er ikke kopiert til den lokale spillkandidaten; kandidaten har fortsatt forrige binærfil. En full gjennomspilling, subjektiv lytting, overføring til standardutgaven og ny full BuildCookRun er ikke utført.

## Alien på skrivebordet

`/home/tom/Desktop/Alien/` har BLEND, GLB, FBX, glTF, BIN, teksturer og forhåndsvisning. Navn, bruksbeskrivelse og interne filstier er ryddet; originalene og eksporthistorikken er bevart i `/home/tom/Documents/AssetHistory/Alien/`. Geometri: 3771 punkter, 6206 trekanter, rigg med 103 bein og tre pakkede teksturer. Dette er fortsatt den tidligere eksportmodellen, ikke en ny egenprodusert modell. En selvlaget figur må modelleres separat i Blender før den betegnes slik eller brukes som egen grafikk. Modellen er ikke lagt inn i Unreal av denne endringen.
