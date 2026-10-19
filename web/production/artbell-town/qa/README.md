# Earl ved studiodøren: QA

Nettleserstatus: **PASS**, Pages 101/101 og enkeltfil med streng CSP 102/102. Faktiske resultater står i [Pages](pages/artbell-town.json) og [enkeltfil](single/artbell-town.json). `tools/artbell-town.py` kjøres fra `web/`; `S47_URL` velger Pages under `/SIGNAL-47/`, ellers brukes `dist-single/index.html` med samme strenge CSP som `readaloud.py`.

Prøven observerer original dekoder, bufferidentitet, kilde-start/-slutt/-stopp, faktisk varighet og lydgraf. Ingen lyd erstattes. Den komplette samtalen bruker ekte E mot studiodøren og ekte Calls, Visitors, Tape og Boy-knapper. Alle 16 Earl-klipp skal starte; fulle utvekslinger skal ende naturlig. Nye valg, Replay og Stop kontrolleres også. Direkte Earl skal gå til vanlig master, uten AM-filter eller radiovolum.

Byen og spillerens posisjon settes med eksplisitte testfixtures før ekte inngang. `ev:boy` representerer den allerede observerte radiosendingen; den beviser ikke en ny gjennomspilling av den hendelsen. Den opprinnelige emnerekkefølgen, vitnepapiret og notatet kontrolleres. Ekte 844 × 390 touch-Use og emneknapper prøves etter at asynkron by-/modellinnlasting og vanlig frameoppdatering er ferdig.

Avbruddsprøvene dekker nytt valg, Av, Close, Escape, nytt UI, telefon, reset, jump, lastet case og tittel, både med aktiv lyd og forsinket genuin dekoding. API-fixtures er navngitt i rapporten. Lasting/tittel følger den ekte menyreisen etter at spilleren har lukket samtalen. Telefonprøven starter to ekte eksisterende Ward-buffere gjennom telefonmotoren. Alle sene resultater må forbli kansellert. En faktisk AudioContext-suspensjon er en separat API-prøve, ikke en påstand om at pausemenyen bevarer en åpen samtale.

Det felles lagrede Art Bell-valget skal styre radio og direkte Earl. Innringere og Kinski skal beholde egne valg. Automatisk Earl prøves med Kinski Av; Art Bell Av skal bevare den eksisterende manuelle Kinski-lesingen. Med Art Bell På skal Kinski-kontrollen utelate Earls linjer og kunne ta over uten overlapp. Mobilvalg og reload prøves via ekte knapper. Enkeltfilen skal virke under streng CSP uten eksterne MP3-forespørsler.

Faktiske kjøringer skriver `artbell-town.json` og skjermbilder i valgt resultatmappe. JSON-filens sjekker og eventuell `fatal` er resultatgrunnlaget. Menneskelig lytting, uttale, stemmelikhet, fysiske høyttalere og målt enhetsytelse er **UNVERIFIED**. Tidligere Art Bell-radio, innringere og Kinski inngår i hovedagentens separate kompatibilitets- og bevaringskontroller.


## Kjøringer og grenser

Kandidaten integrerer main `f1e4616` i `ef73d4c`. CPU-resultatene har eksakt kildecommit; [byggmanifestet](../build-manifest.json) binder de faktiske kildefilene til begge payloadene. Alle 19 kompatibilitetskjøringer består, inkludert de tidligere radio-/Kinski-/telefonreisene, beskrivelser, streng CSP og PR116s getout 50/50. Bygg uten privat lydbank består separat 22/22; sluttbyggene inneholder banken fra `609543f`.

Første Earl-forsøk bestod 93 krav før en manglende Python-import stoppet mobiltesten. Den gamle radiotesten forventet den tidligere innstillingsteksten; den krever nå hele den nye radio-/Caliche-teksten og begge uavhengige øvrige innstillinger. Begge feilbevis er bevart i samtalens `outputs/artbell-town/`.

Første Kinski-mute-forsøk hadde RMS 0,0001987 bare i første målevindu, deretter åtte eksakte nuller. Analyseren tok første prøve etter 40 ms med 2048 samples, 42,67 ms ved 48 kHz. Uendret nykjøring bestod 37/37; ingen produktkode eller måleterskel ble endret. Den opprinnelige rapporten ligger i `outputs/artbell-town/failed-readaloud-mute/`.

Alle 13 spillregresjoner og lydkostnad 17/17 består. [Sluttattesten](validation.json) verifiserer 2673 kildefiler, 1937 Pages-filer og enkeltfilens eksakte hash. Skjermbilder av den nye funksjonen beholdes her; 310 større kompatibilitets- og kapittelbilder er arkivert utenfor repoet i `outputs/artbell-town/integration-qa/`. Ingen ytelsestall her skal leses som fysisk mobil-/PC-ytelse.
