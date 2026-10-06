# Lys ved dineren: samlet vurdering

Status: REVIEWED_WITH_LIMITS. Seks av seks oppsett er ferdige: Low, High og Ultra i 844x390 og 1280x800, DPR 1. [Galleriet](evidence_20261006_v4/index.html) inneholder 156 bytebevarte originalbilder og 20 avledede kontaktark. Alle kontaktark er sett, med utvalgte bilder også i full størrelse. Dette er faktiske spillbilder fra det kontrollerte Pages-bygget etter PR #81.

Kapittel 5 er kjørt fra SARO til naturlig parkering. De siste 300 m er avbildet ved faktiske 40 m-kryssinger; første observerte pose er 291 m fra målet. Kapittel 6 er kjørt 309 m fra plassen ved daggry. Ingen ruteplassering eller klokkeoppdrag brukes under kjøringen. Holdte cab-/fotdiagnoser er merket og er ikke manuell spilling.

## Samlet funnliste

### DL01 / P3: Skiltet ses nesten fra kanten og er delvis skjult fra førerhuset

De holdte skiltbildene før og etter parkering får stol/vindu i forgrunnen og ser skiltplanet nesten fra kanten. De viser neonmiljøet, men støtter ikke en full vurdering av skiltets lesbarhet fra bilen. Ultra-fotbilder og faktisk Ch6-start gir supplerende utsyn. Ingen runtime-feil er påvist av dette funnet.

Alle tolv berørte cab-bilder er lenket med full pose/klokke i [review.json](review.json). Før parkering:

| Nivå / størrelse | Faktisk klokke | Kamera XYZ | Blikkretning | Bilde |
| --- | --- | --- | --- | --- |
| low_844x390 | 05:09:38.53 | (7993.155268, 5.788810, 2003.701751) | (-0.014913, 0.039247, -0.999118) | [Original](evidence_20261006_v4/captures/01-dinerlight_low_844x390_20261006_v4/dinerlight_arrival_011_prepark_sign_lights_on.png) |
| high_844x390 | 05:09:38.27 | (7993.155333, 5.788891, 2003.701655) | (-0.014918, 0.039242, -0.999118) | [Original](evidence_20261006_v4/captures/02-dinerlight_high_844x390_20261006_v4/dinerlight_arrival_011_prepark_sign_lights_on.png) |
| ultra_844x390 | 05:09:38.00 | (7993.155268, 5.788810, 2003.701751) | (-0.014913, 0.039247, -0.999118) | [Original](evidence_20261006_v4/captures/03-dinerlight_ultra_844x390_20261006_v4/dinerlight_arrival_011_prepark_sign_lights_on.png) |
| low_1280x800 | 05:09:38.27 | (7993.155333, 5.788891, 2003.701655) | (-0.014918, 0.039242, -0.999118) | [Original](evidence_20261006_v4/captures/04-dinerlight_low_1280x800_20261006_v4/dinerlight_arrival_011_prepark_sign_lights_on.png) |
| high_1280x800 | 05:09:38.00 | (7993.155268, 5.788810, 2003.701751) | (-0.014913, 0.039247, -0.999118) | [Original](evidence_20261006_v4/captures/05-dinerlight_high_1280x800_20261006_v4/dinerlight_arrival_011_prepark_sign_lights_on.png) |
| ultra_1280x800 | 05:09:39.33 | (7993.155268, 5.788810, 2003.701751) | (-0.014913, 0.039247, -0.999118) | [Original](evidence_20261006_v4/captures/06-dinerlight_ultra_1280x800_20261006_v4/dinerlight_arrival_011_prepark_sign_lights_on.png) |

### DL02 / P3: Ekstra SARO-startbilde i Ultra 1280x800 er svært mørkt

Det ekstra startbildet før kjøreturen er svært mørkt, også HUD. Det ligger utenfor den bestilte siste 300 m ved dineren. Raw post.glitch=0, cinematic=false og HUD/himmelklokke stemmer. UI-fade-opacity ble ikke registrert; årsak og eventuell runtime-feil er UNVERIFIED. Bildet er beholdt og er ikke brukt til lyskonklusjonen ved dineren. Ingen runtime-feil er påvist av dette funnet.

[Bevart original](evidence_20261006_v4/captures/06-dinerlight_ultra_1280x800_20261006_v4/dinerlight_arrival_000_route_start.png): ultra_1280x800, 05:00:01.90, kamera (9.320000, 1.480287, 7.800000), blikk (-0.000000, -0.079915, 0.996802).

## Observerte lys og Ultra-rådata

Panseret har rosa/rødt skjær nær dinerplassen i alle seks oppsett. Det varme frontlysfeltet før parkering er borte etter den naturlige frontlys-av-overgangen; faste vindus-/fasadelys er fortsatt synlige. Den enkelte lyskildens bidrag er ikke isolert. Ch6-ruten viser daggry og HUD 05:20, inkludert startbildet. De eksakte bilde-/manifestlenkene med pose, klokke, preset og SHA-256 står i [review.json](review.json).

[Uavhengig råanalyse](ultra_raw_analysis.json) dekker 60 Ultra-PNG-stater og 64 fotstater, 372 SpotLight-observasjoner og 159 aktive. Den finner null kandidater for samtidig positiv falsk lampe og matchende spot, skipped-only kilde, spot over 45 m fra observert spiller eller feil dinerkilde til fots. 27 registrerte neonalias har negativ styrke både i diner og vei og er forventet OFF-overføring. Dette er ikke en automatisk vurdering av lysrendering.

Ankomstens fotpool får tre aktive dinerkilder ved steg 5 (844) eller 6 (1280); avgangens fotdiagnose ved steg 6. Rå steg 0 er beholdt, også avgangsdiagnosens gamle kameratilstand før ett vanlig kamerasteg. Under kjøring velger Ultra fra `player.pos`, som kan være langt fra førerkameraet. Ingen intern/private want-liste er direkte registrert.

## Kontrollbevis og avgrensning

- [Pakkekontroll](evidence_20261006_v4/capture_checks.json): 2596/2596 PASS. Originalhasher/-mål, komplett responsliste, observerte kodebytes mot Pages-artefakt, ruteintervaller, parkering, kameragjenoppretting og HUD/himmelguard er kontrollert. Råmanifestenes visuelle status er bevart som UNVERIFIED.
- [Opptaksverktøy](setup_sync_checks.json): 32/32 målrettede mock-/kontrakttester PASS. [Pakker](package_setup_sync_checks.json): 20/20 PASS, ingen hoppet over. Dette er egne støttetester, ikke spillintegrasjon.
- [Pages](pages_verification.json): jobb 37465075691 fra main a3727e9, ZIP-digest kontrollert, fem livefiler HTTP 200 og byteidentiske. Alle observerte dokument-/skriptbytes i de tolv opptaksfasene er deretter bundet til samme artefakt. [Lokal runtime](source_identity.json) er byteidentisk med main i 17 registrerte kildefiler.
- [Claudes ene integrasjonsrunde](integration_evidence.json): Ultra 8/8, diner 15/15, Ch5 53/53, Ch6 42/42, drives 23/23, ingen registrerte konsollfeil. Testene på f50b6c1 er gjenbrukt, ikke gjentatt.
- [Fullført sekvensiell kø](capture_queue.json) og [prosesslogg](capture_queue.log) dokumenterer seks avslutninger med exit 0 mellom 13:29 og 14:02 UTC.

To feil i eget opptaksverktøy ble rettet før sluttopptaket: gammelt kamera etter getOut-callback og gammel HUD/himmel etter asynkron Ch6-klokkeoppstart. Råprober og avbrutte forsøk er bevart separat i [diagnostics](diagnostics/attempts_inventory.json); de inngår ikke i lysvurderingen. Ingen game-runtime eller gamle spilltester er endret.

ANGLE SwiftShader er brukt. Ekte GPU/fps, fysisk telefon, lyd og manuell kjørefølelse er UNVERIFIED. Stillbilder verifiserer ikke flimring over tid eller skjulte flater. Presets har faktisk klokkevariasjon og er ikke en identisk-klokke A/B-test. [Manuell dekning](manual_review.json) binder hvert sett kontaktark til filhashene som faktisk ble sett. Ingen nye assets er produsert.
