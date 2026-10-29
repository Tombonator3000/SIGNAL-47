# Dokumentrekvisitter i spillet (ART-01)

Denne mappen binder ferdige dokumentbilder fra [ArtSource/Documents](../../../ArtSource/Documents/README.md) og [ArtSource/EnvironmentGraphics](../../../ArtSource/EnvironmentGraphics/README.md) til faktiske flater i Unreal-spillet. Kilde-JPG-ene endres ikke.

## Første leveranse: Dales skrivebord i Operations

[surfaces.json](surfaces.json) er fasit for hvilke bilder som ligger på hvilke papirer:

| Papirobjekt | Interaksjon | Dokument som åpnes (uendret) | Bilde |
|---|---|---|---|
| office:fieldFolder | office:folder | EV_DALE_INDEX, feltarket | 01 Dales arbeidsmappe |
| office:workingIndex | office:index | EV_DALE_INDEX | 05 Field checks / working index |
| office:clippings | office:clippings | EV_DALE_CLIPPINGS | 06 Local cuttings |
| office:foldedNote | office:note | EV_DALE_NOTE | 08 A note beneath the inspection sheets |
| office:contactCard | office:contact | EV_DALE_CONTACT | 07 Field contact |
| office:deskForms | office:desk | EV_DALE_INDEX, skrivebordet | EnvironmentGraphics 42 SARO service request |

Objektnavn, interaksjons-ID-er, dokument-ID-er, lagring og bevislogikk er uendret. `S47Office::Use` bestemmer fortsatt hvilket dokument som åpnes. Hvert papir får bildets sideforhold rundt samme midtpunkt. Bildet ligger på en egen tynn flate 0,8 mm over papiret, uten kollisjon, slik at interaksjonsstrålen fortsatt treffer selve papiret. Mangler en tekstur, beholdes det gamle ensfargede papiret med tekstetikett.

Privatmappen (`office:privateFolder`) og Row One-arket (`office:arrayServiceSheet`) har ingen ferdige motiver i pakkene og beholder etikettene sine.

## Import

Teksturene ligger i `/Game/Signal47/Art/Documents` som sRGB, BC7, kvadratisk potens av to (maks 2048) og full mip-kjede. Kjør bare under prosjektets vakt og reservasjon, som andre tunge Unreal-jobber:

```sh
S47_DOCPROPS_REPORT=/absolutt/ny/rapport.json UnrealEditor-Cmd Signal47.uproject \
  -run=pythonscript -script=<prosjekt>/Tools/documentprops/import_assets.py \
  -unattended -nullrhi -nosplash -NoSound
```

Rapporten lagrer kildehash per bilde og de målte UV-hjørnene til `/Engine/BasicShapes/Plane`, som UV-radene i `S47Office.cpp` bygger på.

## Kontroll

`-S47ProductionViews -S47ProductionSuite=documents` tar fire faste kameravinkler ved skrivebordet i dag-, skumrings- og nattlys og kjører `FS47Office::AuditDocumentArt`: seks ark bundet, synlige, minst 1024 px tekstur, plassert over riktig papir, og alle seks interaksjonspapirer finnes. Dette er statiske kamerafixtures, ikke bevis for ekte input.
