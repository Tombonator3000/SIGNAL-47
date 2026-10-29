# Lokal installasjon av rammen SC01 til SC13

**Status:** Installert lokalt. **Gjelder:** `~/Games/Signal47/Linux`, UE 5.8.4 / Linux Vulkan. **Sist kontrollert:** 29. oktober 2026.

Tom ba om at rammen ble installert. Kandidaten er pakket fra main `3dcba420b2e6810655133489a6a1af6b8039b966`, det vil si PR228 til PR232. Den inneholder servicebilen og dørrettingen (PR229), arkivpermen 20:30 (PR230), rammen `FS47Spine` med plassholdere for SC04, SC08 og SC11 (PR231), Codex' arkivkompatibilitet (PR232) og kinoplakatene (PR228).

| Steg | Faktisk resultat |
| --- | --- |
| Pakking | `Scripts/package-linux.sh`, BUILD SUCCESSFUL, rc 0. Binær SHA256 `5181336c775e218ddfacb6c9046ae55dda83e7cb6bfa21386253c64a076817c9`. |
| Fullmatrise | `screenshots.sh` offscreen mot kandidaten: 111 av 111 tilfeller OK, rc 0, 18:41 til 19:36. Tittel, kontrollrom, finalen, notatbok, konsoll, array, arkiv og selvtest er gjennomsett av Claude. |
| Installasjon | Gjennom tungjobbreservasjon og `Tools/cows/install.py`: reflink-kopi av forrige installasjon, rsync uten Saved, 68 payload-noder lik kandidaten, 134 Saved-noder uendret. PASS_INSTALLED 19:41. |
| Installert røyktest | Installert `Signal47.sh` med `-S47Case=0 -S47EveningTimeline -S47Jump=chapter2 -S47SpineContract=sc08 -S47SelfTest`: selvtestene består, og rammekontrakten SC08 består med 11 kontroller, blant annet faktisk stråle mot plassholderen, lagring med SC08 ventende, klokkehopp til 04:20 og kapittel tre som SC09 04:25. |
| Opprydding | `prune-test-builds.py` fjernet fire eldre testpakker. Gjeldende kandidat, forrige installasjon og to nyeste eldre pakker er beholdt. |

Forrige installasjon ligger som tilbakekopi i arbeidsmappa for installasjonen. Offentlig nedlasting er ikke endret og er fortsatt Farm R1.

## Grenser

1. SC04, SC08 og SC11 er plassholdere: gul stolpe, kort og klokkehopp. De er tydelig merket i spillet.
2. SC04 er ikke prøvd etter en ekte SC03-slutt, bare med testkroken. Lagringskontrollen i rammekontrakten er serialisering, ikke ny lasting fra disk.
3. Earl er på vakt til 04:00, men SC08 legger ham til 04:00 til 04:20. Rettes i akt 2.
4. Menneskelig spilling fra 18:00 til finalen, FPS og lytting er UNVERIFIED. Det er dette Tom nå kan prøve.
