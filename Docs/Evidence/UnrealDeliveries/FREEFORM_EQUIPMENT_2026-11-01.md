# Fysiske arbeidsstasjoner, Tripo-pumpe og Jeep-frontrute

**Status:** Gjeldende. **Gjelder:** Lokalt installert Unreal/Linux-runtime `b98c6fe1`, samlet PR311 og PR312. **Sist kontrollert:** 1. november 2026.

Installert kl. 16:35 CET med bevart personlig Saved. Dette er en lokal oppdatering, ingen offentlig spillutgave. [Sammendrag](FreeformEquipment-2026-11-01/summary.json) og [originalt Freeform-resultat](FreeformEquipment-2026-11-01/freeform-result.json).

## Hva som er levert

- Fire faktiske CRT-skjermer betjener status, mottaker, tracking og OPS. Nærkamera og peker bruker eksisterende skjermglass; ESC går tilbake til rommet. Tiden, antennene og hendelsesregissøren fortsetter ved utstyret. Pause og vanlige dokumentmenyer holder tiden.
- Spolebåndutstyret ved telefonen har fysiske ORIGINAL/CONTROL, PLAY/STOP, REWIND, MARK IN/OUT og RETAIN-knapper. Sammenligning krever faktisk avspilling i nærheten. Søk i opptaket gir ikke hørt dekning. Papiroriginaler går fortsatt via skriver og fysisk arkiv.
- Dagskort varierer faktiske frekvens-, gain-, båndbredde-, retnings-, elevasjons- og tidsinnstillinger innen fire støttede kilder. Tre oppgaver per døgn, stabile gjennom lagring; gamle kort beholdes. Pulsaropptaket beholder 18 sekunder. Den eksisterende kontekststyrte regissøren med 16 hendelsesfamilier er beholdt, og faktiske feeder-/antennefeil gir vedlikeholdsoppgaver.
- REGULAR-pumpen leverer opptil én tankenhet per fylling, i kvart-enheter for $1. Parkert bil, avstand og frie hender kontrolleres. Bare fullførte måleintervaller betales; pause/lasting bevarer tank, kvittering og uferdig intervall. Diesel er inkompatibel.
- Toms GLB fra skrivebordet er bevart og klargjort privat i Blender, med original PBR og tre målte LOD-er: 31 967 / 17 581 / 7 991 trekanter fra 1 928 097. En privat stasjonskopi fjerner bare den gamle REGULAR-geometrien; samme bounds og 70 materialidentiteter beholdes. Øy, diesel, canopy og butikk beholder ankre. Ny pumpemodell brukes i Freeform.
- Jeepens deformerte frontrute-/speilgruppe er erstattet lokalt i en separat Blender-derivat med rett ramme, plan rute og plane speilflater. Dashboard, instrumenter, sete, hjul og kjøring er bevart. Alle sju native deler og deres faktiske LOD-er er importkontrollert. Speilene bruker miljørefleksjon, ikke et bakoverkamera. Original GLB/Blender-kilder og forrige derivat er bevart privat.
- Den parallelle tittelendringen i PR312 inngår i samme installasjon. Tittelen mister 47 med synkronisert lyd etter faktisk fullført Story; ingen personlig profil er forhåndslåst opp av prøvene.

## Kontrollgrunnlag

| Prøve | Resultat og grense |
| --- | --- |
| Editorbygg og målrettet spillprøve | PASS, 1939 selvtester, 77 kontroller og 2 disk-/Flow-gjenopprettinger før samordning med tittelkilden. |
| Samlet Linux-pakke | PASS, 1957 selvtester, 77 Freeform-kontroller / 2 restores. Faktiske CRT-vinduer, native peker-/Walker-handlinger på pumpe og båndknapper, originalopptak/kontroll/merking, tank/betaling og sju monterte Jeep-deler inngår. |
| Pakket tittel | PASS, 11 kontrollpåstander og egen completion-linje. Synlig High/Vulkan-vindu, erklært sluttfixture og syntetisk Slate-aktivering; bilder og lydfil oppbevares av titteloppgaven. Telleparseren er rettet til faktiske `PASS:`-linjer; original logg er bevart. |
| Måler etter fylling | En kort tidlig frame viser temporalt etterslep i tallene. Vanlig isolert Continue fra original midtmåling lar resten leveres, og et statisk bilde viser korrekt 4,00 tank, 1,00 levert og $4. Vanlig autosave bekrefter samme tilstand. Ingen tank-/betalingsverdier er fabrikkert. |
| Installert Fortsett | PASS, kort Vulkan-prøve fra isolert kopi av personlig lagring. Alle 149 Saved-elementer er uendret. |

Kamera-, spiller- og bilplasseringer er erklærte fixtures. Ingen naturlig hel kjøretur, flerdøgns-/skrekkbalanse, menneskelig lytting eller full regresjon hevdes. Den eldre SARO-renderstansen er fortsatt åpen. Nytt nabobygginteriør og separat hjulrigg gjenstår. Ingen VotV-kode eller assets er kopiert.

## Bilder

[OPS på faktisk CRT](FreeformEquipment-2026-11-01/operations.png), [mottaker](FreeformEquipment-2026-11-01/receiver.png), [fysisk lydutstyr](FreeformEquipment-2026-11-01/audio.png).

[Pumpe før](FreeformEquipment-2026-11-01/pump-before.png) og [ny pumpe etter naturlig fullføring av gjenopptatt fylling](FreeformEquipment-2026-11-01/pump-after.png).

[Jeep før, Toms originalbilde](FreeformEquipment-2026-11-01/jeep-before.png) og [pakket faktisk førerplass etter](FreeformEquipment-2026-11-01/jeep-after.png). Bildene har forskjellig sted, lys og klokkeslett; de er ikke et identisk kameramålepar.
