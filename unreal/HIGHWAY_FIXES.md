# Sammenhengende highway og åpningstur

24. oktober 2026, `codex/highway-continuity`. Tom melder manglende Roswell-sperre, taus startbil, brå naturskifte ved Motor Court og to parallelle strømnett. Godkjent mål er én detaljert hovedlinje med nødvendige avgreninger, fungerende motor og sammenhengende natur gjennom områdeskiftet.

## Endringer

- Roswell følger historiens eksisterende åpning i kapittel 5/6 også i pendlerbilen. Fire fysiske sperrestolper følger synligheten; den permanente grensen mot Roswell beholdes. Hinderindeksene flyttes ikke.
- Motoren bruker lydregisterets `engineIdle`, `engineDrive`, `engineStart` og `engineOff`. Loopene har faktisk lavpassfilter. Stopp kansellerer oppstartsrampen, og rask ny innstigning beholder ikke gammel stopptid.
- Eksportens tomme veivegetasjon var skjult og ble aldri aktivert etter runtimefylling. Kun de prosedyreeide drawbarna for road/saro aktiveres, med eksisterende frø, dekning og foreldregrense.
- Et felles visuelt landskapslag bruker road som kanonisk ramme. SARO flyttes med Three-offset `(8024,0,-300)` inn i samme ramme; ved områdeskift flyttes bare den visuelle roten tilbake. Geologi, naturdekor og strømnett beholder dermed plassering relativt til spilleren. Logiske områder, spillgulv, ruter og historietilstander beholdes.
- En 200 m overlapp av eksisterende plante-/steinprototyper bruker samme høyde og veidekning som runtimevegetasjonen. Ingen nye gulv eller blokkere. Eksisterende Caliche-HISM beholder lokale instansmatriser.
- Én detaljert hovedlinje på eksisterende road-X15 beholder strømlydens og byens ankre. SARO, motellet, ranchen og trafostasjonen har avgreninger. Gamle stolper og høye ledninger fjernes målrettet; delte gjerdewirer og vrakscenen beholdes. Ufullstendig kilde/import beholder opprinnelig nett atomisk.

## Reprodusering

`Tools/highwaypower/prepare_cpu.py` gjør kun de to ignorerte Web-ledningsmeshene CPU-lesbare før cook. Ingen tredjeparts råassets eller importerte Web/Free-filer legges i Git. Runtimeklonene bevarer vertexdata, materialer og gjerdeindekser og fjerner bare validerte høye ledningstriangler.

`-S47HighwayAudit=barrier|seam` er avgrenset til isolert developerprofil. Valgfri `-S47HighwayShots=<mappe>` tar overgangsbilder. Audit bruker faktisk kjørekontroller, native komponenttilstand og reelle dekodede motoropptak. Isolert PCM-render fra faktiske loopparametre er en numerisk lydkontroll, ikke menneskelig lytting eller måling av hele lydmiksen.

## Kontrollstatus

Implementering ferdigstilles. Native bygg, pakket kontroll, visuell vurdering og levering er foreløpig ikke bekreftet. Ingen full verdensrunde eller PSO-kø inngår. Menneskelig gjennomspilling, lytting og helruteytelse er ikke dokumentert her.
