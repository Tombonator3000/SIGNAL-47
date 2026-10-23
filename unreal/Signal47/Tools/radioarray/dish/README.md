# SARO arraymodell

Original Blender-geometri laget fra Toms to VLA-referansebilder. Ingen bildepiksler, tredjepartsmesher eller logoer er inkludert. Formen har parabolpaneler, åpent radialt fagverk med dybde, ring- og diagonalavstivere, fire feedarmer, mottaker, elevasjonslager, åpen yoke, azimutlager, serviceplattform, rekkverk, skap, kabelrør, stige og fire betongfundamenter.

`SARO_Array.blend` er redigerbar kilde med alle LOD-nivåer. `build_dish.py` gjenskaper kilden, eksportene og fire CPU-renderinger med Blender 5.2.2. Kjør fra hvilken mappe som helst med `blender -b --threads 6 --python /full/sti/build_dish.py`. Legg til `-- --skip-render` for å regenerere modellene uten nye bilder.

## Importkontrakt

- Enhet meter, Blender Z opp, X som elevasjonsakse. Azimut roterer om Z.
- FBX har `axis_forward=-Y`, `axis_up=Z`, enhetsskalering og ingen bakt romtransform. Unreal-importøren håndterer centimeter og `force_front_x_axis`.
- Hver eksport har én mesh ved lokal origin `(0,0,0)` og skala `(1,1,1)`.
- Sett Base ved verdensbunn, Azimuth ved z 7,8 m og Elevation ved z 12,4 m. Azimuth og Elevation er uavhengige animasjonsdeler. Elevation har skålåpning mot +Z ved zenit.
- Elevation har radius 9 m, parabolvertex z 1,1 m og fokallengde 7,2 m. Feed er z 8,3 m og varsellysets topp z 9,2 m relativt elevasjonspivoten.
- Samme pivoter og skala for LOD1 og LOD2. Ikke automatisk sentrer objektene under import.
- `materials.json` oppgir lineær PBR-grunnfarge, metall, ruhet, emissjon og alfa for de åtte materialplassene. Ingen teksturer kreves. FBX beholder plassene; Unreal-materialene bygges fra sidecaren.
- GLB per del er reserveeksport. `SARO_Array_Assembly.glb` inneholder de tre LOD0-delene i korrekt montert stilling.

`verification.json` inneholder trekanttall, lokale bounds, pivoter og asymmetriske orienteringspunkter. `verify_exports.py` importerer alle ni FBX-er tilbake i Blender og kontrollerer antall, mål, skala, pivoter og materialplasser. `export_roundtrip.json` er den faktiske rapporten.

Nærmodell: 26 210 trekanter. LOD1: 18 250. LOD2: 6 510. Sentrumspanelene er ekte trekanter uten sammenfallende quad-hjørner; UE-import skal bevare trekanttallene. Bare de materialplassene hver del bruker gir geometri; LOD0 har til sammen 11 brukte materialseksjoner. Blender-renderinger er kildeinspeksjon og er ikke runtime- eller ytelsesbevis.
