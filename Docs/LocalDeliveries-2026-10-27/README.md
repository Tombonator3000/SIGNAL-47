# Lokale leveranser samlet 27. oktober 2026

Denne samlingen bevarer ferdige konsept-, bilde- og dokumentleveranser fra den lokale arbeidsmappen. Den endrer ingen spillkode, installasjon eller personlige lagringer. Samme SHA-256 publiseres bare én gang; eksisterende filer i repoet og [designbibelen](../DesignBible-2026-10-27/README.md) er lenket fra [filindeksen](FILES.md).

20 manglende filer er kopiert (3,23 MiB), 153 identiske filer peker til eksisterende repo-/designbibelinnhold. Tre eldre README/verktøytilpasninger fra dokumenteksporten er erstattet av dagens ArtSource-kilder. Alle originale arbeidsfiler er urørt. I kopierte HTML-gallerier er bare lokale lenker flyttet til filenes dedupliserte plassering; begge kontrollsummer står i [manifestet](manifest.json).

- Konseptbilder er forslag; de dokumenterer ikke implementert spill. UI-materialet har seks nye forslag og eldre referanser.
- Bildene fra `skjermbilder/` er historiske Unreal-opptak fra 24. oktober. Råloggene beholdes i den lokale arbeidsmappen og publiseres ikke. De attesterer sine opprinnelige prøver, ikke dagens kombinerte bygg.
- `resultater/ankomst-og-kontor/` er Blender-render og proveniens, ikke runtime-opptak.
- Redigerbare Blender-modeller, ku-konsepter, dokumentgrafikk og miljøgrafikk som allerede finnes byteidentisk i ArtSource, er referert uten dobbeltkopier.

Rålogger, rå samtalearkiver, lisensierte tredjepartoriginaler, Windows-byggpakken, private bygg, Saved og cacher er utelatt. Den historiske UI-gallerikoden er bevart som referanse; den er ikke en ny spillimplementering.

## Historiske leveringskvitteringer

[NP1](receipts/NIGHT_PRODUCTION_RUNTIME.md), [T02](receipts/T02_RUNTIME.md), [SC01](receipts/SC01_RUNTIME.md) og [historieplanen](receipts/HISTORIE_IMPLEMENTERINGSPLAN.md) er uendrede tekster fra de dokumenterte kildegrenene. Relative lenker i disse tekstene følger deres opprinnelige plassering under `unreal/`; de skal leses som historiske kvitteringer. Dagens statusfiler gjelder for senere leveranser. Se [SC01-kildeindeksen](receipts/SC01_INDEX.md).

SC01s ferdige kilde og pakkede tekniske aksept er bevart på eksterne grener. Den er ikke integrert i dagens `main`-baserte MetaHuman-/terreng-/atmosfærebygg ved denne innsamlingen. SC02 er fortsatt forberedelse og er ikke aktivert.

## Kontroll

SHA-256 er kontrollert for alle kopier og eksisterende pekere; hovedgalleriets lokale filreferanser fungerer. [Kontrollrapporten](verification.json) fører også 18 allerede manglende avhengigheter i historiske HTML-fragmenter; disse er ufullstendige arkivkilder, slik den opprinnelige LES_MEG.txt dokumenterer. Ingen Unreal-/Blender-/pakkejobb er startet. Prosjektbiblioteket ble søkt først og bredere én gang; treffene beskrev andre arbeidsflyter, så ingen bibliotekkode eller løsning er gjenbrukt.
