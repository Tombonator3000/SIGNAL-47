# Originalmodeller for feltreisen

Bestilt 25. oktober 2026, prioritert av Tom foran skyggemannmodellen. Pakken gjelder STATION01-felthytten, feltinstrumentene og SARO-arkivet. Tidligere bybygg er allerede levert i M6. Dales lukkede kontor og skyggemannmekanikken inngår ikke.

## Modellpakken

| Familie | Deler | LOD0 / LOD1 / LOD2 |
|---|---:|---:|
| Felthytte | 5 | 5506 / 3618 / 2352 |
| Feltinstrumenter | 13 | 20500 / 10572 / 6750 |
| SARO-arkiv | 11 | 13258 / 8286 / 5264 |

Tre redigerbare Blender-originaler med pakkede, relative originalteksturer, samlede GLB-er og 87 FBX-/87 GLB-eksporter. Geometrien er egenprodusert. Feltinstrumentene omfatter teodolitt/pier, feltlampe med separat hode/linse/deksel, mottaker/skala, telefon/rør, timingbenk/logg, diagram og bevisrekvisitter. Arkivet omfatter reoler, skap, kartotek, bokhylle, bord, feltboks, brannskuff, 047-boks, bryterbord, samtaleopptaker og serviceperm. Den allerede plasserte kassettopptakeren beholdes.

## Integrasjon

S47FieldModels installerer hele pakken etter validering av assets, tre LOD-er, målestokk, materialer og nøyaktige forgjengere. 73 presise blad-/indekserstatninger bevarer andre gamle flater, kabelbruddet, markører, gulv-/gangsoner og alle storyproxies. Dør, telefonrør, lampedeksel, arkivboks og skuff følger sine opprinnelige foreldre. Mottakerens skala og feltlampens linse følger eksisterende materialtilstand; papirstrimmel, KSU-linser og fallpapir beholdes som levende originalobjekter.

Originale GLB-attributter og beholdte indekser kontrolleres før import. glTF-importen av restmesher mistet tynne kabeltrekanter. Disse kandidatene ble avvist. De samme kontrollerte restene eksporteres derfor via vår målte FBX-aksekontrakt, og alle 13 native restmesher må beholde eksakt trekanttall. Arkivets første kildeeksport hadde en udefinert materialreferanse; den ble rettet før akseptert eksport. Ett opprinnelig nativeasset har færre degenererte flater enn råeksporten, så runtime låser det faktisk målte forgjengertallet i metadata.

## Faktisk kontrollstatus

Kildekontroller: PASS for tre lagrede Blender-originaler, 87 faktiske FBX-er og 87 GLB-er. Nativeimport F4: PASS, 1191 porter. Editorbygg F2 og første avgrensede visuelle felthytteprøve: PASS. Pakket kandidat, endelig visuell gjennomgang og levering føres etter gjennomføring. Ingen påstand om fysisk Xbox-prøve, full gjennomspilling eller ny ytelsesgaranti.

Kilder: Signal47/Tools/fieldstation, fieldinstruments, archiveprops og fieldintegration. Bevisrot: /home/tom/scratch-bm/Signal47-field-models. Prosjektbibliotekets historiske Blender MCP-profil ga ingen dokumentert løsning for denne produksjonen; prosjektets egne verifiserte eksport-/PBR-verktøy er gjenbrukt. Ingen tredjeparts råmodeller eller nye plugins er hentet.
