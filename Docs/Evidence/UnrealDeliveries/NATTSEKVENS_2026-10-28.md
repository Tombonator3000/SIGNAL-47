# Ny nattrekkefølge, 28. oktober 2026

**Status: Gjeldende** · **Gjelder:** kilde `b68d3a08` og målrettet kandidat R2, ikke installert standard. **Sist kontrollert:** 29. oktober 2026 kl. 00:33.

Leveransen går gjennom [PR209](https://github.com/Tombonator3000/SIGNAL-47/pull/209) fra `codex/night-sequence-20261028`. Main med Claudes PR206, PR208 og PR210 samt produksjonsverktøyene fra PR211 er integrert, med alle Story-kroker beholdt. Kilde og pakket kandidat er kontrollert hver for seg. Ingen installasjon eller offentlig release inngår.

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

| Kontroll | Resultat | Nøyaktig grunnlag |
| --- | --- | --- |
| Editor og selvtester | PASS | Editor R11 og pakket R2 på `b68d3a08`. Pakken består 1 709 selvtester. |
| Narrative datakontrakter | PASS | 36 Python-tester etter integrasjon av main/PR210. |
| Integrert nattforløp | PASS | Pakket R2: 74 kontroller, åtte nye disklesere med full Flow-restore. Aggregat, port, naturlig SKY01, strømbrudd, telefon, T01, SC02-port, SC03-grense, Locked og Printed inngår. |
| Naturlig himmelstart | PASS | HI-papiret gir 21:30 uten override. Samme SKY01 og forløpt tid beholdes over 21:40 og 22:05. |
| Grafisk himmel og lagring | PASS | Native SKY R7: 24 kontroller av alle fem formasjoner, faktisk okklusjon og gjenoppretting. Feltkameraets sammenligning med/uten lyset gir faktiske piksler i `sky01-field-photo.png`; bildet er åpnet og kontrollert. |
| Fersk ordinær Continue | PASS | To nye native prosesser fra R5s faktiske journallagring, High og Low-calm. Kun kameraretning og isolert calm-innstilling er endret i uttrykkelige kopier. Begge viser buen og to monterte feillys. |
| Pakking R2 | PASS | Frosset kilde `b68d3a08`, `candidate-r2/Linux`. Ingen materialfeil ved cook; kandidatens åtte manifestfiler er hashkontrollert også etter prøvekjøring. |
| Strøm og faktisk glød | PASS | Alle 27 antennebeacons og `yard.glow.points[6]` avleses før feil, ved faktor 0.12 under feil, etter T01 og ved hver diskrestore. Originalfargene kommer tilbake uten kumulativ dimming. |
| Eldre SC02-lysbue | PASS | Pakket R2: 14 Arc/calm-kontroller fra navngitt tidligere faktisk T01-lagring. Kroppens fremkomst, kort utbrudd, modalpause og to diskrestores inngår. |

T01-resultatet anvendes før resultatmodalen slipper: SARO-strømmen kommer tilbake etter fysisk lampeavlesning, mens SC02 fremdeles venter på full H02-presentasjon. SaroPower gjenoppretter nå normalglød og antennebeacons før settled-flagget hindrer videre endringer; det stopper tap av fargebaseline gjennom Continue. Første pakke R1 bestod den eldre 71-kontrollen, men ble avvist etter dette konkrete kildefunnet. R2 har den utvidede faktiske fargeavlesningen. Tidligere restore-mismatch er rettet ved å arkivere den allerede eksisterende referansen ved overlevering. Feil i testens telefonmål og gamle krediteringsantall er rettet og prøvd på nytt.

SKY R5/R6 sto helt inntil porten og ble korrekt skjult av en opak metallstang. Diagnostikken viste vanlig, upauset kamera og synlig billboard, med faktisk treff i stangen. R7 beviser først denne blokkeringen og bruker deretter et gangbart ståsted fem meter bak porten. Produksjonens lysbane og siktkrav ble beholdt.

Begge native trafobilder viser et renderingsvarsel: ray tracing-geometri bruker 83.495 MiB av et budsjett på 400 MiB, over varslets 20 prosent. Prøvene krasjet ikke. Varselet er ikke et ytelsesbevis eller en avklaring av de separate diner-krasjene i FIX-01.

## Bevis og grenser

Bevismappe: `~/scratch-bm/Signal47-night-20261028`. Kildebindingene ligger i `source-final-r2.json`, `source-native-r5.json` og `source-sky-r7.json`; kjøreplaner, guard-logger og `*.result.json` fører faktisk exitkode, forventet PASS-tekst og ressurskonkurranse. `candidate-r2-manifest.json` og `candidate-r2-acceptance.json` binder pakke, kilde og avgrenset aksept. `native-proof-preservation.json` og `packed-r2-preservation.json` bevarer originale bevisfiler byteidentisk utenfor `/tmp`. Kamerafiksturene er ført i `trafo-continue-camera-fixtures.json`.

Testdriveren bruker produksjons-API-er, faktiske panel-/journal-/lamperays og ferske disklesere. Aggregatets virkelige oppvarming og lastoverføring samt portens nærhet og retur kjøres uten syntetisk oppgavecredit. SC01-fakta og ett originalfoto hentes fra en uttrykkelig navngitt tidligere prøve. Telefon-, felt- og returposisjoner samt road-start er merkede testfiksturer. En separat SC03-grenseprobe bruker tidligere faktisk SC02-flukt med eksplisitt testrevisjon, uten å gi ny dispatch- eller tildelingscredit.

Dette beviser ikke ny menneskelig gjennomspilling, fersk SC02-flukt, full SC03-gjennomføring, manuell helkjøring, fysisk gamepad, subjektiv lytting, fullmatrise, ytelse på andre maskiner eller målt helspilltid. Disse grensene består også etter merge. Ingen installasjon eller offentlig release inngår.

Generelle Unreal-referanser fra prosjektbiblioteket er lest og merket UNVERIFIED; prosjektets eksisterende historie-, strøm-, lagrings- og effektløype er videreført. Ingen ekstern kildekode er kopiert.
