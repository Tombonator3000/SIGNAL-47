# MESA DINER: selvstendig modul og runde 8

Bygd etter kontrakten i `todo.md` og `ART_BRIEF.md` på `1a6b1f9`, levert til main gjennom PR #44 (`fb08526`). Dineren i «All Night» skal brukes etter Roswell-veien. Dette er en isolert støtteleveranse; Claude eier innkobling, historien, samtalene, lagringen og én samlet integrasjonstest.

## Åpne leveransen

Fra `web/`, med prosjektets avhengigheter installert:

```sh
python3 tools/dinerpreview.py --port 8485
```

Forhåndsvisningen bruker en privat inngang og Vite-konfigurasjon under `/tmp`. Den ordinære spillinngangen og publiseringskonfigurasjonen er bevart.

Ni faste utsnitt er tilgjengelige: Arrival, Outside, Counter, Booth, Phone, Clipping, Menu, Sign og Worst. Det siste bruker et diagnostisk kamera bak disken, utenfor kundens gangsone. De andre er ståhøye utsnitt fra radiusklare steder. High/Low, skiltbryter, grålysning, dørblad og Recreate kan prøves i previewen. `--fallback` viser reserveflatene uten runde 8; privat forhåndsvisning på 8486 er også tilgjengelig for dette.

## Innkobling for Claude

- Last `DINER_ART` med `loadArtFor(DINER_ART)` før `new Diner(new THREE.Vector3(-8000, 0, 0))`. Modulen bruker eksisterende `artImage()` og delte `artTexture()` for skilt, meny, avisfoto, rutegulv og mintlaminat.
- Legg `group` i scenen. Soner, kollidere, treffflater og ankre oppgis i verdenskoordinater, forskjøvet med origo. Nord er -Z; gangflater ligger på y=0.
- Sonene heter `lot`, `walk`, `door`, `inside` og `phone`. Treff-ID-er: `door`, `counter`, `coffee`, `waitress`, `driver`, `menu`, `clipping`, `payphone`, `jukebox`, `window`, `booth`, `sign` og `rig`.
- Ankre: `truckPark`, `arrive`, `outside`, `inside`, `stool` og `phone`. `truckPark` er plassen for SAROs lastebil, med nesen mot nord. Dinerens semitrailer er egen kulisse.
- Egne objekter: `door`, `coffeePot`, `cup`, `clipping`, `payphoneHandset`, `waitressHead` og `driverHead`. Kapitlet styrer dørblad, kopp, håndsett og hoder.
- Kall `setSignLit(on)`, `setDawn(k)` og `update(dt,t)`. `setDawn` bruker 0 for natt og 1 for grålysning. Eget `dinerFlood` bruker høyst ti plasser, og gruppen har ett eget halvkulelys. Ingen andre ekte lys eller skygger.
- Runde 8 leveres som kildefiler. Claude utvider `art.ts` med de to nye bildene når området kobles inn. Foreslåtte ID-er er `dinerBooth` og `dinerWall`. Kall `setSurfaceArt(booth,wall)` etter konstruksjonen med sRGB-teksturer i RepeatWrapping, henholdsvis repeat `[2,2]` og `[0.5,0.5]`. UV-ene er i meter. Metoden beholder eksternt eierskap og oppdaterer begge kvalitetsvariantene; modulen endrer eller disponerer aldri disse samplerne. Previewen laster dem privat fra de leverte filene. Uten kall brukes de enkle reserveflatene.
- `dispose()` frigir bare modulens egne ressurser. Delte teksturer fra bildebufferen disponeres aldri av dineren. Bare én levende modul skal eie `dinerFlood`.

Treffflatene er usynlige mesh på lag 31 med offentlige grupper på lag 0 som videresender raycast. Registrer gruppene i eksisterende `Interaction`. Ved deaktivert område må kapitlet deaktivere interaksjonene, for eksempel ved å sette de offentlige gruppene på lag 31. Bare `group.visible=false` er ikke en interaksjonssperre. Ingen endring i kjernens Raycaster kreves.

`objs.door` er en hengselgruppe, lukket ved Y=0 og åpen ved Y=+π/2. Previewens gangruter åpner det faktiske bladet først. Claude styrer dørtilstand og eventuell bevegelig dørkollider i kapitlet. De statiske veggkolliderne lar selve åpningen være fri. Sett spillerens bakkehøyde til 0 i området.

Skilt, fem menyretter og forside tegnes i kode. «PECOS VALLEY SENTINEL», ranchens navn og avisfotoet er oppdiktet. Forsiden har ingen eksakt dato; «July '47» står som blyantlapp på rammen.

Skiltbildet inneholder stolpene. Det brukes som to separate 4×2 m forsideplan ved y=0,734, så føttene lander på bakken og teksten leses riktig fra begge sider. Ingen ekstra synlige stolper legges til. Skiltets slukkede tilstand demper albedo og fjerner både emisjon og flomlys; de røde og cyan fargene finnes fortsatt i den originale bildeteksturen.

## Runde 8

To JPG-er, RGB/sRGB/JPEG85: `diner/tex_booth_vinyl.jpg` i 512×512 og `diner/tex_wall_panel.jpg` i 1024×1024. Innebygd ChatGPT-bildegenerator er brukt. Hver flate fikk én generering og én rettelse, deretter helbildeskalering og eksport. Eksakt modellversjon er ikke eksponert. Fulle prompts og byteidentiske valgte kildekopier står i `src/assets/art/production/round8-2026-10-04/manifest.json`; se `ART_ROUND8_DELIVERY.md`.

Begge flater er inspisert i vanlig 2×2-repetisjon. Kunstskinnet er visuelt godkjent. Panelet har ingen vesentlig tonal skjøt, men små fiberavvik ved vertikal repeat er dokumentert; fullstendig sømløshet er ikke påstått. Ingen ekstra grafikk er produsert utover disse to bestilte flatene.

## Kontrollstatus

Sluttkilden til `Diner.ts` har SHA-256 `44f9f2c81af027ab8a4721b41589f463e8c22a1369e66c78e60ef24ef2b1352b`. Rapporter og uredigerte bilder ligger i `evidence/diner-2026-10-04/`.

- PASS: TypeScript uten emit og Python-syntaks etter veggrettelsen. 79 eksisterende kode-, test- og kjernedokumentfiler er byteidentiske med `fb08526`. Ingen endring i World, art-loader, hovedinngang, kapitler, UI eller eksisterende tester.
- PASS: 37/37 kontraktkontroller med runde 8, 38/38 etter Recreate. Alle 13 samtidige `Interaction.pick` prøves fra lovlige, radiusklare steder med vanlig 2,4 m rekkevidde. En inert disk maskerer ikke kaffekannen. Tre ruter med ekte `Player.update` prøves begge veier: arrive via outside/inside til stool, inside til phone og stool til menyen.
- PASS: ni utsnitt × High/Low × 1280×800/844×390, totalt 36 sluttbilder. Filstørrelser, pikselmål, viewport og filhash er kontrollert. Ingen fangede konsollfeil eller sidefeil. 19 til 54 tegnekall, også i det diagnostiske Worst-utsnittet. Den siste headless-konstruksjonen tok 69,9 ms; tre Recreate-runder tok 67,8, 42,0 og 23,6 ms.
- PASS: reserveflate uten runde 8, 36/36 og 37/37 etter Recreate. Ressursrydding av 67/67 geometrier, 55/55 materialvarianter og 7/7 observerte lerretsteksturer. Med runde 8 prøves 5/5 gjenværende kart i materialene; de to utskiftede reservekartene forblir eid og ryddes av modulens egen teksturliste. Delte bildesamplere er ikke disponert. Tre Recreate-runder gir stabilt antall GPU-ressurser.
- PASS: uavhengig visuell review av alle 36 sluttbilder, inkludert 19 enkeltbilder i full størrelse. Root har i tillegg inspisert den korrigerte fasaden og de faktiske runde-8-flatene i innlogget lokal nettleser. Menypriser, avisfoto, skiltføtter og riktig tekst på begge skiltsider er kontrollert. `visual-review.json` registrerer omfang og begrensninger.
- PASS: to nye eksporter, sRGB/JPEG85, kilde-/eksporthash og 27 tekniske readback-kontroller fra grafikkleveransen. Root har inspisert begge vanlige 2×2-ark. Det er ikke påstått fullstendig sømløshet for panelet.

Visuell kontroll fant først ekstra skiltstolper og deretter en avisramme foran glasset. Skiltet bruker nå bare bildets stolper. Fasaden har et fullt panelveggfelt ved z[0,6,2,15], og den søndre vindusrekken begynner ved 2,15. Avisen står på denne veggen; soner og ankre er bevart. `review-before-clipping-wall.jpg` er uttrykkelig historikk fra forkastet geometri. De 36 `preview-*`-bildene viser sluttversjonen.

Previewens Phone-kamera er så nært at topp og bunn av apparatet kuttes. I mobil-Clipping ligger toppoverskriften delvis bak verktøylinjen. Disse bildene dokumenterer derfor ikke hele telefonen eller hele avisoverskriften. Claude bør kontrollere begge fra flere normale ståpunkter i sin integrerte spilltest.

IAB-observasjonen viste 27,7 ms etter omlasting og 1159,9 ms ved den første kalde, tidligere revisjonen. Disse og headless-tidene er målinger fra ulike kjøresituasjoner, ingen ytelsesgaranti. CUA-bildene har DOM-viewport 1280×800, men selve opptaket er 1280×720; de brukes bare som ekstra visuell review. De 36 ordinære kontrollbildene har de bestilte pikselmålene.

Uendrede grønne kapitteltester gjenbrukes. Fullspilltesten gjentas av Claude etter innkobling. Ekte telefon, berøringskontroller, FPS, Safari og lyd kan ikke verifiseres av denne isolerte forhåndsvisningen.
