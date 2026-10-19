# Historie og hendelser, 19. oktober 2026

Lokal spillbar utvidelse på `codex/story-events-20261019`, startet fra main `d1bfbc0f937915186fb3fe908db78b52b410b7b3`. Nyere main `c06bf331803611b94b438b75bcbe0aa58fa44583`, med Art Bell-radio, kjøpte lyder og nytt bakteppe, er tatt inn på arbeidsgrenen. Tom bestilte arbeidet direkte. Leveringsrutinen i `CODEX_TASKS.md` er lokal commit, uten push eller PR. Utvidelsen er implementert i spillet, men er ikke flettet til main eller publisert.

Fire nye vitnesamtaler og fire gjenlesbare notatkopier binder telefonmynten, patruljebilen, gutten på Night Line og den manglende kua til eksisterende personer. Vitnene svarer som mennesker; de faste opptakene svarer ikke. Returanropet kan oppleves etter rom 6, også ved en sen ankomst i grålysningen. 1947-sendingen varer 47 sekunder og gir det avbrutte programmet tilbake. Motell-TV-ens bilde er den faktiske scenen og kan beholdes etter Continue. Akseptkriteriene står øverst i [HENDELSER.md](../../../HENDELSER.md).

## Produksjonsløype

| Ansvar | Verktøy og versjon | Inn til ut | Verifisering |
| --- | --- | --- | --- |
| Spill og tilstand | Three.js 0.170.0, TypeScript 5.6.3 | Eksisterende `web/src` til spillscenen og samme saksformat | Typecheck og faktiske scenarioer |
| Nettspill og enkeltfil | Vite 6.4.3, singlefile 2.3.3, Node 22.23.3 | Kilde og bevarte assets til `dist/` og `dist-single/index.html` | Begge bygg, undermappen `/SIGNAL-47/` og offlinefil |
| Automatiserte reiser | Python Playwright 1.64.0, Chromium 156.0.8078.4, ANGLE/OpenGL | Virkelig nettleser, spilltilstand, bevegelse og UI-klikk til logger/bilder | Sekvensielt via prosjektets testlås, RTX 3080 bekreftet av WebGL-rendereren |
| Bilder og lagring | Eksisterende runtime-capture og SaveStore | Faktisk biltransform til TV-ramme; `{id,url}` til delt fotolager | Pikselforskjell med/uten bilen, Continue og lokal reserve uten IndexedDB |

Eksisterende avhengigheter ble lenket inn fra `/home/tom/SIGNAL-47/web/node_modules`, som avtalt i oppgavefila. Ingen installasjon eller versjonsendring. Ingen nye rasterassets eller Kinski-klipp. Den etablerte visuelle retningen er beholdt; bildene under er runtime-bevis. Nyere mains valgfrie lydbank er kopiert fra den eksisterende lokale private klonen til den ignorerte mappen, uten Git-data. Kildene er bevart og sammenlignet i `licensed-preservation.json`; ingen private kildefiler er lagt til i Git. Testen uten banken kjøres på et eget bygg før banken tas med.

## Åpne og prøv

Lokal forhåndsvisning: `http://127.0.0.1:18447/SIGNAL-47/` mens testserveren er åpen. Enkeltfilen er også bevart i samtalens `outputs/signal47/SIGNAL-47-historie.html`. Nøyaktig størrelse og hash står i `../build-manifest.json`. Den bruker radioens eksisterende reserveløsning for strømmet musikk; den lokale nettforhåndsvisningen bruker musikkfilene som Pages. Begge har de nye Art Bell-klippene og den lokale valgfrie lydbanken.

Snakk med vitnene igjen etter de tilhørende hendelsene. De nye emnene vises etter den relevante bakgrunnssamtalen; nestlederen må ha kommet tilbake. Vitnearkene ligger i Papers, TV-rammen i Photos. Begge kan åpnes igjen etter Continue.

## Bevis og kontroller

Siste status og hvilke etterprøver som erstatter tidligere feil står i [validation.json](validation.json). Første integrasjonskjøring er bevart i [integrated/summary.json](integrated/summary.json); den inneholder de opprinnelige testfeilene. Detaljer og faktiske antall står i hver `.log` og scenariots JSON. `radioarchive.log` dekker 51 kontroller av den virkelige radiomodulen, direktøren og dekodede Art Bell-planer i Node; `artbell-core.log` dekker de 48 eksisterende Art Bell-kontrollene. Typecheck og bygglogger ligger her. `../build-manifest.json` identifiserer kilde, offlinefil og hele Pages-treet. Varighet i sammendragene inkluderer venting i testkøen og er ingen ytelsesmåling.

- [Vitnesamtale i mobilformat](integrated/recheck/story-mobile/woman_followup.png)
- [Vitneark i mobilformat](integrated/recheck/story-mobile/witness_document.png)
- [Faktisk TV-bilde som kan åpnes igjen](integrated/tv/tv_document.png)
- [Bil parkert sør på tomta, i faktisk runtime-capture](integrated/tv/tv_actual_south_lot.png)

Disse fire representative PNG-ene følger den lokale commiten. Øvrige skjermbilder fra testrekkene er bevart lokalt og ignorert av Git på grunn av størrelsen. Tekstlogger, resultat-JSON og diagnoser følger leveransen.

`storyexpansion.py` bruker eksplisitt API-oppsett for hendelsesforutsetninger, deretter faktiske klikk i samtalene og Papers. Den prøver også hovedløkkas telefonvindu ved faktisk kapittel 5-klokke 05:03 og 05:15. Dette er ikke en menneskelig førstegangsreise. `moteltv.py` bruker Player.update gjennom kontordøra, vanlig interaksjonsstråle, en negativ prøve gjennom vegg, faktisk render med/uten bilen og Save/Continue. De eksisterende kapitteltestene prøver den samlede progresjonen. Mobilformat er 844x390, PC-format 1280x800. Samtalebildene tas etter at endelige CSS-animasjoner er spolt ferdig, så hele teksten er synlig.

Kjør fra `web/` med `~/.venvs/signal47/bin/python`. For GPU-kjøring settes `S47_GPU=1 PYTHONPATH=tools/gpu`. Vanlig bygg ble servert med byte-områder fra `tools/rangeserve.py` på `http://127.0.0.1:18447/SIGNAL-47/`. Uten `S47_URL` bruker testene enkeltfilen. Bruk testkøen som beskrevet i `CODEX_TASKS.md` og kjør én tung nettlesertest om gangen.

## Funn som ble rettet i løkka

- Returanropet hadde et tidsvindu som var slutt før kapitlet begynte. Grålysningens generelle sperre stengte det også ved en senere ankomst. Egen prøve ved 05:15 feilet på kandidaten før siste retting og består på sluttkandidaten.
- Telefonen journalførte hele opptaket og ga mynten også når spilleren gikk midt i anropet. Nå må hele opptaket oppleves; et delvis anrop kan prøves igjen.
- 1947-bulletinen var tekststyrt, rundt 26,5 sekunder, og hoppet videre i det ordinære programmet. Den er nå 47 sekunder, med fast manus og gjenopptak fra samme posisjon.
- TV-kameraet kunne miste en bil parkert lenger sør, og bildet kunne ikke åpnes igjen. Rammen er nå basert på bilens faktiske transform og lagres i eksisterende fotolager.
- To uavhengige agentgjennomganger fant at skjermens projeksjon alene tillot funn gjennom veggen. En kontroll av fri sikt og en negativ veggprøve er lagt til.
- Første testrekke fant feil i tre eldre testskript: bytesten ga ikke lazy-loading tid til å starte kjøringen, og radio-/reisereplikktestene returnerte en funksjon fra `evaluate`, som Playwright kalte uten argumenter. Bare testoppsettet ble rettet. Opprinnelige logger er bevart under `final/`; `verified/` er neste runde, som fant for kort pause før reisereplikker etter den gjenopptatte sendingen. Den eksisterende roen på ti sekunder er nå bevart før nye innslag. `final-checks/` er en senere, ufullført kjøring før nyere main ble tatt inn: kapittel 4 fikk tidsavbrudd ved start og kjøringen ble avbrutt ved kapittel 5. Den regnes ikke som en grønn sluttrunde. Reserveprøven for TV-bildet bruker samme skjuling av IndexedDB som kapitteltesten; et tidlig testoppsett med en kastende getter ble korrigert. Tidlige TV-prøver og den første nye lydproben traff samme funksjonsretur i testens feilinjeksjon; oppsettet er korrigert.
- Nyere main la til ekte vertsopptak. Radiogjenopptaket bevarer derfor også den dekodede planen, bufferposisjonen og valget av stemme, og nullstiller lydklokkens referanse ved retur. Valg av eller på under bulletinen blir prøvd. Den eksisterende Art Bell-prøven av avbrudd er flyttet til et ordinært, ledig program før den venter at bulletinen godtas; de faktiske planlagte kildene må fortsatt stoppes.
- En fast bulletin på 47 sekunder opptar mer av den korte turen til dineren. Den første regresjonsprøven ved 18 m/s kom fram før det neste reiseinnslaget kunne begynne. Første testtur bruker nå 12 m/s for å prøve både hele bulletinen og gjenopptaket før ankomst; kravene om minst fire innslag og minst sju sekunders vanlig program før neste innslag er beholdt. Den eksisterende regissøren endres ikke for å presse inn valgfrie innslag. `trip-budget.json` viser en reproduserbar prøve av den virkelige tilstandskoden ved begge hastigheter, uten bilfysikk; nettleserprøven bevarer faktiske prøvesteg og bulletinens grenser.
- En TV-prøve telte 307 mot 309 shaderprogrammer over hele de siste 17,2 sekundene av bærebølgen. Utslaget lot seg ikke gjenskape i to instrumenterte fullkjøringer. Den presiserte prøven måler rett før og etter den faktiske bildeopptakelsen og krever minst ett vellykket opptak uten nye program-ID-er under opptaket. Den består 44/44, med 304 mot 304 og ingen nye ID-er. Hele intervallet beholdes som diagnostikk; forhåndskompilering under bærebølgen er tillatt. Ingen produksjonskode ble endret og ingen faktisk hakking påstås. Original feil og diagnose er bevart i `integrated/cowstv-shader-investigation/`.

## Grenser og neste prøve

Ytelse i FPS er UNVERIFIED, selv om nettlesertestene bruker GPU-oppsettet. Ingen fysisk mobil, menneskelig førstegangsreise eller subjektiv lydmiks er vurdert her. Nye vitnesamtaler og notatkopier er tekst uten egne Kinski-opptak; eksisterende klipp er bevart, og oppleseren markerer tekst uten full dekning som utilgjengelig. Neste avgrensede prøve er å spille byen etter et par faktisk opplevde hendelser og vurdere rytme, tydelighet og telefonens lydsekvens.

Bibliotekskilder: [Playwright/SwiftShader, PASS](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/losninger/testing/playwright-swiftshader-three-lokalt.md) og [Pages-lagring, PASS](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/losninger/bygg/pages-deler-localstorage.md). Søk etter historie/hendelsesregissør ga ingen egnet ferdig løsning. Kildene ble brukt til å kontrollere prosjektets eksisterende test- og lagringsmønstre, ikke som en ny motor eller fortellerkanon.
