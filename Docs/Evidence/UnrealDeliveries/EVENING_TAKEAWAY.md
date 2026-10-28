> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Takeaway og samlet åpning, 24. oktober 2026

På `codex/evening-runtime` etter bil-/inngangsleveransen `5783f925`.
Takeaway-posen og kaffen er én porsjon med én lagret plassering.
Pakket spillkode: `f83f3dc9`, med core `92dc7a47` innflettet.
Alle åtte sluttkontroller består på samme Linux-pakke. Claude integrerer.
Nyere core `6614cd90` med den videre anomaly-piloten inngår ikke i den frosne
sluttpakken. En separat `git merge-tree` finner bare oppgavelistekonflikt, ingen
kildekonflikt; det er ikke et bygget eller kjørt integrasjonsbevis.

## Det spilleren gjør

- Se på den lille avsettingsplassen og trykk E: **Set down takeaway**.
  Faste plasser finnes på SAROs kontrollromsskrivebord, annexets arbeidsbord,
  dinerdisken, benken ved dineren og privatbilens panser/passasjersete.
- Se på posen og trykk E: **Pick up the takeaway bag**.
- Ved skrivebordet eller annexbordet tilbyr servietten **Eat supper (12 minutes)**,
  både når posen bæres og når den ligger der. Et firesekunders opphold lar
  tolv spillminutter gå. Papir, innpakning og tom kaffekopp blir liggende.
- Kamera, påslått lommelykt og åpne menyer/notatbok skjuler det som bæres.
  Etterpå kommer samme pose tilbake. Håndmodellen har ingen treff-/gangkollider.

Måltidet er valgfritt og tilbys etter D01, med plass i tidsbudsjettet og uten
pågående samtale eller viktig hendelse. Det kan ikke spise av finalens tid.
Førstegangsteksten forklarer at posen kan settes ned og måltidet vente til SARO.

Dinerens gamle hånd-/disktallerken viste først samme betalte porsjon to ganger.
Sluttkoden skjuler disktallerkenen mens porsjonen holdes; ordrebildet viser én
burger. Takeaway-posen erstatter deretter begge de gamle matvisningene.

## Tilstand og lagring

`evening.takeaway` har versjon 1, `mode` (`none`, `held`, `placed`, `eaten`)
og `spotId`. Kjøps-/dialogflagg forblir i den eksisterende pakken.
`FS47EveningPack::EatTakeaway()` setter `food_eaten` kun for én allerede
betalt og utlevert, uspist porsjon. Scenemarkør, ventende svar og dokumenter
endres ikke av håndteringen.

Eldre `food_taken` uten plassering migrerer til båret pose. Ugyldig eller
ukjent avsettingsplass gir posen tilbake til hånden. En allerede spist porsjon
kommer aldri tilbake. Bilplassene følger samme bilpose ved flytting og lasting.

Konsumering, sted og 720 sekunders klokkeøkning committes før lagringsforespørselen.
Overgangen er midlertidig og lagres ikke: en lasting under den viser det ferdige
måltidet uten å spille av tidshopp eller mat en gang til. Den vanlige klokken
fortsetter rundt tidshoppet; det er ikke et nytt fast klokkeslett som settes.
En full tolvminutters pause må passe før historiens neste reserverte grense.
Foto-/utskriftsstempler og historisk 1947-tid endres ikke.

Målene er små, håndplasserte servietter/flater. De har bare interaksjonssporing,
ingen fri fysikk eller nye gangbokser. Når de ikke kan brukes, slås query-kollisjon
av; tom prompttekst alene ville fortsatt blokkert andre mål i dette systemet.

Den gamle workTable-treffboksen gikk til høyde 1,05 m og omsluttet matmålene.
Den følger nå bordplaten som et 1 cm tykt plan ved 0,775 m. Originalt fotavtrykk,
ID og arkivhandling beholdes; en egen vanlig trace/bruk-kontroll åpner fortsatt
arkivpanelet etter måltidet. Gang-/bordkollisjon er uendret.

## Kontrollscenarioer

Kjør på ledig maskin med isolert `-UserDir`, minst 10 GiB tilgjengelig og
`Scripts/unreal_guard.py`. Ingen installed-game-grace brukes.

- `-S47Evening=burger -S47EveningTakeaway -S47SelfTest -S47EveningShots=<mappe>`
- Samme med `-S47TakeawayEatPlaced` for måltid fra bord og lasting under overgangen.
- Bil-/inngangskontroller fra [bilrapporten](EVENING_VEHICLE_FIXES.md).
- Full burgerrute: `-S47EveningRoute=burger -S47VoiceLevels`.

Oppsett, kameravinkler og dialogvalg i testene bruker API. Hver plassering,
opptak igjen, telefon og dokumentbruk må først treffe med vanlig blikkstråle
og fokus, og går så gjennom spillerens vanlige OnUse/HUD/interaksjonskjede.
Dette er ikke en påstand om menneskelig tastaturgjennomspilling.

## Resultater på sluttpakken

| Kontroll | Resultat |
|---|---|
| Editorbygg | PASS, 6.07 s |
| Linux cook/pakking | PASS, 111.04 s uten materialfallback |
| Selvtester | PASS, 1298 sjekker |
| Måltid fra hånden | PASS, 66.85 s; 16 fysiske bruksforløp, 10 disk-/verdenslastinger |
| Måltid fra bordet | PASS, 61.29 s; 17 bruksforløp, 11 lastinger inklusive under overgangen |
| Begge biler | PASS, 45.54 s; alle fem bilder visuelt kontrollert |
| Inngangsmatrisen | PASS, 174.44 s; 48 passeringer, 48 terskelavslag, 8 normale dørvekslinger, 4 ferske lastinger og 1 gammel terskelsave |
| Hel burgerrute | PASS, 256.47 s |
| Kort gammel historie | PASS, 3/3: residual/notatbok, nattetelefon og SARO 07-kjøring |

Begge matvariantene prøver alle seks avsettingssteder, vanlig telefon og
arbeidsordre mens posen bæres, og kamera/lykt/notatbok uten synlig håndpose.
De 21 ferske disk-/verdenslastingene sammenligner også faktisk overflateposisjon,
spillerposisjon, klokke, kjøpsflagg, ventende svar, scenemarkør og dokumenter.
Etter måltidet avvises nytt spisevalg uten ny klokkeøkning. Den siste
arkivkontrollen bruker et uttrykkelig kapittel-2-oppsett, og beviser at den
opprinnelige bordhandlingen fortsatt åpner panelet; det er ingen full
kapittelgjennomspilling.

Måltidet committer 720 sekunder én gang. Den uavbrutte overgangen bruker
fire vanlige spillsekunder i tillegg; varianten lastet under den har ingen
ny overgang eller gjentatt tidsøkning. Inngangskontrollene er den samme
før/etter-telefon, parkert/flyttet og levende/lastet-matrisen som i
[bilrapporten](EVENING_VEHICLE_FIXES.md).

Burgerruten bruker vanlig kjørekontroller og walker mellom mål, og API for
oppsett/dialogvalg. Betalt ordre med ventende svar og D01 lastes fra disk;
18:00-ankomsten skjer én gang og den gamle rutinen fortsetter. De tre korte
regresjonene bruker gamle hopp-fixtures. Den tidligere 6/6-regresjonen og
47-sekundersfinalen er historiske bevis på `66d2b5a6`, ikke seks nye
sluttkjøringer på denne pakken. Menneskelig tastaturgjennomspilling,
lytting og sammenhengende fullnatt er fortsatt **UNVERIFIED**.

Lydregresjonen har positive faktiske samples, peak og RMS etter kanalfilter: 10 ulike car-radio-klipp, 13 ulike physical-klipp, 13 ulike phone-klipp. Raske API-dialogvalg avbryter noen takes; dette er ikke en ny full lyttetest. Ingen lyd er produsert eller endret her.

## Bilder og bevis

Elleve grunnbilder for takeaway er visuelt gjennomgått, pluss bordvariantens etterbilde/arkivpanel, de fem bilbildene, inngangen før/etter telefon og rutebildene. Kameraoppsett bruker API. De raske dialog-/lasteoppsettenes transiente toastmeldinger ryddes kun ved opptak; HUD og faktisk prompt beholdes. Modalbilder tas med sanntidsticker fordi spillklokken står stille i paneler. Rutens telefonbilde er tatt før talestart; takeaway-scenarioets separate telefonbilde viser faktisk Dale-tekst. Skumringsbildet bruker et uttrykkelig fremoverkutt til 18:35, ikke 35 målte spilleminutter. De tre korte regresjonsbildene er også gjennomgått.

| Visning | Runtimebilde |
|---|---|
| Båret pose og kaffe | [PNG](/home/tom/scratch-bm/Signal47-takeaway/takeaway-held/takeaway-held-diner.png) |
| Satt ned i dineren | [PNG](/home/tom/scratch-bm/Signal47-takeaway/takeaway-held/takeaway-diner-set-down.png) |
| På SARO-skrivebordet | [PNG](/home/tom/scratch-bm/Signal47-takeaway/takeaway-held/takeaway-desk-set-down.png) |
| Annex før måltid | [PNG](/home/tom/scratch-bm/Signal47-takeaway/takeaway-held/takeaway-annex-set-down.png) |
| Papir og tom kopp etter måltid | [PNG](/home/tom/scratch-bm/Signal47-takeaway/takeaway-held/takeaway-eaten-annex-trace.png) |
| Etter lasting under måltidet | [PNG](/home/tom/scratch-bm/Signal47-takeaway/takeaway-placed/takeaway-eaten-annex-trace.png) |
| Telefon med posen skjult | [PNG](/home/tom/scratch-bm/Signal47-takeaway/takeaway-held/takeaway-phone-with-bag-stowed.png) |
| Notatbok med posen skjult | [PNG](/home/tom/scratch-bm/Signal47-takeaway/takeaway-held/takeaway-notebook-bag-stowed.png) |
| Originalt arkivpanel | [PNG](/home/tom/scratch-bm/Signal47-takeaway/takeaway-held/takeaway-original-work-table-panel.png) |
| Privatbilens egen dør | [PNG](/home/tom/scratch-bm/Signal47-takeaway/vehicles/entrance-vehicles-wagon-door-open.png) |
| Privatbilens førersete | [PNG](/home/tom/scratch-bm/Signal47-takeaway/vehicles/entrance-vehicles-driver-seat.png) |
| Plan SARO 07 | [PNG](/home/tom/scratch-bm/Signal47-takeaway/vehicles/entrance-vehicles-service-truck.png) |
| Inngangen før telefon | [PNG](/home/tom/scratch-bm/Signal47-takeaway/entrance/entrance-0-outside.png) |
| Inngangen etter telefon/lasting/flytting | [PNG](/home/tom/scratch-bm/Signal47-takeaway/entrance/entrance-7-inside.png) |

Kandidat: `/home/tom/scratch-bm/Signal47-takeaway/candidate/Linux/`. Pakket kilde er
`f83f3dc99ace2e6083d83d7375a46875670fe78c`. Nybygd spill-ELF er byteidentisk med den pakkede.
Alle seks ELF/pakkefiler har uendrede SHA-256 etter kontrollene. Rålogger,
21 matlagringer, PNG-målinger/hasher, faktisk lydnivå og visuell gjennomgang:
`final-audit.json`, `audio-regression.json`, `visual-review.json`,
`package-provenance.json` i samme evidensmappe.

Første bygg avviste navneskygging i testdriveren. Første runtime-prøve fant
den omsluttende bordproxyen; neste fullførte alle handlinger, men avdekket
at skjermbildet av et åpent panel ventet på den pausede spillklokken.
Disse feilene og den doble serveringsvisningen er rettet. Opprinnelige logger/bilder og den forrige komplette kontrollpakken er beholdt separat.
Avbrutte forsøk med konkurrerende Unreal-jobber teller ikke som PASS.
Alle godkjente sluttkjøringer er uten observerte konkurrerende jobber,
uten installed-game-grace og med minst 10 GiB tilgjengelig ved oppstart.
Ingen andres prosess er stoppet.

Prosjektbiblioteket ble søkt før implementering. Den generelle profilen
`repoer/prosjekter/quodsoler--unreal-engine-skills.md` er UNVERIFIED og ga ingen
spesifikk kompatibel løsning. Prosjektets egne interaksjons-/save-mønstre er brukt.
Repoets PUBLIC-status og ingen åpen PR for denne grenen er kontrollert.

Midlertidige Content-lenker er fjernet, original Kelvora gjenopprettet og
egne OpenOrder-endringer ryddet. Genererte modeller og lyd beholdes utenfor Git.
Bilrettelsene og maten leveres på samme gren, med oppgaveliste, minne og logg.
Ingen push, PR eller installasjon; Claude integrerer.
