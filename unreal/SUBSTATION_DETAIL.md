# Caliche: lampehus og kontakt med terrenget

## Endringen, 28. oktober 2026

Den eksisterende trafostasjonen beholder form, plassering, gjerde, skilt og funksjoner. Fem originale Blender-deler tilfører åpen lampehette/bezel, kabelinnføring og sidekoblingsboks, små fundamentfester, steinklynger og en uregelmessig grusskråning. Sistnevnte følger målte trekanter fra eksisterende terreng i stedet for å la grusbedets høye, rette kant stå eksponert.

Statuslampens emitter lå inne i glasset og hadde ingen skygger. Den er flyttet 4,9 cm ut fra meshens lokale origo, 1 cm foran glassflaten, og bruker et bredt spotlys med skygger. Det eksisterende 95-lumen-grunnlaget, svake/sterke tilstander, servicebryter og T02-forløp styrer fortsatt samme lampe. Ingen ny elektrisk spillmekanikk eller lydsløyfe er innført.

Tre lokale floraeksklusjoner holder grusbed, kontrollplint og gangrampe fri for plantene som tidligere vokste gjennom dem. Øvrig terrenggeometri, natur, himmel og belysningsoppsett er bevart. Port, journal, bryter og T02-fotoreferanser holdes åpne.

## Kilde og native integrasjon

- Redigerbar kilde: `Signal47/Tools/substationdetail/Substation_Detail.blend`, med pakkede egne PBR-bilder, GLB og FBX.
- Fem meshes: totalt 6 406 / 3 344 / 1 754 trekanter i tre håndlagde LOD.
- Native assets: `/Game/SubstationDetail`. Den målte Substation-aksekonvensjonen gjenbrukes. Ingen eksisterende native assets overskrives.
- Runtime validerer alle fem meshes, materialer, bounds og LOD før installasjon. Bare grusskråningen har QueryOnly/Visibility; den blokkerer ikke brukestrålen. Tre faktiske native gulvtraces må stemme med de 212 eksporterte topptrianglene før de brukes som ganghøyde.
- Dekor er uten kollisjon. Ingen navigasjonsendring. `S47SubstationDetailOff` kan slå av detaljsettet; `S47PropsOff` gir også opprinnelig fallback.

Originale former og materialer fra prosjektet. Ingen nye kjøpte eller lisensierte assets. Prosjektbibliotekets Blender/Unreal-referanse var UNVERIFIED og ble bare brukt metodisk. [blender-skills](https://github.com/arjun988/blender-skills) ved `8f778d2405a214b508d4c7d80742be8e43acdd52` ga prinsipper for synlige lampehus, plassering og eksport; ingen installasjon eller kopiert kode.

## Verifikasjon og leveringsstatus

Blender-kildekontroll, fem native importer og editor/Linux-pakke består. Første kildekontroll fullførte geometrien, men rapportskriving feilet fordi en UV-variabel overskrev bounds; rettet i kontrollskriptet, deretter PASS uten geometriendring. Dagsoversikten er faktisk sammenlignet med samme kamera/tid i installert baseline. Skulderens overgang og ryddet grusplass er synlig. Nærbilder ved svak og sterk nattlampe er vurdert. T01-kontrollen består journal, service, faktisk lampe, sammenligning, rampe/gjerde, faktiske brukestråler og normal diskreload.

T02 består 65 API-kontroller, 15 faktiske diskrestores og 38,90 meter gangrute. Hele den kanoniske 110-matrisen består etter én kjøring på 53 minutter og 54 sekunder. Alle 110 logger og PNG-er er komplette, uten motorfeil; native detaljsettet er bekreftet i hver kjøring. [PR184](https://github.com/Tombonator3000/SIGNAL-47/pull/184) går mot main. Samme testede pakke installeres etter fletting med backup og Saved-unntak. Avgrensning: automatiserte Vulkan-kjøringer og vurderte bilder, ikke menneskelig gjennomspilling, lytting eller helspill-FPS.

Bevis og uendrede råforsøk: `/home/tom/scratch-bm/Signal47-substation-environment-20261028`. Claudes TB2-kilder er nå flettet via PR179, men har fortsatt ikke native integrasjon i denne pakken. De er ikke endret her. Fotolab R1, motel R4, SC01, MetaHuman og personlige lagringer bevares.
