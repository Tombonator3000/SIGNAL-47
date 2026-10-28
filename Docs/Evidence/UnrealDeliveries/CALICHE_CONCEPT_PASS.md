> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Caliche, konseptpass 24. oktober 2026

Status: Ferdig og installert i ~/Games/Signal47/Linux. Ni visuelle runder er gjennomført. Den fryste sluttpakken består alle 110 regresjonsscenarioer, elleve lokale runtimeprøver og visuell kontroll av 42 faktiske spillbilder.

Målet er én sammenhengende gatebit ved bensinstasjonen, SAUCER CRASH 1947 og 1420 AM, med redigerbare Blender-kilder og bakken rundt. Radio-/butikkarkene i Concepts/Caliche-2026-10-23 og Caliche-byen er form-, material- og lysreferanser. Kontrollene bruker spillerhøyde 1,62 m i dagslys 12:00, skumring 18:30 og natt 02:15. Render fra Blender og faktiske spillbilder holdes adskilt.

## Visuelle runder

1. Første pakke ble avvist: kantete bakkeoverganger, terrasserte kjeglefjell, for mørk skumring, gammel takplate og lysflekk foran radiovinduet.
2. Andre runde ga mykere kanter, mer innredning og større avstand til fjellene, men beholdt feil materialkoblinger i native FBX.
3. Tredje runde fjernet den gamle uskjermede punktlampen foran studioet og lot skiltlampene følge gatebelysningen. Bakkeavviket ble isolert mot original bakke og kilde-UV-er.
4. Fersk import av bare egne genererte mesh-assetter rettet årsaken: 82 ShoulderBlend-trekanter hadde fått betong, 38 DustFeather-trekanter hadde fått grus. Korrekte materialer brukes nå av faktiske mesh-seksjoner. Bildene viste fortsatt for tomme bygningskanter, jevn radiopuss og kaldt bensinstasjonslys.
5. Tolv ujevne plante-/steinøyer, større pussreparasjoner og dempet finkorn, varmere lysrør, mer brutt fjelloverflate og tydeligere skylag. Pakket vurdering fant fortsatt glatt radiopuss, for sterke grå reparasjonsfelt og for jevnt areal foran pumpene.
6. Mineralstruktur og roligere reparasjoner på radiopussen ble visuelt beholdt. Seks slitasjefelt følger originalbakken, men gjennomsnittlig atlasalpha ganger vertexalpha ga bare rundt 10 prosent synlighet på jord og 4 prosent på betong. Native FBX ble kontrollert mot alle 58 704 Surface-hjørner: materialer, høyder, UV og alpha samsvarer med kilden.
7. Tydeligere avbrutte hjulspor og stoppmerker med uendret geometri/kanter. Den gamle horisontens svarte, innbakte nattfjell skjules lokalt når den nye fjellmodellen faktisk er synlig; opprinnelig synlighet gjenopprettes utenfor området eller ved fallback. Pakket kontroll fjernet den svarte mesablokken og viste matte, brukte betongfelt. Jordsporene ble avvist fordi de hadde raske sidebølger og lignet renner.
8. Roligere og tydelig avbrutte mørke jordspor; alpha, støvmarger, betong og samtlige meshdata bevares. Fokusbildene i dagslys/skumring og 36 gatebilder på begge tidslinjene ble visuelt beholdt. Innvendige sluttbilder viste derimot mørkt studiobakrom og blanke lobbyoppslag.
9. Metrisk veggtre, originale trykte lobbyoppslag og et lite loggark. Fysisk opal taklampe bakover i studioet og direkte lys under eksisterende bordlampe, med samme eksponering og strømstyring. Tolv faktiske bilder ved dag, skumring og natt ble visuelt beholdt av hovedagent og uavhengig vurderer. Studioets utstyr, stol, plater og treflater er lesbare; vinduet er klart, og fasadelyset følger døgnet. 51/51 kildekontroller, 249 native kontroller og fire portable Blender-kilder består. Den komplette sluttkontrollen på den samme fryste pakken består.

Kanon, klokkegrenser, fotoankre og interaksjoner beholdes. Den tidligere antenneleveransen med 27 replikerte antenner inngår i kandidaten.

## Sluttkontroll og levering

- 36 gatebilder: seks spillerhøye kamerastillinger, begge tidslinjer og tre klokkeslett. Seks egne bilder viser lobby og studio ved de samme lystilstandene. Alle 42 bilder er visuelt gjennomgått og bundet til sluttpakkens faktiske filhasher.
- 1305 selvtestsjekker og elleve lokale runtimeprøver består, inkludert 27 antenner, dør, 6,1 m gange inn i lobbyen, buzzer, radio og fotokamera. Tre horisontprøver bekrefter ny visning og de to reservevisningene. A02-piloten består; de fire faktiske fotografi-/sammenlignings-/notatbokbildene er også gjennomgått.
- Hele eksisterende `Scripts/screenshots.sh`: 110 av 110 scenarioer består. Åtte sluttfaser bruker samme uendrede spillpakke. 249 native kontroller og 239 native filhasher består. Fire originale Blender-filer er åpnet med faktiske relative/pakkede bildereferanser uten å endre kildefilene.
- Installert payload: 40 filer hashkontrollert. Alle 51 eksisterende Saved-filer er uendret. Sikkerhetskopi: `/home/tom/scratch-bm/Signal47-caliche-concept/installed-backup-1792817380617549384`.

Sluttpakkens SHA-256: `6103dd97769d675de05c77bf112cf568e1fbb9536335a458c71b83e6986113af`. Native kildecommit: `f63aba3a715529e800ae3a31b8ce9e90f593441c`. Kompilert C++/config-stempel: `266f6fc2`. Dette er lokal fletting og installasjon; ingen ny ekstern spillutgave er publisert.

Skrivebordsleveranse: `~/Desktop/Signal47 Caliche konseptpass`. Lenkene under gjelder rapportkopien i denne mappen. Åpne [SPILLBILDER.html](../../../unreal/SPILLBILDER.html) for de 42 faktiske bildene med valg av kamera, klokkeslett og tidslinje. `delivery-manifest.json` inneholder filhasher, og `evidence/` inneholder kontrollrapporter og testdrivere.

| Redigerbar modell | Original Blender-kilde i leveransen |
|---|---|
|1420 AM, lobby og analogstudio|[1420_AM_station.blend](../../../unreal/source/Tools/radioarray/station/1420_AM_station.blend)|
|Suvenirbutikk og utstilling|[Saucer_Crash_1947.blend](../../../unreal/source/Tools/calichestreet/souvenir/Saucer_Crash_1947.blend)|
|Gatebakke, grus, slitasje, stein og planter|[Caliche_Ground.blend](../../../unreal/source/Tools/calichestreet/ground/Caliche_Ground.blend)|
|Fjerne geologiske mesaformer|[Caliche_Mesas.blend](../../../unreal/source/Tools/calichestreet/backdrop/Caliche_Mesas.blend)|

## Prioritet for videre Blender-modeller

| Prioritet | Bygning | Interiør og deler | Begrunnelse |
|---|---|---|---|
|0|Gatebiten med bensinstasjon, suvenirbutikk og 1420 AM|Pumper, kiosk, analogstudio, butikkutstilling, skilt, innganger og kontakt med bakken|Fastsetter material-, detalj- og lysnivået før flere bygg lages|
|1|Mesa Diner|Disk, kjøkkenåpning, båser, servering, telefon og toalettgang|Første møte med byen og mange nære spillerhandlinger|
|2|Sheriffkontoret|Resepsjon, skrivebord, arkiv, veggkart og bakrom|Historie, samtaler og dokumenter trenger troverdige nærflater|
|3|Motell og resepsjon|Rom 6, bad, gang, skranke og TV-hjørne|Spilleren oppholder seg tett på inventaret over tid|
|4|Vaskeri og fôrbutikk|Vaskemaskiner, kurver, arbeidsbenk, sekker og varelager|Gir hovedgaten variasjon og hverdagsliv med gjenbrukbare moduler|
|5|Plaza-kinoen|Fasade, billettluke, plakatkasser og foajé|Tydelig landemerke; full sal først hvis spillerreisen trenger den|
|6|Kirken|Inngang, benker, alter og lite sakristi|Avslutter bysilhuetten; interiør prioriteres etter faktisk bruk|

Ørkenen trenger gjennomgående mineraljord, grus og brutt asfalt med myke overganger, variasjon i stein og tørre planter, erosjon/avrenning og fjell med tydelig avstand og geologi. Neste områder bør ta utgangspunkt i den godkjente gatebiten og måles fra spillerhøyde i de samme tre lystilstandene.

## Bevis og begrensninger

Arbeidsrot: ~/scratch-bm/Signal47-caliche-concept. Avviste kandidater og bilder er bevart separat. Automatiske kamerastillinger og API-scenarioer er ikke menneskelig gjennomspilling eller lydlytting. Bildelikhet vurderes visuelt; en vellykket import eller testkjøring alene betyr ikke at konseptmålet er nådd.


## Redigerbare kilder

Reviderte originaler ligger under `Signal47/Tools/`: radio i `radioarray/station/1420_AM_station.blend`, butikk i `calichestreet/souvenir/Saucer_Crash_1947.blend`, bakke i `calichestreet/ground/Caliche_Ground.blend` og bakgrunnsfjell i `calichestreet/backdrop/Caliche_Mesas.blend`. De fire originalene er åpnet med Blender og faktiske bildereferanser kontrollert. Eksporter, teksturer, generatorer, opprinnelsesdata og kontrollrapporter følger kildene.

Bensinstasjonens eksisterende [Blender-original](/home/tom/Assets/signal47-free/own/gas_station/s47_gas_station.blend) er beholdt. Antenneleveransen med separate bevegelige deler og LOD-er ligger fortsatt i [SARO_Array.blend](</home/tom/Desktop/Signal47 radiostasjon og array/Antenne/SARO_Array.blend>). Den nye gateleveransen inneholder de fire reviderte kildesettene.

Radioens offentlige lobby er spillbar; selve kringkastingsrommet er fortsatt avskilt. Modell og innvendige kamerabilder er levert, uten endring av dørlås, historie eller spillerens adgang. Byen utenfor den avtalte gatebiten beholder eldre detaljnivå.
