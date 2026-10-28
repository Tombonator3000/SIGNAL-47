> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Besøksparkering, inngang og Dales kontor

26. oktober 2026. Bestilling fra tre spillbilder: naturlige små besøksskilt, ankomst hvor som helst på parkeringsområdet, en ordentlig inngang ved rampen og tilgjengelig, bedre modellert Dale-kontor.

## Endringer

- To originale Blender-skilt på 46 × 60 cm erstatter tre 2,30 m brede tavler. Emaljeflate, avrundet plate, U-profil, bolter og lesbar P / VISITOR / PARKING-tekst.
- Personbilens midtpunkt kan stå hvor som helst i den asfalterte besøksflaten, X[-18.2,-7.8], Z[10.5,20.5], i enhver retning. Oppmerkingen er veiledende. Vanlig kontroll krever fortsatt fart under 0,35 m/s i over 0,7 sekunder. Kollidering og servicebilens egne regler beholdes. Faktisk bilstilling lagres. Gamle stasjonære saker på området fullfører samme ordinære ankomsthandling ved lasting.
- Den eksisterende vestre rampedøren er registrert som visitor entrance. Samme dør-ID, hengsel, gangsoner og lagring beholdes. Blender-modeller gir baldakin, beslag, karm, terskelrist, glatte rekkverk, håndtak, sparkplate og ENTRANCE-skilt. En liten SARO / RECEPTION-plakett er plassert ved siden av døren. Ny kort gangsti kobler parkeringen til rampen.
- Dales kontor kan undersøkes ved ankomst og på kvelden. Nøkkelen ligger i den fysiske serviceskuffen; den låser opp en faktisk dør og gangbar terskel. STATION 01 beholder sin egen historieport. Åtte originale Blender-assemblies gir stålpult, kontorstol, skap, bevegelig skuff, nøkkel med tagg, dørbeslag, romflater og innredning. Faktisk leste dokumenter gjenopprettes i notatboken ved lasting, uten å gi ulest materiale eller fremtidige historieflagg.

VHS-filmen er fortsatt ikke produsert. Denne bestillingen gjør ikke den gamle filmpakken spillbar eller hele den planlagte nattutvidelsen ferdig.

## Kilder og modeller

- [Inngang og skilt](../../../unreal/ArtSource/ArrivalEntrance/README.md), med `SARO_ArrivalEntrance.blend`, tre native assets og tre LOD-er per asset. 13 982 LOD0-trekanter totalt.
- `ArtSource/Office/Signal47_Operations.blend`, åtte native assets, 406 redigerbare komponenter og 62 176 LOD0-trekanter. Egen import/verifikasjon i `Signal47/Tools/office`.
- Native innhold ligger i `/Game/Signal47/Art/ArrivalEntrance` og `/Game/Signal47/Art/Office`. Alle modeller og teksturer er originale. Ingen nye kjøpte assets eller endringer i Unity/web.

Prosjektbiblioteket ble søkt først og deretter én gang bredere. Profilene `ahujasid--mcp-for-blender` og `x-2108195576206488058` ga bare generelle, ikke lokalt verifiserte referanser. Ingen bibliotekskode eller eksterne assets ble kopiert. Prosjektets eksisterende målte FBX/PBR-import ble brukt.

## Kontroll og levering

Arbeidsbevis: `/home/tom/scratch-bm/Signal47-arrival-office-20261026`.

Blender-gjenåpning, native import og fersk native gjenåpning er gjennomført. Første pakke består 1 525 selvtester og ordinær kjøring, ankomst, faktisk gange begge veier via rampen og disk-/verdenslasting. Den avdekket samtidig et hult siktepunkt i nøkkelringen, et overlappende resepsjonsskilt, overbelyst nattinngang og 80,146 MiB resident RT-geometri mot eksisterende 80 MiB-varselgrense. R2 retter nøkkelens siktepunkt, plakett og nattlys, og består faktisk kameratreff/Player::OnUse gjennom begge inngangsdørene og 21 bildeoppsett. R2 avdekket også at kontorlys feilaktig fulgte RX bank 3, samt at den nye åpningstesten leste feil klokketype. Romlyset følger nå bygningens ekte strømbrudd, og testen leser kveldslagringens kanoniske klokke. Første forsøk på separate RT-proxyer gav ingen målt effekt: UE 5.8.4 bygger dem bare for Nanite-mesher. Rom og innredning bruker nå Nanite med full kilde-/rasterfallbackgeometri og separate 60 % RT-proxyer. R3 måler maksimalt 79,811 MiB i 598 aktive prøver, med hardware RT og uendret 80 MiB-grense. Ingen global terskel eller grafikkfunksjon er slått av.

R3 består fysisk kontor ved ankomst, eldre kapittel én, Low, kontorlagring, 1 525 selvtester og eksisterende T02 med 65 sjekker og 15 ferske disk-/verdensinnlastinger. Bildekontroll bekrefter innredning, lys, nøkkel før opptak og gjenopprettet kontor. Tverrstilt parkering registrerte korrekt ankomst, men den automatiske gangruten klippet en eksisterende pullert. R4 endrer bare denne testruten til en fysisk passasje rundt den faktiske bilen og gjennom den nye nordre gangstien; produksjonskode og assets er uendret fra R3.

## Feil grunnlag, tilbakeføring og samlet R5

R4 bestod sine målrettede prøver og ble midlertidig installert. Deretter viste installasjonskvitteringens tidligere binærhash at standardspillet allerede var NP1 (`5621b29d…`, testet kilde `2c05e33d`), mens arbeidskopien var på eldre `41430ade`. R4 ville dermed fjerne tidligere leverte funksjoner. Dette var en reell integrasjonsfeil, ikke en feil i spillerens lagring. Den komplette tidligere NP1-payloaden ble straks gjenopprettet og hashkontrollert. Alle 44 personlige Saved-filer/61 noder var uendret. `rollback-newer-baseline.json` dokumenterer tilbakeføringen. Den planlagte R4-oppryddingen ble ikke anvendt.

R5 tar inn den eksakte testede NP1-deltaen: seks private Dale-tekster og eget arrayserviceark, faktisk lest/ulest-status, normal/stor tekst og avgrenset rulling, fem himmelfaser med separat opptrådt/observert/fullført-status og korrekt LineLights-sikt/lagring. 43 runtime-/narrativfiler ble kopiert byteidentisk og Evening.cpp fikk de fire NP1-linjene for fysisk lesevalidering; Office.cpp/.h ble flettet med de nye modellene og tidlig adgang. `np1-source-integration.json` og `office-np1-merge-source.json` binder endringene. Kuassets/-kode og eksisterende Audio/Free/Web/Props/Kelvora-avhengigheter var allerede byteidentiske med NP1 og ble ikke erstattet. Den separate SC01-kandidaten var ikke installert, inngår ikke i R5 og er beskyttet.

## Avgrenset sluttkontroll av R5

Etter Toms bekymring om at prøvingen tok for stor plass, ble siste runde begrenset til berørt funksjon og én samlet oppstart/bilde-/RT-prøve. Ingen ny 110-matrise eller helnatt ble kjørt. Alle seks kjøringer bruker samme R5-pakke og egne profiler:

| Pakket kontroll | Faktisk resultat |
| --- | --- |
| Kontor ved ankomst, High | 10 kontroller, fysisk skuff/nøkkel/dør/papir, 2,95 m faktisk gange og fersk disk-/verdenslasting før telefonsamtalen |
| Dale-dokumenter, Low | 191 kontroller, 5 disk-/verdenslastinger, separat lesestatus, faktisk stor tekst og PageDown/PageUp |
| Gammel tverrstilt parkeringssave | Ankomst på området, ekte gangrute og kameratreff/bruk gjennom begge inngangsdører, bevart bilstilling etter lasting |
| Samlet oppstart, bilder og RT, High | 1 581 selvtester, 21 bildeoppsett og kontrollert avslutning |
| LineLights, High | 12 kontroller, 3 disk-/verdenslastinger, sikt/okklusjon, pause, avbrudd og legacy |
| Himmelforløp, High | 16 kontroller med faktisk sikt og full verdensgjenoppretting |

Ni utvalgte R5-originalbilder av inngang, skilt, kontor, nøkkel og dokumentvisning er faktisk vurdert i `runtime-r5/visual-review.json`. De øvrige opptakene er bevismateriale, ikke individuelt pixelgodkjent. R5s aktive CSV-utsnitt har 596 prøver og 77,2798 til 79,811 MiB AlwaysResident RT-geometri; eksisterende grense er 80 MiB. Dette er ikke total VRAM, FPS eller nattens maksimum.

R5-binærens SHA256 er `93e2fad3aa156325b8083159fc09828bc474731d9d05832263efb1c01cbfd890`. `source-snapshot-r5.json` binder produksjonsinngangene, og installasjonskontrollen krever samme pakke i funksjons-, bilde- og RT-bevisene. Samme R5 er installert i standardspillet. `installation-r5.json` er `INSTALLED_VERIFIED`; `installed-smoke-r5/result.json` er PASS på alle åtte kontroller gjennom faktisk standardstarter. Payloaden er identisk med den testede pakken, og alle 61 personlige Saved-noder, inkludert 43 regulære filer, er uendret før/etter. De nye testprofilene ligger separat.

Canonical prune er faktisk anvendt etter leveringen. `prune-applied-r5.json` inneholder plan og utføringsresultat: ti gamle pakker/70 genererte payloadfiler pensjonert, 38 233 681 084 logiske bytes. Gjeldende R5 og to eldre komplette R4/R3 beholdes, sammen med den eksplisitt beskyttede aktive SC01-R4-kandidaten. Kilder, Blender, native assets, personlige Saved, Engine-filer, logger, bilder og bevis er bevart. Logiske bytes er ikke en påstand om faktisk frigjort diskplass. R4 og R3 mangler NP1-funksjoner og skal ikke brukes som ny standard ved en eventuell senere tilbakeføring.

## Gjenstående avgrensninger

Eldre utvendige lampehus er ikke fullt oppgradert. `exterior-light-audit.json` bekrefter ekte native lyskilder i kode, men beskriver grove hus og kildeberegnede monteringsfeil: 12,3 til 17,5 cm gap bak fløylampene, parkeringsarmatur/emitter inne i veggen og ingen egen armatur-/diffusormodell under ny baldakin. Dette er koordinatfunn fra kilde, ikke nye runtime-målinger. Modellering, montering og etterkontroll står åpne.

## Fab etter R5-leveringen

Offisiell Fab 0.0.13 er installert i `/home/tom/.local/opt/UnrealEngine-5.8.4/Engine/Plugins/Fab`: 190 byteidentiske filer og matchende Build ID 55116800. Native plugin og innlogging er faktisk brukt. Via pluginens + er «Modular Post Apocalyptic Desert Environment» fra Just B Studios (`be5ed72e-f542-41f4-b747-bdb343eef609`) importert bare til `fab-plugin/FabStaging/Content/JustBStudios` og tilhørende external actors. Innholdet har 518 pakkefiler/1 977 336 454 bytes. Native assetregister viser 381 leverandørassets, inkludert 93 StaticMesh, 181 Texture2D og 3 World.

`fab-plugin/final-fab-report.json` og `desert-content-inventory.json` binder faktisk import og bevarte filer. Etter vanlig lukkeforespørsel og lukkede vinduer fikk editorprosessen en avslutningsfeil, returncode -11. Den er stoppet og maskinens jobbvern er kontrollert ledig; ren editoravslutning er ikke bestått. Import og inventar var ferdig før avslutningen, og filene består etterpå.

Old West Learning tilbød «Install from launcher» i UE-pluginen, dokumentert i `old-west-open-ui.json.png`. Etter brukerens godkjenning og innlogging ble pakken lastet ned gjennom den [offisielle Linux Epic Games Launcher Preview](https://launcher.epicgames.com/) 0.103.0, via Develop → Fab → My Library → Create project. Launcher viste «Recently completed / OldWestLearning / Completed» uten aktiv nedlasting. Ingen ekstern downloader er brukt.

`fab-plugin/old-west-launcher-report.json` dokumenterer komplett prosjekt i `/home/tom/.local/share/com.epicgames.eglv2/FabLibrary/Old West Learning-785da98f/OldWestLearning.uproject`: 3 227 filer/20 485 854 361 bytes, hvorav 3 207 uasset og 12 umap, ingen uferdige eglpart. EngineAssociation er 5.1. Prosjektet er ikke åpnet, konvertert eller importert i UE 5.8.4; Launcheren er latt åpen med brukerens innlogging.

Begge pakkene er dermed lastet ned, men ingen av dem er migrert eller plassert i SIGNAL / 47-verdenen. Ingen MetaHuman-figur eller full lampeoppgradering er produsert. Fab-arbeidet endrer ikke installert R5.

## Terrengvurdering, uten implementering

`terrain-assessment.json` anbefaler sammenhengende grunn med målrettede stein-/klippemesher og naturlige grusoverganger. Epics innebygde Mesh Terrain er Experimental i lokal UE 5.8.4 og kan vurderes på ett avgrenset klippeparti før eventuell videre bruk. Ingen kjøp, aktivering, ny terrengmodell eller test er gjort. Spillbare høydefunksjoner, historiegulv, parkering/ramper og den delte SARO-/veitransformen må bevares. Nedlastede naturassets er kandidater, ikke visuelt godkjent eller ferdig integrert terreng.

VHS, nye aktive møter, samlet SC-/kveldskoordinator, menneskelig fullnatt, lytting og fysisk Xbox er ikke attestert av denne leveransen. Ingen ekstern spillutgivelse er publisert.
