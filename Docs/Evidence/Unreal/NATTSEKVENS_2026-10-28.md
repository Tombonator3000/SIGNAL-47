# Ny nattrekkefølge, 28. oktober 2026

**Status: Under arbeid** · **Gjelder:** nattsekvensen på Codex sin arbeidsgren, ikke installert standard. **Sist kontrollert:** 28. oktober 2026.

Codex arbeider på `codex/night-sequence-20261028` fra main. Claudes observasjonsarbeid fra PR206 er flettet inn, med alle Story-kroker beholdt. Ingen installasjon eller offentlig release inngår.

## Spillerreisen

1. Overlevering og mottakerreferanse ved SARO, deretter de fire observasjonene fra 19:00 til 22:50 og SC01. Relékoblingen krever SC01, uten krav om T01.
2. Når den ubetjente surveyen er overlatt til seg selv, passerer natten til 00:13. Restsignalet og strømfeilen kommer sammen. K3 forblir åpen, antennemotorene stopper på faktisk posisjon og konsollhandlinger kan ikke omgå feilen. SARO får svak nødlyssetting.
3. Fylkets telefonlinje varsler at Caliche Junction-matingen er ute. Samtalen må avsluttes før serviceoppdraget er kvittert. Telefonen viser en engelsk skriftlig samtale; nytt stemmeopptak er ikke produsert av Codex.
4. Den faktiske overgangen til kjøring på hovedveien løfter klokken til minst 01:35. En spiller som når stasjonen til fots kan også fullføre oppdraget. Klokken går aldri bakover.
5. Fysisk journal, panelkontroll og avlest lampe gir T01-resultatet. Panelet alene gjenoppretter ikke strømmen. Etter faktisk avlesning kommer SARO-mating og K3 tilbake, uten krav om først å returnere med kortet.
6. SC02 starter først etter kvittert oppdrag, T01 og presentert resultat. Klokken løftes til minst 02:00. Den kraftige lysbuen kommer fortsatt bare ved kroppens fremkomst.
7. Etter presentert SC02-flukt løftes klokken til minst 02:15 før SC03s rapportfase. Tilbake ved SARO kan restsignalet undersøkes videre.

## Vedvarende effekt og himmel

Den rolige blåhvite buen og to fastmonterte amberfeillys oppstår med nattens feil og fortsetter også etter T01. De er egne native komponenter, uavhengige av det korte kraftige SC02-utbruddet og servicelampens måleverdi. Feillysene har en rolig firesekunders syklus; calm-flash gir jevnt dempet lys. Effekten følger spillpause og modalpause. Ingen nye effektassets eller tredjepartslisenser er tilført.

SKY01 har første tidsgrense 21:30 og ett lys over SARO-portens nordsektor. Observasjonsarbeidets korte klokkehopp til 22:05 og 22:50 utløser ingen flerlys før midnatt. Senere skytrinn følger kapittelprogresjonen. Faktisk sikt, valgfri observasjon og eksisterende lagringsformat beholdes.

`AS47Story::AdvanceClockTo` er felles monoton koordinator for oppgavehopp. Observing bruker den ved start og arkivering; den reserverer 00:13-hendelsen og bevarer senere lagrede klokkeslett.

## Lagring og kompatibilitet

Nye netter bruker `evening.serviceRouteVersion=2`. Versjon 1 og eldre lagringer beholder sin opprinnelige oppdragsrekkefølge og får ingen ny feil eller observasjonsplan retroaktivt.

`nightService` bevarer den nye prologens faktiske klokke, spillerpose, mottaker, K3, notater, dokumenter og antenneposisjoner under trafoturen og etter retur. Uferdig solve eller printermating gjenopptas ved en stabil handling, henholdsvis låst signal eller ferdig printerark. En avbrutt snapshot før transienten får hendelsen tilbake; normal lagring venter til transienten er ferdig. En lagring under kjøring bruker spillets etablerte trygge avstigningspunkt, med nattens aktuelle klokke og tilstand bevart.

## Kontrollstatus

Under arbeid. Python-pakkens 36 tester, editorbygg R2 og 1 708 native selvtester består. Første nattkontrakt bekrefter observasjonene og strømbruddet, men feiler samlet restore-kontroll. Den undersøkes med feltvis diagnostikk. Første SKY01-start brukte feil bevismål og ble avvist; gyldig mål og nytt bilde gjenstår. Pakket kandidat er foreløpig UNVERIFIED.

Testdriveren bruker ordinære produksjons-API-er, faktiske journal-/panel-/lamperays og ferske disklesere med full Flow-restore. SC01-fakta og ett originalfoto hentes fra en uttrykkelig navngitt tidligere prøve. En separat SC03-grenseprobe bruker tidligere faktisk SC02-flukt med eksplisitt testrevisjon. Det er ingen påstand om ny menneskelig gjennomspilling, fersk SC02-flukt, full SC03-gjennomføring eller målt helspilltid. Nøyaktige resultater og bevis legges til etter kjøring.

Bevismappe: `~/scratch-bm/Signal47-night-20261028`. Generelle Unreal-referanser fra prosjektbiblioteket er lest og merket UNVERIFIED; prosjektets eksisterende historie-, strøm-, lagrings- og effektløype er videreført.
