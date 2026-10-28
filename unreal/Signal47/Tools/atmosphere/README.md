# Avgrenset atmosfæreløype

`prepare_grass.py` kopierer prosjektets eget `M_CN_Foliage` til et separat `M_CN_GrassWind`. Basematerialet, busker/trær og alle eksisterende mesh-filer beholdes. Gressvariantens WPO bruker den eksisterende MPC-klokken og vindstyrken, med faste røtter, høyst 2,2 cm toppforskyvning og uttoning mellom 20 og 70 meter. Retningen er en rolig fast sørvestretning, i tråd med eksisterende vinddriver. Bare `Grass_A/B` i Caliche og motorveiovergangen får varianten. Vanlig runtimeflora bruker fortsatt sin eksisterende vind.

Kjør bare materialskriptet gjennom prosjektets sekvensielle Unreal-kjørevern mot et prosjekt med eksisterende native Caliche-assets. Skriptet erstatter ikke et allerede eksisterende resultat automatisk. Materialet er en original graph-utvidelse av prosjektets egen kilde og bruker eksisterende teksturer.

Diagnostikk i faktisk pakket spill:

- `-S47LegacySmallShadows`: behold også direkte skygger fra de små steinene.
- `-S47LegacyGrassWind`: bruk gamle statiske detaljgressmaterialer.
- `-S47LightPerf -S47AtmosphereLog`: mål eksisterende native frame/game/draw/GPU-tid og les faktisk WindTime/WindAmp fra MPC.
- `-S47SmokeTest -S47AtmosphereReview=/absolutt/utmappe`: to HUD-frie bilder med samme kamera ved t=8 og t=10. Dette er et API-scenario, ikke menneskelig input.
- `-S47FogScattering`: kun eksperimentell svak FSSS ved SARO på High/Ultra i mørke/skumring. Standard er av, og Low holder alltid effekten av. Innstillingene for godkjent himmel, tåketetthet, eksponering og lys endres ikke av sammenligningen.

Se `unreal/ATMOSPHERE_2026-10-27.md` for den faktiske leveringsstatusen. Materialbygg eller en vellykket cook alene er ikke visuell aksept eller ytelsesbevis.
