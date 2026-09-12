# Menu14 — tryggere ny nattevakt og første arkivprøve

12. september 2026. Avgrenset videreføring av den eksisterende Unity-menyen og checkpoint-systemet etter brukerens «Fortsett prosjektet». Dette er en **testkandidat**, med en kontrollert forbedring av dagens spill og forarbeid til designbibelens M1/M2. Det er ikke full implementering av tre saker, manuelle save-plasser eller nye historieområder.

## Resultat

Ved oppstart med en eksisterende checkpoint står **CONTINUE CHECKPOINT** først. Uten checkpoint starter første knapp en ny nattevakt. Menyen sier nå korrekt om en checkpoint-fil er funnet; den faktiske lastingen validerer filen og kan avvise den.

Ny nattevakt fra en eksisterende sak, pause eller prologens sluttkort åpner en egen bekreftelse. Avbryt bevarer meny/pausetilstand, tid og aktiv sak. Bekreftelse uten en åpen dialog gjør ingenting. Escape er koblet til avbryt; GUI-koden ber om førstefokus på «Keep current shift». Faktisk tastaturfokus, Enter/Tab og native museklikk gjenstår å kontrollere på et tilgjengelig skrivebord.

Ved bekreftet ny sak kopieres både aktivt checkpoint og backup med originale bytes til en unik `PreviousCases/<tid>-<id>`-mappe under sakens lagringsmappe. Kopiene lukkes og flushes før de aktive filene tømmes. Fotoarkivet og innstillingene berøres ikke. Pågående skriving eller innlasting blokkerer byttet. Feil ved forberedelse av kopier holder spilleren i gjeldende verden med en avbrytbar dialog. Hvis tømmingen feiler etter kopiering, beholdes gjenopprettingsfilene også; dette er ikke en transaksjon over en hel mappe ved strømbrudd.

`PreviousCases` er foreløpig et lokalt gjenopprettingsarkiv, ikke designbibelens kommende meny for tre samtidige saker. Ulagret fremgang beholdes i minnet ved avbrytelse; den går tapt ved uttrykkelig bekreftet ny nattevakt, slik dialogen beskriver.

## Arkivsaken P04/P05

[Spillerark, kildekort og fasilitatorfasit](PAPER_CASE.md) konkretiserer de første arkivoppgavene. [Diagrammet](archive-reference.svg) viser to versjoner av samme oppstilling. Det er tydelig merket som designmateriale, ikke historisk negativ eller spillerfoto.

Egenkontrollen fant at v0.1s vedlikeholdskort røpet at den korrigerte kopien var ufullstendig før spilleren sammenlignet dokumentene. Det justerte prøvekortet gir sporbarheten mellom B-12 og STATION 01, mens A/B-sammenligningen må begrunne endringen. Diagrammet er rendret og visuelt inspisert. M1s blindtest og K2s tidsbudsjett er fortsatt uverifisert; det historiske negativet er en navngitt produksjonsavhengighet, ikke et allerede ferdig asset.

## Kontroller og bevis

| Port | Status | Faktisk grunnlag |
| --- | --- | --- |
| Unity-bygg | PASS | 6000.3.22f1, eksisterende scene og lys bevart, ikke-development Linux-spiller |
| Meny-/checkpoint-kontrakter | PASS | 28 API-styrte kontroller i faktisk spiller; også gjennom pakke-launcher etter utpakking |
| Gjenoppta eldre sak | PASS, avgrenset | Kopiert fullført v1-sak med kun fotostier flyttet til isolert profil og envelope-hash oppdatert; faktisk scenelasting, to dekodede foto med uendrede bytes |
| Feil/avbryt | PASS | Pågående writer, blokkert recovery-mappe, ugyldig checkpoint, bekreftelse/avbryt og to separate arkiveringer |
| Visuell meny | PASS, avgrenset | Originale Unity-bilder av definerte API-oppsatte tilstander; 1280×800, Mesa Intel ARL; egen gjennomgang |
| Native brukerreise | UNVERIFIED | Ingen tastatur/mus sendt til skrivebordet; bildet eller API-kallet beviser ikke et klikk |
| Ytelse og bevegelse | UNVERIFIED | Ingen ny frame-måling eller himmelkontroll i bevegelse; Visual10-tall gjenbrukes ikke |
| Pakke | PASS | Kilde-/payloadhash, lisensfiler, utpakking, kjørerettigheter og oppstart via pakkens launcher med isolert profil |
| M1 forståelse/spilletid | UNVERIFIED | Forfattergjennomgang og prøvemateriale, ingen ny leser eller målt gjennomspilling |

Detaljer og filidentiteter: [verifikasjonsrapport](Evidence/verification.json), [runtime-kontroller](Evidence/result.json) og [byggmanifest](Evidence/build-manifest.json). Alle sju PNG-filer i Evidence er originale spillercaptures. Syntetiske checkpoint-bytes brukes i feilscenarioene; de er ikke utgitt for en normal spillerreise. Den historiske saken kommer fra den tidligere dokumenterte Visual10FinalActive-reisen. Originalkilden kontrolleres uendret etter prøven.

Oppdagede og korrigerte feil i dette passet: direkte sletting ved «New», overgang selv om sletting feilet, misvisende oppstartsstatus og gjenbruk av oppstartsbeskjeden inne i bekreftelsesdialogen. Testoppsettet ble også korrigert fra PNG til spillets faktiske JPEG-format; den første mislykkede testen er beholdt lokalt, og det endelige resultatet inkluderer reell dekoding av begge eksponeringene.

## Kjør igjen

`bash Automation/build-menu14-linux.sh` kompilerer gjeldende scene uten å regenerere eller bake den. `bash Automation/run-menu14-checks.sh /sti/til/Start-SIGNAL47.sh /sti/til/fullført-testprofil` oppretter en ny isolert profil, flytter kun kopienes fotoreferanser, kjører API-scenarioene og tar originale bilder. Ingen input sendes til skrivebordet. Testmodus krever både egen argumentverdi og en merket `menu14-test-*`-profil; vanlig spilling kjører ingen av disse scenarioene.

Native Chapter09-reisens Continue-koordinat velges nå fra observert `menuLayoutVersion`; eldre Visual10/NightSky12-spillere støttes fortsatt. Ny-reise-scriptet avviser profiler med eksisterende checkpoint. Disse native scriptendringene er syntakskontrollert, men ikke kjørt på det låste skrivebordet.

Fullfør blindprøven for P04/P05 og native meny-/checkpoint-reisen før de respektive portene lukkes. Neste implementeringsområde i roadmapen er en avgrenset sak-/lagringsflyt og områdeovergang, deretter arkivsekvensen. Nye miljøer og hele treplass-systemet er ikke startet i dette passet.

## Lokal leveranse

Testpakke: `Menu14-9c76ff2cf81b`. Åpne [Start-SIGNAL47.sh](../../Artifacts/Releases/Menu14-9c76ff2cf81b/Start-SIGNAL47.sh) fra den lokale pakken, eller pakk ut [Linux-arkivet](../../Artifacts/Releases/SIGNAL47-Menu14-9c76ff2cf81b-Linux.tar.gz). Hele pakkemappen må følge med. Standardfilen `Spill-SIGNAL47.sh` starter fortsatt Visual10.

Unity-kilder SHA-256: `9c76ff2cf81bbd024ea37c57224d55f54a8c6b62f0bd767641492fe16ef076f2`. Pakken er lokal og ikke pushet til GitHub i dette passet.
