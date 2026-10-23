# Radiostasjon og array, 23. oktober 2026

Originale Blender-kilder for 1420 AM i Caliche og en gjenbrukbar VLA-lignende radioantenne.
Stasjonen følger `Concepts/Caliche-2026-10-23/03-radiostudio-1420-am.png` og Toms konseptark på skrivebordet.
Antennen følger de to vedlagte fotografienes form: parabolpaneler, fagverk, fire feedarmer,
elevasjonslager, dreiesokkel og serviceplattform. Ingen fotopiksler eller innhentet modellgeometri er brukt.

## Kilder og gjenskaping

`dish/SARO_Array.blend` og `station/1420_AM_station.blend` er redigerbare kilder.
Begge mapper har Python-generator, FBX, GLB, materialsidecar og kildeforhåndsvisninger.
FBX-kildene er i meter med Z opp. Unreal-importen bruker centimeter.
Blender 5.2.2 LTS er brukt. Se delmappenes kontrakter og faktiske verifiseringsrapporter.

Prosjektbiblioteket ble søkt med Blender, radiostasjon, antenne, array og spillteknologi.
`repoer/prosjekter/ahujasid--mcp-for-blender.md` ble lest som verktøyreferanse.
Den dokumenterer en historisk kandidat uten ny spilltest; ingen kode eller modeller derfra er gjenbrukt.
Det installerte Blender-programmet og prosjektets eksisterende FBX/glTF-import er brukt.

## Unreal-kontrakt

`S47AuthoredArray.cpp` erstatter de gamle instansdelene med Base, Azimuth og Elevation.
Alle dagens 27 antenner bruker samme kildemodell. Eksisterende plasseringer, skalaer,
retning, beacons og natthendelser styrer dem videre gjennom `DrawDishes`.
`-S47LegacyArray` gir gammel geometri for sammenligning i samme kandidat.
Kildemodellens akselhøyder er 0, 7,8 og 12,4 meter; feedlampen er 9,2 meter over elevasjonspivoten.
Tre LOD-er har totalt 26 210, 18 250 og 6 510 trekanter per komplett antenne.

Radiostasjonen plasseres gjennom `Config/Props/radio_station.json`.
`S47RadioStation.cpp` erstatter kun de aktuelle gamle bygningsflatene.
Studioets eksisterende vinduspanel, ON AIR, 1420 AM-skilt, mast og historieproxies beholdes.
En egen lobbydør gir adgang til lobbyen; studioet har egen skillevegg.
`trim_station.py` komponerer radiokuttet med eksisterende bensinstasjonskutt i de delte bymeshene.
Patcher nekter delvis påføring ved manglende eller endret kildegeometri.

## Import

Etter generering av begge modellsettene:

```sh
Tools/radioarray/import.sh
```

Importøren bruker legacy FBX, eksplisitt koordinatpolicy og 90 graders korreksjon målt med den asymmetriske prøven.
Blender-punktet `(2,4,6)` skal bli Unreal `(200,-400,600)` cm; importen stopper ved avvik.
`S47_RADIOARRAY_ONLY=probe` kjører bare prøven. Standard importerer begge modellsettene og uniontrimmen.


Kjør fra `unreal/Signal47`. `UE_ROOT` er prosjektets UE 5.8.4-installasjon.
Importen skriver kun `/Game/RadioArray` og en rapport ved skriptet.
Bevar originalenes `Content/Web`, `WebData`, `Free`, lyd og lisenskilder.

## Prøvestatus

Blender-render er kildeinspeksjon. FBX-roundtrip er eksportkontroll.
Unreal-import, pakket runtime og målt ytelse rapporteres separat i `unreal/RADIO_ARRAY.md`.
En kildepreview eller vellykket kompilering alene er ikke en godkjent utskifting av installert spill.
