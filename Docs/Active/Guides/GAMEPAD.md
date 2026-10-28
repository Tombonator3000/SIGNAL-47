> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../STATUS.md).

# Xbox-styring i Unreal

Arbeidsgren `codex/xbox-controller`, 24. oktober 2026. Tom bestiller styring med Xbox-kontroller, commit og push. Eksisterende tastaturbindinger beholdes separat. Sluttkilde `3d016a07` er bygget og pakket i kandidat X4. Alle 1373 selvtester og elleve native kontrolleropptak består de målte kriteriene. 16 målrettede regresjonsscenarioer består. Den komplette 110-runden ble stoppet etter Toms beskjed om å begrense testomfanget; den hevdes ikke fullført. PR149 er flettet til unreal-port-core, og samme testpakke er installert i den vanlige spillstarteren.

| Handling | Xbox |
|---|---|
| Gange / styre bilen | Venstrestikk |
| Se rundt | Høyrestikk |
| Bruk, start nøkkel, gå inn/ut | A |
| Andre valg / tilbake i paneler | B |
| Feltkamera | X |
| Hopp | Y |
| Løp, hold | Klikk venstrestikk |
| Nærmere blikk / kameraets lukker | LT / RT |
| Gass / brems og revers i bil | RT / LT |
| Lykt / bilradio | D-pad opp / ned |
| Kart / les beskrivelse | D-pad venstre / høyre |
| Notatbok / saksbrett | View eller LB / RB |
| Pause | Menu |
| Menyvalg og bekreftelse | D-pad eller venstrestikk, A |
| Sider og faner | LB / RB |
| Peker i paneler, klikk og dra | Høyrestikk, hold A |
| Rulle paneler under pekeren | LT / RT |

Stikkene har radial dødsone på 18 prosent, og ganghastigheten er proporsjonal. Høyrestikkets blikk bruker sekunder og en responskurve for presis sikting. Kontrollerhastighet og invertert Y lagres separat fra musen i settings.json. Knappetekst følger siste brukte enhet. Frakobling og tapt vindusfokus nullstiller input. Overgangen fra panel til spill krever at holdte knapper slippes og aktive stikker/triggere returnerer til nøytral. Avbrutt pekerdrag kanselleres før syntetisk museknapp slippes.

Linux på denne maskinen viser et virtuelt Sunshine Xbox Series-endepunkt. Testharnessen oppretter en separat, midlertidig Xbox 360-enhet gjennom uinput og endrer ikke Sunshine-enheten. Elleve X4-opptak bruker faktisk kernel til SDL til Slate til gameplay på et privat X11-display, med fokus verifisert på hvert eget spillvindu. `RenderOffscreen` brukes ikke til native input: Linux-utgaven av Unreal velger en null-applikasjon i den modusen. Det er fortsatt ingen prøve med en fysisk Xbox-kontroller.

`-S47GamepadLog` logger faktisk bevegelse, kamera og inputtilstand. Den isolerte plattformtesten bruker `-RenderOffscreen -S47GamepadBackgroundTest`, prosesslokal SDL-filtrering og egen UserDir. Testunntaket tillater bakgrunnsinput kun med begge flaggene; pekeren i denne testen rutes logisk uten å flytte skrivebordsmusen. Vanlig spill krever aktivt vindu.

Bevis og testharness ligger i `~/scratch-bm/Signal47-xbox`. X4 har egen kildefrysing og SHA256 for kjørbar fil og fem containere; ingen av de tidligere feilede forsøkene overskrives.

| Kontroll | Gjeldende resultat |
|---|---|
| Linux-bygg og cook | PASS, kandidat X4 |
| Pakket selvtest | PASS, 1373 sjekker |
| Native plattformopptak | 11/11 fullført, målte numeriske kriterier PASS |
| Gange, blikk og stopp | PASS, faktisk posisjon og orientering |
| Bil: gass, styring, brems og revers | PASS, faktisk bilfysikk; stikkens Y er ikke gass |
| Pause og notatbok | PASS, native Menu/View og B gir forventede modaloverganger |
| Holdt input etter pause | PASS, forblir sperret inntil stikker/triggere er nøytrale |
| Frakobling med holdt stikk | PASS, umiddelbar stopp; nøytral ny tilkobling står stille |
| Innstillinger | PASS, Xbox endrer til 1.15/On; ny prosess leser samme fil uten museendringer |
| Dokumentets A og panelenes B | PASS, opplesning og lukking målt |
| Målrettet regresjon | PASS, 16 ferdige scenarioer fra samme pakke |
| Hele screenshots.sh | IKKE FULLFØRT, testomfang redusert etter Toms beskjed |

Første virtuelle LT-hopp rett fra null til halv trigger ble utsatt av SDL til slipp. Getteren hadde rett verdi, men halv- og nullhendelsen kom i samme batch; Unreals aksekoalescer beholdt null. En egen diagnose mot motorens SDL-bibliotek dokumenterer dette. Den endelige bilprøven bruker vanlig forbevegelse før halv trigger, og både halv LT og LT 0.6 gir reell brems/revers. Det er ingen særbehandling av denne testfellen i produksjonskoden.

Selvtestene dekker knappetildeling, UI-ruting, uavhengige tastaturbindinger, dødsone, tidsbasert blikk og inputporter. Native opptak dokumenterer bare de målte handlingene. Faktisk kortdrag med lagret flytting og kansellering, hvert konkret fane-/mottaker-/terminalvalg, radiobytte, utløserens digitale terskler, ny aktiv input etter reconnect og vindusfokustap er fortsatt UNVERIFIED i native løype. Fysisk kontrollerergonomi, klienttransport, vibrasjon og Windows/XInput er også UNVERIFIED. Testpakken er installert i `~/Games/Signal47/Linux`. 40 payloadfiler er hashverifisert, og 48 Saved-filer er byteidentiske. Sikkerhetskopi: `/home/tom/scratch-bm/Signal47-xbox/installed-backup-1792853722324495699`. PR149: `fda74798`; testet runtimekilde: `3d016a07`.

Automatisk aksept gjelder denne avgrensede kontrollerendringen. Framover velges relevante tester etter risiko; den komplette verdensrunden brukes ved brede endringer eller påkrevd main-fletting. Flere agenter kan dele kode og gjennomgang, mens maskinregelen fortsatt tillater én tung UE-/Blender-jobb om gangen.
