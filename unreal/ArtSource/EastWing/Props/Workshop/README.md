# EASTWING-01 verkstedrekvisitter

`build_workshop.py` genererer 17 gjenbrukbare modeller i lokalt meterrom, Blender Z opp og front -Y. Root kjører skriptet sekvensielt etter `require-idle PASS`, via felles produksjonslås. Skriptet endrer ingen bygg- eller runtime-filer.

Dekning: skuffeskap, rød verktøykiste, grønn stålbenk, treetasjes delevogn, FUSES- og RELAYS-kasser, kabeltrommel, låst kjemikalieskap, dekkstabel, brannslukker, pegboard, veggverktøy, rulleportsegment, slukrist, lysrørarmatur, eksisterende fylt hylle og ny tom galvanisert hylle.

## Gjenbruk og opphav

- `SM_EW_FluorescentFixture`: den originale V10_TwinFluorescent-geometriblokken i `Unity/Blender/Source/visual10_lab_details.py` kjøres lesende med samme mål og detaljdeler, med nye materialbindinger og UE-eksport. Originalens globale sceneopprydding/eksport kjøres ikke.
- `SM_EW_ShelvingStocked`: faktisk eksisterende FBX-geometri fra `unreal/Signal47/Tools/archiveprops/exports/SM_ArchiveProps_ShelvingStorage.fbx`, dokumentert som prosjektets eget arbeid i samme mappes README. Originale UV-er/materialer beholdes, pivot flyttes til senter XY/gulv og modellen roteres slik at fronten vender -Y. Bildeteksturer kopieres byteidentisk til `ReusedTextures/` og relinkes under eksport; opprinnelig oppløsning bevares.
- Skap, benk og den tomme hyllen tilpasser konstruksjonsdetaljene fra prosjektets egne `archiveprops/geometry.py`: brettede platekanter, fysiske håndtak, bolter, kryssavstiving og ben. Øvrige modeller er ny original Blender-geometri, uten tredjepartsmesh, innlastede kommersielle teksturer eller Tripo.

Hver modell får `.asset.json`, `.license.json`, `.blend`, FBX LOD0/1/2 og faktisk Blender-forhåndsvisning. Lisens-ID `LicenseRef-ProjectOwned` beskriver opphav, ikke en ny publiseringslisens fra tredjepart. Native UE-import og spillerinteraksjoner eies av Claude og er ikke bevist av Blender-eksport.

## Viktige integrasjonsnotater

- Alle geometrier har senter XY/pivot Z=0. Armaturens underkant er Z=0; opphenget er +0,10m.
- Portsegmentet er 3,5m bredt, 0,25m høyt. Monter/animér segmentene fra bunnsenter, og behold fri bilåpning. Det er en modell av ett segment, ikke en fungerende portkontroller.
- Slukets totale mål er 0,5 x 0,3m, topp +0,0535m. Senk modellen så risten ligger plant med gulvet. Hullene er geometri, ikke gulvmaling.
- Felles utility lager en grov UCX-boks; ved åpne hyller, armaturer, portsegmenter og små interaksjonsdetaljer må integratoren velge passende kollisjon og ikke sperre gjennomganger.
- Rørmaterialets emissive og faktiske lys skal styres av eksisterende strøm-/lyssystem. Ingen permanent selvlysende runtime-tilstand legges inn av modellen.
