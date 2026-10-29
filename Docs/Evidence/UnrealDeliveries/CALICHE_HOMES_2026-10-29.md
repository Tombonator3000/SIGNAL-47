# Caliche-boliger og innkjøringsskilt

**Status:** Levert som målrettet pakket kandidat R2; [PR219](https://github.com/Tombonator3000/SIGNAL-47/pull/219) mot main. **Gjelder:** ENV-02, kilde `1665723fda9484b168755709089678fcfb8e6749`, UE 5.8.4/Linux/Vulkan. **Sist kontrollert:** 29. oktober 2026 kl. 06:02 CET.

[Pakket R2-kvittering](../../../unreal/Evidence/ENV02-2026-10-29/packed-r2-receipt.json) binder den faktiske binæren, alle åtte payloadfiler, rute-/fallbackresultater og de to vedlagte originale pakkede skiltbildene. Dette er en akseptert kandidat, uten ny installasjon eller offentlig release.

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
| Editorbygg | R2, R3 og endelig R5 PASS. | R1 avviste TMap-likhet før kildefrys; R4 avviste skygget variabel og TObjectPtr-deduksjon i testkode. Råforsøkene beholdes. |
| Editorens innkjøringsrute og Off-fallback | Rute R4 PASS, 634 kontroller og seks komplette PNG-er. Separat Off-fallback R1 PASS, 481 kontroller. | Tidlige ruter R1/R2 ble avvist ved start på testforventninger til normalt flyttet A02-figur og fabrikkopprettede etterkommere. Ingen menneskelig input hevdes. |
| Pakking og kildefingeravtrykk R2 | PASS, source `1665723f`; 13 459 fryste runtime-inputfiler, 8 384 371 451 logiske byte, uendret gjennom pakking. Åtte payloadfiler er SHA256-bundet. | Pakket R1 er avvist og bevart: TownSignal Head/Span manglet i cook, slik at TB2 og Homes korrekt beholdt originalene. R2 inkluderer cook-avhengigheten uttrykkelig. |
| Pakkede selvtester R2 | PASS, 1709 kontroller i samme ruteprosess. | Ingen fullmatrise eller ytelsesaksept. |
| Pakket innkjøringsrute R2 | PASS, 634 kontroller på 126,6301 s: 469,75269 m nord, 334,90908 m sør og 10,141795 m faktisk Walker-gange. Exit 0, ingen registrert tungjobbkonkurranse. | API Autopilot/TestInput/TestMove. Sørgående start og første case1-lagring er uttrykkelige isolerte fixtures. |
| Pakket disk/title/Continue R2 | PASS, fersk diskleser, ordinær title/Continue/full Flow, bevart klokke/bil/player/dører/pack og etterfølgende vanlig Flow-lagring ved ledig SaveBlock. | Samme native verden og systemer gjenbrukes; dette er ingen ny map-/UWorld-innlasting. Personlige Saved er ikke brukt. |
| Pakket samlet Off-fallback R2 | PASS, 481 kontroller på 7,9813 s. Alle åtte gamle TB2-meshforgjengere, 28 TB2-deler, beskyttede bindinger og fire gamle stolpebokser beholdes; ingen Homes-noder eller nye stolpebokser. | Eksplisitt Off, ingen fysisk reise, PNG eller fallback-Continue. Korrupt/manglende assembly er ikke simulert i denne prøven. |
| Pakkede bilder R2 | Seks faktiske PNG-er på 1600×872 har komplett signatur, alle chunk-CRC-er, IEND ved EOF og uendret hash. Alle seks er gjennomsett i kontaktflate; begge skilt er åpnet i fullstørrelse. | Førerhustekstens subjektive lesbarhet er fortsatt UNVERIFIED. Bilder er ikke FPS- eller menneskelig kjørebevis. |

Nordturen bruker ordinær `BeginCommute` ved z2335 og normal World/Drive-oppdatering, uten bilplassering under turen. Etter normal brems etableres én uttrykkelig separat sørgående startfixture ved z1875; resten av sørfragmentet bruker samme produksjonskontroller. `GetOutHere` gir faktisk avstigning. Walker-runden holder seg innen den opprinnelige hovedgatesonen, med faktiske bakkestråler; ingen fasadegange eller adgang til lukkede boliger hevdes.

Den faktiske utviklersakens snapshot kopieres uttrykkelig til isolert case1-disk før ordinær title/Continue. Deretter brukes vanlig `Flow.WriteSave` og en ny diskleser. 64 faktiske beskyttede noder og fire proxybindinger avleses etter Continue. Eksporterte hus-/dør-/TV-/kjøkken-/smoker-ankre kontrolleres mot rå eksport; normal A02-referanse og Props/Anomaly-komponenter bindes til sine eksisterende produsenter. Patruljebilens to linser/to lys og figurens prisetikett beholder faktisk komponent-, mesh-, material- og poseidentitet. Ingen generell dispensasjon for ukjente etterkommere er innført.

Off-prøven leser de fire gamle stolpeboksene både i aktiv gangkollisjon, `DriveData` og den allerede opprettede kjørerens hindringskopi, før og etter vanlige produksjonsrammer. Gamle plasser ved z1880/2335 finnes nøyaktig én gang; tilsvarende nye plasser ved z1895/2315 mangler. Meshnavn, faktisk synlighet, trekanttall og opprinnelig materialkjede er avlest uten filflytting eller material-/posisjonsforcing.

[Sørskilt fra ordinær nordgående start i pakken](../../../unreal/Evidence/ENV02-2026-10-29/packed-r2-south-sign-northbound-start.png) og [nordskilt fra separat sørgående fixture](../../../unreal/Evidence/ENV02-2026-10-29/packed-r2-north-sign-southbound-fixture-start.png) er originale bildebytes. [Bevisinngangen](../../../unreal/Evidence/ENV02-2026-10-29/README.md) skiller tidligere editorbilder fra pakket R2.

Binæren `candidate-r2/Linux/Signal47/Binaries/Linux/Signal47` er 314 163 392 byte, SHA256 `1576b56fe68ca0bf34b4227f0da1d074f576c98212cb0df073f1d867e5d2fc2e`. Den lokale `candidate-r2-manifest.json` har SHA256 `df901b3b04402084cbd324b90987d6fda6aa73d5a903d4eb03c0c448d1b048fb`; `runtime-inputs-r2.json` har `dcc5fb8ae42414951ce2d0ccef5f9dcf5e6b84aacc58304485b0d890853fd3c5`. Alle payloadhasher er tatt inn i den portable R2-kvitteringen lenket øverst.

Rågrunnlaget i `~/scratch-bm/Signal47-caliche-homes-20261029` omfatter `package-plan-r2b.json`, `homes-packed-route-r2.log`/`.result.json`, `homes-packed-fallback-r2.log`/`.result.json` og fingeravtrykkene. Detaljresultater og disk-/poseavlesninger ligger under `/tmp/S47-caliche-homes-packed-route-r2/Saved/CalicheHomesContract/01A1860262B07CD6B354610914B1F7A3` og fallbackens `/tmp/S47-caliche-homes-packed-fallback-r2/Saved/CalicheHomesContractFallback/01A186047CC07A979ED5190F26DECFAD`.

Bibliotekssøket gav ingen prøvd erstatning for prosjektets eksisterende Unreal 5.8.4-løype. Generelle FBX- og Meshy-referanser er ikke brukt som nativebevis. Pakkeloggen har et VSM-varsel om Non-Nanite Marking Job Queue; ytelse, nattlige husplaner, subjektiv lytting og menneskelig input er ikke akseptert. Ingen fullmatrise, installasjon i Games eller offentlig release er utført. Avviste R1-forsøk og øvrige rålogger er bevart; PR219 går mot main med eksakt head og kontroller verifisert før fletting.
