# Caliche-boliger og innkjøringsskilt

**Status:** Under arbeid. **Gjelder:** ENV-02, egen kildegren uten installasjon. **Sist kontrollert:** 29. oktober 2026.

Fire bolighus har egne Blender-kilder og tolv native deler med tre forfattede detaljnivåer. Dører, trinn, de tre levende vindusbildene, H02-glasset, mennesker og eksisterende interaksjonsankre beholdes. Husene åpnes ikke for innvendig utforskning. Materialinstansene bruker byens eksisterende PBR-foreldre og teksturer.

528 gamle hustrekanter erstattes presist. Alle 48 trinntriangler og H02s to glasstriangler beholdes. To originale skiltkopier beholder grafikk, tekst, UV og retning, men flyttes fra z1880/2335 til z1895/2315. Sørskiltet står dermed foran den uendrede commute-starten ved z2335. Hver stolpe følger sitt eget terrengpunkt; fire stolpehindringer flyttes samlet i både gang- og kjøredata.

| Kontroll | Faktisk resultat | Begrensning |
| --- | --- | --- |
| Blender-kilde og fersk leser | PASS, 424 kontroller; faktisk lagring, gjenåpning og 36 FBX-filer. | Ingen runtimeaksept fra kildetesten. |
| Forfattede detaljnivåer | 8636 / 5932 / 3880 trekanter samlet. | Ingen målt FPS eller menneskelig vurdering av overganger. |
| Presise kutt og skilt | PASS; 528 hustrekanter, 76 skilttriangler og bevarte trinn/glass. | Lett geometri- og kildekontroll. |
| Trim-FBX | PASS, sju faktiske filer med hjørnenormaler, farger, UV og materialordning kontrollert. | Første forsøk avviste indeksert normallagring; neste avslørte standard sRGB-eksport. Leseren dekoder faktiske indekser, og eksporten bevarer lineære kildefarger. Ingen verdiavviksgrense er lempet. |
| Native-import R4 | PASS, 2642 kontroller, 19 mesh og 15 materialinstanser. | Ingen komponentaktivering fra import alene. |
| Fersk lagret native-leser | PASS, 1371 kontroller; alle 34 uassets og 291 Town-avhengigheter er hashbundet. | Leseren endrer ingen assets. |
| Staging | PASS med identiske import-/leserbevis og komplett faktisk Town-inventar. | Hele forgjengeren beholdes ved avslag. |
| Statiske kamerabilder R1 | PASS, 18 PNG-er fra faktisk editorbinær bygget fra 96e56b0d, åpnet i daglys, skumring og natt. Begge skilttekstene er synlige. | Kamerafixtures; ingen kjøring eller husinngang. Kjent rendereradvarsel om resident RT-geometri forekommer. |
| Editorbygg | R2 og R3 PASS. | Første bygg ble startet før kontrakten var fryst og avvises på TMap-likhet i testkode. |
| Innkjøringsrute | R1/R2 avvist ved start på testforventninger til normalt flyttet A02-figur og fabrikkopprettede descendants. | Ingen kjørt rute, disk-Continue eller samlet fallback er ennå akseptert. |
| Pakket kontroll | UNVERIFIED. | Må kvitteres fra konkret kandidat. |

Innkjøringskontrakten skal bruke ordinær nordgående commute-start og normal kjørekontroll, med sammenhengende posisjonsmåling. En separat sørgående start er en uttrykkelig utviklerfixture. Gange begrenses til den eksisterende hovedgatesonen; ingen fasadeadkomst hevdes. Lagring prøves i isolert case 1 med fersk leser og ordinær title/Continue. Faktiske bilder må ha komplett PNG, CRC og IEND før testen kan bestå.

Rågrunnlaget ligger i `~/scratch-bm/Signal47-caliche-homes-20261029`. Bibliotekssøket gav ingen prøvd erstatning for prosjektets eksisterende Unreal 5.8.4-løype. Generelle FBX- og Meshy-referanser er ikke brukt som nativebevis. Ingen fullmatrise, installasjon i Games eller offentlig release er utført.
