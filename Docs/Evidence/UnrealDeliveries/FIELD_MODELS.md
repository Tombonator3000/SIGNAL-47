> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

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

Kildekontroller: PASS for tre lagrede Blender-originaler, 87 faktiske FBX-er og 87 GLB-er (285/78/356 kontroller). Nativeimport F4: PASS, 1191 kontroller. 609 ekstra kontroller av faktiske lagrede native LOD-posisjoner mot FBX: PASS. Editorbygg F3b og pakket kandidat F3 fra 093a4326: PASS.

Fem fokuserte pakkede scenarioer PASS: feltbilder med 1400 selvtester, arkivbilder, feltundersøkelsen med faktiske foto/fremkalling/innsamling, arkivhendelsen og Continue fra samme profils faktiske lagring. Feltforløpet bruker native API-plassering og handlinger; det er ikke en menneskelig kjøretur.

Alle 18 faktiske spillerhøydebilder i High/Clean ved dag, skumring og natt er åpnet og visuelt vurdert. F1 og F2 ble avvist visuelt: gamle lys- og støvplassholdere ble store hvite skiver ved døren. F3 begrenser nære lyskort og gamle støvkorn til fysisk små flater, uten å endre fjerne signalgløder, ankere eller tidsstyring. Dør, instrumenter og lokalt lys er synlig i sluttbildene. Eksisterende mølleffekter rundt lampene beholdes.

To faktiske fremkalte nattfoto og transitdokumentet er også åpnet. Nattfotoene er svært mørke; de beviser foto-/bevisløypa, ikke god motivlesbarhet. Fysisk Xbox, fri menneskelig gjennomspilling, lytting, full natt og helruteytelse er ikke verifisert. Eksisterende rødt RT-minnevarsel står fortsatt synlig. Ingen ny sky-, driver- eller global stabilitetsgaranti. T01/T02 ved trafostasjonen og skyggemannmekanikk er egne gjenstående oppgaver.

305 kopierte kildefiler er hashkontrollert i `/home/tom/Desktop/Signal47 feltmodeller 25okt`, med tre Blender-originaler og `Galleri.html`. Akseptanse: `Signal47/Tools/fieldintegration/acceptance.json`. Bevis: `F3-runtime.json`, `F3-visual-review.json` og kandidatens kilde-/pakkehash i bevisroten. PR156 og installasjonsresultat føres separat etter bekreftelse.

Kilder: Signal47/Tools/fieldstation, fieldinstruments, archiveprops og fieldintegration. Bevisrot: /home/tom/scratch-bm/Signal47-field-models. Prosjektbibliotekets historiske Blender MCP-profil ga ingen dokumentert løsning for denne produksjonen; prosjektets egne verifiserte eksport-/PBR-verktøy er gjenbrukt. Ingen tredjeparts råmodeller eller nye plugins er hentet.
