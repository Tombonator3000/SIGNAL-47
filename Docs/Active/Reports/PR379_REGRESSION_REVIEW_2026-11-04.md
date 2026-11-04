# PR379: samspill og regresjonsdekning

**Status:** Under arbeid. **Gjelder:** statisk gjennomgang av PR379 head `cc071f4684a66302db45f56ef93ba4e1bf9e1f7c`, med nye testkandidater på Codex-nattgrenen. **Sist kontrollert:** 4. november 2026.

## Avgrensning og faktisk Git-status

GitHub API viste MERGED, flettet 4. november 05:59:56 UTC (06:59:56 Europe/Oslo), merge `0d9c58153463ae15497d437d5bdfb5f6e9958767`. Opplysningene om åpen PR og head eea7faa1 er dermed eldre enn denne kontrollen. [PR379](https://github.com/Tombonator3000/SIGNAL-47/pull/379). Gjennomgangen gjelder hovedsakelig `37fdc80b..cc071f46`: 18 C++-kilde-/headerfiler, 972 tillegg og 142 slettinger. Senere tittel- og feltplanendringer på nattgrenen inngår ikke i den flettede PR-en; Claudes ucommittede fotorealismearbeid inngår heller ikke.

Dette er en eksplisitt kode-/testgjennomgang, ikke leveringsaksept. Ingen Unreal-jobb er startet i denne gjennomgangen. Claude kjørte Unreal i egen kopi under undersøkelsen. Nye native tester nedenfor må bygges og kjøres i et koordinert Linux-vindu med production_lease/unreal_guard. Personlige Saved, installert spill og Claudes kilder er urørt.

Biblioteksøk fant ingen relevant Unreal-løsning for disse lagrings- og økonomiforløpene. Kortet `losninger/testing/bilder-for-og-etter-med-worktree.md` gjelder Node/Playwright og brukes ikke som Unreal-testbevis; ingen ny arbeidskopi er opprettet.

## Funn 1: eldre duplikat kan blokkere hele printerkøen

**Prioritet: høy for berørte eldre lagringer. Status: konkret statisk kodefeil; ikke runtime-reprodusert i denne gjennomgangen.**

Ny `PrinterIndex` velger første allerede påbegynte ark globalt ([Freeform.cpp, linje 319](https://github.com/Tombonator3000/SIGNAL-47/blob/cc071f4684a66302db45f56ef93ba4e1bf9e1f7c/unreal/Signal47/Source/Signal47/Private/S47Freeform.cpp#L319)). En eldre `duplicate_print`-jobb kan ha en beholdt original uten den senere CAPTURE CLOCK-linjen. `CollectDuplicate` bevarer originalen og avviser kopien, men lar jobben bli stående som ventende ([linje 588–600](https://github.com/Tombonator3000/SIGNAL-47/blob/cc071f4684a66302db45f56ef93ba4e1bf9e1f7c/unreal/Signal47/Source/Signal47/Private/S47Freeform.cpp#L588)). Dermed velger printeren samme jobb igjen. Å velge en nyere observasjon løser ikke køen. Før printerendringen foretrakk CollectChart den valgte jobben hvis denne selv ventet på papir.

Dette er et eldre format som produksjonskoden uttrykkelig håndterer, ikke bare tilfeldig ødelagt JSON. Forelderen til commit `3b6a6c1c` skrev originaler uten CAPTURE CLOCK-linjen; originalen lå i Story-dokumentene. Den eksisterende legacy-printertesten fjerner bare feedfelt på en Chart-jobb. Den nyere duplikatprøven bruker en nettopp produsert original med gyldig tellerlinje. Ingen av dem dekker kombinasjonen.

**Foreslått rettelse:** gi en uproduserbar eldre kopi en eksplisitt avvist/parkert køtilstand som ikke blokkerer nyere ark. Bevar originalens bytes, ID og stempel, og behold forklaringen om at en ny hendelsesobservasjon kreves. Ikke sett duplicateCollected, ikke konstruer tellerdata og ikke gi betaling. Tilstanden må overleve lagring. Deretter prøves både gammel og gyldig moderne duplikat i samme kø.

**Ny testkandidat:** `-S47Freeform -S47FreeformTest -S47PrinterLegacyQueueTest`. To jobber: gammel duplikat med original i dokumentarkivet, og en nyere Chart-jobb. Disk/Flow-gjenoppretting, forsøk på gammel kopi, velg ny jobb og prøv å samle originalen. Krev fremdrift for ny jobb og byteidentisk gammel original uten ekstra penger. Testen skal avsløre dagens køblokkering; den er ikke et ferdig produksjonsrettet PASS.

## Funn 2: disk/Flow-gjenoppretting dekker ikke menyens Continue

**Prioritet: høy testmangel; ingen påvist lagringsfeil.**

`FS47FreeformTest::Restore` skriver `GameState()` til en testfil, leser JSON og kaller `StartGame` med `bDev=true` ([testkode linje 95–108](https://github.com/Tombonator3000/SIGNAL-47/blob/cc071f4684a66302db45f56ef93ba4e1bf9e1f7c/unreal/Signal47/Source/Signal47/Private/S47FreeformTest.cpp#L95)). Dette prøver faktisk disk og gjenoppretting av spilltilstand. Det prøver ikke `WriteSave → FS47Saves::Save → ContinueSave → OpenSave`, slotmetadata, valg av siste lagring, prosessavslutning eller tittelmenyens callback.

**Nødvendig tillegg:** lagre via vanlig manuell slot i isolert UserDir, gå til tittel og velg faktisk Continue. Avslutt deretter hele prosessen, start med samme isolerte profil og velg Continue igjen. Verifiser sessionMode, valgt slot/dag, beholdte dokument-ID-er, foto-originaler, kameraeierskap, printerfremdrift, betaltstatus og pakkeinnhold. Bruk aldri en personlig profil. Ikke tell en ny direkte StartGame-fixture som dekning av denne testen.

## Funn 3: lagergrense og avvist helpakkelevering manglet native dekning

**Prioritet: middels. Statisk produksjonskode ser korrekt ut; ingen påvist duplisering eller tap.**

Purchase kontrollerer lager pluss allerede betalt pakke før betaling. Deposit validerer alle varene før lageret oppdateres og pakken tømmes. Eksisterende shoptest dekker tre kjøp for $35, lastet pakke gjennom disk/Flow-gjenoppretting og én levering. Den prøver ikke kapasitetsgrensen eller avvisning av en pakke hvor bare én vare ikke får plass.

**Ny testkode:** eksisterende `-S47ShopBasketTest` er utvidet med eksplisitt lagerfixture 98 sikringer. Kjøp én til samlet 99; neste kjøp skal avvises uten belastning. Kjøp også relé. Øk sikringslageret til 99 etter kjøpet og forsøk levering: begge varetyper, saldo og leveringstellere skal være uendret, hele pakken beholdt. Frigjør én plass og lever samme pakke én gang. Dette er kodeklar, ikke bygget eller kjørt.

**Videre konkrete tilfeller:** disk/Flow-gjenoppretting mens den betalte pakken fortsatt står på butikkdisken, og etter dropping før ny henting; malformed contents med 0, negativ, brøk og over 99. Produksjonskode avviser disse mengdene, men eksisterende selvtest prøver bare ukjent varenavn.

## Funn 4: deltestene beviser ikke sammenhengende økonomi- og originalvern

**Prioritet: høy testmangel; ingen ny betalingsfeil påvist statisk.**

Fotoregistrering krever beholdt original, riktig mål og fremkalt bilde, og setter betaltstatus før inntekt. Fotobenkens start revaliderer nærhet, strøm, dør og stadium. Dette er riktige barrierer. Likevel kjøres kamera, fotoøkonomi, printer og butikk som separate profiler/fixtures. De gir ikke bevis på at penger, dokumenter, kamera og pakker forblir konsistente gjennom samme spillerøkt og ordinær lagring.

Gjennomførbar integrasjonstest på Linux, med én isolert profil og fast seed:

1. Start ny Freeform uten kamera. Hent det fysisk i fotolaben; fotografer ett kvalifiserende feltmål med vanlig shutter. Noter original-ID og startbudsjett.
2. Forsøk OPS-registrering før fremkalling. Krev ingen betaling. Start fremkalling fysisk. Lagre midt i badet og bruk vanlig Continue; krev samme original og gjenværende prosess, ingen kopi eller kreditering.
3. Fullfør og registrer bildet én gang. Registrer igjen og gjenta etter prosessrestart/Continue; saldo øker bare ved første gyldige registrering. Originaldata er identiske gjennom hele forløpet.
4. Gjennomfør en vanlig observasjon til original i printeren. Lagre midt i papirføringen. Avbryt strømmen og gjenopprett; fremdrift skal bevares, ikke dobles. Samle original, utfør kontroll, registrer og arkiver etter eksisterende krav.
5. Kjøp to sikringer og ett relé for $35 ved faktisk butikkdisk. Lagre før henting, hent/last og kjør tilbake; lagre lastet og lever ved mottaket. Krev nøyaktige lagerøkninger, én betaling og én levering, også etter ny Continue.
6. Sammenlign sluttbudsjett med start + faktisk opptjente, unike foto-/arbeidsbeløp − $35 − eksplisitt registrerte øvrige utgifter. Krev uendrede gamle originaler, ingen ekstra claims og ingen betalt pakke etter levering.

Deterministisk hendelsesoppsett kan brukes i en automatisert variant og må merkes som fixture. En naturlig spillerreise skal i tillegg bruke vanlig forflytning, interaksjon og lagringsmeny; teleportering beviser ikke adkomst.

## Funn 5: Story-lås og kamerareset testes for smalt

**Prioritet: middels testmangel; ingen bekreftet ny Story-regresjon.**

Produksjonskoden har eksplisitte Story-låser i EastWing.cpp og avviser garasjeadgang i World.cpp. Kamera/proxy gjenopprettes i Chapter1.cpp, mens Flow lagrer Freeform-eierskap bare i Freeform. Disse grenene er statisk gjennomgått.

StoryDriveTest kontrollerer colliderflagg og en Blocked-posisjon; det er ikke faktisk gange/Jeep mot inngangene. Kameratesten skifter til Story og kontrollerer kameraets posisjon, men ikke fysisk henting og opprinnelig progresjon.

Nødvendige tilfeller:

- Ny Story: gå mot personaldør, servicedør og garasjeport; prøv bruk. Ingen passasje eller åpning. Kjør Jeep mot garasjeporten; ingen passasje eller progresjonsendring.
- Gjenta etter Freeform → tittel → Story i samme prosess, både med og uten hentet Freeform-kamera.
- I Story: hent originalkameraet med faktisk sikting/bruk og kontroller riktig neste historiesteg. Prøv arkivinteraksjonen og normalt kamerabruk; krev korrekt proxy og ingen Freeform-eierskapsfelt.
- Story-lagre før/etter kamerahenting og etter fremkalling. Vanlig Continue skal bevare kapittel, stadium, kamera, bilde/fremkalling og klokke uten å arve Freeform-tilstand.

## Kontroller som faktisk foreligger

Følgende eksisterende lokale result.json og runnerkvitteringer ble lest i denne gjennomgangen; de er ikke kjørt om igjen. Runnerne har returncode 0 og ingen stop_reason. De gjelder tidligere kandidater gjennom natten, ikke én samlet test av alle endringene ved siste head.

| Kjøring | Faktisk avgrenset resultat | Disk/Flow-returer |
| --- | --- | --- |
| printer-r2 | PASS, 26 kontroller | 2 |
| camera-r4 | PASS, 23 kontroller | 2 |
| shop-r2 | PASS, 34 kontroller | 2 |
| ops-r1 | PASS, 37 kontroller | 0 |
| guidance-r1 | PASS, 28 kontroller | 1 |
| array-r2 | PASS, 21 kontroller | 1 |

Kildene ligger lokalt under `~/scratch-bm/Signal47-night-support-20261103/` og `/tmp/S47-night-20261103-<kjøring>/Saved/FreeformTest/result.json`. Claudes 2281-selvtest/fullregresjonsresultat ved a6acc316 er tidligere loggført; dette er ikke en ny fullregresjon av cc071f46. Foto53/4 og Story-lås er tidligere loggført og er ikke et nytt sammenhengende akseptresultat her.

Nye testkandidater skal bygges med prosjektets editorbygg og kjøres med én tung jobb om gangen, isolert UserDir, `-game -S47Freeform -S47FreeformTest` samt det aktuelle flagget. Bruk production_lease gjennom hele bygg/testvinduet og unreal_guard.run_owned for prosessene. En ren prosess-exit er ikke nok: kontroller testens result.json, antall kontroller, faktisk feilårsak og at vakten ikke registrerte overlapp. Ingen forventet-feiltest skal telles som PASS før produksjonsfeilen er rettet og samme test faktisk består.

## Anbefalt rekkefølge

1. Bygg/kjør den nye legacy-køtesten, rett isoleringen av avvist eldre kopi og kjør køtesten pluss eksisterende printertest.
2. Kjør utvidet shoptest og ordinær Save/Continue gjennom meny og prosessrestart.
3. Kjør den sammenhengende foto/arbeid/butikk-reisen og Story-modusbyttet ovenfor.
4. Oppdater [STATUS](../STATUS.md) og [TODO](../TODO.md) med faktisk resultat. Merge, statisk gjennomgang og grønne deltester er forskjellige bevis; ingen av dem alene erklærer hele leveransen ferdig.
