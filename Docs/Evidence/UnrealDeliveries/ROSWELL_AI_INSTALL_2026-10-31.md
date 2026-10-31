# Roswell: nysgjerrig AI, fotlyder og lokal installasjon

**Status:** Referanse. **Gjelder:** Linux/Vulkan, runtimekilde `de8962fa79bd874b5ceeda02860548820d2aa5a4`, PR291, merge `ad6b6fc5`. **Sist kontrollert:** 31. oktober 2026, installert kl.18:22 CET.

Små Roswell-aliens har nå individuell oppfatning og minne om faktisk sett eller hørt spiller, lytting, lusking, titting, skjuling og flukt. F10-menyen velger 1, 2, 3 eller 6 individer, åtte aktive sekunders stilleståing, nysgjerrig atferd, eksplisitt testjakt, bytte av atferd på stedet, takvandring og samlet nullstilling. Jakt er en utviklerprøve uten historieskade. Vanlige bakgrunnsfigurer beholder den sky rollen; den store Pale-trusselen er separat.

Hver figur har egen pose på den bevarte Tripo-riggen, avstandsstyrt gait, fotankre/IK og kontaktstyrt romlig Foley. Stående og pauset kropp lager ikke nye fottrinn. Vegger/tak demper lyden. Bevegelse trenger komplett rute og ferskt kollisjonssveip; planlegging er avgrenset både per rute og på tvers av gruppen.

Annex-taket var fortsatt synlig, men hadde mistet query-fysikken da den transiente modellen med bakvindu ble opprettet. En presis query-boks for den faktiske takplaten opprettes nå bare etter kontroll av dagens synlige render-triangler i 25 punkter. Den utskårne bakveggen gjenopprettes ikke. Historiske interaksjonsbokser regnes ikke som gulv, og ingen figur går direkte fra bakken til taket.

## Utført kontroll

- UE5.8.4-editorbygg og **1804 selvtester** består. Nyeste SARO Story-løkke fra main er med.
- Editor og ferdig Linux-pakke består én målrettet Vulkan-prøve på High, 1280x720: seks native kropper, ståtid, pause, nysgjerrig forflytning med reelle fotkontakter og opprettede lydkilder, eksplisitt jakt og full nullstilling. Seks separate takfigurer beveger seg på Y3.2 med mellom 3,30 og 6,51 meter faktisk forflytning og 13 til 27 fotkontakter hver i takdelen.
- Den ferdige pakken er installert i standardmappen `~/Games/Signal47/Linux`. Installert startfil består en kort prøve på 22 aktive sekunder med seks figurer, ståtid, pause og lusking/lydkontakt. Tak/jakt er kontrollert i pakken; installert røyktest gjentar ikke hele prøven.
- Alle 68 payload-noder matcher kandidaten. Alle 152 personlige Saved-noder, inkludert 124 filer, er uendret. Testene bruker ny Case0 under egne `/tmp`-profiler, ikke personlig Fortsett.
- Prune-verktøyet har pensjonert én eldre Roswell-payload, sju genererte filer. Kilder, modellen, originalbilder, Saved og logger er bevart. Gjeldende kandidat og to eldre komplette pakker beholdes. Et midlertidig ekstra vern gjelder den Saved-frie rollback-kopien `Signal47-roswell-ai-20261031/previous-installed`, nødvendig for å kunne reversere denne installasjonen; den inneholder forrige SARO-payload. Ingen nye motorjobber kjører fra denne leveransen, og tungreservasjonen er frigitt.

[Maskinlesbar kvittering](RoswellAI-2026-10-31/receipt.json), [før](RoswellAI-2026-10-31/before.png), [pakket lusking](RoswellAI-2026-10-31/curious.png), [pakket jaktfase](RoswellAI-2026-10-31/hunt.png), [installert lusking](RoswellAI-2026-10-31/installed-curious.png).

Dette er skriptstyrte prøver av den ordinære gruppen og menyhandlingene. En 16-sekunders separat lydprøve av Foley-banken er generert lokalt, men menneskelig lytting og opplevd animasjonskvalitet er ikke godkjent av disse kontrollene. Naturlig møtefrekvens gjennom en hel natt, flere by-/motellplasseringer og Toms vurdering av tempo/lydnivå videreføres i PLAY-31-37. Ingen offentlig spillrelease er publisert. Lokale kjørelogger, kandidat, lydprøve og rollback ligger under `~/scratch-bm/Signal47-roswell-ai-20261031/`.
