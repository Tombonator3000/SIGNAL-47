# T02: Operations-kontor og kontrollert feltoppfølging

25. oktober 2026. Kilde `5b9fcb3c31c56cb125d10e08756e548067413c44`, levert gjennom [PR162](https://github.com/Tombonator3000/SIGNAL-47/pull/162), flettet i `d152fb6d6cb7b034d51604a18bf21c4eb94c0017`. Pakket binær SHA256: `7531698e9695c595ddba2868ab37e2f839c16a82d7e47b77e42f8c2cfdbaa86b`. Fokusert pakket aksept og standardinstallasjon består. Ingen CI-jobber er rapportert; dette er ikke CI-PASS eller full spillaksept.

## Spillerreisen

Arkivreisen gir tilgang til egen serviceskuff, Operations-reservenøkkel og faktisk kontor med dørblad, kollider, gangterskel, møbler og private arbeidsark. Feltkopien kan tas uten VHS. Nye versjonerte kveldssaker venter etter P04/P05 og Nora på feltkopien og uttrykkelig avreise fra kontorkortet. Å ta mappen starter ingen overgang. Gamle saker får ingen ny sperre; arkivskuffen og Noras nøkkel er separate.

Valgfri T02 i kapitteltre/fire krever fysisk normalreferanse, åtte gjenkjennelige referansepunkter og fire faktiske kontroller: isolasjonens uavhengige respons, hjelpetestens puls og mørke slutt, panelsammenligning og full lampesekvens. Bekreftelse trenger eget ekte instant-back-foto uten albumkvote eller STATION 01-film. Uavklart gir ingen bekreftet avvikscredit. Én fysisk filing i SAROs skiftmappe beholder originalt kort og historikk uten doble noter. K3, byforsyningen, STATION 01, Nora og C beholder sine egne roller.

## Faktisk aksept

Bevisrot: `/home/tom/scratch-bm/Signal47-t02-20261025`. `t02-acceptance-r1.json` binder 23 rapporter og 25 faktisk gjennomgåtte originalbilder; `T02_VISUAL_REVIEW_R1.json` avgrenser bildeaksepten.

| Kontroll | Resultat og omfang |
|---|---|
| Pakkede kjøringer | Alle 15 PASS: ti suitekjøringer og fem vanlige ferske Continue-prosesser. |
| Selvtest | 1 437 kontroller PASS. |
| T02 High/Low | Hver består 65 API-kontroller og 15 writer/fresh-reader/full-Flow-restores. |
| Uavklart og T01 | Henholdsvis 30/6 og 53/19 kontroller/full-Flow-restores PASS. |
| Sammenhengende native-reise | 41 kontroller, fire disklesere, 932,2 m ut, 966,7 m retur og 124,1 m gange PASS. Ingen testflytting eller Flow-last etter START. |

Kapittelstart, arkivforutsetninger, P04/P05/Nora, første korridorplassering og navngitt turmål er fixtures. API-prøvene har også felt-/truck-/returplassering. Native bruker eksisterende turkontroll, overføring, parkering, utstigning og Walker. Albumkvotefixture bruker kopireferanser til ekte fotograferte piksler, ikke 36 eksponeringer.

Full-Flow-restores er i samme prosess. De fem separate Continue-prosessene bruker vanlig tittel/Continue uten kontraktflagg; seed-lagringene stammer fra de beskrevne fixtures. Beholdte bytes, stabil scene/minutt og oppstart er kontrollert, men ingen fersk minnedump eller ny serialisering foreligger. Manglende-foto-seed og ingen krasj beviser derfor ikke alene at positiv credit ble fjernet fra ferskt minne; negative full-Flow-prøver er eget bevis.

Bildene støtter ståsted og original eksponering. Mørke ledere og lampestatus kan ikke avleses selvstendig fra fotografiet. Full kontor-/døroversikt, hele previewtittelen, nedre papir/transkript, scrolling og separat fotoside er ikke pikselakseptert. Kontroller og kortbevaring støttes av runtimebevis, ikke filnavn på verdensbilder.

RT-prøven viser 78,8071 MiB alltid resident geometri i 629 aktive bilder av 900, under uendret 80 MiB-grense. Dette er motormålt RT-geometriminne, ikke total VRAM, FPS eller generell stabilitet.

## Faktiske rettinger

Tidligere FAIL er bevart: sikting etter initial Place brukte gammelt øye; lave stolpebånd var skjult av fareskilt/skur; JSON-snapshots delte mutable data. Egen båndpost utenfor gjerdet gir full LOS uten fjernet geometri. Terrengmålingen samsvarer med faktisk FloorY; posten er jordfestet. Modal-gjenåpning, Caliche-tittel, lokalt HUD-mål, uforanderlig Notebook-kort, duplikatfiling og typestreng anskaffelsestid er rettet og regressjonsprøvd.

## Levering og åpne grenser

`post-install-final-r1.json` bekrefter installasjon i `/home/tom/Games/Signal47/Linux` og faktisk standard-launcher-smoke: 40 payloadfiler stemmer, 40 personlige Saved-filer, 57 noder, er nøyaktig uendret. Full rollbackbackup er etterkontrollert.

Canonical prune pensjonerte nøyaktig to gamle payloads, 7 621 635 760 logiske bytes. Tre komplette pakker beholdes: T02, full T01 R2-rollbackbackup og T01 R2-kandidat. Kilder, Saved, bevis og Engine er bevart; logisk størrelse garanterer ikke fysisk frigjort plass. Tørrplanens reviewhelper brukte feil actionnavn; canonical apply fulgte de to riktige stiene. Korrigert etterkontroll er bevart.

Menneskelig Roam, mus/Xbox, lytting, fullnatt og pacing står åpne. Original 72-sekunders VHS og pause-/strøm-/save-transport er fortsatt uprodusert og valgfrie; ingen ny film eller vhs_seen er laget. H2 samlet og stealth er ikke ferdigmeldt. Biblioteksøk fant ingen anvendelig løsning; egne eksisterende modeller og API-er ble brukt uten ny plugin.
