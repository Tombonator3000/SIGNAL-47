> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Mesa Diner, 24. oktober 2026

Mesa Diner har original Blender-fasade og interiør, parkering, neonskilt og trekkvogn med kjølehenger. Den nye fasaden følger konseptets høye kremfargede bånd, sammenhengende røde stripe, dype vindusprofiler, tealfliser og varmt synlig interiør. Skiltet er flyttet tett ved fasaden til Blender `[2.65,-6.8,0]`. Støvete maskert asfalt/grus, sju lokale ørkenplanter og kontaktstein erstatter den rene parkeringsflaten.

Elleve deler har tre LOD-er med 110791 / 54460 / 25430 trekanter, originale PBR-kart og pakket redigerbar `Tools/mesadiner/Mesa_Diner.blend`. Botanikken gjenbruker bare prosjektets egne Caliche-kilder. Ingen nye eksterne assets eller kjøp. Prosjektbiblioteket ga ingen testet dinerressurs; eksisterende original mesh-/FBX-import brukes.

Dinerens faktisk eksporterte root Three(7979.95,4.309006389843889,1999.95), yaw0, gulv, gangsoner, folk og dynamiske kaffe-/telefon-/matobjekter er bevart. Originalsignets ID/handling beholdes med ny fysisk trefflate og lysplassering. Komplett original diner beholdes som fallback dersom det nye kitet er ufullstendig. Skjult original geometri mister også query-kollisjon, og bare faktisk fjernet dekor mister sine gamle hinderbokser.

Døren står fysisk åpen i den eksisterende åpningen. Nytt åpent blad og skiltfot har faktisk gangkollisjon. Ingen ny dørmekanikk, historie eller betalingsregel innføres.

## Kontrollert kandidat D2

Spillkilde `9ff022a3`, modellkilder/native innlemmet i samme commit. `Tools/mesadiner/acceptance.json` binder eksakt executable og alle fem innholdscontainere til kilden. Råbevis: `~/scratch-bm/Signal47-mesa-diner`.

- 320 faktiske kildekontroller av FBX/GLB, UV-er, mål, materialer, masker og pakket Blender.
- 47 kontroller av lagrede native mål/pivoter, materialbindinger og alle tre LOD-er. Maskene er lest tilbake i eget editorprogram. Editorbygg og pakket cook bestod.
- 1383 selvtester og ni målrettede pakkede scenarioer bestod. Normal gange gjennom døren begge veier, stopp mot bladet, gulv og faktiske dør-/disk-/kaffetreff. Dag, skumring, natt og nærfasade fra spillerhøyde, interiør, eksisterende matkjøp/bruk med ti disk-/verdenslastinger, samt to gamle kameramotiv.
- Ti sluttbilder er faktisk åpnet og vurdert. Fasadeplassering, neonlys uten hvite nærflekker, sammenhengende materialer, mat ved faktisk disktopp og inngang er kontrollert. Ingen fullverdens- eller ny PSO-matrise kjøres i dette passet, etter Toms begrensning av testomfanget.

R1-utside ble videreført mot konseptet. D1-testen fant en 14cm gulvlist gjennom inngangen og en levetidsfeil ved timerens selvavslutning. D2 deler listen rundt By±0.65m og gjør kontrollen trygg med uendret gulvkrav. Lyspunkter 3.9cm fra signflaten ble flyttet til omtrent55cm for å fjerne hvite flekker over neon. Mislykkede råforsøk er beholdt.

Referansebildet har fortsatt rikere overflateslitasje og små naturdetaljer. Menneskelig input, lytting, fullnatt og helruteytelse er ikke prøvd her. Takeaway-testens raske lastinger viste utviklervarsel om always-resident raytracing-geometri83.107MiB over20prosent av400MiB-budsjettet. Ingen minnekrasj oppstod; ingen ytelsesgaranti gis på grunnlag av dette.

## Levering

Kilder, separat Blender-arbeidsscene med kamera/lys, originalen, eksportene, nødvendige originale hjelpekilder og ti spillbilder er samlet i `~/Desktop/Signal47 Mesa Diner 24okt`. `SPILLBILDER.html` viser fasade og interiør. Blender-presentasjonslys/-bakke er kun for kildevisning; spillbildene er native Unreal.

PR152 er flettet i61598105 og testet kandidatD2 installert på standardstien. 40 payloadfiler er hashidentiske, 48 Saved-filer er byteidentiske. Verifisert backup: `/home/tom/scratch-bm/Signal47-mesa-diner/installed-backup-1792864850565907985`. Ingen normal spillerøkt er startet. Fem eide Content-lenker er fjernet med mål bevart, og begge OpenOrder-filer er gjenopprettet.
