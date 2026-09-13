# SplatQuality24 — kvalitet før større miljøer

Vurdert 13. september 2026 etter brukerens tilbakemelding på Hybrid23. To avgrensede kildeanalyser ble utført med GPT-5.6-Luna; hovedagenten kontrollerte kildekoden, sammenholdt resultatene og gjorde vurderingen. Ingen ny spillfunksjon eller grafikkmotor er innført.

## Beslutning

**PlayCanvas-arbeidsflyten er nyttig for SIGNAL / 47. Gabor beholdes som forskningsreferanse.** Neste splat-prøve bør starte med en godt rekonstruert, rettighetsavklart skann og sammenligne visningen før og etter Unity-import. Hybrid23 er bevis på en teknisk integrasjonsvei, men gir ikke grunnlag for å avvise fotorealistiske splats.

## Hvorfor vår prøve ble grov

[Hybrid23-konverteringen](../Hybrid23/conversion.json) fordelte 150 000 tilfeldig samplede Gaussians over 172,83 m² av en generert modell. Hver fikk én farge fra modellens tekstur, samme tangentradius på omtrent 2,38 cm og fast opasitet. Ingen optimalisering mot flere kamerabilder ble kjørt. Dermed er prøven en grov ny representasjon av eksisterende geometri, uten rekonstruerte detaljer eller informasjon om usette områder. Den genererte modellens antatte bakside og manglende interiør er egne kildebegrensninger.

Den teksturerte modellen var skarpere enn akkurat denne konverteringen. Det resultatet består, men skal ikke generaliseres til trente splats mot vanlig 3D. Et fint konseptbilde gir heller ingen garanti for et fint navigerbart miljø; selvstendig genererte kameravinkler kan være geometrisk inkonsistente.

## Hva vi kan overføre

| Del | Konkret nytte og grense |
| --- | --- |
| God kildeskann | PlayCanvas-demoen starter med en detaljert skann av et ekte interiør. Det er et annet utgangspunkt enn vår genererte modell. [Artikkel](https://blog.playcanvas.com/turning-a-gaussian-splat-into-a-videogame/) |
| Kollisjonsgeometri | SplatTransform kan lage en separat trekantmodell fra voxeldata. Vi kan prøve denne gjennom Blender/Unity og kontrollere dørbredde, gulv og vegger med spillerens faktiske kollisjon. Dette erstatter ikke presise treffbokser for gåter. [Dokumentasjon](https://developer.playcanvas.com/user-manual/splat-transform/collision/) |
| Lysmatching | Artikkelens lysstyrkerutenett tilpasser vanlige objekter til den skannede belysningen. Det er en nyttig idé for rekvisitter, men gir ikke i seg selv ny belysning av splatten. Vår proxybaserte lampetest er en annen mekanisme. [Artikkel](https://blog.playcanvas.com/turning-a-gaussian-splat-into-a-videogame/) |
| Rensing og inspeksjon | Filstatistikk og filtrering hjelper med ødelagte verdier og løsrevne partikler. De kan ikke gjenopprette manglende motivdetaljer. [SplatTransform](https://github.com/playcanvas/splat-transform) |
| Detaljnivå og lasting | Streamed SOG deler store scener i romlige biter og detaljnivåer. Unity-støtte for denne konkrete strømmeformen må verifiseres separat; vanlig SOG-import beviser den ikke. Lavere detaljnivå reparerer ikke dårlig kildemateriale. [Dokumentasjon](https://developer.playcanvas.com/user-manual/splat-transform/streamed-sog/) |

SplatTransform er MIT-lisensiert. Det gjelder verktøykoden, ikke alle skannene i galleriet. For eksempel er Schindelars [Sanatorium Inside – Part01](https://superspl.at/scene/8429e5e2) oppført med **CC BY-NC 4.0** og er derfor ikke klarert til vårt kommersielle spill. Ingen slik skann er lastet ned eller lagt i repoet.

## Gabor: interessant, men en annen produksjonsvei

[Forskningsartikkelen](https://arxiv.org/html/2504.11003v1) viser bedre fine stoffmønstre ved å la hver primitiv bære fargevariasjon. Forsøkene bruker små objekter fotografert fra mange vinkler. Rapportert rendering er 65–95 FPS mot 130–176 for 2DGS på RTX 3090; dette er ikke Unity-ytelse eller et løfte for vår Intel-maskin.

[Modellkoden](https://github.com/haato-w/3d-gabor-splatting/blob/main/scene/gabor_model.py) lagrer ekstra frekvenser, faser og vekter. En vanlig PLY-importør gjengir ikke disse effektene automatisk. [Rasterizeren](https://github.com/haato-w/diff-gabor-rasterization/blob/main/setup.py) bygger CUDA-kode; ingen ferdig URP-integrasjon ble funnet i de undersøkte repoene. [Hovedlisensen](https://raw.githubusercontent.com/haato-w/3d-gabor-splatting/main/LICENSE.md) og [rasterizerlisensen](https://raw.githubusercontent.com/haato-w/diff-gabor-rasterization/main/LICENSE.md) begrenser bruk til forskning/evaluering uten særskilt kommersiell tillatelse. Konvertering av resultatet til mesh er ikke i seg selv en klarering av slik bruk. Ingen installasjon eller trening er utført.

## Neste representative prøve

Anbefalt, ennå uutført:

1. Velg én eksisterende trent skann med rettigheter som dekker spillbruk, helst en steinvegg eller ruin som passer STATION 01. Behold originalen og inspiser nærbilde, skråvinkel og silhuett i kildens støttede visning.
2. Importer en avgrenset del i den isolerte Unity-prøven. Bevar høyeste kildekvalitet først og sammenlign tilsvarende kameravinkler. Da kan vi skille kildefeil fra import-/visningsfeil før komprimering.
3. Bygg en sammenhengende kort rute med vanlig gulv, dør, lampe og fotografi. Kontroller overgangene, lampens bevegelse, fotografiets separate kamera og bildehastighet over hele ruten. Bruk minne-/tidsgrensene fra de tidligere prøvene.
4. Velg splats bare der de gir tydelig bedre miljødetalj innenfor maskinens rammer. Behold lesbar dokumentasjon, kabelgåter og bevegelige objekter som vanlig Unity-geometri. Vurder detaljnivå/streaming når scenens størrelse krever det.

## Faktisk kontroll denne gangen

SplatTransform **3.4.2 / 0cb47cd** leste Hybrid23-filen med `--stats json null`, begrenset til 1 GiB og 30 sekunder. Avslutningskode 0; 150 000 Gaussians, null ekstra SH-bånd og ingen NaN/Inf i rapporterte kolonner. [Original statistikkutskrift](Evidence/splat-transform-stats.log) er bevart; inputhash er kontrollert mot Hybrid23. Dette er en numerisk inspeksjon, ikke en ny visuell test eller ytelsesmåling. Ingen ny Unity-kjøring ble utført, og de tidligere åpne grafikk-/inputtestene er fortsatt åpne.
