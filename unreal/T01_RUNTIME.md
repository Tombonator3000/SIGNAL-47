# T01: Caliche-inspeksjon og retur til SARO

25. oktober 2026. Kilde `85aee47585c5201dabb5d8fef973f6b0c6ec606e` på `codex/t01-service-route`, fra levert core `f60d30eb`. R2 er akseptert innenfor det skriptede omfanget: 14 pakkede funksjonskjøringer og 11 faktisk gjennomgåtte originalbilder. Samme kilde og pakke er bevart gjennom prøvene. PR/merge/installasjon gjenstår ved dette loggpunktet; Q3 er fortsatt installert. H2.1 samlet er åpen fordi T02 og nødvendig kontorreferanse gjenstår.

## Spillerreisen

Nye kveldssaker får en lokal inspeksjonsoppgave etter Dales telefonoverlevering. Ta SARO 07 sørover til den merkede grusavkjørselen ved Caliche Junction. Les servicejournalen utenfor gjerdet, kjør den eksisterende tresekunderskontrollen og sammenlign det fullførte panelresultatet med den faktisk tente lampen. Returner til SARO og bruk skiftmappen i kontrollrommet én gang. Mappens vanlige visning åpnes; ingen ekstra bekreftelsesmeny er lagt til.

Rackstrøm og RX0 til RX1 kan gjøres før turen. Bare RX1 til den ubetjente nattsurveyen venter på arkivering. Deretter fortsetter den opprinnelige survey-/relé-/dip-/residualrekken, uten ny nedtelling eller tidshoppfunksjon. Caliche-kretsen, STATION 01, SAROs K3 og byforsyningen er separate systemer.

`serviceRouteVersion=1` velges eksplisitt for nye saker. Gamle, ukjente eller manglende versjoner får ingen ny sperre, og utviklerstarter etter åpningen beholder tidligere løype. `serviceFiled` valideres etter at den aktuelle sakens Evening-, Substation- og routine-data er lest. Arkiveringscredit uten komplett fysisk record og tilhørende forfatterfaktum avvises. Fakta fra en fysisk kontroll kan synkroniseres når en annen samtale slipper pakken, uten doble dokumenter.

Etter filing/Continue viser HUD neste faktiske mottakerhandling: gjenopprett RX bank 3, kalibrer på 1419.900 MHz eller notch highway-reléet nær 1420.1. Notch-målet tømmes når Survey begynner. Dette gjelder den nye, validerte ruten i Shift; legacy og senere kapitler får ingen ny målskriver.

Eksisterende modeller, kontroller, lagringsformat og opptak brukes. Ingen nye assets, opptak, spillerautopilot, T02, kontor, VHS eller stealth inngår.

## Faktisk aksept

Bevisrot: `/home/tom/scratch-bm/Signal47-qa-fixes-20261025`. R2-pakke: `candidate-t01-r2/Linux`. Binær SHA-256: `81af1808a19e89b805de8093a4863d9005345d24c613cb96039024e8dd710de9`.

| Kontroll | Resultat | Faktisk omfang |
|---|---|---|
| Editor04 og Linux package02 | PASS | 7,95 s editorbygg; BuildCookRun102,30 s, exit0, ingen cook-materialfeil. |
| Pakket selvtest | PASS | 1 424 kontroller, smoke og kontrollromsbilde. |
| Pakket API/disk | PASS | 53 kontroller og 19 faktiske store-leser/full-Flow-restores, 92,10 s, exit0. D01/RX/journal/kontroll/lampe/mappe, pause, idempotens, ugyldige/eldre saker og opprinnelig videre tidslinje. Felt-/truck-/returposisjoner er fixtures. |
| Pakket native-reise | PASS | 10 kontroller, 932,3 m ut, 966,7 m retur, 55,4 m gange. Faktisk vestlig utstigning, dør, rom og én fysisk arkivering; 297,38 s veggtid. Ingen flytting eller omlasting etter START. |
| Ferske Continue-prosesser | PASS | Sju vanlige nye prosesser: journal, observert resultat, filed-RX0/RX1, ekte gammel kapittel5 og to tilstander fra den faktisk kjørte R2-turen. Originale lagrings-/fotobytes, normal load, faktisk minuttklokke og bilder kontrollert. |
| D01 med mat | PASS | Eksisterende Takeaway-driver, reelle brukshandlinger og lagringer. Etter-D01-data har route1, filing=false og korrekt matstatus; diner-/parkering-/posisjonsfixtures er uttrykkelige. |
| High/Low og Large text | PASS_SCOPED_T01_VISUAL | Elleve nye originaler gjennomgått. Hele avreisemålet, tre T01-oppgaver med riktig avkrysning, ferdig lampe, returinstruks, legacy og alle tre rutinemål er leselige. |

`T01_VISUAL_REVIEW_R2.json` binder alle 11 bildehash. `T01_VISUAL_REVIEW_R1.json` forblir FAIL med uendrede originaler. `t01-r2-final-binding.json` bekrefter samme 40 payloadfiler før/etter alle 14 kjøringer. Dette er ikke en generell ytelses- eller menneskelig spillaksept.

Native-kontrollen bruker eksisterende navngitt tur/autopilot, kjørekontroller, områdeoverføring, parkering og Walker-kollisjon/gulv. D01, målvalg og én innledende spillerplassering er fixtures. Truck-ray prøver tilgang; påstigning velges via API. Testen gjør ingen Place/Enter/MoveDriven/MoveParked eller mellomlasting etter START. Den beviser ikke en menneskelig `TruckChoice -> Roam`-tur, fysisk Xbox, lyd eller opplevd varighet. Testtid er ikke målt spillerpacing.

Den primære kontrakten leser ekte filer med en ny FS47SaveStore og starter full Flow på nytt i samme prosess. De separate Continue-prøvene starter derimot vanlige nye prosesser fra tittelen uten kontrakt-, case-, kapittel- eller Evening-fixture. Beholdte bytes, normal innlasting, faktisk minuttklokke og originale PNG-er kontrolleres. Generell in-memory-dump finnes ikke; persisterte felter omtales ikke som nye minneavlesninger.

`t01-frozen-source-r2.json` og `t01-tested-source-binding-r2.json` binder 1 839 filer til testet commit. `candidate-t01-r2-payload.json` og full snapshot binder 40 payloadfiler med bytes/moduser. De faktiske observed-/filed-storene beholder identisk lokalt resultat og fullføringstid, mens arkivering går fra false til true. Genererte OpenOrder-logger er bevart i scratch og tatt ut av kildeendringen.

## Feilene som faktisk ble funnet

Første native-editorreise bestod felt og returparkering, men en fast østlig ganglinje etter vestlig utstigning traff trucken. Testen velger nå faktisk utstigningsside og går fysisk rundt; ingen flyttefallback ble lagt inn. Editorbygg02 hadde en lokal variabelskyggefeil, rettet før bygg03.

R1 bestod funksjonskjøringene, men11 originale bilder avdekket tre tomme HUDmål etter filed-Continue. `T01_VISUAL_REVIEW_R1.json` er derfor FAIL og beholdt. `85aee475` retter den konkrete mangelen; tre nye R2-bilder viser korrekt strøm-/kalibrerings-/relétekst uten kutt. R1 er ikke retrospektivt omdøpt til PASS.

To scratch-postchecks antok UTF8 for native UTF16/BOM-eksporter og avviste dem. BOM-bevisst lesing ble rettet, originale feilrapporter bevart og ekte filbytes urørt. SaveStore-filene er UTF8; eksportene er ikke kopiert inn som saves. Dette var parserfeil, ikke en ny spillets lagringsfeil.

## Levering og det som står åpent

PR/merge/installasjon og kontrollert pruning føres etter faktisk utførelse. Ingen ny hel 110 er kjørt for T01. Q3s 110/110 gjelder Q3; R2 bruker målrettede kontroller av endret oppgave-, lagrings- og UI-flyt mot core. Ingen nye renderer-/materialendringer eller VSM-ytelsespåstander inngår.

Menneskelig Roam-navigasjon, mus/Xbox-komfort, lytting og ubrutt fullnatt står åpne. H2.1b/T02 trenger fysisk kontor/nøkkel/feltmappe og fire faktiske feltkontroller. Ny original 72s Dale-VHS krever egen produksjon og nonloop pause-/strøm-/save-transport. Eksisterende STATION01/Nora/K3/C og hovedrute med uavklart bevis skal bevares. Se fortsatt åpne punkter i [todo.md](todo.md).

Prosjektbiblioteket ble søkt først og én gang bredere. Den leste `ChiR24/Unreal_mcp`-profilen oppgir UE5 til5.8 og MIT, men lokal installasjon/bygg/runtime er UNVERIFIED. Ingen kode eller plugin derfra ble gjenbrukt; prosjektets eksisterende systemer er grunnlaget.
