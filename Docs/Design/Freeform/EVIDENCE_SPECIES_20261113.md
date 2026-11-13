# Feltbevis som skiller de ukjente (Freeform)

**Status:** Uavklart (forslag til Tom). **Gjelder:** Freeform, feltarbeid, notatboka, arkivpermen og Wards vurdering. **Sist kontrollert:** 13. november 2026 mot kilden på `claude/freeform-20261112` (lest, ikke bygget eller spilt).

Toms idé 13. november: bevis av ulike slag fra feltet (bilder, geiger, målinger, prøver) logges og legges i arkivet med bilder og tegninger, slik at spilleren gradvis forstår hva slags vesener han har med å gjøre. Inspirasjon er løkka i UFOPHILIA (måle → utelukke → fotografere). Vi tar bare den generelle ideen om å utelukke ut fra bevis. Arter, navn, bevistabell, grensesnitt og ordlyd lages fra det spillet vårt allerede har.

Gjeldende regler som dette må følge: [DECISIONS](../../Active/DECISIONS.md) (tidsnivåer 9. november, Freeform etter Story, vev inn det som finnes), [milepælene](../../Active/TODO.md) og [game bible](../GAME_BIBLE.md) (vanlige forklaringer kan være riktige; et avvik fastslår ikke art eller hensikt).

## 1. Mål og løkke

**Mål.** Det spilleren finner om natta, skal samle seg til egne mapper for hvert ukjent vesen, slik at han over 30 dager selv ser at det er flere slag og hvordan de skiller seg fra hverandre. Spillet gir aldri et navn, en opprinnelse eller en forklaring. Spilleren setter sammen bildet, gir hver mappe sitt eget navn, og kunnskapen gjør nettene tryggere og teller hos Ward.

**Løkka i fem steg:**

1. **Merk:** noe skjer ute (lyd på walkie, spor, lys, dyr som blir stille, et glimt).
2. **Sikre:** ta bilde, ta opp på bånd, les geigeren, se på bakken, se hva den gjør når lykta treffer.
3. **Logg på stedet:** et kort stempel (toast) og en linje i notatboka med det samme, i riktig mappe hvis spillet vet hvilken, ellers som «ukjent tilhørighet».
4. **Sammenlign:** i notatboka (TAB) eller i permen på SARO viser mappene hva som passer og hva som er utelukket. Spilleren legger løse funn i en mappe eller åpner en ny.
5. **Konkluder:** når en mappe har nok ulike bevis, gir spilleren den et navn og arkiverer den. Det gir en linje hos Ward, litt betaling under ukas tak og en vane som hjelper resten av måneden.

## 2. De ukjente og hva som skiller dem

### Kinds i koden (arbeidsnavn, vises aldri i spillet)

| Arbeidsnavn | I koden | Kan møtes | Mappe |
| --- | --- | --- | --- |
| De små | `ES47AlienKind::Roswell`, besøkskode 0, `ES47Taker::Roswell` | Stedsbesøk etter mørkets frembrudd, dørbesøk, innendørs titting; jakter etter 38 s (`S47AlienKinds.cpp:175`) | Ja |
| Den lave | `Pale`, besøkskode 1 (krabbekroppen), og veiglimtets «crawler» | Stedsbesøk når som helst etter mørket, mest på åpen mark (`S47FarmSite.cpp:185`); bryter stillhetsregelen fra natt 4 etter 02 (`S47FarmSite.cpp:716-737`) | Ja |
| Den høye | `Tall`, besøkskode 2, og veiglimtets «tallfigure» | Bare fra 03:00 (`S47AlienKinds.cpp:200`, `S47FarmSite.cpp:185`) | Ja |
| Sølvmannen | `Nordic`, bare figur (`VisitCode` −1, `S47AlienKinds.cpp:192`) | Høydedrag, til fots, i lyskjeglen, over veien; figur nær fra uke 2 | Ja, men stopper ved TREKK |
| Den som lyser | `Cosmic`, bare figur, egen glød (`S47Happenings.cpp:1687`) | Som sølvmannen, sjeldnere (vekt 0,08) | Ja, men stopper ved TREKK |
| Skyggen | `""`/`"shadow"`, nesten svart (`S47Happenings.cpp:1582`) | Standard figur på høy- og lav-netter | **Nei.** Forblir uløst under SHADOWS |

Mannen med hatten ved pumpa er humor og holdes utenfor.

### Matrise: hvert vesen × seks bevistyper

**F** = finnes og skiller allerede i koden. **U** = liten utvidelse (samme system, ny variant per vesen). **–** = gir ikke noe (det er også et spor).

| | BILDE (form) | BÅND (lyd) | BAKKE (spor) | LYKT (reaksjon) | GEIGER | DYR |
| --- | --- | --- | --- | --- | --- | --- |
| De små | Liten, stort hode, ofte flere (1–6, `S47RoswellGroup.cpp:142`) **F** | Tonestiger som stiger og ender i et spørsmål, pause etter (`S47AlienKinds.cpp`, `SynthRoswell`) **U** på feltbåndet | Små bare fotspor, steiner snudd med fuktig side opp (bibelen, stedet) **U** | Trekker seg unna lykta; lykt av og stå stille, så ser den forbi deg (`S47FarmSite.cpp:698-705`) **F** | Svak økning **U** | Hundene tier (`dogs_hush`) **U** |
| Den lave | Lav, foldet, blek, rask (`S47FarmSite.cpp:586`, `:741`) **F** | Dobbelt puls som et hjerte, raspende pust (`SynthPale`) **U** | Avtrykk i firer, tett, utfor kanten i løp (`S47RoadGlimpse.cpp:116-121`) **F** på vei, **U** på stedet | Bryr seg ikke om lykta; fra natt 4 etter 02 stopper den ved deg selv om du står stille (`S47FarmSite.cpp:716-737`) **F** | Ingenting **U** | Kuer og coyoter flykter (`animals_flee`) **U** |
| Den høye | Høy, lang, langt unna, sitter iblant (`S47FarmSite.cpp:547-553`) **F** | Dype langsomme pulser, to toner som slår mot hverandre, metall som knirker (`SynthTall`) **U** | Lange smale avtrykk, et langt steg mellom (`S47RoadGlimpse.cpp:116-121`) **F** på vei, **U** på stedet | Fryser i den første lyskjeglen; «shh» i lyset innen 12 m (`S47FarmSite.cpp:556-567`) **F** (ny finding **U**) | Klar økning **U** | Fuglene letter (`S47AnimalWarn` Birds) **U** |
| Sølvmannen | Menneskelik, for stille, på høyden (`S47Marginalia.cpp` «THE SILVER MAN») **F** | Rene akkorder som sakte går ut av stemning (`SynthNordic`) **F** på mottakeren | – | Borte når den blir sett **F** | – | – |
| Den som lyser | Lyser innenfra, «stjerner inni» **F** | Pip i grupper som teller, så et svar (`SynthCosmic`) **F** på mottakeren | – | Synes i eget lys (`S47Happenings.cpp:4574`) **F** | – | – |

Kontroll: ingen to rader er like. Sølvmannen og den som lyser deler bare tomme felt og skilles på bilde og bånd. De tre som kommer nær, skilles på hver kolonne.

**Ingen ny mekanikk:** alle kolonnene er systemer spilleren allerede bruker (kamera og mørkerom, kassettspiller, dagslyslesing av stedet, lykta, geigeren, dyrevarslene). «U» betyr at systemet får en variant per vesen der det i dag er likt for alle.

Fysisk prøve er bevisst utelatt fra matrisen. Fragmentet og jordprøven har fast svar «origin remains unassigned» (`S47FreeformWorld.cpp:1094-1096`, `S47Happenings.cpp:5321-5355`), og et mikroskop skal ikke bli en artsidentifikator (bibelen). Tapt tid er også utelatt, fordi Freeform ikke skal forklare hvordan noen blir borte.

## 3. Logging uten tur tilbake til en datamaskin

### Der beviset tas

- **Stempel:** kort toast med bare mappe og bevistype, for eksempel `UNKNOWN 2 — GROUND`. Aldri en egenskap eller et navn.
- **Notatboka (TAB) med en gang:** KNOWLEDGE › Who & what › THEM får mappene, side om side med blyantnotatene som finnes (`S47Marginalia`). Mappa er alltid med i lomma. Hele poenget er at spilleren aldri må til SARO for å sammenligne.
- **Funn uten kjent tilhørighet** (spor neste morgen, bånd tatt langt unna, viltkamera) havner under `UNPLACED` og kan legges i en mappe der og da.

### I arkivet (permen på SARO)

`S47RecordsBinder` får skilleark **UNKNOWNS** etter FIELD + BENCH. Der ligger hver mappe som en ark-dossier med:

- spillerens egne bilder (albumutskrifter knyttet til mappa, forstørrelser foran),
- en blyanttegning som spillet tegner selv i samme stil som dokumenttegningene (`S47Drawings`). Spilleren kan ikke tegne i dag, så tegningen fylles ut med det han faktisk har sett,
- målinger og båndlinjer med dato og sted.

Permen er stedet for å **arkivere** en mappe (gi navn, sende til Ward). Å lese og sammenligne kan gjøres overalt.

### Mappa fylles i tre trinn

| Trinn | Krav | Hva tegningen og teksten viser |
| --- | --- | --- |
| OUTLINE | 1 bevistype | Bare en form eller en lyd: et omriss langt unna |
| MARKS | 3 bevistyper fra minst 2 netter | Høyde, gange, spor og lyd |
| HABITS | 4 bevistyper der LYKT er én av dem, fra minst 3 netter | Én praktisk vane i blyant: hva den gjør med lys, stillhet og lyd |

### Utelukking vises, spilleren konkluderer

- Hver mappe har seks bokser i en rad (BILDE … DYR): fylt, tom eller satt til «–» når spilleren har sett at den ikke gir noe.
- Når et løst funn åpnes, får hver mappe et blyantmerke: **passer** (✓), **strider mot det du har** (strek), **ukjent** (?). Merkene sammenligner bare med det spilleren allerede har funnet. Databasen er hans egen og ikke en ferdig liste.
- Spillet godkjenner ikke tilordningen med en gang. Et feil plassert funn blir strøket når et senere funn motsier det, slik `StruckBy` virker for blyantnotatene. Bekreftelsen kommer samlet når mappa når neste trinn (lærdom fra Obra Dinns tre om gangen: samlet tilbakemelding gjør gjetting dyrt).
- Konklusjon: når mappa er på HABITS, kan spilleren skrive et navn (egen tekst, maks 24 tegn, ordsjekk) og arkivere den (`FILE AS ONE`). Spillet sier aldri om navnet er «riktig». Det sjekker bare at bevisene i mappa ikke strider mot hverandre.

### Følger (merkes og leses av andre systemer)

- **Ward:** ukenotatet nevner antall atskilte mapper med nok bevis («Keep them apart. Three separate things, by your own notes.»). I 30-dagersvurderingen gir arkiverte mapper poeng (se spørsmål 5).
- **Penger:** første gang hver bevistype kommer i en mappe, betales den som feltregistrering under ukas felles tak på $35 (ingen ny pengestrøm). En arkivert mappe gir $5 én gang.
- **Tryggere netter:** når en mappe er på HABITS, kommer vanen som tanke og som linje i notatboka når dette vesenets tegn høres eller ses igjen:
  - de små: «Lamp off. Keep still. It looks past you.» (regelen finnes, `S47FarmSite.cpp:698-705`)
  - den lave: «Still won't do with this one. Get indoors, or get up high.» (tar innen 1,9 m og følger over tak bare innen 14 m, `S47FarmSite.cpp:653`, `:749`)
  - den høye: «Put the lamp on it first. It stops. Then back away.» (fryser i første lys, `S47FarmSite.cpp:556-561`)
- Mappene sier aldri hva de er, hvor de kommer fra eller hva de gjør med folk.

### Ord i spillet

Spilltekst må bestå ordsjekkene som finnes (`S47Thoughts.cpp:169`, `S47FreeformCases.cpp:910`, `S47Marginalia::NamesKind`). Det betyr ikke «alien», «ufo», «saucer», «grey», «silhouette», «taken», «third», 47, 1947, 8.15 eller «cable». **Mappene nummereres derfor 1–5 og ikke med bokstaver**, fordi en frittstående stor C er forbudt. Nummeret deles ut i den rekkefølgen spilleren først logger dem, så nummeret sier ikke hvem det er.

## 4. Tempo over 30 dager

| Uke | Tidsnivåer og sperrer i koden | Hva mappene typisk når |
| --- | --- | --- |
| 1 (natt 1–7) | 18–24 hint, 00–02 glimt, fulle møter etter 02; stor natt 2 eller 3; de små og den lave på stedsbesøk; den høye fra 03 | 1–2 mapper på OUTLINE, ofte bare bånd eller bilde langt unna. Ingen mappe kan nå HABITS (3 netter) |
| 2 (natt 8–14) | Figur nær fra uke 2 (`S47Happenings.h:35`); den lave bryter regelen fra natt 4; veiglimt én gang i uka | De små på MARKS og kanskje HABITS; den lave på MARKS; den høye eller sølvmannen på OUTLINE |
| 3 (natt 15–21) | Dyre oppgraderinger fra dag 15 (radiopeiler); fylkesinspektøren skriver dag 22 | Første arkivering mulig; den høye på MARKS; sølvmannen eller den som lyser på OUTLINE |
| 4 (natt 22–30) | Grensene kryper en time (glimt fra 23, fulle møter fra 01) | De tre som kommer nær, kan nå HABITS; de to fjerne stopper på MARKS. Vurderingen ved dag 30 |

Kobling til milepæler:

- **M3 Etterforskning:** skive 1 og 2 (merker på funn, mapper i notatboka og permen, båndet og forstørrelsen som bevis).
- **M4 Folk og steder:** rykter fra Caliche (servitøren, Baca, innringere på 1420) kan legges som «hearsay» i UNPLACED. De teller aldri som bevistype, men peker mot et sted.
- **M7 Freeform 1.0:** balanse av trinnkravene mot 30 dager med `-S47NaturalStand=tour`, Wards poeng og vanene som tryggere netter.

## 5. Gjenbruk og nye skiver

### Hva som gjenbrukes

| Del | Hvor |
| --- | --- |
| Vesenenes temperament, lyd og hvilken som dominerer natta | `Public/S47AlienKinds.h:20-49`, `Private/S47AlienKinds.cpp:175-215` |
| Hvem som kommer på stedet, og atferden per vesen | `Private/S47FarmSite.cpp:183-187`, `:543-567`, `:698-749` (`VisitWho`, `S47FarmSite.h:169`) |
| Fotoregistrering per gruppe | `Private/S47FreeformDirector.cpp:1756-1773`, `:1924-1929` (`strange.<gruppe>.n<natt>`) |
| Fotspor på veiskulderen per figur | `Private/S47RoadGlimpse.cpp:116-121`, `S47Happenings.cpp:4586-4593` |
| Funnlageret | `RecordEvidence`, `Private/S47Freeform.cpp:1651-1686`; lagret som `findings`/`findingDays` (`:4930`) |
| Blyantnotater om dem, med StruckBy og ordsjekk | `Public/S47Marginalia.h:9-60`, `Private/S47Marginalia.cpp:69-96`, `:165` |
| Kassettspilleren og det rare på båndet | `Public/S47Tape.h:27-34`, `FS47Tape::OddSound` (`Private/S47Tape.cpp:53-60`) |
| Geigeren | `Private/S47Happenings.cpp:5321-5355` |
| Dyrevarsler | `Public/S47AnimalWarn.h:28` (`EKind{Birds,Coyote}`) |
| Permen | `Public/S47RecordsBinder.h:10-45`, `Private/SS47Binder.h`, `Private/S47FreeformBinder.cpp:112-128` |
| Notatboka | `Private/SS47Notebook.cpp:591`, `:900` |
| Tegnestil | `Public/S47Drawings.h:8-24` |
| Wards vurdering | `Private/S47Predecessor.cpp:115-131` |

**Avvik som må rettes:** OPS-kvitteringen skriver i dag «Nordic figure», «Cosmic figure» og «grey/pale/tall visitor» (`S47FreeformDirector.cpp:1767-1773`, `:2032`). Det gir spilleren navnet og bryter `NamesKind`. Titlene må bli nøytrale beskrivelser («a figure on the rise», «something low»).

### Skive 1: merker på funnene som finnes, og mappe i notatbok og perm (minste som merkes)

- **Hva:**
  - Ved `RecordEvidence` og fotoregistrering legges et merke `{vesen, bevistype, dag}` på funn der vesenet allerede er kjent i øyeblikket:
    - fotoregistrering per gruppe (`roswell`, `pale`, `tall`, `nordic`, `cosmic`, `crawler`→den lave, `tallfigure`→den høye),
    - veispor (`tracks:low`, den høye),
    - `rule-still-held` (de små, LYKT) og `rule-pale-broken` (den lave, LYKT),
    - stedsbesøkets feltside (`VisitWho` når siden skrives, `S47FarmSite.cpp:860`).
  - Nummer deles ut ved første merke.
  - Toast `UNKNOWN n — TYPE`.
  - THEM-siden i notatboka viser mappene med seks bokser og trinn.
  - Permen får skillearket UNKNOWNS (lesing, ingen arkivering ennå).
  - Wards ukenotat får én linje om antall atskilte mapper.
  - OPS-titlene gjøres nøytrale.
- **Filer:** `S47Freeform.h/.cpp` (nytt `FS47Dossier` i økta), ny ren `S47Dossier.h/.cpp` (merker, nummer, trinn, selvtest), `S47FreeformDirector.cpp` (merke og titler), `S47FarmSite.cpp` (merke ved feltsiden og reglene), `S47RoadGlimpse.cpp` (merke på spor), `SS47Notebook.cpp` (THEM), `S47RecordsBinder.h/.cpp` og `SS47Binder.cpp` (skilleark), `S47PredecessorText.cpp` (ukelinje).
- **Lagring:** `dossier: { order: ["pale","roswell"], tags: { "<findingKey>": { "k": "pale", "t": "ground", "d": 9 } } }`. Interne id-er vises aldri. Eldre lagringer uten feltet laster tomt, og merker kan bygges opp igjen fra eksisterende funn der nøkkelen avslører gruppen (`strange.*`, `rule-*`).
- **Prøver (selvtest, ingen motormatrise):**
  - rekkefølgen er stabil,
  - ingen streng i mappene feiler `NamesKind` eller `S47Script::LintText`,
  - ingen to vesener har lik signatur i tabellen,
  - lagring tur-retur, og en gammel lagring gir tomt resultat,
  - trinn regnes riktig (OUTLINE/MARKS/HABITS med nattkrav).
  - Én rask sjekk i spill: F10-hendelse med stedsbesøk og bilde gir toast og en linje i THEM.
- **Størrelse:** omtrent 500–700 linjer C++, ett bygg og én rask sjekk. Tom merker det med en gang: stempelet i felt og mappa som vokser i TAB.

### Skive 2: egne signaturer der alt er likt i dag, og løse funn

- **Hva:**
  - Feltbåndet (`OddSound` for kropp/besøk) spiller vesenets egen signatur fra `S47AlienKinds::Render`, lagret som `OddKind` i `FTake`.
  - Dagslysteksten på stedet får en variant per vesen (`S47FarmSite.cpp:874-887`).
  - Geigeren på et besøkssted får en faktor per vesen.
  - Dyrevarselet velges etter vesen (Birds for den høye, Coyote/kuer for den lave, hundene tier for de små).
  - Den høye som fryser, blir eget funn `rule-tall-froze`.
  - UNPLACED med ✓/strek/? og tilordning, og StruckBy-stryk ved motsigelse.
- **Filer:** `S47Tape.h/.cpp`, `S47FarmSite.cpp`, `S47Happenings.cpp` (geiger, dyr), `S47AnimalWarn.h/.cpp`, `S47Dossier.cpp`, `SS47Notebook.cpp`.
- **Lagring:** `tape[].oddKind`; `dossier.tags[].assigned` (spillerens valg) mot `sure` (kjent ved opptak); `dossier.struck[]`.
- **Prøver:**
  - signaturene skilles (eksisterende `S47AlienKinds::SelfTest` utvides til båndnivå),
  - én tilordning blir strøket av en motsigelse,
  - lyd sendes til Tom som WAV (regelen om lydprøver).
- **Størrelse:** omtrent 700–900 linjer, ett bygg og en lytteprøve.

### Skive 3: arkivering, vaner og tempo

- **Hva:**
  - Navn og `FILE AS ONE` i permen.
  - Ward-poeng og $5.
  - Vanene som tanke og notatlinje når tegnene kommer igjen (lest av `S47FieldRadio`/walkie og stedets varsler).
  - Blyanttegning per trinn (`S47Drawings`-stil, tre trinn × fem mapper).
  - Balanse over 30 dager.
- **Filer:** `S47Dossier.cpp`, `S47FreeformBinder.cpp`, `S47Predecessor.cpp` (`FTally` + `Points`), `S47Thoughts.cpp`, `S47FarmSite.cpp`, `S47Drawings2.cpp`.
- **Lagring:** `dossier.filed: { "<kind>": { "name": "...", "day": 19 } }`.
- **Prøver:**
  - navnet går gjennom ordsjekk,
  - Wards poeng endrer ikke terskelen på 6 for en spiller uten mapper,
  - vanen kommer bare etter HABITS,
  - flernatt med `-S47NaturalStand=tour`,
  - kritikerrunde før installasjon.
- **Størrelse:** omtrent 800–1 000 linjer og tegninger, to bygg og en flernattsprøve.

## 6. Risiko, hva som må unngås, og åpne spørsmål

### De tre klagene på UFOPHILIA

- **Bratt læringskurve og uklar opplæring:** første bevis kommer av seg selv fra noe spilleren uansett gjør (bilde, bånd). Den første vanen (de små ser forbi deg i mørket) finnes allerede som lærdom. Ingen manual. Guide-fanen får høyst én prosedyrelinje om hvordan mapper føres, aldri hva de inneholder.
- **Tilbake til laptopen:** mappene og sammenligningen ligger i notatboka. Permen trengs bare for å arkivere.
- **Kjedelig fram og tilbake:** ingen sjekk krever reise. Tegnet logges der det skjer.

### Andre farer

- **Skrekk blir sjekkliste:** seks bokser er nok. Ingen prosent og ingen «2/6 found». Boksene fylles med blyant, ikke haker. Mange netter gir ingenting nytt. Skyggen forblir uløst. Det fjerne paret når aldri helt fram.
- **Fasit i teksten:** spillet setter aldri navn, opprinnelse eller hensikt. Tittelen på OPS-kvitteringen rettes i skive 1.
- **Bryte Freeforms regler:** ingen forklaring på hvordan noen ble borte, tapt tid er ikke bevis, ingen forbudte ord, og tidsnivåene styrer fortsatt når noe kan ses (mappa kan aldri vise mer enn tidsnivået viste, som `TierOf` i blyantnotatene).
- **Belønning for fare:** penger bare under det eksisterende taket, så det ikke lønner seg å oppsøke bortføring.
- **For mange vesener:** vi lager ingen nye. Crawler og tallfigure slås sammen med den lave og den høye.

### Åpne spørsmål til Tom (anbefalt standard i parentes)

1. **Er veiglimtets «crawler» den lave og «tallfigure» den høye, altså samme mappe?** (Ja. Det gir færre vesener og bedre spor på veien.)
2. **Skal sølvmannen og den som lyser ha egne mapper som stopper på MARKS uten arkivering?** (Ja. De kommer aldri nær av seg selv, så de forblir rare.)
3. **Skal skyggen aldri få en mappe?** (Ja, den blir under SHADOWS. Én ting skal være uløst.)
4. **Skal navnet være spillerens egen tekst eller et valg blant blyantord spillet foreslår?** (Egen tekst med ordsjekk, og et forslag fylt inn fra trekkene.)
5. **Hvor mye skal arkiverte mapper telle hos Ward?** (+1 for to arkiverte og +2 for alle tre som kommer nær. Terskelen på 6 står, så rutine alene fortsatt ikke holder.)

## Kilder

- UFOPHILIA-anmeldelser:
  - The Xbox Hub, laptopen bare i campingvogna, «the constant back-and-forth is annoying»: https://www.thexboxhub.com/ufophilia-review/
  - Use a Potion, «bare-bones tutorial», «a tense investigation into a tedious commute»: https://www.useapotion.com/2026/02/ufophilia-impressions-close-encounters-of-the-clunky-kind/
  - Playfront, løkka i fire faser og analyse-PC-en: https://playfront.de/en/?p=1154234
- Utelukking og lesbarhet:
  - NME om Phasmophobias journal, der fra-liste med alle kandidater erstattet avkrysning: https://www.nme.com/news/phasmophobia-devs-share-improvements-to-the-journal-system-3158844 (lest gjennom søkesammendrag, ikke åpnet)
  - Film Stories om Obra Dinns tre om gangen, der samlet bekreftelse demper gjetting: https://filmstories.co.uk/?p=83249
