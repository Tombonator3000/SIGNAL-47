> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Radiostasjon og array

Ferdig 2026-10-23 kl. 23:53. Sluttkode `3aa41334`, fra arbeidsgren codex/radio-array-assets, lokalt flettet til unreal-port-core. Ingen push/PR eller publisering.

## Leveranse

1420 AM følger konseptarket: lys pusset fasade, lobby, analogt studio med mikser/VU-metere, spolebånd, mikrofonarm, telefon, platespiller, stol og vinylhyller. Blender-kilden beholder 1136 redigerbare objekter. Ny VLA-lignende parabol har paneler, fagverk, fire feedarmer, elevasjonslager, dreiesokkel og serviceplattform. Alle 27 antenner bruker samme modell i tre instansgrupper. Eksisterende plassering, azimut/elevasjon og beacons er bevart. Tre LOD-er: 26 210 / 18 250 / 6 510 trekanter. Stasjonens eksporterte interiør: 43 268 trekanter.

Kilder/eksporter/gjenskaping: Signal47/Tools/radioarray/README.md. Kopier med teksturer og eksport på ~/Desktop/Signal47 radiostasjon og array. Prosjektbibliotekets ahujasid--mcp-for-blender-profil ble lest som historisk verktøyreferanse; ingen bibliotekskode/modell gjenbrukt. Ingen referansefotopiksler er brukt som tekstur.

## Kontroll

- Blender 5.2.2: ni antenne-FBX-roundtrips og 32/32 stasjonseksportkontroller PASS. Null nullarealtriangler. Rensingen endret ingen synlige koordinater, UV eller materialer.
- Unreal 5.8.4: 68/68 strenge assetkontroller, målt koordinatprøve, pivoter, materialer, LOD-er, kollisjon og seks uniontrim-mesh. Bensinstasjonen bevares. Rapport: Signal47/Tools/radioarray/verification/unreal-assets.json.
- Slutteditorbygg PASS på 11,05 s; endelig Linux-pakking PASS på 133,01 s. Ingen materialkompileringsfeil. Loggene ligger i evidensmappen.
- Integrasjonspakken fra `66f41e22`: 1278 selvtester, arraybevegelse, lukket dør, vanlig brukssporing/åpne/gå 6,1 m inn og buzzer-dialog PASS. Terskelgap mellom radius-eroderte gangsoner ble funnet og rettet før denne pakken.
- Første bildeprøve avdekket gammel bildeplate midt i studioet. Native capture-PASS ble visuelt avvist. Avvist bilde og egen FAIL-vurdering er bevart i graphics-dawn-before-backdrop-fix; opprinnelig testpakke i candidate-before-backdrop-fix.
- Sluttkode `3aa41334` endrer bare bildeplatens offset til world X 8029,66, 2 cm foran bakveggens trepanel. Atlas/tilstander, fotoankre, proxyer, dør og kollisjon er uendret. Editor/Linux-pakke, 1278 selvtester, fire nye modellvinkler og pilotregresjon er tatt på nytt på sluttpakken. Uendrede dør-/arrayfunksjoner er dokumentert med integrasjonspakken over.
- Sluttbilder: High/VHS, 2560x1440, antenne forfra/bakfra, fasade og studio ved daggry. Native capture/rammetider PASS uten konkurrerende tung prosess. Bildene er visuelt kontrollert. Korte kameraintervaller er ingen fullrute-ytelsesgaranti. Nattbilder fra gang- og pilotprøvene.
- N01/N02/A02/P01-pilot PASS på sluttpakken. API-rute og faktiske renderbilder; menneskelig input, lytting, forståelse og fullnatt er ikke prøvd her.

## Runtimebevis

Evidensrot: ~/scratch-bm/Signal47-radioarray.

- sluttpakke, `selftest`: PASS. `/home/tom/scratch-bm/Signal47-radioarray/runtime/selftest-1792791590907226487/report.json`
- før bakveggretting, `array-motion`: PASS. `/home/tom/scratch-bm/Signal47-radioarray/runtime/array-motion-1792788983624423429/report.json`
- før bakveggretting, `door-closed`: PASS. `/home/tom/scratch-bm/Signal47-radioarray/runtime/door-closed-1792789169219982207/report.json`
- før bakveggretting, `door-use-walk`: PASS. `/home/tom/scratch-bm/Signal47-radioarray/runtime/door-use-walk-1792789281494793797/report.json`
- før bakveggretting, `buzzer`: PASS. `/home/tom/scratch-bm/Signal47-radioarray/runtime/buzzer-1792789667676400620/report.json`

Bilder/rammer/pakkehash: graphics-dawn/meta.json. Pilot: pilot-final/results.json. Import: unreal-verification-final.json. Installasjon: install-result.json.

## Installert

~/Games/Signal47/Linux er kontrollert mot kandidatens 40 payloadfiler. 51 Saved-filer er byteidentiske før/etter, også innstillinger og lagringer. Tidligere installasjon er bevart i `/home/tom/scratch-bm/Signal47-radioarray/installed-backup-1792792300737671855`. Ingen brukerprosess ble stoppet. Maskinporten avbrøt bare egne kjøringer ved samtidige jobber; avbrutte forsøk er ikke godkjent bevis. Midlertidige Content-lenker og egne genererte OpenOrder-endringer er ryddet. Andre lokale dokumentendringer er bevart.
