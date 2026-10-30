# Faktisk nattkjede fra ny sak

**Status:** Under arbeid. Ingen prefikskjøring eller hel natt er akseptert ennå. **Gjelder:** SC01 til SC13, Night R1 til R9 på de eksakte byggene i kvitteringen. **Sist kontrollert:** 30. oktober 2026.

Prøven starter en ordinær tom case1 fra ny sak, kjører personlig bil til SARO, går inn med native Walker/kameraray og gjennomfører D01. Den bruker faktiske spillhandlinger, dokumenter og disk/fullFlow. Senere kapitler skal bare konsumere faktiske foreldre med søsken-PASS, samme chain_id og eksakt filhash. Navngitte scenariofixturer i andre rapporter er separate målrettede bevis.

| Forsøk | Faktisk resultat og grense |
| --- | --- |
| R1, 32,649s | FAIL før pendling. Kontrakten sjekket kjørestatus før den vanlige 0,4s callbacken. Rettet med avgrenset venting på faktisk oppstart. |
| R2, 258,279s | FAIL ved D01-fokus etter 2623m pendling, 62m gange og én disk/fullFlow. Rettet faktisk fokus på den viste Continue-knappen og tast ned/opp. |
| R3, 460,848s | D01 og Observing videre, deretter FAIL ved østdør. Ekte ankomst hadde allerede åpnet døra; testbruken lukket den. Nytt raybevis åpner bare ved behov. |
| R4, 522,565s | Dørrettingen og alle fire Observing-løp/ærend gjennomført. FAIL på eldre 22:50-testtid mot ny 23:00-koordinator. Tidsporten måler nå filing og koordinator separat. |
| R5, 661,432s | Ny tidsport PASS med sju ferdige tidsoppgaver, fire ark og nattlyd tilbake. SC01-rapport/Fortsett avvises fordi kun horror.callCursor endres fra 2 til 1. Klokke, posisjon, kildedokumenter og 27 encodere er identiske. Innlastingen klamper en ferdig terminalpeker til siste aktive linje. Produsenten rettes; ingen testport er fjernet. |

R5s rå tidsbevis viser clock_before_file−6882,856, clock_after_file−4200 og clock_after_coordinator−3599,695. Observing er ferdig/inaktiv, Rx.Stage er1, DoneRows er7 og desert_still erfalse. Dette lukker den konkrete eldre tidsfeilen, ikke SC01 eller hele prefikset.

[Portabel forsøkskvittering](../../../unreal/Evidence/NIGHT-2026-10-30/attempts-r1-r5.json) binder bygg, kildefrys, prosessresultater og råfilhash. Rå før-/etterfiler, feilresultater og hver testet modul er bevart privat. Tellerne kan inneholde gjentatte kontroller per frame; de er ikke antall unike testcaser.

R6 fra R8-bygget5e87cf75 har deretter faktisk SC01-PASS og identisk etterrapport/Fortsett. Ny vanlig relay090 sender arrayet tilbake til kalibrering042 gjennom fysisk drift, og senere mottakersøk090 endrer ikke holdet. Outage-råfil ved00:13:47 viser alle27encodere042/48, etter kontroll av K3/strøm/beacons og disk/fullFlow. Denne delgrensen har chain_id01A18AF0B6157401B89384CEA946786A; den er ikke et ferdig nattprefiks. R6 stopper deretter på en faktisk fysisk sperre ved servicebilen, x9,30/z3,08, før T01-turen. Prefikset får FAIL etter740,977s,5292egne kontroller/firedisk,84,55m nativegange og2623,16m pendling. [R6-kvitteringen](../../../unreal/Evidence/NIGHT-2026-10-30/attempt-r6.json) bevarer SC01-PASS og27/042-delen separat fra den mislykkede videre ruten. Testens gangrute rettes rundt det eksisterende kollisjonsgrunnlaget. Fylkestelefonen er kontrollert; T01/SC02/SC03 er ikke ferdigakseptert. Den videre faktiske SC04 til SC13-kjeden og finale-RHI er fortsatt UNVERIFIED. Ingen pakke, fullmatrise, installasjon eller offentlig release inngår i disse forsøkene.


R7 fra R9-bygget `e5913b51` og modul `c48cc043` gjennomfører SC01 og SC02 med faktiske søsken-PASS i samme kjede. Den rettede gangruten når servicebilen med native gange/ray, vanlig kjøring gir tidshopp til 01:35, og T01 gjenoppretter feederen med normalt feillys og rolig lysbue. SC02 gjennomfører første fangst, retry, faktisk flukt og kjøretur tilbake til SARO. Deretter gjennomføres de fysiske Caliche-hendelsene i SC03, inkludert Ortiz, termosen, vogn/spor, savnet-saken og deputy. Adapteren venter på SC03-grensen med273kontroller/16lagringsforløp/531,24m gange; disse tellerne er separate fra root.

R7 får likevel FAIL etter1450,894s ved det siste rutevalget til den parkerte bilen, før retur til SARO. Root har3324kontroller/fem diskforløp/86,90m gange/6859,00m kjøring. SC02-checkpoint er392304bytes med SHA1 `4B93D5440D8A8B8F9649609D60BDA15CAA038F53`. Det finnes ingen akseptert SC03-forelder for SC04 ennå. [R7-kvitteringen](../../../unreal/Evidence/NIGHT-2026-10-30/attempt-r7.json) binder alle delgrensene og det faktiske avbruddet.

Kildeanalysen viser at alle48gamle rutemål lå innenfor bilens konservative, roterte AABB ved den faktiske parkeringsposen. Ny kilde `1f23752b` legger til én kandidatring og leser gulvhøyden ved målet gjennom områdets eksisterende gulvprovider. Alle kollidere og strenge native krav til gange,3D-rekkevidde og ray beholdes; rådiagnostikk skrives før PASS/FAIL. R10-editor består10,171s med modul `1d588f6d`; NightR8 ble kontrollert avbrutt av Codex før ny kildefrys, som beskrevet nedenfor. Den nye ruten er fortsatt UNVERIFIED.


R8 er **INTERRUPTED_BY_CODEX**, uten kontraktresultat. Lesende kildekontroll fant at World::Park setter driving=false straks, men beholder cinematic i0,4s. Den gamle stage19 kunne dermed be om vanlig lagring før transaksjonen var ferdig. Codex sendte SIGTERM bare til sin wrapper med verifisert PID og eksakt privat plan; guard ryddet kun sin egen barnesesjon, og etterfølgende require-idle besto. Wrapperen videreførte InterruptedError uten å skrive night.result.json. Dette er et avbrudd, ikke et nytt runtime-FAIL eller PASS.23råfiler, logger, kildefrys og separat avbruddskvittering er bevart privat.

Ny kilde `4f93a8a3` skriver faktisk parkeringsstatus og venter deretter maks20s på vanlig cinematic-/modalfri tilstand og tom SaveBlock. Den lukker eller nullstiller ingenting; timeout beholder råtilstand og blokkeringstekst. R11-editor består10,175s med modul `ade134e0`. NightR9 starter fra ordinær ny sak med samme638pinnede inputs; både siste bilrute og sluttgrensen trenger fortsatt denne faktiske prøven.
