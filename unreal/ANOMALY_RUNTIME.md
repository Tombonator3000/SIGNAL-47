# Første spillbare anomaly-pilot

Arbeid på `codex/anomaly-pilot`, fra `a4626053`. Kveld og tidslinje 2 er grunnlaget. Claude integrerer; ingen push, PR eller installasjon inngår i oppdraget. Kilde, editorbygg, pakket kandidat og installert spill er forskjellige leveranser.

## Spillerreisen

Etter Dales telefonoverlevering kan spilleren lese SAROs skiftmappe og hente det eksisterende feltkameraet. Mappen har de daterte serviceopplysningene og sendeplanen fra EveningShift. N01 er ordinært arbeid før residualen. Etter signalet legges disse kildene ved siden av den faktiske mottakerutskriften; spilleren vurderer hva rekkefølgen støtter og etterlater en forespørsel om inspeksjonsresultatet ved den fysiske mappen. En tidligere klage beviser ingen felles årsak.

En valgfri referansetur bruker SARO 07 og den eksisterende kjøreverdenen. Normal kjøring flytter ikke kapittel eller klokke til All Night. Ved vaskeriet i Caliche kan en opallampe og en blek plate først ligne en person. Spilleren fotograferer, flytter ståsted, undersøker braketten og tar et nytt bilde. Sidevedsidekontrollen kan forklare den vanlige silhuetten. En utilstrekkelig dokumentert vurdering kan føres som uavklart og korrigeres senere.

I suvenirvinduet står én fysisk pappfigur med gul prisetikett og splitt i nedre kant. Referansebildet tas med det vanlige kameraet. Etter midnatt kan samme figur ha en annen plassering; varianten skifter bare utenfor nært syn, med lukket panel og fri scene. Den nye eksponeringen sammenlignes med den lagrede kveldsfilmen. Identitet og posisjon kan etterprøves; hvem som flyttet figuren forblir ukjent. Spilleren følger opp ved å undersøke detaljene fysisk.

Etter bildesammenligningen kan en kort kjent printerrytme høres fra det stengte radiostudioet. P01 venter på lukket panel og fri scene etter Earls normale slutt 04:00. En tekstet lydobservasjon støtter spillere som ikke hører klippet. Fasaden kan undersøkes, men lyden beviser ingen ny person og åpner ingen samtale.

Notatbokens funn åpner de samme undersøkelsespanelene. Vurderinger kan revideres uten å samle samme bevis på nytt. Bildene bruker albumets lagrede bildepiksler, originale eksponeringstid og sted. Instantfilmens eksisterende 90 sekunders fremkalling beholdes; et åpent panel pauser klokka, og et ufremkalt bilde gir tydelig beskjed.

## Grenser og opptrapping

Kvelden beholder normal belysning og den kontrollerbare lampesilhuetten. De gamle store telefon-, bil- og lysopptrinnene avbryter ikke referansefasen. Etter midnatt kommer den flyttede figuren, og etter fire det avgrensede studioetterspillet. Dette bruker felles hendelseskoordinering; ingen ny tilfeldig skremmemotor innføres.

Lysbølgens 04:55-grense, Earls slutt 04:00, P13 og finalen 05:29:00 til 05:29:47 er bevart. Ingen anomalikvote sperrer kartarbeidet. 1420.405 MHz, 27 antenner, az 026, 1947-materialet og skillet mellom A og C endres ikke. Ingen designregel er bøyd for sterkere skrekk i denne piloten. B-12, mørkerommet og senere filmhendelser fullføres ikke av det tidlige kameraet.

Trafostasjon, Dales kontor, VHS, nye bygningsskall og de senere UFO-/aliensekvensene tilhører planens senere leveranser.

## Lagring og gamle saker

Alle nye undersøkelsesfelt ligger i `anomaly` i eksisterende save v3, med egen utvidelsesversjon 1. Foto-ID-er, klokker, variant, eksponering, kontroll, vurdering, valgt N01-handling og etterspill lagres separat. N01s lokale manusstatus bevarer også et feilvalg og et ventende revisjonsvalg; ferdige vurderinger speiles gjennom kveldspakkens egne scener når den er ledig.

Arkivets medfølgende 17:10-bilde er selv en faktisk runtime-eksponering. JPEG, kildehash, ståsted, bildekoordinater og eksponeringstid finnes i `Signal47/Content/Anomaly`. Den lagrede saken bevarer denne bildekilden som en vanlig ID/url/clock-fotopost; en senere programoppdatering kan ikke erstatte den med dagens referanse.

Det nye `routine`-kontrollpunktet bevarer skiftets faktiske klokke, mottakerinnstillinger, strøm/relé, dokumenter, notater, kamera og posisjon før surveysekvensen starter. Den tidsstyrte surveysekvensen bruker fortsatt sitt eksisterende residualkontrollpunkt. P01s korte lyd fullføres før en ny lagring kan skrives.

Saker uten anomaly-data får ingen oppdiktede eksponeringer eller fullførte kontroller. En datert arkivreferanse støtter et sent førstebesøk. Gammel `ev:cutout` beholdes som hendelseshistorikk; figuren spilles ikke av igjen eller omgjøres til en løst A02. N01/N02 og et ærlig uavklart resultat gir tilgjengelige kontroller. Gamle saker beholder tidslinje 1 og sine historiske tider.

## Verifisering

Status oppdateres med faktisk bygg og runtimebevis før levering. Et kompilert system er ikke bevis på at en spiller oppdaget eller forstod endringen.

| Kontroll | Status | Grunnlag |
|---|---|---|
| Første editorbygg | PASS | 181 handlinger, 143,58 s; før siste innfletting av stemmeruntime. |
| Samlet editorbygg etter core-merge | PASS | Bygg 4, deretter inkrementelle rettelsesbygg. Sluttbygg kontrolleres før pakking. |
| Editorpilot, ekte eksponeringer og disk-/verdenslasting | PASS | Editor-2: hel N01/N02/A02/P01-rute, fire ekte fotografier, åtte disk-/verdenslastinger og korrigert N01-vurdering. 1271 selvtester på dette inkrementet. |
| Pakket pilot | UNVERIFIED | Sluttpakking og retest gjenstår. |
| Referanse, avvik, sammenligning og notatbokbilder | UNVERIFIED | Krever faktisk runtimeopptak og visuell kontroll. |
| Sen ankomst, migrering og tidligere hendelse | UNVERIFIED | Separate testscenarier implementert. |
| Kort gammel kapittelregresjon og Continue | UNVERIFIED | Sekvensielle pakkekontroller implementert. |
| Ekte menneskelig input, lytting, forståelse og stigende uro | UNVERIFIED | Krever menneskelig gjennomspilling og planens femspillertest. |
| Full natt uten testkutt og faktisk total spilletid | UNVERIFIED | Eget senere krav; pilotens fixturehopp beviser ikke dette. |

Testverktøyet er `Signal47/Scripts/anomaly-pilot.py`, med `--scenario all` for piloten og migrering eller `--scenario regression` for det korte kapittelutvalget. Det krever isolert profil, ledig maskin og minst 10 GiB tilgjengelig. Det starter aldri mot installert spill og signaliserer bare egne prosesser ved avbrudd. `-S47AnomalyRoute=pilot|late|legacy|already-seen|missing-phone` og `-S47AnomalyShots=<mappe>` gir uttrykkelige kontrolltilstander og bevis. `-S47AnomalyState=routine|n02|reference|records|anomaly|comparison|p01|notebook` holder hver relevante tilstand for inspeksjon etter den samme bevarte beviskjeden; bruk en ny `/tmp`-profil. `--scenario archive` lager den særskilt merkede dagsoperatørkilden ved faktisk 17:10.

API-handlinger, testposisjoner og tidskutt merkes i `events.json` og generert minuttabell. De fire påkrevde bildene er HUD-opptak; `reference-photo.jpg` kommer fra en faktisk kameraeksponering. Ingen av delene betegnes som menneskelig tastaturinput.

## Kilder

Prosjektbiblioteket ble søkt først og deretter én gang bredere. EngineWire-profilen oppgir UE 5.0 til 5.8, MIT og UNVERIFIED installasjons-/teststatus. Den gir ingen relevant undersøkelsesmodell, og ingen kode ble gjenbrukt. Prosjektet bruker lokal UE 5.8.4 og egne eksisterende systemer: Chapter1/FieldCamera/Album, Flow/SaveStore, EveningShift sine dokumenter og scener, Town/TownEvents og notatbokens funn. Synlighet ble kontrollert som PRIVATE; ingen synlighet ble endret.
