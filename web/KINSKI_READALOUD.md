# Kinski-opplesning

Bestilt direkte av Tom 19. oktober 2026. Arbeidsgren: `codex/kinski-read-aloud`, fra main `bada636`. Dette er en lokal leveranse til gjennomgang, uten push eller publisering.

## Bruk

Åpne Settings på tittelskjermen eller pausemenyen, og sett **Kinski read-aloud** til **On**. I et dokument velger du **Read document** for selve papirteksten eller **Read description** for forklaringen ved siden av. I en samtale velger du **Read aloud**. Knappen blir **Stop reading** under lasting og avspilling. Valget huskes på denne enheten.

Stemmen er en syntetisk Kinski-parodi, generert lokalt i VoiceStudio med OmniVoice. Ingen av innspillingene er Klaus Kinski som leser spillets tekst. Spillets eksisterende engelske tekst brukes ordrett. Radiostemmene og historien beholder sin eksisterende virkemåte.

Lydfilene følger begge byggene. Enkeltfilbygget `dist-single/index.html` har lydene innebygd og kan brukes uten nett eller en kjørende VoiceStudio. Dokumentopplesning henter klippet som spilles; opptil fire dekodede fortellerklipp og 32 MiB holdes i minnet. De 24 korte telefonklippene forhåndslastes sammen med spillets vanlige lyder, slik at svar og teksting kan starte uten en ny nettforespørsel. Hovedvolumet styrer også opplesningen.

Kinski-valget er av som standard. Papirer og vanlige samtaler leses ved knappetrykk. Telefonstemmer starter automatisk når du svarer, ringer opp eller åpner telefonsamtalen. Ved nye valg i Ward-samtalen leses bare nye replikker. Lukk panel, bytt samtaletekst, trykk Stop reading eller åpne pausemenyen for å stoppe. Spilleren får beskjed dersom teksten mangler en innspilling. Endret eller dynamisk tekst erstattes aldri med en feil replikk.

## Akseptansekriterier

1. Valget er av fra start, kan slås på i begge innstillingsmenyer og huskes etter gjenåpning.
2. Dokumenter og samtaler med opptak spilles fra ekte medfølgende lydfiler, i tekstens rekkefølge, ved et uttrykkelig knappetrykk.
3. Stop, lukk og ny tekst stopper manuell opplesning, også under lasting/dekoding. Telefonen settes på vent sammen med lydklokken ved pause; den stoppes ved avslutning, nye valg, last og ny natt.
4. Vanlig bygg virker under `/SIGNAL-47/`, og enkeltfilbygget har ingen ekstern avhengighet for stemmen.
5. Manglende opptak gir synlig tilbakemelding; mobilens tekstpaneler og innstillinger kan rulles og brukes.

## Tekstdekning

Korpuset har 337 unike klipp og 4924 ord. De opprinnelige 328 klippene dekker 84 avsnitt fra selve papirene, 52 beskrivelsesavsnitt og 192 samtalelinjer. Telefonene bruker 24 taletekster: ni nye opptak og 15 gjenbrukte opptak. Det dekker de statiske papirvariantene og replikkene i samtalepanelene, også de endelige variasjonene mellom kontrollmetoder. Rene skillelinjer og markøren for blyantskrift leses ikke opp. Innhold som `-39 LY` beholdes.

Papirene S-03-logg, solve-printout og telex har løpende klokkeslett eller meldinger. Beskrivelsene til frame01 til frame04 og bortføringsfotoene har dynamiske felt. Disse lesingene får beskjed om at opptak mangler. Hver knapp krever opptak av hele sin tekst; en tilgjengelig beskrivelse kan derfor fortsatt leses selv om papirteksten er dynamisk. Korte feilvalgsvar, valgknapper, titler og radio er utenfor korpuset.

## Verifisering og kildegrunnlag

Resultatene og avgrensningen av tekstkorpuset føres i `production/kinski/` og `log.md`. Automatisk filkontroll, talegjenkjenning og nettleseravspilling bekrefter tekniske egenskaper. Hvor lik, sint og morsom stemmen høres ut, må vurderes ved lytting.

Telefonutvidelsen bruker det eksisterende telefonfilteret, 320 til 3200 Hz, og SFX-/hovedvolumet. Teksting og samtaleslutt beregnes fra hele opptakets faktiske varighet. Spillerasiden i samtalen til Nora forblir stille. Den samme Tomás-filen brukes både i hytta og på returlinjen i rom 6. Prologens taleløse anrop og nedtellingen på 47 sekunder beholder sine tider.

[Telefonmanuset](production/kinski/telefonmanus.md) samler alle 24 talereplikkene. Testgrunnlaget ligger i `production/kinski/qa/README.md`.

Prosjektbiblioteket er søkt før implementering. Gjenbrukte kilder er VoiceStudio-oppsettet og SIGNAL / 47s eksisterende `AudioSys`, dokumentvisning, samtalepaneler og innstillingslagring. Ingen ekstra TTS-tjeneste eller nettleserstemme brukes under spilling.


Telefonutvidelsen: 49/49 PASS i nettbygget, 50/50 PASS i offlinebygget og 28/28 PASS i telefonmotoren. De eksisterende opplesningsprøvene er fortsatt 37/37 og 38/38 PASS. Nåværende offlinefil er 44 600 610 byte, med 11 805 932 byte stemmelyd og samlet 24 minutter og 6 sekunder. Første spillfil er bevart lokalt som `SIGNAL-47-Kinski-v1.html`.
