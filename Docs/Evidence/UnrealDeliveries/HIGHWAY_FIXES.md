> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Sammenhengende highway og åpningstur

24. oktober 2026, `codex/highway-continuity`. Tom melder manglende Roswell-sperre, taus startbil, brå naturskifte ved Motor Court og to parallelle strømnett. Godkjent mål er én detaljert hovedlinje med nødvendige avgreninger, fungerende motor og sammenhengende natur gjennom områdeskiftet.

## Endringer

- Roswell følger historiens eksisterende åpning i kapittel 5/6 også i pendlerbilen. Fire fysiske sperrestolper følger synligheten; den permanente grensen mot Roswell beholdes. Hinderindeksene flyttes ikke.
- Motoren bruker lydregisterets `engineIdle`, `engineDrive`, `engineStart` og `engineOff`. Loopene har faktisk lavpassfilter. Stopp kansellerer oppstartsrampen, og rask ny innstigning beholder ikke gammel stopptid.
- Eksportens tomme veivegetasjon var skjult og ble aldri aktivert etter runtimefylling. Kun de prosedyreeide drawbarna for road/saro aktiveres, med eksisterende frø, dekning og foreldregrense.
- Et felles visuelt landskapslag bruker road som kanonisk ramme. SARO flyttes med Three-offset `(8024,0,-300)` inn i samme ramme; ved områdeskift flyttes bare den visuelle roten tilbake. Geologi, naturdekor og strømnett beholder dermed plassering relativt til spilleren. Logiske områder, spillgulv, ruter og historietilstander beholdes.
- En 200 m overlapp av eksisterende plante-/steinprototyper bruker samme høyde og veidekning som runtimevegetasjonen. Ingen nye gulv eller blokkere. Eksisterende Caliche-HISM beholder lokale instansmatriser.
- Én detaljert hovedlinje på eksisterende road-X15 beholder strømlydens og byens ankre. SARO, motellet, ranchen og trafostasjonen har avgreninger. Gamle stolper og høye ledninger fjernes målrettet; delte gjerdewirer og vrakscenen beholdes. Ufullstendig kilde/import beholder opprinnelig nett atomisk.

## Reprodusering

`Tools/highwaypower/prepare_cpu.py` gjør kun de to ignorerte Web-ledningsmeshene CPU-lesbare før cook. Ingen tredjeparts råassets eller importerte Web/Free-filer legges i Git. Runtimeklonene bevarer vertexdata, materialer og gjerdeindekser og fjerner bare validerte høye ledningstriangler.

`-S47HighwayAudit=barrier|seam` er avgrenset til isolert developerprofil. Valgfri `-S47HighwayShots=<mappe>` tar overgangsbilder. Audit bruker faktisk kjørekontroller, native komponenttilstand og reelle dekodede motoropptak. Isolert PCM-render fra faktiske loopparametre er en numerisk lydkontroll, ikke menneskelig lytting eller måling av hele lydmiksen.

## Kontrollstatus 24. oktober 2026 kl. 22:07

Kandidat H3 fra `464078bef26b45f1c1237b8778957d4dd8bca342` består editorbygg, pakking, 1392 selvtester og syv korte pakkede scenarioer. Sperretesten kjører faktisk bilkontroller og gangport både lukket og åpen, med opprinnelig tilstand gjenopprettet. Dag og natt kjører to reelle områdeoverleveringer hver, motor på tomgang/under kjøring og vanlig utstigning/stopp/ny innstigning. Største samplede kanoniske øyesteg er 2.4055 m dag og 3.7938 m natt; ingen koordinatrammehopp. Diner, takeaway med ferske disk-/verdenslastinger, arkiv og kontrollrom består på samme pakke.

Åtte faktiske før-/etter-bilder er vurdert av root og uavhengig hjelper. Fjellprofiler og én strømlinje består i begge retninger. Dag bruker faktisk Evening-førerhus; natt er en isolert chapter1-fixture og dokumenterer silhuetter, mens bakken er for mørk til å vurdere fine plantedetaljer. Motellets eksisterende bygnings-/lysproxyer skifter fortsatt med området; denne rettelsen gjelder naturen. Geologien har fortsatt eksisterende stiliserte former/materialer.

De to gjerdetrådklonene beholder rastergeometri og materialer, men utelates fra hardware RT-refleksjoner/GI: de er 6 mm brede og kaster allerede ikke skygge. Nye stolper og kraftledninger beholder RT. Originalassets og globalt minnebudsjett urøres. Den røde utvikleradvarselen om 83.107 MiB always-resident av 400 MiB ved 20%-terskelen finnes også i forrige D2-pakkes takeaway-bilder med nøyaktig samme tall. Dette er et eksisterende, urettet punkt, ikke en ny H3-regresjon.

H1 avvises på manglende RT-proxy ved transient mesh. H2 avvises på testfixturens timing mot åpningsturens forsinkede start; bilen kjørte fra byens vanlige startsted i stedet for testpunktet. Begge råforløp er bevart. H3 venter på ferdig StartTrip før fixtureplassering og har korrekt mesh-policy. Ingen skjult konsoll-/grafikkoverstyring for å få rene bilder.

`Signal47/Tools/highwaypower/acceptance.json` binder testkilde, seks pakkehasher, logger og bilder. Råbevis: `~/scratch-bm/Signal47-highway-continuity`. Ingen full 110-scenariorunde eller PSO-kø inngår. Menneskelig lytting/input, full natt og helruteytelse er ikke målt. PR [153](https://github.com/Tombonator3000/SIGNAL-47/pull/153) er flettet i `acaa99d7`, og samme testede H3 er installert kl. 22:11. Alle 40 payloadfiler er hashverifisert, alle 46 Saved-filer er uendret. Forrige payload er verifisert i `/home/tom/scratch-bm/Signal47-highway-continuity/installed-backup-1792872595117367731`. Lokale råbevis har full før-/etter-manifest; offentlig akseptfil inneholder bare leveringsstatus og antall.
