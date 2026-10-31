# Spilltestrettelser31. oktober 2026

**Status:** Gjeldende. **Gjelder:** Lokal Unreal/Linux-installasjon etter Toms avsluttede spilltest. **Sist kontrollert:**31. oktober 2026.

Kilde [PR272](https://github.com/Tombonator3000/SIGNAL-47/pull/272), head `f32d4553`, flettet i main `c2f0b8b0`. Linux-pakken er installert31. oktober kl. 10:39. Ingen offentlig spillutgave er publisert.

## Levert

- Gamle saker som faktisk har fullført SC03, men mangler Spine, fortsetter på SC04. Opprinnelige vitnefakta og rapporttider beholdes. Ukjente eksisterende Spine-versjoner overskrives ikke. Servicebilen tilbyr retur til SARO; egen bil og servicebil har separate parkerings- og lagringsdata.
- Innovervendte, lukkede motellsolider er vendt i 15 modelldeler med bevart geometri, materialer og UV-er. Dørene har samme naturlige treuttrykk.
- Caliche har ti fysiske, gangbare rom med bevegelige inngangsdører, innvendige gulv og møbelkollisjon. Sheriffinngangen har understøttet plattform og trinn. Alle inngangene, bensinstasjonen og radiolobbyen starter åpne også i eldre lagringer; senere manuell lukking lagres. Dette er Toms midlertidige testregel.
- Earl sitter ved stolen i studioet. Bensinstasjonsmannens føtter følger gulvet fremfor disken. Pale figure bruker Toms originale Tripo-modell og skjelett med prosedyrestyrt bevegelse i SC02/04/08/11. Hereford-modellen følger hele flokken og eksisterende missing-flagg. Kyrne har ikke fått en ny skjelettanimasjon.
- Radioen bruker kontinuerlig mottakerstøy. Trafostasjonen har bevegelig lysbue, variert lys og romlig 60/120/180 Hz-summing med elektrisk støy. Rolig blink respekteres. Subjektiv lydvurdering gjenstår.
- LOGIC-01 til09 er rettet: SC11-restaurering, usett SC02-retur, egenbil, Ortiz-mål, Ward-mål, kurerhint, faktiske striptider, journalattribusjon og bortføringsvinduer uten nytt tidstap. Dette er tekniske rettelser, ikke bevis på en full gjennomspilling.
- High med maskinvare-RT og minst 8 GiB VRAM får en 640 MiB-geometripool. Low og mindre/ukjente kort beholder 400 MiB. Varslingen og 20%-grensen er uendret.

## Kontroll og grenser

UE5.8.4-editorbygg og Linux-pakking består. Pakken består 1772 selvtester og 15 kontroller av en isolert kopi av den faktisk blokkerte lagringen, gjennom vanlig Continue. Samme15 kontroller består etter lokal installasjon. Dette kontrollerer gjenoppretting, faktiske produksjonslabeler og interaksjonsmål; ingen manuell kjøretur eller fysisk kontrollerprøve påstås.

En skriptstyrt native gåprøve går 6.4 m gjennom sheriffdøren og inn til resepsjonsdisken. Gulv er under hele ruten; største avvik mellom gulvberegning og sporet mesh var 0.12 m under trinnpasseringen. Ti rom og native deler lastes også i pakken. Ikke hver inngang er manuelt gjennomspilt.

Pakket Low viser faktisk Tripo-skinning gjennom gange, lytting og grep. Den navngitte modellprøven bruker dagslys og en kontrollert plassering; den er ikke et naturlig alienmøte eller AI-/helspillsbevis. High viser640MiB på RTX3080 med 10 GiB VRAM, Low går tilbake til 400 MiB. Rødt resident-poolvarsel er ikke synlig i siste pakket High-bilde.

Alle 68 payload-noder er kontrollert mot kandidaten. Alle 126 Saved-noder er uendret før/etter installasjon; de 100 opprinnelige Saved-filene er også hashkontrollert mot kopien før arbeidet. Personlige lagringer og foto er ikke publisert. Forrige installerte payload er bevart lokalt som tilbakeføring. En eldre testpakke er pensjonert: sju genererte filer, 4.706 GB logisk. Gjeldende og to eldre komplette testpayloads beholdes. Reflinkdelte data gir ikke tilsvarende løfte om faktisk frigjort plass. [Avgrenset kvittering](PlaytestFixes-2026-10-31/receipt.json).

Menneskelig lytting, subjektiv skrekk-/komfortvurdering og vanlig sammenhengende SC01 til SC13-spilling gjenstår. Snapzoom/lydsting er fortsatt et forslag, ikke en tvungen kameraeffekt.

## Bilder

Bilder er bevart uendret. Motell-/Earl-bildene og innvendig sheriffprøve er editorkontroller, delvis før siste RT-pooljustering. De røde varslene i disse historiske bildene beskriver ikke sluttpakken. Pakket/installert bevis er merket særskilt.

- Motell, samme faste kamerasted og02:44: [før](PlaytestFixes-2026-10-31/motel-before.png), [etter geometri og døruttrykk](PlaytestFixes-2026-10-31/motel-after.png).
- Radiostudio, samme kamerasted: [før](PlaytestFixes-2026-10-31/earl-before.png), [Earl ved stolen](PlaytestFixes-2026-10-31/earl-after.png).
- Sheriff: [native gåprøve innenfor døren](PlaytestFixes-2026-10-31/sheriff-entry.png), [pakket inngang med trinn](PlaytestFixes-2026-10-31/sheriff-front-package.png). Toms originale førbilde står uendret i [spilltestmappen](../Playtest-2026-10-31/01-sheriff-floating-parts.png).
- Tripo: [pakket Low-modellprøve](PlaytestFixes-2026-10-31/alien-package-low.png), [Herefords i faktisk flokk](PlaytestFixes-2026-10-31/cows.png).
- Trafo, samme tilnærming i en kopiert sak: [forrige pakke](PlaytestFixes-2026-10-31/trafo-before.png), [ny effekt](PlaytestFixes-2026-10-31/trafo-after.png). Stillbilder dokumenterer synlig effekt, ikke opplevd animasjon eller lyd.
- [Installert pakke med faktisk gjenopprettet SC04 og Return to SARO](PlaytestFixes-2026-10-31/recovery-installed.png).
