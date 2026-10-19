# Kinski-opplesning

Bestilt direkte av Tom 19. oktober 2026. Arbeidsgren: `codex/kinski-read-aloud`, fra main `bada636`. Dette er en lokal leveranse til gjennomgang, uten push eller publisering.

## Bruk

Åpne Settings på tittelskjermen eller pausemenyen, og sett **Kinski read-aloud** til **On**. I et dokument velger du **Read document** for selve papirteksten eller **Read description** for forklaringen ved siden av. I en samtale velger du **Read aloud**. Knappen blir **Stop reading** under lasting og avspilling. Valget huskes på denne enheten.

Stemmen er en syntetisk Kinski-parodi, generert lokalt i VoiceStudio med OmniVoice. Ingen av innspillingene er Klaus Kinski som leser spillets tekst. Spillets eksisterende engelske tekst brukes ordrett. Radiostemmene og historien beholder sin eksisterende virkemåte.

Lydfilene følger begge byggene. Enkeltfilbygget `dist-single/index.html` har lydene innebygd og kan brukes uten nett eller en kjørende VoiceStudio. I det vanlige Pages-bygget hentes bare klippet som spilles; opptil fire dekodede klipp og 32 MiB holdes i minnet. Hovedvolumet styrer også opplesningen.

Opplesningen er av som standard. Ingen tale starter bare fordi et panel åpnes. Lukk panel, bytt samtaletekst, trykk Stop reading eller åpne pausemenyen for å stoppe. Spilleren får beskjed dersom teksten mangler en innspilling. Endret eller dynamisk tekst erstattes aldri med en feil replikk.

## Akseptansekriterier

1. Valget er av fra start, kan slås på i begge innstillingsmenyer og huskes etter gjenåpning.
2. Dokumenter og samtaler med opptak spilles fra ekte medfølgende lydfiler, i tekstens rekkefølge, ved et uttrykkelig knappetrykk.
3. Stop, lukk, ny tekst og pause stopper avspilling, også når en fil fortsatt lastes eller dekodes.
4. Vanlig bygg virker under `/SIGNAL-47/`, og enkeltfilbygget har ingen ekstern avhengighet for stemmen.
5. Manglende opptak gir synlig tilbakemelding; mobilens tekstpaneler og innstillinger kan rulles og brukes.

## Tekstdekning

Korpuset har 328 unike klipp og 4798 ord: 84 avsnitt fra selve papirene, 52 beskrivelsesavsnitt og 192 samtalelinjer. Det dekker de statiske papirvariantene og replikkene i samtalepanelene, også de endelige variasjonene mellom kontrollmetoder. Rene skillelinjer og markøren for blyantskrift leses ikke opp. Innhold som `-39 LY` beholdes.

Papirene S-03-logg, solve-printout og telex har løpende klokkeslett eller meldinger. Beskrivelsene til frame01 til frame04 og bortføringsfotoene har dynamiske felt. Disse lesingene får beskjed om at opptak mangler. Hver knapp krever opptak av hele sin tekst; en tilgjengelig beskrivelse kan derfor fortsatt leses selv om papirteksten er dynamisk. Korte feilvalgsvar, valgknapper, titler og radio er utenfor korpuset.

## Verifisering og kildegrunnlag

Resultatene og avgrensningen av tekstkorpuset føres i `production/kinski/` og `log.md`. Automatisk filkontroll, talegjenkjenning og nettleseravspilling bekrefter tekniske egenskaper. Hvor lik, sint og morsom stemmen høres ut, må vurderes ved lytting.

Begge bygg, de pålagte spilltestene og de nye opplesningsprøvene passerer. Vanlig bygg: 37/37. Offlinefil med streng CSP: 38/38. Detaljer og faktisk grunnlag finnes i `production/kinski/qa/README.md`. Offlinefilen er 44 213 509 byte, med 11 534 432 byte stemmelyd som utgjør 23 minutter og 33 sekunder.

Prosjektbiblioteket er søkt før implementering. Gjenbrukte kilder er VoiceStudio-oppsettet og SIGNAL / 47s eksisterende `AudioSys`, dokumentvisning, samtalepaneler og innstillingslagring. Ingen ekstra TTS-tjeneste eller nettleserstemme brukes under spilling.
