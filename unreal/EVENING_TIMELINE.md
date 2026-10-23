# Kveld, natt og daggry i runtime

Kildearbeid i `codex/evening-runtime`, 23. oktober 2026. Tidsplanen ligger i `S47Timeline.h`; historien velger planen med `AS47Story::bEveningTimeline`. Bygg, selvtest og bilder kjøres samlet av hovedagenten. Dette notatet er kodegjennomgang og tidskontrakt, ikke bevis på gjennomspilt natt.

## Én datoakse og kompatibilitet

Ny sak bruker `timelineVersion=2`. Søndag 13. april 18:00 er `-21600`, midnatt er `0`, mandag 14. april 05:29 er `19740`. HUD, datolinjer og fotografier bruker eksisterende klokkeformattering med modulo ett døgn. Varigheter og gjenbesøk bruker de sammenhengende lagrede sekundene; ekspeditørens totimerskrav tåler derfor midnatt.

Sak uten versjonen tilhører gammel tidslinje (`timelineVersion=1`). Flytens lagringsmigrering merker dette uttrykkelig. Den spiller videre fra gammel klokke og beholder 02:13-referansene. Ingen lagrede tekster, papirer, fotografier eller eksponeringstider kjøres gjennom tekstomskriving. `TimelineText` brukes bare ved utvalgte steder som lager en ny 1986-referanse. TAPE A og E10 fra 1947 beholder egne klokkeslett. Begge tidslinjer deler finalen.

## Tidligere og nye tider

| Hendelse | Tidligere sak | Ny kveldssak |
|---|---|---|
| Innkjøring | Ingen | Sen ettermiddag, før skiftet; ingen minutt-HUD |
| Dale overleverer | Papir merket 23:20; spillstart 23:41 | Telefon ved SARO 18:00; ankomst starter klokka én gang |
| Værtelex | 13. april 22:50 | 13. april 17:09 |
| Nettstatus-telex | 13. april 23:00 | 13. april 17:19 |
| Loggens tidligere K3-feil og spenningsdip | 23:14 / 23:15 | 17:33 / 17:34 |
| Aktiv survey-fil åpnes | RUN860413_2341.DAT, 23:41 | RUN860413_1800.DAT, 18:00 |
| Gjenopprettet K3-logg i testfixture | 23:44 | 18:03 |
| Rutineintervallets slutt | 14. april 02:13:41 | 14. april 00:13:41 |
| Residualen viser seg | 02:13:47 | 00:13:47 |
| Pulsgruppe og bånd B-etikett | 02:14 | 00:14 |
| Første array-hendelse, standard fixture | 02:15:12 | 00:15:12; ordinær rute følger spillerens solve og telefon |
| Kapittel 1, ordinær inngang | Etter første hendelse | Etter første hendelse, omtrent 00:16 |
| Nattkurvens tidsgrunnlag | 02:16 til 05:29 | 00:16 til 05:29; prolog/innkjøring har fortsatt kurven av |
| Guttens radiosamtale, første mulighet | 02:26 | 00:26 |
| SARO mørk / Hondo, tilgjengelig vindu | 02:29 til 02:40 | 00:29 til 00:40 |
| Kapittel 2 | Etter kamerarapporten | Etter kamerarapporten; ingen ny venting |
| Kapittel 3, minimum ved overgang | 03:40 | 02:20 |
| Kapittel 4, minimum ved overgang | 04:35 | 03:20 |
| Kapittel 5, minimum ved overgang | 05:00 | 04:10 |
| Earl / Night Line | Midnatt til 04:00 | Uendret; egen lokalvert på kvelden |
| Synlig lokalstudio / ON AIR | To forskjellige kveldsvilkår i bykode og dialog | Felles 18:00 til 04:00; kveldens vert og Earl adskilt |
| Sheriffens patrulje | 02:45 til 04:30 | Uendret; vanlig bemanning også på kvelden |
| Suvenirmannen går | 04:10 | Uendret; synlig også på kvelden |
| Avisen leveres | 04:45 | Uendret; fysisk avis først etter levering |
| Kvinnen ved vaskeriet går | 04:50 | Uendret; synlig også på kvelden |
| Lysbølgens siste mulighet | 04:55 | Uendret; All Night begynner nå før grensen |
| Roswell morgenradio | 05:00 | Uendret |
| Kveldens gatelys | Bare klokkeslett før lampenes daggrygrense | På fra 18:00; av ved eksisterende individuelle daggrygrenser |
| Dinerens lokale daggrylys | Tvunget minst 0,15 fra kapittelstart 05:00 | Ingen daggryglød 04:10; følger 04:40 til 05:30 |
| Avreise på Roswell Road | 05:20, kunne gå bakover ved sen avreise | 05:20; ny tidslinje holder tidsbudsjettet ved denne grensen |
| CB på siste strekning | 05:28:13 | Uendret |
| THE EVENT | 05:29:00 til 05:29:47 | Uendret |
| E10 / TAPE A, juli 1947 | 02:15:10 til 02:17:47; E10 02:17:00 og 02:17:47 | Uendret historisk opptak, ikke flyttet til midnatt |

## Valgt komprimering og venting

Rutinekalibreringen og den kjente relé-/spenningssekvensen spilles. Deretter komprimerer eksisterende fade den hendelsesløse survey-tjenesten frem til 00:13. Faden sier uttrykkelig «Routine survey completed / 14 APRIL 1986 00:13». Det er ingen virkelig seks timers venting og ingen klokke som går bakover. Senere kapitteloverganger løfter klokka til de nye minimumstidene når aktiviteten er ferdig. Hvis spilleren allerede har brukt mer tid, beholdes den senere tiden.

Ny tidslinje bruker en uttrykkelig sluttfrist for fortellertid: klokka holdes på 05:20 hvis frivillig utforskning varer ut over budsjettet før siste kjøretur. Spilltid, dialog og oppgaver fortsetter; dette er en komprimeringsregel, ikke en påstått anomalihendelse. Roswell Road bruker avstand og fremdrift frem mot 05:29, holder klokketiden ved stopp og går aldri bakover ved rygging. Finalens 47 sekunder følger deretter sin egen hendelsesklokke. Dette bevarer både fri utforskning og uendret finale; varigheten av en hel natt er ennå ikke målt.

## Radio og midlertidig lyd

Kveldens lokalvert bruker en egen rolle (`evening`), undertekster og lydløs taleplassholder, med 85 til 95 sekunders musikkrom. Ingen Earl-/Kinski-take brukes som kveldens vert. Musikken gjenbruker eksisterende radioavspilling. Underteksten opplyser at vertens opptak mangler.

Ved midnatt bytter Caliche til Night Line; Earl signerer fortsatt av 04:00. Før residualen velger programmet vanlige innslag uten omtale av et fremtidig senderutfall. Nye klokkeavhengige formuleringer («after midnight», «since midnight», «yesterday afternoon») får undertekst som uttrykkelig merker manglende nytt opptak, og taus tale fremfor et gammelt opptak med feil ord. Eksisterende replikkopptak uten tidskonflikt kan fortsatt brukes.

## Kontrollgrunnlag

`-S47SelfTest` inkluderer nye kontroller i de eksisterende Caliche-, radio-, dokument- og SaroDark-testene: negativ kveld, positiv natt, begge residualtider, kapittelrekkefølge, uendret 1947-tid, lokalvert/Earl-skille, studioskifte ved midnatt, guttens nye 00:26-grense, sheriffens vakt og kveldens daterte telex. De gamle testene kjører fortsatt med standard legacy-plan.

Statisk gjennomgang: datoformatering og fotosystemet tåler negative sekunder; ekspeditørens første møte lagrer den sammenhengende klokka; arkivtelefonens faktiske første samtale hentes fra nattens WWVB-logg og følger derfor den valgte saken. Gamle fallback-tekster i arkivtelefonens isolerte testfixture er beholdt som legacy.

Prosjektbiblioteket ble søkt på behov og Unreal-lagring. Profilen `quodsoler/unreal-engine-skills` ble kontrollert (UE 5.8, MIT, UNVERIFIED i biblioteket); ingen kode eller nye avhengigheter er hentet derfra. Løsningen tilpasser prosjektets eksisterende klokke-, dialog-, dokument- og kapittelsystemer.
