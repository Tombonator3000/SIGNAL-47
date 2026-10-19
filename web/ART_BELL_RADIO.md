# Art Bell som Night Line-verten

Direkte bestilt av Tom 19. oktober 2026. Arbeidsgren `codex/art-bell-radio`, fra main `aa38e2b`. VoiceStudio og OmniVoice gjenbrukes fra prosjektbibliotekets tidligere kontrollerte oppsett. Ingen nye dialogreplikker eller spillkonstanter skrives.

## Spilleropplevelsen

Når radioen er på AM 1420, leser en syntetisk Art Bell-parodi Earls eksisterende Night Line-replikker. Det omfatter det faste programmet, guttens samtale, avmeldingen klokken fire og vertens innslag på kjøreturene. Innringere og Roswells morgenvert beholder sine nåværende stemmer. Kinski-opplesningen styres separat.

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

Den nye avspillingen består 49/49 i Pages-undermappen og 50/50 i enkeltfila med streng CSP. Opptakene er virkelig dekodet, AM-rutingen og PCM-prøvene er målt, og tastetrykk/berøringer, pause, volum, to mottakere, dynamiske klokker, avbrudd og lagrede valg er prøvd. BOY, avmelding og kjøreinnslag har manusbaserte scenarier gjennom samme stasjonsplanlegger; de vanlige hendelses-/kjøreradiotestene kontrollerer utløserne separat. Full kompatibilitet og spillregresjon kontrolleres før levering.

Referanse, profil, korpus, ASR-avvik, faktiske lydmålinger og testresultater føres under `production/artbell/`. Demoens ASR staver Caliche som Kalish. Menneskelig lytting, uttale av Caliche, stemmelikhet, fysisk PC-/mobillyd, sømløshet mellom tidsfraser og ytelse er UNVERIFIED.
