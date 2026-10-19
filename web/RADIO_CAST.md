# Innringere og nyhetssendingen fra 1947

Direkte bestilt av Tom 19. oktober 2026 etter kildesøket. Arbeidsgren `codex/radio-cast` fra kontrollert main `b6483fe`. Prosjektbibliotekets VoiceStudio-kilde og den eksisterende lokale VoiceStudio 0.5.6/OmniVoice-installasjonen gjenbrukes.

## Spilleropplevelsen

På AM 1420 får innringerne egne syntetiske stemmer, og nyhetssendingen fra 1947 får en egen nyhetsleser. Nyeste main har seks bulletinlinjer. Sammen med åtte kvinnereplikker, åtte mannsreplikker, tre replikker fra en eldre mann og tre fra gutten blir det 28 opptak. Eksisterende engelsk manus brukes ordrett.

Valget **Radio callers & bulletin** er på som standard, lagres i `s47.radioCallerVoices` og er uavhengig av Art Bell-verten og Kinski-opplesningen. Selve bilradioen starter fortsatt av. Roswells morgenvert og CB beholder dagens lyd. Earl i Caliche bruker samme Art Bell-valg som radioverten; se [Art Bell](ART_BELL_RADIO.md). De andre samtalene i byen er uendret.

## Kontrollpunkter

1. Innringerne og de seks nyhetslinjene spiller riktige, dekodede MP3-filer gjennom samme AM-filter, mottakerplassering og radiovolum som før. Tekstingen følger faktisk lydvarighet.
2. Nyhetsinnslaget varer nøyaktig 47 sekunder på den pausefølsomme lydklokken. Opptakene spiller med vanlig hastighet, og pauser fyller gjenværende tid. Deretter fortsetter den avbrutte setningen eller platen fra samme punkt.
3. Hendelsesnotatet og merket gis først etter hele den hørte sendingen. Langsom innlasting, pause, retuning og avbrudd kan ikke gi en falsk fullført observasjon.
4. Valgene for innringere, Art Bell og Kinski lagres separat. To mottakere deler én sending, og reset, lasting, Off og sen dekoding gir ingen gammel avspilling. Feil eller ukjent tekst beholder hele den opprinnelige pratelydlinjen.
5. Begge bygg fungerer, også Pages fra undermappe og enkeltfil under streng CSP. Tidligere stemmefiler og relevante spillreiser bevares og kontrolleres før innfletting.

## Kilder og status

Voksenstemmene bruker Selfie, Butter og oldNerd fra [Kyutais CC0-donasjoner](https://huggingface.co/kyutai/tts-voices/tree/main/voice-donations), med uttrykkelige donasjonsvilkår for talesyntese og stemmereplikering. Gutten og nyhetsleseren bruker fiktivt stemmedesign i eksisterende OmniVoice. Navn og tonehøydemålinger bekrefter ikke donorenes alder eller kjønn. Faktisk casting, kildefiler, festet revisjon, hash, profil-ID og generering står i [produksjonsoversikten](production/radiocast/README.md).

[ABC-nyhetene fra 8. juli 1947](https://archive.org/details/1947RadioNews/1947-07-08-ABC-News-Roswell.mp3) og [Universal-innslaget «Mexico Greets Truman» med Ed Herlihy](https://www.trumanlibrary.gov/movingimage-records/mp72-14) er tidsreferanser for en ny fremføring av spillets oppdiktede manus. Ingen ABC-arkivlyd følger spillet. Originalreferanser og modellvekter holdes utenfor spillpakken.

Banken er ferdig generert: 28 mono MP3 på 24 kHz og 64 kbit/s, 1 024 976 byte og 125,61 sekunder. De seks nyhetsklippene summerer til 25,09 sekunder ved vanlig hastighet. Automatisk dekoding, filhash, nivå og droppkontroll består. Alle 28 har gjenkjennelig automatisk transkripsjon, 21 ordrette og sju med dokumenterte avvik. Guttestemmen ble laget på nytt etter at første design ikke ga gjenkjennelige ord; alle tre endelige gutteklipp er ordrette i kontrollen.

Sluttkandidaten består 70/70 radiotester i Pages og 71/71 i enkeltfilen under streng CSP. Motoren består 77/77, eksisterende Art Bell-motor 48/48 og arkivflyten 51/51. Alle 16 kompatibilitetskjøringer, 13 spillregresjoner, lydkostnad og bygg med og uten privat lydbank består. [QA-oversikten](production/radiocast/qa/README.md) og [sluttattesten](production/radiocast/qa/validation.json) knytter prøvene til faktiske kilde- og bygghasher. Alle sju tidligere lokale profiler og 1670 tidligere MP3 med de to katalogene er bevart. Menneskelig lytting, rollefordelingens klang, uttale, fysisk PC-/mobillyd og ytelse er UNVERIFIED.
