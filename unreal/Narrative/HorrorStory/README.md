# H1: produksjonshistorie for hele natten

## Gjeldende forfatterkilder, 28. oktober 2026

De stoppede scenetekstene er samordnet med [Toms godkjente revisjon](../../HISTORIE_SC02_SC13_REVISJON.md). Pakken har **ni hovedkort, 13 scenarioer, 77 scener og 90 eksterne verdensflagg**. H09s scene-IDer er beholdt som kartarbeid under H08, uten et eget H09-kort. Fire aktive møter ligger ved trafoen, annexet, arrayet og SARO; den separate svarte figuren og SC04-bilmøtet er kuttet. Ortiz etterlater en termose, og den kjente sheriffbetjenten gir savnetsakens loggspørsmål og senere et datert patruljeark.

Levering skjer i fire akter: SC02 til SC05, SC06 til SC08, SC09 til SC10 og SC11 til SC13. Målrettede kontroller kjøres per scenario; full 110-matrise, fletting og installasjon skjer én gang per akt. Nye stemmer for Ward, Ortiz, Earl og Nora lages til slutt.

SC02-kandidaten har faktiske kildeprodusenter; dette notatet gir ingen ny runtimeaksept. **SC03 og videre er fortsatt authored_not_integrated.** Generatoren eksporterer også HorrorPackData.inl som compiled data, men nye scener blir ikke spillbare uten verdensadapter, fysisk innhold og faktiske prøver. SC02-scener, eksisterende flagg og originale SC01-opptak er bevart.

Kontroll: build_pack.py --check og 36 Python-kontraktprøver består. Eksportene omfatter både lesefiler og data til eksisterende runtime; de etablerer ingen fysisk observasjon eller effekt. Den nye Caliche-meldingen krever egen fullført readback og SC02-flukt, og er tilgjengelig selv om SC01-vindushendelsen ikke ble observert.

Les [generert scenariokontrakt](generated/SCENARIO_CONTRACTS.md), [ni produksjonskort](generated/MISSION_CARDS.md) og [verdensprodusentmatrisen](generated/SCENE_MATRIX.md) for gjeldende kildegrunnlag. Den eldre gjennomgangen nedenfor beskriver tidligere forfatterversjoner.

## Historisk forfattergjennomgang fra 25. til 27. oktober


Gjeldende produksjonskilde følger [sammenflettet hovedscenario SC01 til13](../../MISSION_FLOW_1800_ROSWELL.md). `scenarios.json` knytter13 scenarioer og personkunnskap til de eksisterende ti H-kortene.79 engelske scener omfatter de tidligere36 og43 nye delscener for aktiv kveld, inntrengermeldinger, hjelpekontakt, BY02, ARRAY-01 og siste rutekontroll.16 forfatterdokumenter omfatter fem eksisterende og elleve nye, med egne kvitteringer for første melding, utkalling, savnetsak og lokal plansperre. Dette er forfatterkilder med verdensporter, ikke nye spillhendelser eller opptak.

Kveldshjelperen er ny Mara Ortiz, district maintenance, hvit servicevogn23. Hun identifiseres ordinært før synlig tap. Ingen nødvendig eksisterende person fjernes. Første melding, faktisk utkalling, observert løft, etterspor og savnetmelding har hver sin eksterne faktaport. Ward suspenderer lokal06:00-serie tidlig; fysisk auto-/surveyplansperre er en annen kontroll. H08 viderefører denne ordren og gir senere nettverksvarsel etter uavhengige fjernkilder.

BY02 har egne ruter for identifisert originalspole før Earl går04:00 og legitim datert kildekopi etter avgang. TAPE og LIVE MONITOR skal faktisk frakobles hver for seg. Nytt spørsmål får gammelt uendret fragment. Kopi føres som kopi; Earl får ingen replikk etter avgang. ARRAY-01 bruker EveningShifts separate `EV_DALE_ARRAY_SERVICE`, med A / ROW 1 WEST, B / ROW 1 EAST og REF / R1. Fritt A→B/B→A, kort retur, faktisk geometri og separat kommando-/drivkontroll er nødvendige. Lyd gir tid, ikke peiling. T02s kulvertindeks bevares.

Dato: 26. oktober 2026. Engelske produksjonsscener for H01 til H10, skrevet etter HORROR_STORY_PLAN. Dette er nytt manus og adaptergrunnlag. Ingen av de nye oppdragene, fjernmålingene, fiendereaksjonene eller avtalene er koblet inn i spillet. Ingen ny lyd eller film er produsert.

- [Hovedscenario](../../MISSION_FLOW_1800_ROSWELL.md) og [sekundært løsningsoppslag](../../MISSION_DETAILS_REFERENCE.md): 13 sammenhengende scenarioer med alle tidligere32 arbeidskort bevart. Nye delscener blir ikke automatisk skrevet eller integrert av generatoren.
- [Lesemanus](generated/SCRIPT.md): 79 scener med konkrete replikker, observasjonstekst og uavklarte utfall.
- [Scenariokontrakt](generated/SCENARIO_CONTRACTS.md): SC01 tilSC13, eksterne resultater, personkunnskap og tydelig kildeeierskap.
- [Produksjonskort](generated/MISSION_CARDS.md): spørsmål, fysisk handling, kontroll, følge og vei videre for alle ti oppdrag.
- [Bevis og personkunnskap](EVIDENCE_AND_ROUTE.md): hovedrutespor, varsling, siste reisemål og kildebegrensninger.
- [Gjeldende rute og målehull](CURRENT_ROUTE.md): dagens kildeimplementasjon, transporter, paneler, venting og skrekktopper. Menneskelig måling står åpen.
- [Adapterindeks](generated/integration_index.json): kobling fra H-numre til scener og eksisterende kilder/ID-er.
- [Opptaksplan](generated/recording_manifest.json) og [undertekstgrunnlag](generated/subtitles.json): nye cue-ID-er, uten oppdiktede lydtider.

`pack.json`, `missions.json`, `scenarios.json` og JSON-filene i `modules/` er forfatterkildene. `build_pack.py` gjenbruker EveningShift-formatets strenge validering av kanon, personer, kanaler, flagg og referanser. Den lager lesefiler og `HorrorPackData.inl` i denne mappens `generated/`. Den endrer ikke kveldspakkens opptak eller genererte filer og kjøres ikke av spillet; compiled data leses av den eksisterende HorrorPack-verten.

```sh
python3 unreal/Narrative/HorrorStory/build_pack.py
python3 unreal/Narrative/HorrorStory/build_pack.py --check
python3 unreal/Narrative/HorrorStory/test_pack.py
```

H-numrene erstatter ingen P-, N-, A-, T- eller E-ID. Nye `HR_*` er separate forfatterdokumenter. `ctx_hr_*` er eksterne fakta som senere adapter må etablere gjennom faktisk handling. En replikk eller åpnet panel kan ikke erklære en måling gjennomført. `hr_*_recorded` betyr bare at et allerede kontrollert resultat er presentert.

Eksisterende kveldspakke beholder ansvar for innkjøring, mat, overlevering, kontordokumenter, T01/T02 og VHS. Denne pakken skriver den sammenhengende hovedmotivasjonen og nye mellomledd. Ved senere integrasjon skal samme overlevering eller kontorinformasjon ikke spilles to ganger. Nye scener som omarbeider gamle replikker får egne opptak; ingen gamle lydfiler antas å passe automatisk.

H1.2 til H1.4 har nå konkret produksjonsgrunnlag. H1.1 er fortsatt delvis: ruten er kildekartlagt, men ubrutt menneskelig gjennomspilling, opplevd frykt og samlet spilletid er ikke målt.

Senere kildepresisering 25. oktober: H02s utilstrekkelige resultat lar kalibrering fortsette, men nye T01 R2-saker trenger fullført inspeksjon og fysisk filing før Survey. H09 bruker E15, det gamle innrammede 1947-utklippet, uten 04:45-port. Dagens avis i byen er en annen kilde. Ekstern sikring i H08 er en konkret diktert måleavskrift og Wards daterte kvitteringslogg; foto og negative blir med originalene. JSON-kildene er rettet og eksportene regenerert; ingen nye opptak eller runtimeendringer inngår.

## Kildekontroll26. oktober2026

Generator og20 kontraktprøver består, inkludert negative prøver av manglende readback, manglende synlig bortføring, fysisk plansperre, selvproduserte verdensfakta, uisolert kilde, kopiens opphav, Earl etter avgang, arraygeometri/retur, fastlåst A/B-rekkefølge, evidens ved lokal retry og manglende fersk rute-/budkvittering. Sju genererte lesefiler er byteidentiske ved `--check`. Dette beviser kildekontraktene; ingen ny spillkode, faktisk personmodell, sveip, inntrengersans, reiserute, avspillingsutstyr, lyd/film, native disk-Continue eller menneskelig frykt/pacing er prøvd av denne pakken.

Alle nye `hr_*_presented` er presentasjonsstatus. Eksterne `ctx_hr_*` må komme fra faktiske verdenshandlinger. Hver lokal retry bevarer utført arbeid og gir ingen ny bekreftet observasjon, gammel bortføringsteller eller tapt tid. Manusadapteren skal ikke utlede måling eller flukt fra et åpnet dokument. Konkrete tidspunkt, retninger, valgte ståsteder og dataverdier i kvitteringer fylles fra det faktiske resultatet ved senere integrasjon.

Etterkontroll26. oktober: to semantiske porter er delt. Aktuell TAPE/LIVE-velger er gjensidig utelukkende; `BY02_SOURCE_COMPARE` bruker nå lagrede fullførte kildeprøver, ikke begge nåtilstander samtidig. Arrayets innledende kommando-/drivbaseline krever ingen fremtidig A/B-runde; slutthistorikken kontrolleres separat etter returen. Validatoren avviser tilbakefall til begge problemene og direkte selvkrav om scenens egen presentasjon. Tidligere faktisk hørt Dale-linje kreves før annexfragmentet. Beleiringsvarsel/kompositt krever ferdig innsamlet dokumentbunt etter04:04. Første telefonmelding påstår ikke at merkene allerede er undersøkt; lampetesten påstår ikke at returfiling er gjort ved bilen.

En uavhengig lesekontroll fra LineLights-hjelperen bekreftet de to rettede portdelingene og fant at navngitt utkallingskvittering kunne foregripe Ortiz-avtalen. `SC03_DISPATCH_RECEIPT` krever nå den faktiske navngitte oppdragsavtalen i tillegg til meldingsreadback; egen negativ prøve dekker dette. Ingen runtime er prøvd av denne kildekontrollen.

Siste kildekorreksjon: sen BY02-kopi krever faktisk isolert avspilling av den identifiserte kopien og egen `ctx_hr_by02_question_compared`. Konvolutt-/loggmetadata alene gir ingen påstand om hørt fragment eller manglende nytt svar. Den tjuende kontraktprøven avviser manglende faktisk spørsmålssammenligning.

## Verdensprodusenter og scenematrise, 27. oktober 2026

`world_producers.json` er en ny planleggingskilde. Den navngir produsenten i spillkoden for alle 96 eksterne flagg, med fil og symbol som `build_pack.py` kontrollerer ordrett mot kilden. Den har også én matriserad per scene (79), 117 navngitte prøver og en arbeidspakke per SC. `generated/SCENE_MATRIX.md` er lesbar versjon; stemmecues og opptaksstatus hentes fra opptaksmanifestet. Dette er planleggingsdata og endrer ingen runtimeeksport. `test_pack.py` har nå 31 prøver, sju av dem for produsentkartet. Se [forberedelsen av SC02 til SC13](../../SC02_TIL_SC13_FORBEREDELSE.md) for status og avklaringer.

`voice/generate.py --scenario SCnn` velger cuene for ett scenario. Uten valget lages de fem SC01-opptakene som før. Roller uten profil i `casting.json` stopper kjøringen.
