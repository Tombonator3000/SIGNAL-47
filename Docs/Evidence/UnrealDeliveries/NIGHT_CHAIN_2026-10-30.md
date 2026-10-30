# Faktisk nattkjede fra ny sak

**Status:** Under arbeid. Ingen prefikskjøring eller hel natt er akseptert ennå. **Gjelder:** SC01 til SC13, Night R1 til R5 på de eksakte byggene i kvitteringen. **Sist kontrollert:** 30. oktober 2026.

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

Ny vanlig relay-handling sender arrayet tilbake til kalibrering 042, venter på faktisk drift og beskytter surveyholdet mot mottakerens azimutknapp. Den er bygd i R7 / 6abfab4d, men R5 nådde ikke denne handlingen. Fysisk 27/042-hold, T01/SC02/SC03, den videre faktiske SC04 til SC13-kjeden og finale-RHI er fortsatt UNVERIFIED. Ingen pakke, fullmatrise, installasjon eller offentlig release inngår i disse forsøkene.
