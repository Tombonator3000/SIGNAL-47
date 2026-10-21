# SIGNAL / 47: intro før en ny natt

Direkte bestilt av Tom 21. oktober 2026: en egen tekstskjerm i ånden til klassisk, urovekkende TV-fortelling, med samme syntetiske Serling-stemme som resten av spillet. Original tekst, skrevet for SIGNAL / 47. Ingen tekst eller musikk fra The Twilight Zone brukes.

Kandidaten bygger på main `acfc09e`. Den nye introen og alle tre bygg er verifisert i `qa/validation.json`, med eksplisitt dokumenterte før-eksisterende hull i den øvrige Serling-dekningen. Autorisert innfletting, publisert innhold og lokal promotering kontrolleres separat mot det faktiske merge-commitet.

## Spillerflyt

New night åpner et svart kort med hele teksten. Opplesningen starter når Rod Serling narrator er På, som før er standard. Begin night avbryter eventuell tale og starter den vanlige prologen fra 23:41. Kortet blir stående når stemmen er ferdig. Back to title eller Escape avbryter introen. En valgt gammel sak slettes først når spilleren faktisk begynner natta. Continue, Load case og utviklerhopp går direkte til spillet.

Klokke, progresjon og spillagring starter ikke under kortet. Teksten er tilgjengelig også med stemmen Av eller ved innlastingsfeil. Menymusikken stanser under introen. Ingen ny grafikk, musikk, stemmeprofil, modell eller ekstern tjeneste er nødvendig. De fem nye opptakene ligger i en egen bank; tidligere opptak bevares.

## Manus

New Mexico, 1986. Twenty-seven antennas listen to a sky that has never answered. On the night shift, silence is a measurement. It can be filed, signed, and forgotten.

Tonight, the instruments will record something the sky has not sent. A telephone will ring in an empty room. A photograph will remember what no one saw.

Your name is Reyes. Your shift ends at dawn. Between now and then, forty-seven seconds will refuse to belong to any clock.

The record calls it interference.

The record is incomplete.

## Akseptanse og prøvegrenser

- Ny natt viser hele teksten og leser de fem avsnittene i rekkefølge med de faktiske MP3-filene.
- Skip, tilbake, dobbeltklikk og sen eller feilslått dekoding starter aldri en ekstra natt eller foreldet tale. Av krever ingen innlasting av introopptak.
- Continue, Load case og DEV beholder sine eksisterende ruter. Avbrutt intro bevarer sakene.
- Tekst og knapper er lesbare ved 1280 × 800, 844 × 390 og 390 × 844. Pages fra en undermappe og enkeltfilen skal begge fungere.
- Prosjektets påkrevde spill-, lyd-, lagrings- og styrekontroller kjøres på kandidaten. Menneskelig lytting, stemmelikhet og ytelse på ekte telefon er fortsatt egne, uverifiserte forhold.

Bibliotekskilde: [VoiceStudio](https://github.com/debpalash/VoiceStudio), dokumentert som AGPL-3.0 i prosjektbiblioteket. Den allerede installerte utgaven og eksisterende Serling-profil brukes med lokale, installerte modeller. Gjeldende kilde- og modellmerknader fra `../ensemble/profiles.json` og `../ensemble/README.md` gjelder fortsatt.

## Faktiske sluttkontroller

- Fem klipp, 34,20 sekunder og 276 892 byte. Alle fem har full teknisk lydkontroll og ASR uten leksikalske ordavvik; sammeverdi-skrivemåter for tall godtas. Ingen omtak. `audio-verification.json` og `asr.json`.
- Pages og enkeltfil: 70 av 70 kontroller hver, med full naturlig tale, master-PCM, klokke/lagring, skip, avbrudd, tastatur, mobil, forsinket/feilslått dekoding, fulle saker, Continue/Load og DEV.
- 13 påkrevde spillregresjoner og fire øvrige kompatibilitetskjøringer består, inkludert CHAPTER TWO-ringen fra PR #138. Ekstra Serling-prøve har 101/107: alle avspillinger og kapittelkort består, mens seks tekstinventarer har de samme manglende nyere objektopptakene som base acfc09e. Den faktiske basen er bygget og sammenliknet; ingen nye mangler er innført. Se `qa/serling-baseline-comparison.json` og den uendrede feilrapporten.
- Typecheck, intro 39, ReadAloud 67, Serling 47 + tekst 26, saves 19 og steercheck fem kontroller består. Pages, enkeltfil og privatbankfritt reservebygg er kontrollert.
- Seks faktiske bilder er inspisert ved 1280 × 800, 844 × 390 og 390 × 844 i begge bygg, uten lesbarhets- eller kontrollblokkering. `qa/visual-review.json`.
- Alle 3271 tidligere lydfiler og 18 stemmeprofiler med referanser er bevart. Privatbanken er bevart; lydfallbacken er prøvd med og uten privatbank.

Første Pages-prøve hadde to fokusfeil ved retur fra et valgt fullt saksspor. De er rettet. Uavhengig kodegjennomgang fant dessuten manglende tilbakeføring av menymusikk/SFX/ambience ved F2/DEV-lukking; egen livssyklusopprydding og seks nye nettleserkontroller dekker den. Første feilrapport er bevart i leveransens `outputs/intro/first-pages-focus-failure`.

De 84 eksisterende Python-verktøyene har fått eksplisitt klikk på introens ekte Begin night-knapp etter sine 111 New night-grener. AST-kontroll bekrefter at 81 bare har helper/import-tilpasningen. Tre avvik er forklart: korrekt importsti i Blender-undermappen, en foreldet CSP-forventning på 15 kjøpte lyder erstattet med faktisk navneinventar på 16, og musikkprøven som nå først bekrefter den eksisterende 47-sekunders stillheten i gården før den krever musikk. Det siste ble bekreftet i en instrumentert nettleserprøve etter første FAIL; produktets fire relevante lyd-/storyfiler er byteidentiske med base. Originalfeilen er bevart i `outputs/intro/first-music-failure`, og årsaken i `qa/music-diagnosis.json`. Serling-CPU-fixturen fikk også audio.buf/gt for den eksisterende PR #138-kortfunksjonen. Ingen av disse endrer spillmekanikken. `qa/harness-adaptation.json` skiller kildekontrollen fra de faktisk kjørte prøvene.

Menneskelig lytting, stemmelikhet, skuespill og opplevd uttale er UNVERIFIED. Det samme gjelder fysisk høyttaler-/hodetelefonutgang og FPS på ekte PC/telefon. Nettleserprøver og ASR erstatter ikke disse vurderingene. Nyere objekttekster fra senere hovedspillendringer, blant annet kuenes øremerker, linjeskriver/linjeenhet og dinerens sjåførnotat, har fortsatt ingen Serling-opptak. De er uendret fra basen og står synlig i sammenlikningsrapporten; denne bestillingen leverer introen.
