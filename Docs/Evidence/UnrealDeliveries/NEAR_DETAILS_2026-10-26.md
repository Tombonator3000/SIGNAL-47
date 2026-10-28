> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# SARO, detaljer tett på spilleren

Tom valgte punkt 2 i den visuelle prioriteringen etter den leverte Pure Sky-daghimmelen. Arbeidet gjelder synlige svakheter i armaturer, innfesting, optikk og lysplassering langs SARO-inngangen og fasadene. De nylig modellerte dør-, rampe- og kontordelene videreføres.

## Utført modell- og kildearbeid

Tre originale Blender-modeller brukes som tolv fysiske armaturer: sju veggarmaturer, én baldakinarmatur og fire små skiltlys. Støpte metallhus, avrundede kanter, linsepakninger, prismatiske diffusorer, skruer og monteringsplater erstatter de grove formene. Skiltet ved parkeringen får to skjermede lyskastere ved sokkelen; fløyskiltet får to egne veggmonterte lamper.

| Modell | LOD0 | LOD1 | LOD2 |
| --- | ---: | ---: | ---: |
| WallPack | 4428 | 1808 | 480 |
| Canopy | 2820 | 1292 | 508 |
| SignWash | 3356 | 1628 | 692 |

[Blender-kilden](../../../unreal/ArtSource/ExteriorFixtures/SARO_ExteriorFixtures.blend), [gjenskapbar generator](../../../unreal/ArtSource/ExteriorFixtures/build_fixtures.py), FBX-eksporter, originale PBR-kart og målkontrakt ligger samlet i [ArtSource/ExteriorFixtures](../../../unreal/ArtSource/ExteriorFixtures/README.md). Begge Blender-forhåndsvisningene er faktisk vurdert. Linserammeskruenes plassering ble rettet før import. Frisk gjenåpning kontrollerte 63 kildeegenskaper. Dette er modellkontroll, ikke bilder fra spillet.

[Importskriptet](../../../unreal/Signal47/Tools/exteriorfixtures/import_assets.py) bruker prosjektets tidligere målte FBX-/materialløype. Native import av tre modeller med tre LOD-er og fem materialer består. Bare tre eksisterende Web-mesher får CPU-data beholdt, uten å endre opprinnelig geometri eller materialer. Runtime lager separate kopier og fjerner nøyaktig de sju gamle lampehusenes 84 trekanter; resten av geometri, kollisjon, UV-er, normaler og materialer beholdes.

## Montering og lys

- Veggarmaturene møter fasaden, inkludert tidligere svevende fløylamper og parkeringslampen som lå inne i veggen. Fløyens østre lampe er flyttet klar av skiltplaten. Laboratorielampen er senket til monteringshøyde 2,58 m for å gi 27 mm klaring under takutstikket.
- Baldakinarmaturen ligger mot den eksisterende undersiden. Den gamle ekstra baldakinlyskilden pensjoneres, slik at armaturen har én kilde.
- Hver armatur har både fysisk glass og et ekte native Unreal-lys. De erstattede runde glødespritene skjules, mens eksisterende strømdata beholdes.
- Glassgløden følger den samme strømtilstanden som lyset. De nye parkerings- og monumentlysene følger nå SAROs faktiske strømbrudd og gradvise sodium-oppvarming. Parkeringslyset er lokalt redusert fra 700 til 350 lumen etter at emitteren flyttes ut av veggen. Etter faktisk nattbilde reduseres de to nye monumentlysene fra 65 til 25 lumen hver, for å bevare skiltets flate og tekst. Fløyskiltets to lys beholder 1 200 lumen; det kontrollerte nattbildet har lesbar tekst.
- Manglende eller avvikende modeller beholder den tidligere løsningen. `-S47ExteriorFixturesOff` velger det gamle modelloppsettet for sammenligning.

Dag-/nattehimmel og alle seks skyassets er uendret gjennom bygg og import. Det gjøres ingen global endring av eksponering, Lumen eller grafikkprofil.

## Kontroll og levering

Bevisrot: `/home/tom/scratch-bm/Signal47-neardetail-20261026`. Baseline er den faktisk installerte Pure Sky R1, binær `548d4633…`, med 786 frosne filer og to nære sammenligningsbilder. Native import ligger i `import-r1/native.json`.

Første editorbygg fant en C++-feil i boksinitialisering, som er rettet. Første pakking ble stoppet kontrollert før aksept for å senke laboratorielampen under taket. R2 pakket uten den nye assetmappen og viste korrekt fallback, men ble avvist. `DefaultGame.ini` tar nå med `/Game/Signal47/Art/ExteriorFixtures` eksplisitt ved cook. R3s faktiske IoStore-arkiv inneholder alle tre modellene; runtime bekrefter sju erstatninger og nøyaktig 84 gamle trekanter fjernet.

R3s inngang i dagslys og fløyskilt om natten ble faktisk åpnet og godkjent. Nattbildet av monumentet viste for sterk lokal belysning og utløste den ene intensitetsendringen ovenfor. R4 er deretter bygget og pakket uten ny import, med binær `26673e1b033ca1defa3cc2a2b7a4736028fe20af3893616535fffad3729e2819` og 847 frosne kilde-/assetfiler.

Fem sluttbilder fra R4 er faktisk åpnet og godkjent: inngangen i dagslys, parkering/skilt om natten, samt strøm på, av og tilbake i én prosess med spillets faktiske `Cut`/`Restore`. Både diffusorglød og native belysning følger strømmen. Bildene viser at de lave skiltlysene erstatter de gamle stolpene, at skiltteksten har bedre kontrast og at vegg-/baldakinarmaturene sitter riktig. Det uendrede fløyskiltet ble vurdert i R3. `runtime-r4/visual-acceptance.json` og `independent-review.json` binder godkjenningen til bilder, binær og frossen kilde.

Brukeren spilte mellom R3 og sluttkontrollen. En fersk Saved-baseline etter denne økten har 67 noder, inkludert 48 regulære filer. Disse er uendret gjennom alle tre R4-kjøringene og installasjonen; den eldre baseline er bevart som historikk.

Samme R4 er installert i `/home/tom/Games/Signal47/Linux`. `installation-r4.json` er INSTALLED_VERIFIED med 68 identiske payloadnoder. Faktisk standardstarter med isolert profil består elleve kontroller; det installerte inngangsbildet er også åpnet og vurdert. Ingen personlig lagring er brukt som testprofil eller overskrevet.

Canonical prune er anvendt etter kontrollert plan og avsluttet oppstartsprøve. Fire gamle Pure Sky-/R5-payloadsett, inkludert den verifiserte Pure Sky-rollbacken brukt under installasjonen, er pensjonert: 28 genererte filer, 15 397 517 826 logiske bytes. Dette er ikke en påstand om fysisk diskgevinst. Gjeldende R4 og de to eldre R3/R2 beholdes; aktiv SC01-R4 er særskilt beskyttet. Kilder, Blender-filer, assets, Saved, logger og bilder er bevart. `delivery-r4.json` består alle ti sluttkontroller og bekrefter identisk installasjon og uendrede lagringer også etter oppryddingen. Lokal levering, ingen ny PR, merge, push eller ekstern utgivelse.

Kontrollen gjelder nærbilder av det endrede innholdet og den faktiske strømkoblingen, ikke hele spillmatrisen eller en FPS-måling. Low og menneskelig fullnatt er ikke prøvd på nytt. En oppstartsadvarsel om VSM Non-Nanite-kø finnes også i den tidligere godkjente referansen. Det er ikke registrert fixturefallback eller ny RT-budsjettadvarsel i R4.

Prosjektbiblioteket ble søkt først. Den pinnefestede [scenario-blender-expert-referansen](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/dcc/blender/scenario-blender-expert/SKILL.md) ble brukt som lesende arbeidsflytreferanse. Den beskriver Blender 5.2.1, men prosjektets egne tidligere prøvde skript og faktisk lokal import er kompatibilitetsgrunnlaget; ingen eksterne verktøyskript er tatt inn. Bibliotekets Blender/Unreal-ressurs fra @rewind02 er merket UNVERIFIED og brukes ikke som produksjonsbevis.
