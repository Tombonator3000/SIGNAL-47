# SIGNAL / 47 — aktuell overlevering

Oppdatert 10. september 2026: første kapittel «Den andre eksponeringen» er levert som spillbar alpha med alle åtte avtalte kjernekrav PASS på den målte Kubuntu-maskinen. Se [CHAPTER_09.md](CHAPTER_09.md) for omfang, bevis og åpne kvalitetsbegrensninger.

## Åpne denne utgaven

Kanonisk mappe: `/home/tombonator3000t/.codex/.chatgpt-projects/g-p-6aa05604a4708191a65139c1d5f89363/SIGNAL-47`.

Kjør `./Spill-SIGNAL47.sh` i mappen. Den peker på `Artifacts/Releases/Chapter09-86a28b30ad94/Start-SIGNAL47.sh`. Flyttbar pakke: `Artifacts/Releases/SIGNAL47-Chapter09-86a28b30ad94-Linux.tar.gz`. En ny utpakking er prøvd med normal oppstart, ekte tastatur/mus, fokusbytte, pause og QUIT uten testmodus. 173 filer og kjørerettigheter er verifisert. Store lokale spillpakker følger ikke et rent Git-klon.

Spillet fortsetter fra prologen gjennom S-03, ny fotolab, bildeundersøkelse, hypotese, et av to B-12-kontrollforsøk, andre eksponering og ny kapittelslutt. Lagring åpnes etter telefon-/antenneforløpet. Ny sak bevarer bildeeksportene. README beskriver kontroller, lagringsstier og feilhåndtering.

## Kilde og kontroll

Privat repo: Tombonator3000/SIGNAL-47. Arbeidsgren: `gauntlet/chapter-one-09`, startet fra `1c572b1` (main `8a6b4d9` med overleverings-/arbeidsordredokumentasjon). Spillkildecommit: `9329a2d819108796c6f731cc305f16dc3250c651`. Senere commits er levering, automatisering og dokumentasjon.

Unity-kildehash: `86a28b30ad94fc9e67aa0bf0558f0a51f511adadc76323fcf69eca7c72d8c8b9`. Byggpayload: `6b0c99dc455267875ac836594e6f1478fb58515d337313032604325c2de6056b`. Arkiv: `bce4c4c7423a4e7f42792991553fb1ee3386d0f3a9918a2cdb14aee8ab9a1696`. Bevis: `Docs/Evidence/Chapter09/final-9329a2d/` og de avgrensede visuelle-/lagringsrapportene ved siden av.

Begge fulle native ruter, to faktiske gjenstarter, feilvalg, fotointegritet, labpause, feilprofiler, nytt spill, lupe, lyd-/mus-/fullskjerminnstillinger og pakket oppstart PASS. Eksisterende utviklingsregresjon PASS, inklusive 47-sekunderskontrakten. Målt release: 74,993 fps over 121,397 sekunder; p95 15,897 ms, p99 16,187 ms, maksimum 20,087 ms og 0 intervaller over 50 ms. Intel Core Ultra 5 225U / Intel ARL / OpenGLCore / 1280×800 Ultra / 75 Hz, fps-mål 75, vSync 0. Alle 9104 intervaller beholdt; QA-bilder og lydmåling separat.

## Begrensninger og videre arbeid

GPU-tid, faktisk lytting/subjektiv miks og ekstern førstegangsvarighet er UNVERIFIED. Aktiv kjent-løsning-test tok 311 sekunder inklusive negative kontroller; 20–30 minutter for førstegangsspillere er fortsatt en uprøvd ambisjon. Materialer, kontaktlys og enkelte møbler kan videreutvikles. Motell, bil og resten av Roswell-fortellingen er senere innhold. Ikke presenter dette som hele det planlagte spillet.

Behold Unity 6000.3.22f1 / URP 17.3.0 / Input System 1.20.0 / Blender 4.5.13 LTS, originale modeller, metadata, VT323/Scott Buckley og 1419.900/1420.110/1420.405 MHz, 4/7, −39 LY samt 47 spillsekunder etter død linje. Ingen nye kjøp eller fakturerbar 3D-tjeneste er brukt. Gauntlet, installert Brainstorming og tilpasset Dream Loop er dokumentert i kapittelrapporten. Én prøvd lokal Gauntlet-referanse er lagret; ingen automatisk maskinsynkronisering påstås.

Forrige fungerende pakke er beholdt: `Artifacts/Releases/FieldCamera08-af5a85d/` og `SIGNAL47-FieldCamera08-Linux.tar.gz`. `Artifacts/GauntletLinux/` er ordinær byggoutput; bruk den faste kapittelpakken for spilling. Synkroniserte filer under `sources/` og andre prosjekter er urørt.

PR 1–5 var flettet før dette oppdraget. Denne leveransen ligger som [privat PR-utkast #6](https://github.com/Tombonator3000/SIGNAL-47/pull/6), med leveranse-/beviscommit `a795335`; ingen ny merge eller offentlig publisering er autorisert. Ved videre endringer: behold fungerende pakke, bygg ny identitet og gjenta berørte kontroller. Ikke gjenbruk en gammel grønn rapport på endret gameplay.
