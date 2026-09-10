# SIGNAL / 47 — passiv lagring, fortsettelse og lydutgang

Skrivebeskyttet etterkontroll av de tre registrerte sluttkjøringene. Ingen ny kjøring, input, editoroperasjon eller endring i testprofilen er utført i denne gjennomgangen. Rapporten er avgrenset til lagret tilstand, originale foto, prosessforløp og målt lydutgang.

Bygget er commit `9329a2d819108796c6f731cc305f16dc3250c651`; Unity-kilde-SHA256 `86a28b30ad94fc9e67aa0bf0558f0a51f511adadc76323fcf69eca7c72d8c8b9`.
Spillerfil-SHA256: `8027f7d1f9ae7dacfc826fb218adcc1ae7464af098a8a59397e7531c0f7ec0bc`. Byggpayload-SHA256: `6b0c99dc455267875ac836594e6f1478fb58515d337313032604325c2de6056b`.
Alle tre byggmanifest, begge foto-sidecarer og alle tre lydrapporter oppgir samme identitet. `build: 1.0` i lagringsformatet er kun programversjon. Manifestets rene arbeidsmappe gjelder byggetidspunktet.

## Resultat

| Kontroll | Status | Grunnlag |
| --- | --- | --- |
| Lagrede konvolutter | PASS | Alle tre SHA256-kontrollsummer stemmer; format og versjon er 1. |
| Fotokontinuitet | PASS | Samme ID, JPG-SHA, kamera, viewport-markører, tid og metode gjennom kontrollpunktene. |
| Avslutning og fortsettelse | PASS | Tre ulike prosesser avsluttet normalt; to nye prosesser lastet primærfilen. |
| Gjenopprettet sak | PASS | Foto-/kapitteltilstand er identisk ved fortsettelse; verden, instrumenter og notatboktelling er bevart. |
| Lagret slutt | PASS | Begge foto framkalt og undersøkt, sammenligning bekreftet og sak fullført i siste `case.json`. |
| Stikkprøver av lydutgang | PASS | DSP gikk; lydutgang er målt gjennom relevante faser uten fullskalaoverskridelser i utvalget. |
| Lastetid fra Fortsett til kontroll | UNVERIFIED | Disse kjøringene registrerte ikke et tidspar som måler dette. |
| Faktisk lytting og subjektiv miks | UNVERIFIED | Det er ikke lyttet. Målt motorutgang dokumenterer ikke høyttaler-/hodetelefonlyd. |

## Fotografiene er de samme opptakene

| Opptak | Kontrollpunkter | Faktisk JPG-SHA256 |
| --- | --- | --- |
| `s03-field-photograph` | before-development.fixture.json → after-control.fixture.json → case.json | `a38f45c9096d477d4d65730dfd33a599ba0c1d96d15394ef18354604e205a307` |
| `b12-control-photograph` | after-control.fixture.json → case.json | `c37c1cbdd021fde3fc54a6392c56b12b73d5019807094794f23e9a049e9d6d7c` |

Begge originale JPG-er kunne dekodes som 960 × 600 RGB og ble åpnet uendret. Eksakt sammenligning av de tolkede metadatafeltene bestod for ID, absolutt filsti, UTC/lokaltid, bygg, metode, motiv, bildehash, størrelse, kameraposisjon/-retning og begge viewport-markører. Unity-flyttallenes 32-bitverdier stemmer også. Dette er sammenligning av opptaksdata; hele JSON-filene skal ikke være byteidentiske når framkallingsstatus og formatering endres.

Den opprinnelige sidecaren blir stående som eksponert, uframkalt og eksportert. S-03 skifter til framkalt/undersøkt i det andre kontrollpunktet. B-12 finnes først etter feltprøven, og skifter til framkalt/undersøkt i sluttlagringen. Fotobevis legges først i notatboken etter framkalling. Notatboken har henholdsvis 4, 7 og 9 unike bevis-ID-er; ingen dubletter.

Originalfiler og sidecarer:

- [B12-20260910-173958-442230a6.jpg](../../../Artifacts/Chapter09/Profiles/FinalPassive/FieldPhotos/B12-20260910-173958-442230a6.jpg); [B12-20260910-173958-442230a6.json](../../../Artifacts/Chapter09/Profiles/FinalPassive/FieldPhotos/B12-20260910-173958-442230a6.json), sidecar-SHA256 `5133ee8b11ccb54d1735c9b5158fe429103ea5976af66acd818c0689285a9168`.
- [S03-20260910-173623-434440b7.jpg](../../../Artifacts/Chapter09/Profiles/FinalPassive/FieldPhotos/S03-20260910-173623-434440b7.jpg); [S03-20260910-173623-434440b7.json](../../../Artifacts/Chapter09/Profiles/FinalPassive/FieldPhotos/S03-20260910-173623-434440b7.json), sidecar-SHA256 `609342ea351e1c768b691fb31310382ae3d64de532fa0d86d95f7ecd59f925dd`.

## Den faktiske prosesskjeden

| Fase | PID | Start og utfall | Lagret tidspunkt (UTC) |
| --- | --- | --- |
| Final-Passive-Opening | 360464 | Ny start uten aktiv sak; S-03 eksponert, ikke framkalt. | 2026-09-10T17:36:26.4010960Z |
| Final-Passive-Control | 361592 | Fortsett før framkalling; S-03 behandlet, passiv kontroll gjennomført, B-12 eksponert. | 2026-09-10T17:40:01.1336650Z |
| Final-Passive-Finish | 362411 | Fortsett etter kontrollforsøket; B-12 behandlet, sammenligning og kapittelslutt fullført. | 2026-09-10T17:42:13.9864780Z |

Alle tre `process.json` angir `normalMenuQuit=true`, `processExit=0` og `journeyExit=0`; hver spillerlogg har `CHAPTER_QUIT_DRAIN_OK`. Første fasens siste primær-/backuphash er identisk med neste fases starthasher, og dette gjelder også mellom kontroll og slutt. Primærfilene svarer nøyaktig til de to fixture-filene og endelig `case.json`.

Begge nye prosesser har `CHAPTER_CONTINUE_OK backup=False` med forventet lagret tidspunkt. Første observerte spilltilstand er henholdsvis `develop-first` (1 foto, 0 framkalt) og `develop-second` (2 foto, 1 framkalt). Kamera- og kapitteldata er semantisk identiske med lagringen; instrumentverdier, dør, servicegård og avsluttet prolog stemmer. Spilleren har samme posisjon. Største registrerte vinkelavvik er 0,0000153° i pitch, under kontrollens 0,001° toleranse.

Sluttlagringen inneholder den passive kontrollen, to avviste hypoteser og to avviste konklusjoner før den støttede sammenligningen. Dette underbygger at feilvalgene var gjenopprettelige i den registrerte reisen. De 54 kontrollpunktene kommer fra tre eksisterende kjøringer med faktisk tastatur/mus, ikke en ny uavhengig spilltest.

Lastetid er ikke etterkonstruert. De passive loggene har bare en fullføringsobservasjon og tid siden prosessoppstart, uten tidsstempel umiddelbart før Fortsett-klikket. Assembly-lastetidene 0,050 / 0,047 / 0,051 sekunder og Unitys `UnloadTime` er interne deloperasjoner. De er ikke spillerens ventetid. Senere instrumentering i testskriptet gjelder ikke tilbakevirkende.

## Målt lydutgang — dette er ikke lytting

Unity `AudioListener.GetOutputData(..., 0)` samler 2048 prøver omtrent hver 40 ms. Alle fasene oppgir 48 000 Hz og at DSP-klokken gikk. Vinduene kan overlappe eller ha hull; prøvetallene er ikke sammenhengende varighet.

| Fase | Vinduer | Sampleverdier | Høyeste peak | Peak dBFS | Fase ved peak | Verdier med abs ≥ 1 |
| --- | ---: | ---: | ---: | ---: | --- | ---: |
| Final-Passive-Opening | 4071 | 8337408 | 0.222733 | -13.04 | `call` | 0 |
| Final-Passive-Control | 1846 | 3780608 | 0.157972 | -16.03 | `chapter-develop-second` | 0 |
| Final-Passive-Finish | 1774 | 3633152 | 0.132512 | -17.55 | `chapter-comparison` | 0 |

Åpningen har registrerte lydvinduer for mottaker, skriver, ringing/samtale, død linje, støt, kamera og første uframkalte bilde. Kontrollrunden har egne grupper for lab, første print, arkiv, feltkontroll, observasjon og ny eksponering. Sluttrunden har lab, sammenligning, rapport, avslutning og fullført sak. Alle målte nivåer per gruppe, inklusive RMS, ligger i JSON-rapporten.

Fravær av sampleverdier over full skala gjelder bare dette utvalget på kanal 0. Det beviser ikke kontinuerlig fravær av klipping, andre kanaler, fysiske høyttalere eller en god miks. Pausegruppene har lave, men ikke null nivåer (RMS 0,000804 / 0,000448 / 0,001296); grupperingen omfatter overgangsvinduer, og årsaken til restnivået er ikke fastslått her. Eksakt stillhet under pause er dermed ikke bevist.

## Sporbarhet og avgrensning

Alle leste inngangsfiler er SHA256-listet i JSON-rapporten. Testprofilens filsett og hasher var uendret før og etter denne kontrollen. Det er ikke kopiert, oppdatert eller slettet foto eller lagringer.

Automatiseringen bruker observasjonsdata til å finne markørene i bildene. Den dokumenterer betjening og tilstand; den erstatter ikke blind oppdagelsestesting eller dokumentert førstegangsvarighet på 20–30 minutter. Samlet visuell godkjenning, aktiv valggren, feilprofiler, ytelse og endelig pakkestart ligger utenfor denne rapporten.

[Fullstendig maskinlesbar rapport](passive-persistence-audio-review.json)
