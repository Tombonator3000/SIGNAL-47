# Dokumentrekvisitter til SIGNAL / 47

31 ferdige, flate JPG-teksturer fra 26. oktober 2026. Åpne [GALLERI.html](GALLERI.html) for å filtrere samlingen, forstørre arkene og lese kildetekstene. [OVERSIKT.jpg](OVERSIKT.jpg) viser hele pakken.

To senere brukerleverte motiver er logget separat som [bildereferanser til Dales private filer](References/DaleAutopsy/README.md). De har ukjent opphav/dato og inngår ikke i de 31 ferdige rekvisittene.

## Innhold

| Filer | Innhold |
|---|---|
| 01 | Originalt omslag til Dales arbeidsmappe. |
| 02 til 08 | De sju dokumentene fra EveningShift: overlevering, servicejournal, radioskjema, feltindeks, avisutklipp, kontaktkort og privat notat. |
| 09 til 14 | Alle seks private manusutkast: lyslogg, vitneavskrift, forklaringsark, fotokonvolutt, lånt kassett og usendt brev. |
| 15 til 21 | 1947-avisen, original og endret feltprotokoll, arkivhylster, B-12-kort, transitark og tidsprotokoll. |
| 22 til 27 | Systemnotat, fire gamle skriverutskrifter og K3-servicehistorikk. |
| 28 til 31 | Fire nye frivillige bakgrunnsrekvisitter: sladdet lysrapport, avis om motstridende klokker, veterinærark med feltfoto og sjablongkopiert foredragsplakat. |

Bildene ligger i [jpg/](jpg/). [catalog.json](catalog.json) kobler hvert bilde til ID, kilde, engelsk tekst, dato, bruksområde og kontrollpunkter. De nye tekstene står i [supplemental.json](supplemental.json).

## Format og størrelse

Alle dokumenter er progressive RGB-JPEG med sRGB-profil og 4:4:4-farge. De er bevart i generert oppløsning: vanligvis 1086 x 1448 for ark og 1448 x 1086 for kort, opptil 1536 piksler på lengste side. Ingen oppskalering, beskjæring eller opptegning ble brukt under konverteringen. JPEG-kvalitet er 86.

De 31 JPG-filene bruker 12 143 169 byte, omtrent 12,1 MB. De tilsvarende PNG-originalene bruker 89 438 281 byte. Reduksjonen er 86,4 prosent; enkeltfilene er 285 467 til 593 960 byte. Bare JPG-leveransen følger Git. PNG-originalene er bevart utenfor repoet.

Dette er flate, ugjennomsiktige ark uten rombakgrunn. Et papir med ujevn ytterkant har ingen alfakanal. Katalogen er oppslag for senere import; den endrer ikke et eksisterende spilldokument eller lagringen av funn.

## Opphav og kanon

Bildene er laget med innebygd imagegen. Brukerens ti bilder var visuell inspirasjon til papir, maskinskrift, sladding, vedlegg og håndskrift. Ingen bokside eller X-Files-rapport er kopiert inn i leveransen. Hele det endelige promptsettet, råbildehasher og JPG-hasher er lagret i [generation.json](generation.json). [generation_history.json](generation_history.json) beskriver støttebildene og revisjonen av avisutklippene.

De sju EveningShift-tekstene følger `Narrative/EveningShift/modules/office.json`. Seks private tekster følger `DALE_OFFICE_DOCUMENTS.md`. Arkivtekstene følger de eksisterende C++-dokumentkildene. Kildeuttrekk og teksthasher kontrolleres maskinelt. Faste verdier, manglende dato, usikkerhet og åpne observasjoner beholdes. Fortellerprefikser vises som faktisk håndskrift eller layout, og er ikke nye observasjoner.

E15 er en generert nytolkning med komposisjonen fra masterfotoets crop B: flat mesa, høyt lys, lavt kort halespor til høyre og to ranchere ved gjerdet. Det er ikke en pikselidentisk beskjæring. Den skal vurderes mot eksisterende foto og sammenligningsgrunnlag før eventuell utskifting i spillet. E09A bevarer de kvalitative retningene, men er ikke et målsatt kart som kan erstatte eksisterende geometrisk overlegg.

De fire ekstra arkene er nyskrevet fiksjon til mulig senere bruk. De etablerer ingen nye hovedrutebevis. De seks private utkastene blir heller ikke registrert i runtime av denne leveransen. Ingen spillkode, fortelleropptak, VHS-film, pakking eller installasjon er endret.

Prosjektbiblioteket ble søkt før verktøyvalg. Profilen `ressurser/x-2105024134073946209.md` om ImageGen-teksturprojeksjon ble lest; den er UNVERIFIED og gjelder Three.js. Ingen teknisk løsning fra den ble gjenbrukt i Unreal. Repoets engelske manus og eksisterende ID-er styrte innholdet.

## Kontroll og gjenbruk

[verification.json](verification.json) skiller formatkontroll fra manuell nærlesing av de faktiske JPG-filene. OCR var bare en supplerende, ufullstendig kontroll av blant annet håndskrift; det hevdes ikke eksakt OCR-samsvar. Galleriet er kontrollert i nettleser med gruppefilter, åpning av transkripsjon og bildelenker. Dette er kontroll av kildeassets, ikke runtime- eller spilltest.

Fra reporoten:

```sh
python3 unreal/ArtSource/Documents/Tools/build_catalog.py --check
python3 unreal/ArtSource/Documents/Tools/verify_assets.py --no-report --review-file unreal/ArtSource/Documents/manual-review.json
python3 unreal/ArtSource/Documents/Tools/build_gallery.py
```

`Tools/prepare_assets.py --inputs /absolutt/inputs.json` formatkonverterer et komplett sett lokale imagegen-originaler. Innputtfilen må ligge utenfor repoet og inneholde `assets` med `id`, full `prompt` og absolutt `source_path`. Verktøyet validerer alle kilder før noe skrives. Det genererer verken bilder eller nye tekster.

Verktøyene krever Python 3 og Pillow. Den manuelle vurderingen er bundet til hver vurdert JPG med SHA-256; et endret bilde krever ny vurdering.
