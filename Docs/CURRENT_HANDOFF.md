# SIGNAL / 47 — aktuell overlevering

Oppdatert 10. september 2026. Grafikkpass10 er levert som en kontrollert, spillbar oppdatering av første kapittel. Fotolab, materialer, metall, lys, vegetasjon og fysiske lydeffekter er vesentlig løftet. **Hele referansebildenes kvalitetsnivå er fortsatt ikke nådd.** Nærdetalj, brede gulvreflekser, malingsavskalling og terrengvariasjon står åpne i [grafikkrapporten](VISUAL_10.md).

## Åpne utgaven

Kanonisk mappe: `/home/tombonator3000t/.codex/.chatgpt-projects/g-p-6aa05604a4708191a65139c1d5f89363/SIGNAL-47`.

Kjør `./Spill-SIGNAL47.sh`. Den peker fast på `Artifacts/Releases/Visual10-eb0748660027/Start-SIGNAL47.sh`. Flyttbar pakke: `Artifacts/Releases/SIGNAL47-Visual10-eb0748660027-Linux.tar.gz`. Ny utpakking,173 filer, rettigheter, normal start, bevegelse, fokusbytte, pause og avslutning er kontrollert uten testmodus. Lokale pakker følger ikke et rent Git-klon.

Det komplette kapittelforløpet er bevart: nattevakt, S-03, fotolab, bildeundersøkelse, hypotese, aktiv eller passiv B-12-prøve, andre eksponering og lokal avslutning. Lagring åpnes etter telefon-/antenneforløpet. Ny sak bevarer eksportarkivet. README beskriver kontroller og lagring.

## Kilde og verifisering

Privat repo Tombonator3000/SIGNAL-47, gren `gauntlet/visual-reference-10`, fra main `c48616a` etter brukerens merge av PR6. Grafikk-/spillkildecommit `d026cc3d431ed0e99b43e06e2e6e204ef842cdba`. Senere leveranse-/dokumentasjonscommits endrer ikke den testede Unity-kilden.

Unity SHA `eb0748660027744a8ad717c16a51a1e8801f4908b758e4ca191e24108c9a2c01`; byggpayload `eb2bbdcc925e3e016b6e5c90996cb386ada41bb2004e42e1954b07713601ebb6`; arkiv `8f83fba8197dd3650d6ca608efdaefc05c44f12eb138bc3f64b42b98121368dd`. Bevis: `Docs/Evidence/Visual10/final-d026cc3/`.

PASS: ny normalstart, begge forsøksveier, to faktiske prosessgjenstarter, negative valg, labpause, varige fotohasher, lupe/gjenåpning, innstillinger og pakket oppstart. Hele den aktive kjente-løsning-ruten tok ca343 sekunder inklusive negative kontroller og ekstra inspeksjon; dette er ikke førstegangsspillertid. Eldre uendrede kjernetester og feilprofiler står fortsatt i Chapter09-bevisene.

Målt release:73,412fps/121,533s etter20s oppvarming, p95 15,368ms, p99 19,364ms, maksimum30,718ms og ingen intervaller>50ms. Intel Core Ultra5 225U / Mesa Intel ARL / Ubuntu26.04 Linux7.0 / OpenGLCore /1280×800 Ultra /75Hz, mål75, vSync0. Alle8922 intervaller beholdt, bilder og lydmåling separat. GPU-tid og subjektiv lytting UNVERIFIED. Samplet lydutgang uten fullskaleklipping; ingen ekstern spilltesting påstås.

## Bygging og videre arbeid

Bruk `bash Automation/build-visual10-linux.sh`. CPU-lysberegningen trenger faktisk grafikkenhet og grafisk sesjon; ikke `-nographics`. Bygget validerer UV/lysatlas og synlige emissive materialer. Eldre byggeskript regenererer scenen uten bakte lysdata og skal ikke brukes som oppskrift for denne grafikkpakken.

Behold Unity6000.3.22f1/URP17.3.0/Input1.20.0/Blender4.5.13, opprinnelige modeller/kilder, metadata, VT323/Scott Buckley og1419.900/1420.110/1420.405MHz,4/7,−39LY og47 spillsekunder.23 installerte originalfiler fra kostnadsfrie kilder er hash-/lisenskontrollert. Gauntlet og Brainstorming lest; Dream Loop brukt som metode. Gauntlet-læring er oppdatert og validert lokalt, ikke automatisk synkronisert.

Forrige kapittelpakke `Chapter09-86a28b30ad94` og feltkamera08 er beholdt. Ingen brukerlagring, synkronisert `sources/` eller andre prosjekter er endret. Bil, motell og resten av Roswell er senere innhold. Ingen kjøp, ny merge eller offentlig publisering er gjort. Leveransen er samlet i [privat PR-utkast #7](https://github.com/Tombonator3000/SIGNAL-47/pull/7), med bevis-/leveransecommit `8769546`. Ingen merge er utført.
