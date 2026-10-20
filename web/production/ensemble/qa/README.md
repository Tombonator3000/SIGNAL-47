# Stemmer og Serling: nettleserbevis

Status: **PASS** for den lokale sluttkandidaten. Fire nye funksjonskjøringer, 21 kompatibilitetskjøringer, 13 spillregresjoner, lydkostnad og 12 CPU-kommandoer er fullført. Privatbankfri reserve består 22/22. Faktiske kommandoer, resultatfiler, kilde-/bygg-/lydhasher og grenser står i `validation.json`. PR, innfletting og publisert kontroll dokumenteres separat etter faktisk levering.

Lydkostnadens første målevindu med Art Bell Av endte med fem ekstra aktive kilder mot grensen på fire, uten lydbortfall, ny dekoding eller stasjonsresynkronisering. Én uendret gjentakelse består 17/17 med samme grenser. Begge resultater og begrensningene er ført i `cost-retry.json`.

## Nye funksjonsprøver

`tools/ensemble.py` prøver den komplette banken med 100 stemmeklipp: 18 Stanton, 19 Stack, 13 Lynch, 49 Bacall og ett Price-klipp. Hvert klipp må starte som faktisk dekodet MP3 med riktig rolle, ID, varighet og lydrute. Fullbank-prøven avbryter hvert klipp etter identitetskontrollen. Den påstår derfor ikke full naturlig avspilling av alle 100.

Separate spillerreiser bruker ekte E eller touch Use mot Caliche, samt faktiske samtalevalg, Replay og Stop. Utvalgte fulle køer må avsluttes naturlig uten overlapp. Nora-telefonene bruker eksisterende manus og ekte dial/ringback- og scenehandlere. CB følger kapittel 4s eksisterende utløser og faktisk dekodet lydtid. Price prøves både på returlinjen i rom 6 og i felthytta, med den eksisterende firesekunders carrier-ledetiden. Telefon går gjennom 320 til 3200 Hz og SFX; CB går gjennom sin opprinnelige smale høyttaler og radiobuss; nærsamtale går til hovedvolumet.

Prøven skiller eksplisitte manus-/progresjonsfixtures fra faktisk input. Den kontrollerer rollefiltrering av YOU, sceneanvisninger og andre personer, utskifting av køer, avslått valg, utilgjengelig tekst, lastesvikt, pause, avbrudd og sen dekoding. Mobilprøven bruker 844 ganger 390, ekte touch og gjeninnlasting av uavhengige innstillinger.

`tools/serling.py` kontrollerer full tekstdekning gjennom den faktiske Serling-resolveren. Hele banken spilles ikke gjennom i nettleseren. Den separate PCM-/korpusrapporten må dokumentere full filkontroll. Nettleseren prøver representative virkelige leseflater i de forberedte områdene, ekte Zoom med 650 ms stabilitet, V og HUD, dokumentbeskrivelse og notatbok.

Alle fem mellomkapittelkort prøves gjennom hovedkodens faktiske avslutningskobling, også begge variantene av kapittel 1. Overskrift, oppsummering, neste kapittel, tittel og klokke må leses i vist rekkefølge. Den synlige A-/B-fasen må følge den faktiske talen. Skjermbilder tas av begge faser på skrivebord og mobil. Ekte lydtid må forlenge det tidligere 6,4 sekunders kortet til siste fullstendige lydsegment er ferdig. Egne prøver holder ekte dekoding eller oppvåkning av lyd uavklart til produktets egen lastefrist har latt kapitlet gå videre. B skal fortsatt få lesbar skjermtid etter feil. Porten slippes først etter fremdriftsbeviset. Card-null, reset og Av må kansellere umiddelbart uten å vekke gammel tale eller starte senere B-tale.

Begge harnessene bruker `S47_URL` for Pages fra `/SIGNAL-47/`. Uten variabelen kopieres enkeltfilen til en ren midlertidig mappe med den eksisterende strenge CSP-en. Enkeltfilprøvene krever null eksterne MP3-forespørsler. Ingen policy svekkes, og ingen lyd erstattes med syntetiske testbuffere.

## Tidligere prøver

De gamle Kinski-, Art Bell-, radio-, kapittel- og raske progresjonsprøvene setter de to nye stemmevalgene eksplisitt Av i sitt eget oppsett. Felles `start_night()` og lydprober endrer ikke standardvalgene. De nye funksjonsprøvene starter med faktiske standardvalg På.

Lastesjekkene teller separate navnerom: 47 ordinære lyder, 24 eksisterende telefonklipp, 11 karakterklipp for telefon/CB/feltlinje og enten null eller 15 private lisensierte lyder. Store dialog- og fortellerbanker skal fortsatt lastes ved behov.

## Statusmatrise

| Prøve | Pages | Streng CSP enkeltfil |
|---|---|---|
| Ensemble | PASS 175/175 | PASS 176/176 |
| Serling | PASS 107/107 | PASS 108/108 |
| Tidligere stemmeprøver | PASS | PASS |


Menneskelig lytting, stemmelikhet, rolleklang, uttale, fysisk høyttaler-/hodetelefonlyd og enhetsytelse er UNVERIFIED. Primærkilder og referanseopptak ligger utenfor repoet. Offentlig tilgang er ikke dokumentert klonesamtykke eller gjenbrukslisens.

Aktuell byggekandidat: 87f5b7556636571d71d5ae0de087092bbf5105d8be97e0c11add4d162b865b05 (enkeltfil, 180160203 byte). Runtime TS/CSS: `90d50f93ba593d8d3ebd78284a0e694ce41bf63dd5afb63a4fe81dc5a568425e`. `visual-review.json` fører de faktisk inspiserte bildene og begrensningene; mobilens rullede samtalebilde beviser bare det viste utsnittet.
