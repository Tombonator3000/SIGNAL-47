# Graphics pass 01 — verifikasjon og omfang

Dato: 2026-09-09. Lokal versjon: 0.2.0.
Utgangspunkt: den faktiske nedlastbare SIGNAL_47_vertical_slice_prototype.zip.
Ingen spillmotor byttet inn fra en ekstern CDN. Selvstendig WebGL1-renderer, vanlig JavaScript og WebAudio.

## Implementert
Ny graphics.js skiller rendering, materialer, modeller og animasjon fra spillets tilstander.
Det finnes to lokalt genererte teksturer: et 2048 × 2048 material-/panelatlas og en animert
512 × 256 CRT. Statiske flater tegnes samlet. Antennemodellen gjenbrukes for elleve antenner.
Lys bruker beregnede lyspunkter, analytiske møbelskygger og myke gulv-kontaktskygger — ikke en tung skyggekart-pipeline.
Skjermgrafer oppdateres inntil ti ganger per sekund, uavhengig av bildeoppdateringen.
Grafikkinnstillinger for eksponering og reduserte effekter er koblet til rendererens faktiske parametere.

Game.js beholder strøm, kalibrering, interferens, signal, papir, telefon, skjult forsinkelse,
koppeanimasjon, antennevending og sluttkort. Interaksjonshøyder følger de nye modellene.
Nye skap/stol har kollisjon; alle fem interaksjonsområdene har tilgjengelige gangruter.
Restart kansellerer gamle engangstimere og fjerner gjenstående menyer/varsler.

## Faktisk utført
1. `node --check` på begge JavaScript-filene: bestått.
2. Spill- og DOM-kode kjørt i Chromium med kun WebGL-API-et erstattet av en opptaker.
   Dette verifiserer JavaScript, generering av geometri/teksturer, UI-handlinger og tilstander,
   men er ikke en test av nettleserens GPU-driver. Resultatene finnes i verification/logic-tests.json.
3. Den virkelige telefonsvar-funksjonen ble kjørt, lydsekvensens forsinkelse fikk gå ut,
   og den opprettede fristen ble kontrollert til omtrent 47 sekunder etter opptaksslutt.
4. Tidsgrensene for fall, knusing og slutt ble kontrollert med eksplisitte testtidspunkter.
5. Gangruter kontrollert på et 0,2-meters rutenett mot spillets faktiske kollisjonsrektangler.
   Dette erstatter ikke en fysisk spilltest av museblikk og sikte-/avstandsfølelse.
6. De faktiske shaderne ble kompilert og lenket med Mesa EGL 1.5 / llvmpipe.
   Opptatte VBO-er, teksturer, kameramatriser og draw calls ble rendret til kontrollbilder.
   Ingen GL-feil. To kameravinkler ble visuelt inspisert, og proporsjoner/materialtetthet justert.
7. Kontrollbildet med påslått mottaker brukte 27 draw calls og 39 236 trekanter.
   Dette er ikke en FPS-måling og dokumenterer ikke 60 FPS på en fysisk enhet.

## Ikke verifisert eller ikke implementert
- Full ende-til-ende-spilling i en vanlig nettleser med aktiv WebGL/GPU.
- Bildefrekvens på Kubuntu-PC, mobil eller nettbrett.
- Musefangst på brukerens nettleser, fysisk inputfølelse og opplevd lydmiks.
- Touch-kontroller, ekte pause av historiens timer, dynamiske skyggekart eller fotorealistiske importmodeller.
- Synkronisering av lokal kode med Lovable-prosjektet. Grafikkjobben er sendt dit separat;
  ingen ferdig nettbasert grafikkversjon er bekreftet gjennom denne lokale pakken.

## Prosjektfiler
index.html: meny, instrumentpanel og grafikkinnstillinger.
game.js: spilltilstander, interaksjoner, signalanalyse, input og lyd.
graphics.js: alle lokale modeller/materialer, shaders og tegning.
start.sh / start.bat: valgfri lokal statisk server.
previews/: faktiske separat-rendrede kontrollbilder.
verification/: maskinlesbare kontrollresultater.
