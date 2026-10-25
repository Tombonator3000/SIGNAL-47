# H1: produksjonshistorie for hele natten

Dato: 25. oktober 2026. Engelske produksjonsscener for H01 til H10, skrevet etter HORROR_STORY_PLAN. Dette er nytt manus og adaptergrunnlag. Ingen av de nye oppdragene, fjernmålingene, fiendereaksjonene eller avtalene er koblet inn i spillet. Ingen ny lyd eller film er produsert.

- [Lesemanus](generated/SCRIPT.md): 36 scener med konkrete replikker, observasjonstekst og uavklarte utfall.
- [Produksjonskort](generated/MISSION_CARDS.md): spørsmål, fysisk handling, kontroll, følge og vei videre for alle ti oppdrag.
- [Bevis og personkunnskap](EVIDENCE_AND_ROUTE.md): hovedrutespor, varsling, siste reisemål og kildebegrensninger.
- [Gjeldende rute og målehull](CURRENT_ROUTE.md): dagens kildeimplementasjon, transporter, paneler, venting og skrekktopper. Menneskelig måling står åpen.
- [Adapterindeks](generated/integration_index.json): kobling fra H-numre til scener og eksisterende kilder/ID-er.
- [Opptaksplan](generated/recording_manifest.json) og [undertekstgrunnlag](generated/subtitles.json): nye cue-ID-er, uten oppdiktede lydtider.

`pack.json`, `missions.json` og JSON-filene i `modules/` er forfatterkildene. `build_pack.py` gjenbruker EveningShift-formatets strenge validering av kanon, personer, kanaler, flagg og referanser. Den lager bare lesefiler i denne mappens `generated/`. Den skriver ingen C++-include, endrer ikke kveldspakkens opptak eller genererte filer og kjøres ikke av spillet.

```sh
python 3 unreal/Narrative/HorrorStory/build_pack.py
python 3 unreal/Narrative/HorrorStory/build_pack.py --check
```

H-numrene erstatter ingen P-, N-, A-, T- eller E-ID. Nye `HR_*` er separate forfatterdokumenter. `ctx_hr_*` er eksterne fakta som senere adapter må etablere gjennom faktisk handling. En replikk eller åpnet panel kan ikke erklære en måling gjennomført. `hr_*_recorded` betyr bare at et allerede kontrollert resultat er presentert.

Eksisterende kveldspakke beholder ansvar for innkjøring, mat, overlevering, kontordokumenter, T01/T02 og VHS. Denne pakken skriver den sammenhengende hovedmotivasjonen og nye mellomledd. Ved senere integrasjon skal samme overlevering eller kontorinformasjon ikke spilles to ganger. Nye scener som omarbeider gamle replikker får egne opptak; ingen gamle lydfiler antas å passe automatisk.

H1.2 til H1.4 har nå konkret produksjonsgrunnlag. H1.1 er fortsatt delvis: ruten er kildekartlagt, men ubrutt menneskelig gjennomspilling, opplevd frykt og samlet spilletid er ikke målt.
