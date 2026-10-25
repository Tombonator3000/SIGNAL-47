# H1: produksjonshistorie for hele natten

Gjeldende design: [sammenflettet hovedscenario SC01 til13](../../MISSION_FLOW_1800_ROSWELL.md), med ekstra ARRAY-01, tre Calichebesøk, gradvis flere himmellys og bevarte randomhendelser. [Dales kontorvedlegg](../../DALE_OFFICE_DOCUMENTS.md) har seks nye frivillige tekstutkast rundt eksisterende materiale. De10 genererte hovedkortene/36 scenene inneholder ennå ikke hele denne reisen. Senere produksjon redigerer JSON-kildene og regenererer, uten doble overleveringer, kontordokumenter, T01 eller P13.

SC01 til04 samler den aktive kvelden. Fem skrevne trusselmøter, synlig bortføring og eksisterende tilfeldige hendelser bygger mot fast finale. H07 er eskalert koordinering etter tidligere møter; H08 er fortsatt suspensjon og nytt nettverksvarsel. Nye engelske delscener, hjelpekontakt og person er ikke produsert. Revider forbindelsene før integrasjon; gamle «kanskje bare lampefeil»-replikker passer ikke etter faktisk inntrengerhendelse.

Dato: 25. oktober 2026. Engelske produksjonsscener for H01 til H10, skrevet etter HORROR_STORY_PLAN. Dette er nytt manus og adaptergrunnlag. Ingen av de nye oppdragene, fjernmålingene, fiendereaksjonene eller avtalene er koblet inn i spillet. Ingen ny lyd eller film er produsert.

- [Hovedscenario](../../MISSION_FLOW_1800_ROSWELL.md) og [sekundært løsningsoppslag](../../MISSION_DETAILS_REFERENCE.md): 13 sammenhengende scenarioer med alle tidligere32 arbeidskort bevart. Nye delscener blir ikke automatisk skrevet eller integrert av generatoren.
- [Lesemanus](generated/SCRIPT.md): 36 scener med konkrete replikker, observasjonstekst og uavklarte utfall.
- [Produksjonskort](generated/MISSION_CARDS.md): spørsmål, fysisk handling, kontroll, følge og vei videre for alle ti oppdrag.
- [Bevis og personkunnskap](EVIDENCE_AND_ROUTE.md): hovedrutespor, varsling, siste reisemål og kildebegrensninger.
- [Gjeldende rute og målehull](CURRENT_ROUTE.md): dagens kildeimplementasjon, transporter, paneler, venting og skrekktopper. Menneskelig måling står åpen.
- [Adapterindeks](generated/integration_index.json): kobling fra H-numre til scener og eksisterende kilder/ID-er.
- [Opptaksplan](generated/recording_manifest.json) og [undertekstgrunnlag](generated/subtitles.json): nye cue-ID-er, uten oppdiktede lydtider.

`pack.json`, `missions.json` og JSON-filene i `modules/` er forfatterkildene. `build_pack.py` gjenbruker EveningShift-formatets strenge validering av kanon, personer, kanaler, flagg og referanser. Den lager bare lesefiler i denne mappens `generated/`. Den skriver ingen C++-include, endrer ikke kveldspakkens opptak eller genererte filer og kjøres ikke av spillet.

```sh
python3 unreal/Narrative/HorrorStory/build_pack.py
python3 unreal/Narrative/HorrorStory/build_pack.py --check
```

H-numrene erstatter ingen P-, N-, A-, T- eller E-ID. Nye `HR_*` er separate forfatterdokumenter. `ctx_hr_*` er eksterne fakta som senere adapter må etablere gjennom faktisk handling. En replikk eller åpnet panel kan ikke erklære en måling gjennomført. `hr_*_recorded` betyr bare at et allerede kontrollert resultat er presentert.

Eksisterende kveldspakke beholder ansvar for innkjøring, mat, overlevering, kontordokumenter, T01/T02 og VHS. Denne pakken skriver den sammenhengende hovedmotivasjonen og nye mellomledd. Ved senere integrasjon skal samme overlevering eller kontorinformasjon ikke spilles to ganger. Nye scener som omarbeider gamle replikker får egne opptak; ingen gamle lydfiler antas å passe automatisk.

H1.2 til H1.4 har nå konkret produksjonsgrunnlag. H1.1 er fortsatt delvis: ruten er kildekartlagt, men ubrutt menneskelig gjennomspilling, opplevd frykt og samlet spilletid er ikke målt.

Senere kildepresisering 25. oktober: H02s utilstrekkelige resultat lar kalibrering fortsette, men nye T01 R2-saker trenger fullført inspeksjon og fysisk filing før Survey. H09 bruker E15, det gamle innrammede 1947-utklippet, uten 04:45-port. Dagens avis i byen er en annen kilde. Ekstern sikring i H08 er en konkret diktert måleavskrift og Wards daterte kvitteringslogg; foto og negative blir med originalene. JSON-kildene er rettet og eksportene regenerert; ingen nye opptak eller runtimeendringer inngår.
