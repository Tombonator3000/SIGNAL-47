# Seks fysiske skrivebordsark, ART-01

**Status:** PASS for denne avgrensede delleveransen. **Kontrollert:** 29. oktober 2026, UE 5.8.4 / Linux Vulkan. **Pakket kilde:** `343e594d277ceb04b09e585994967e8297c6b99d`. Kandidat R1 er separat fra installert Farm R1 og offentlig nedlasting. Samlet ART-01 står fortsatt åpen.

Seks ferdige dokumentmotiver er nå bundet til Dales eksisterende papirer i Operations: arbeidsmappe, indeks, avisutklipp, notat, kontaktkort og serviceskjema. [Kildeliste og importoppskrift](../../../unreal/Signal47/Tools/documentprops/README.md), [editorbevis](../../../unreal/Evidence/DOCUMENTPROPS-2026-10-29/editor-r1-receipt.json) og [pakket R1-kvittering](../../../unreal/Evidence/DOCUMENTPROPS-2026-10-29/packed-r1-receipt.json) binder motiver, faktiske native assets, kontrollresultater og originale bilder.

## Bevaring og faktisk binding

Codex overtok etter HANDOFF-regelen kl. 09:25 da Claudes hjerteslag var eldre enn 75 minutter og oppgaven fortsatt var åpen. Alle tolv opprinnelige deltafiler og 34 råbevisfiler, inkludert begge motorloggene, er sikret byteidentisk. Claudes `sc03`-arbeidskopi er urørt. De seks kilde-JPG-ene og seks opprinnelige native teksturene er fortsatt byteidentiske. Privat backupkvittering er hashbundet i den pakkede kvitteringen.

Hvert motiv får en egen Engine Plane 0,8 mm over sitt eksisterende papir, med bildets sideforhold, uten kollisjon eller skygge. Interaksjonsstrålen treffer fortsatt originalpapiret. Dokument-ID-er, interaksjons-ID-er og bevislogikk bruker de opprinnelige `S47Office::Use`-grenene. Privatmappen og arrayarket beholder sine eksisterende etiketter. Manglende tekstur gir gammel papirflate og etikett.

Auditen leser faktisk artkomponent, slot0, foreldremateriale, nøyaktig Map-tekstur og materialparametre: UseMap=1, NativeLighting=1, MapIsLinear=0, Roughness=0,88, Metalness=0, Emissive=0 samt MapU/MapV. Den krever også riktig Engine Plane, plassering, mål, NoCollision og ingen skygge. UV-radene er faktisk materialavlesning; Plane-bufferens UV-hjørner ble målt under originalimporten, ikke avlest fra GPU i runtime.

## Editor og pakket R1

Første editorbygg ble avvist på en UE_LOG-makro i en if/else-gren. Etter avgrensede klammer består bygg R2. Editor fra `8577fce3` består 1720 selvtester, tolv dag/skumring/nattbilder med 72 faktiske materialauditer, og den fysiske dokumentreisen med 580 kontroller, 4,551080 m API-gange, 47 fokuserte Slate-tastetrykk, fem diskforløp og elleve kunst-auditer. Begge prosesser avsluttet normalt med exit0. Claudes eldre R2 med tolv komplette PNG-er og PASS-markør hadde exit143; den brukes derfor ikke som bevis på normal avslutning.

Main gjennom installasjons-PR225 er flettet uten runtimeendring. Pakket R1 fra `343e594d` består bygg/cook/archive, exit0 på 404,42 s. Alle **13 468 fryste runtime-inputfiler / 8 380 827 507 byte** er uendret etter pakking. Åtte payloadfiler er hashbundet. Spillbinæren er 314 438 016 byte, SHA-256 `4cc8ecfd63c6754a50783d3678f0dd1ee3a7bccc71e8ebcd5b33dbce670304ff`.

| Pakket kontroll | Faktisk resultat |
| --- | --- |
| Fysisk dokumentreise | 1720 selvtester, 580 kontroller, 4,545209 m API-gange, seks forskjellige papirtilnærminger og faktisk kameraray/Focus/ordinær Player OnUse. Riktig registrert dokument og faktisk Slate-tittel/brødtekst for alle seks. |
| Lagring | Fem isolerte Writer/fersk leser/full Flow-forløp: ulest meny, ett lest dokument, eldre lagring uten leseliste, reelt delvis gjenopprettet og alle dokumenter lest. Kilde/disk/gjenopprettet office-tilstand og feltarkets faktiske tidspunkt består. Elleve art-auditer rundt forløpene består. |
| Dokumentleser | 47 faktiske fokuserte PageDown/PageUp/Escape-tastetrykk, eksisterende privatinnhold og serviceskjema ved stor tekstskala. 24 komplette UI-PNG-er er CRC/IEND/SHA- og bildeverifisert. |
| Materialer og bilder | Tolv komplette originale dag/skumring/nattbilder, fire kamerafixtures per lysfase, 72 faktiske materialauditer. Alle tolv bilder er åpnet. |
| Avslutning | Begge pakkede drivere avslutter normalt med exit0 etter lagret sluttresultat/siste komplette PNG. Ingen tungjobbkonkurranse eller guard-stopp. |

Den fysiske reisen bruker et uttrykkelig kapittel-/korridorutgangspunkt med nødvendige tidligere oppgaveflagg. API-styrt Walker, kameraray, ordinær interaksjon og Slate-input er faktisk kjørt; dette er ikke menneskelig input. Bildeserien bruker egne statiske kamerafixtures og er kjørt separat. `S47ProductionExit` er opt-in, rydder egen timer og midlertidig gulv/FOV/cinematic-tilstand, og gir ikke PASS før siste komplette bilde. Dokumenttestens sluttresultat må også være skrevet før PASS/exit0.

## Bildegranskning og grenser

[Originale eksempelbilder og oversikt](../../../unreal/Evidence/DOCUMENTPROPS-2026-10-29/README.md) viser seks forskjellige ark med riktig orientering og sideforhold uten åpenbar sveving eller z-kamp. Overskrifter og skjemafelt er lesbare i nærbildene. Liten brødtekst er ikke jevnt lesbar fra bordavstand i 1280×720; den faktiske dokumentmodalen prøves separat. Eksisterende bordlampe dekker øvre del av indeksarket. Tre originale pakkede UI-bilder er åpnet i tillegg til alle tolv miljøbildene.

Editorbildene har en eksisterende RT-always-resident-advarsel ved 83,94/400 MiB. Det er ingen FPS-, OOM- eller samlet GPU-minneaksept. Menneskelig input, subjektiv lytting, VHS, fotografier, ARRAY-01-møte, øvrige plakater/bilder og fullmatrise er **UNVERIFIED av denne leveransen**. Ingen installasjon eller offentlig release er utført.

Etter lukket motorvindu ble standard prune-plan kontrollert og kjørt. Bare sju gamle Motel R1-payloadfiler / 4 475 466 770 logiske byte ble pensjonert. Farm R1, Dispatch R1, ny ART R1, begge eksplisitte FIX-01-sammenligninger og installasjonsrutinas tilbakekopi er bevart. Alle øvrige registrerte payloadhashes og alle 91 ferskt målte filer i installasjonens egen Saved-mappe er uendret. Kilder, råbevis, bilder og pakkeskall er bevart. De to egne genererte OpenOrder-loggene er sikret og gjenopprettet; ingen andre filer er tilbakestilt.
