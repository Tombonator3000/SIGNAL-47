# Art Bell som Night Line-verten og Earl i Caliche

Direkte bestilt av Tom 19. oktober 2026. Arbeidsgren `codex/art-bell-radio`, opprinnelig fra main `aa38e2b`, med nyere main `d6dc68b` integrert gjennom `66667c3`. VoiceStudio og OmniVoice gjenbrukes fra prosjektbibliotekets tidligere kontrollerte oppsett. Ingen nye dialogreplikker eller spillkonstanter skrives.

## Spilleropplevelsen

Når radioen er på AM 1420, leser en syntetisk Art Bell-parodi Earls eksisterende Night Line-replikker. Det omfatter det faste programmet, guttens samtale, avmeldingen klokken fire og vertens innslag på kjøreturene. Innringere og Roswells morgenvert beholder sine nåværende stemmer. Kinski-opplesningen styres separat. Den direkte samtalen med Earl bruker samme stemme og valg; se avsnittet under.

Valget **Art Bell radio host** finnes i Settings og pausemenyen, er på som standard og huskes på enheten. Radioen er fortsatt av når en ny natt starter. Valget Off bruker den opprinnelige pratelyden. Settingsteksten forklarer at stemmen er en syntetisk parodi. Art Bell har ikke lest spillets manus.

## Kontrollpunkter

1. Radioens ekte brukerhandling starter medfølgende, dekodede MP3-filer for riktig vertstekst. Tekstingen følger opptakets varighet og den aktuelle tidsmeldingen.
2. Hele vertens eksisterende manus dekkes, også dynamiske tidsmeldinger, uten å gi innringerne samme stemme.
3. Bilradioen og diner-radioen deler én sending. AM-filter, romlig plassering og Radio-/hovedvolum fungerer som før.
4. Pause, stasjonsbytte, Off, ny natt og lasting stanser eller gjenopptar riktig lyd uten forsinket gammel avspilling. Kinski og øvrige radioelementer fungerer fortsatt.
5. Vanlig bygg virker fra Pages-undermappen, enkeltfilbygget trenger ingen genereringstjeneste, og relevante spilltester består.

## Produksjonsstatus

VoiceStudio-profilen er `494881fc`, merket Art Bell og syntetisk radio-parodi. Referansen er en 4,40 sekunders frase fra [Art Bells CD-hilsen fra 2003](https://archive.org/details/Artbell.com_Commemorative_Disc). Kildelyd, referanse, profil og modellvekter følger ikke spillet.

115 medfølgende MP3-filer utgjør 2 065 028 byte og 247,98 sekunder filvarighet. Genereringen bruker eksisterende OmniVoice med engelsk, 32 steg, seed 42 og hastighet 1. Filkontrollen består for alle 115, uten manglende filer, droppede chunker eller dekodede samples som klipper. De 1555 Kinski-filene og gammel katalog er faktisk sammenlignet mot Git og bevart uendret.

Sluttkandidaten inkluderer privatbanken fra `SIGNAL-47-assets` på `12afec3`, med 15 lisensierte lyder i både Pages- og enkeltfilbygget, slik CI bygger. Enkeltfilen er 98 514 754 byte, SHA-256 `fb62e9c4894fad42752f55fa929493805896f0c5285f61b6f348641e2b0f12e7`. [Byggmanifestet](production/artbell/build-manifest.json) identifiserer kandidaten, kildefilene og de to byggene. Et separat bygg uten privatbank består fallback-prøven 22/22; dette er ikke sluttbygget.

Den nye avspillingen består nå **51/51 i Pages-undermappen og 52/52 i enkeltfila med streng CSP**, med motorprøver **48/48**. Dette inkluderer siste retting for å bytte bort fra og tilbake til en allerede dekodet sending: riktig forskyvning og én kilde gjenopptas, og nøyaktig én levende caption gjenopprettes. Opptakene er virkelig dekodet, AM-rutingen og PCM-prøvene er målt, og tastetrykk/berøringer, pause, volum, to mottakere, dynamiske klokker, avbrudd og lagrede valg er prøvd. BOY, avmelding og kjøreinnslag har manusbaserte scenarier gjennom samme stasjonsplanlegger; de vanlige hendelses-/kjøreradiotestene kontrollerer utløserne separat.

Ny parvis lydkostprøve består 17/17 og måler On/Off/On til 29,364/29,396/29,586 parameterkall per bilde. Den gamle grensen på 1500/s feilet også for grunnarbeidet uten Art Bell; dette feilforsøket er bevart som historikk. Den oppdaterte grensen på under 36 per rendret bilde bygger på de parvise målingene. Firesteds-`soundcheck` består 4/4 med null målte lydutfall og henholdsvis 12,45/12,43/29,93/29,52 parameterkall per bilde i kontrollrommet, ute, stående førerhus og kjøring med radio.

Den lokale sluttkandidaten er ferdig validert. Alle 12 kompatibilitetsprøver består, inkludert lisensiert bank 38/38, eksisterende Kinski-opplesning 37/37 og 38/38, telefoner 49/49 og 50/50, beskrivelser 115/115 og 116/116 samt streng CSP. Alle 13 spillregresjoner består: walkthrough 57, kapittel 1 til 6 med 75/76/72/69/55/44, utviklermeny 14, kjøreturer 23, naturlyd 7, diner 15 og musikk Pages/enkeltfil 12/5. [QA-oversikten](production/artbell/qa/README.md) lenker sluttloggene og skiller historiske forsøk i `before-warm-retuning-fix` fra sluttbevis. Publiseringsstatus føres separat.

Testverktøyene er tilpasset den nye lisensierte banken: den isolerte telefonprøven håndterer `LICENSED`-globben, beskrivelser venter på full preload, og walkthrough/CSP skiller 47 ordinære, 24 telefon- og 0/15 lisensierte lyder. CSP-forventningene for fonter og bilder er bevart. Feillogger fra tidligere forsøk er beholdt. Seks nye UI-bilder følger QA-materialet; 282 større regresjonsbilder ligger utenfor repoet i samtalens `outputs/artbell/integration-qa/`.

Referanse, profil, korpus, ASR-avvik, faktiske lydmålinger og testresultater føres under `production/artbell/`. Demoens ASR staver Caliche som Kalish. Menneskelig lytting, uttale av Caliche, stemmelikhet, fysisk PC-/mobillyd, sømløshet mellom tidsfraser og ytelse er UNVERIFIED.

## Earl ved studioet i Caliche

Direkte bestilt av Tom etter innfletting av radiostemmene. Gjenbruker prosjektbibliotekets VoiceStudio-kilde, den eksisterende lokale 0.5.6-installasjonen og Art Bell-profil `494881fc`. Ingen ny referanse eller modell trengs. En separat bank på 16 replikker og 150 ord holder de tidligere 115 radioklippene, tidsmalene og katalogen uendret.

Spilleren bruker buzzeren ved 1420 AM før klokken fire. Earl svarer automatisk med Art Bell-stemmen, også med Kinski-opplesning Av. Nye samtalevalg spiller bare de nye EARL-linjene. Spillerens YOU-svar, asider og papirer blir aldri lest med Earl-stemmen. Samme Art Bell-innstilling gjelder radio og samtale; ingen ekstra preferanse innføres.

Kontrollpunkter:

1. Begge åpningene og alle fire emner bruker riktige opptak fra de 16 manuslinjene. Eksisterende krav for calls, tape og den observerte gutten beholdes.
2. Earl spilles som nær samtale gjennom hovedlyden, med naturlig bufferlengde. Radioens stasjon og program blir ikke erstattet av dialogen.
3. Nytt svar, lukking, Replay/Stop, Art Bell Av, telefon, manuell Kinski og reset/load/title/jump avbryter gamle kilder og sene dekoderresultater.
4. Kinski-knappen bruker øvrig fortelling når Art Bell er På. Art Bell Av gir den opprinnelige tekst-/Kinski-opplevelsen, uten automatisk Earl.
5. Begge bygg, streng CSP, ekte E/touch og samtaleknapper, lagring og tidligere stemmereiser kontrolleres. Alle 1698 gamle MP3, tre kataloger og eksisterende profiler bevares.

Produksjonen på `codex/art-bell-town` er ferdig: 16 MP3, 370 496 byte og 44,92 sekunder. ASR er ordrett for 14 klipp; to har bare dokumenterte skrive-/tall-/homofonavvik. Alle tidligere stemmefiler og de 12 lokale profilene er uendret. Kilde, faktisk generering, filhash og automatisk ordkontroll ligger i `production/artbell-town/`.

Funksjonsprøven består 101/101 i Pages og 102/102 i enkeltfilen med streng CSP. Alle 16 opptak er faktisk dekodet og spilt gjennom, med naturlige bufferender og direkte hovedvolum. Ekte E, touch-Use, emnevalg, Replay/Stop, lagrede valg og avbrudd er prøvd. Korpus-/by-/observasjonsfixtures og API-avbruddene skilles fra den ekte kontrollreisen i [QA-oversikten](production/artbell-town/qa/README.md). Alle 19 kompatibilitetskjøringer, 13 spillregresjoner og lydkostnad 17/17 består. [Sluttattesten](production/artbell-town/qa/validation.json) kontrollerer at kildefiler og faktiske bygg fortsatt matcher. Privatbanken er med i sluttbyggene; separat fallback uten banken består 22/22. Testet enkeltfil er 100 410 706 byte, SHA-256 `d2d7cc9d047daa429a728ae3f445306359af6398c81fe9df88c87e14c0e692d7`. Publisering kontrolleres etter vanlig PR/merge og føres i leveringsbeviset. Menneskelig lytting, stemmelikhet, uttale og fysisk PC-/mobillyd er UNVERIFIED.
