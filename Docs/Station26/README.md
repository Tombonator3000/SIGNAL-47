# Station26 — første sammenhengende feltreise

Arbeid startet 13. september 2026 fra flettet Scan25/PR21 og WorldCase22-hovedspillet. Dette er neste produksjonskandidat, ikke ferdig 5–6-timersspill. Tidligere pakker og brukerprofilen beholdes.

## Avgrenset spillerreise

Etter den eksisterende B-12-saken og P04/P05 gir feltfolioen ved arkivbenken avreise til STATION 01. Et avgrenset feltområde inneholder transit, opprinnelig fundament og flyttet merke, avskjermbar lampe, kabelsløyfe med åpne kuttflater og hytte med tidslogg. P06 og P07 kan gjøres i valgfri rekkefølge. P08 krever begge funn og et separat kabelbilde. P09 sammenholder de to tidsregistreringene. Spilleren kan vende tilbake, framkalle begge filmene på SAROs eksisterende våtbenk og gjenåpne originalene i notatboken.

Feltområdet ligger i en egen aktivert del av den eksisterende Unity-scenen. Avreise flytter spilleren til et fast ankomstpunkt etter checkpoint; det er foreløpig ingen kjørefysikk eller separat lastet målscene. Hele området og alle nødvendige komponenter følger samme bygg. Dette gjør gjenopptak og retur konkret prøvbare uten en tom reiseknapp.

Historisk oppstilling vises som et diagram. Et historisk negativ er ikke produsert, og tekstutskriften av mottaksfragmentet er ikke en innspilt skuespillerstemme. Motellet er fremdeles senere innhold. Dette passet må derfor beskrives som en spillbar feltsekvens, ikke fullført K3 med all endelig historie- og lydproduksjon.

## Visuell retning og gjenbruk

Eksisterende [stasjonskonsept](../DesignBible13/Visuals/concept-01-survey-station.png) og [Hybrid23-forlegg](../Research/Hybrid23/Visuals/station01-reference-gpt25.png) brukes videre. Den teksturerte bygningen er den allerede genererte og klippede Hybrid23-modellen som vanlig mesh; detaljer fra Archive17 gjenbrukes. [Kildeopphav](../Research/Hybrid23/asset-provenance.json) og `Unity/Assets/Signal47/Art/Station26/source-manifest.json` sporer filene. Ingen ny betalt tjeneste er bestilt.

Scan25 forblir isolert. En separat kode- og bildevurdering finner at den lille cyanfeilen kan skyldes lokal geometri eller rasterpresisjon, men dette er ikke bekreftet med en ny kjøring. Lavere detaljnivå og en geometrisk adskilt kontroll er neste mulige splatprøve. Ingen shaderretting, ny ytelsesgevinst eller produksjonsgodkjenning av splats påstås her.

## Observerbare krav

- Avreise krever støttet P05 og vellykket lagring; feil skal bevare spillerens område.
- Begge oppgaverekkefølger og feilvalg bevarer korrekte kunnskapsporter.
- To nye feltbilder kommer fra det virkelige kameraet, eksporteres og framkalles; eksisterende bildebytes bevares.
- Kilder, funn, lampetilstand og område følger checkpoint/gjenstart. Gamle v1-saker åpner uten feltframdrift.
- Bygg, faktiske runtime-bilder og en nyutpakket prøvepakke får egne bevis. Native input, full kapittellengde, subjektiv lyd og ytelse godkjennes bare med egne kjøringer.

## Verifikasjon

Faktisk Unity-spiller og nyutpakket Kubuntu-pakke er kontrollert 13. september 2026. Scenariene er API-styrte; dette er ikke en tastatur-/musprøve eller ytelsesmåling. [Samlet bevis](Evidence/verification.json) og [testdekning](TEST_PLAN.md) skiller utført arbeid fra åpne porter.

| Kjøring | Resultat |
| --- | --- |
| STATION 01 direkte, 1280×800 | 162 PASS |
| STATION 01 fra utpakket starter, 1600×900 | 162 PASS |
| Eksisterende arkivforløp fra samme utpakkede starter | 118 PASS |
| Eksisterende signal-, telefon-, modell- og hendelsesforløp | 42 PASS |
| Meny og gjenoppretting av historiske saker | 50 PASS |

Begge oppgaverekkefølger, interaksjonsstråler, gangruten gjennom hytta, fire faktiske kamerafiler, framkalling, gjenåpning og scenegjenstart består. Feil ved checkpoint og manglende grunn-/feltfoto prøves bare i isolerte profiler. Fem historiske kildefiler er hashkontrollert uendret. To lightmaps beholdes, og 28 kollidere tilhører feltområdet. Unity-kilden samsvarer med byggmanifestet, og alle 173 pakkefiler er kontrollert etter utpakking og test.

Den visuelle gjennomgangen rettet speilvendte etiketter, innvendig vegg som skjulte fasaden, mørk gangflate og for lav tekstkontrast. Faktiske [ankomstbilder](Evidence/Packaged1600/01-arrival.png) og [framkalt kabelbilde i UI](Evidence/Direct1280/08-developed-cable.png) viser sluttkandidaten. Terreng og flere instrumenter er fortsatt enkle prototyper. Eksporterte JPEG-originaler ligger separat i `Evidence/FieldPhotos/`; PNG-bildene med HUD er runtime-dokumentasjon.

Den avsluttende kodegjennomgangen fant at en mislykket fotoeksport kunne sperre retur mens ny eksport bare fantes på SARO. Returfolioen tilbyr nå ny eksport lokalt. Den korrigerte kandidaten prøver reell eksportfeil, gjentatt feil med identiske bevarte piksler og vellykket lagring etter reparert fotomappe. Tidligere 150-kontrollers bevis er bevart lokalt under `Artifacts/Station26/BeforeExportRecovery/`; sluttresultatene over gjelder det korrigerte bygget med 162 feltkontroller.

## Prøv kandidaten på Kubuntu

Nyeste lokale hovedspillkandidat er `Station26-8b181fb4830e`. Åpne `~/Nedlastinger/Station26-8b181fb4830e/Start-SIGNAL47.sh`. Hele mappen kan flyttes samlet. Den navngitte pakken finnes også under `Artifacts/Releases/`; den er ikke en binærutgivelse på GitHub.

Fortsett den eksisterende saken. Fullfør B-12 og arkivets P04/P05, og bruk **FIELD TRAVEL**-folioen ved arkivbenken. På STATION 01: E bruker rekvisitter, C åpner kameraet og Space eksponerer. Retur skjer via feltpanelet. Begge nye filmer kan framkalles etter samme retur. Vanlig standardstarter er fortsatt Visual10; den eldre WorldCase22-pakken og brukerlagring er bevart.

Kilde-SHA: `8b181fb4830ee7956234a8f24b077dfe92457cb6c6049ae74c6592553df527d4`.
Byggpayload: `877ad6a6898fdedb1c13983dba0b2e581cbca6cf956c803b67d5fe1ce3739e65`.
Arkiv: `SIGNAL47-Station26-8b181fb4830e-Linux.tar.gz`, 151 841 006 byte; SHA `b9c72948896c1d18777206fb2311e3177cdbce7da6ba1863bc9d416842e0d3c3`.

Native input, blind leseforståelse, målt spilletid, subjektiv lyd og separat releaseytelse er fortsatt **UNVERIFIED**. Neste samlede innhold er oppfølgingen av feltfunnene på SARO og den avgrensede overgangen mot SIERRA MOTOR COURT, sammen med manglende historisk foto-/lydproduksjon og nødvendige spillertester. Motellet og resten av spillet er ikke implementert i dette passet.
