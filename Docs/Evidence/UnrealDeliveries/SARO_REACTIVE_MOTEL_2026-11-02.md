# Tredje nattleveranse: reaktivt romlys og ett motellskilt

**Status:** Referanse. **Gjelder:** Unreal Linux/Vulkan, runtime `20252b015b7267d04f0a702bc674fc5858989c65`, [PR329](https://github.com/Tombonator3000/SIGNAL-47/pull/329), main-fletting `b0bddcfe`. **Sist kontrollert:** 2. november 2026.

Installert kl.02:12 CET. Installert Fortsett består fra en isolert lagringskopi, og alle164 personlige Saved-elementer er kontrollert uendret. Forrige payload og Saved er bevart privat. Ingen offentlig spillrelease.

## Spillendring

Freeform-regissøren har én ny handlingsbetinget hendelse,17 kataloghendelser totalt. En faktisk fysisk OFF-handling kan senere etterfølges av tent romlys ved retur. En gammel lagret OFF-verdi armerer ingenting. Minst60 sekunder, forlatt rom, over25 meters avstand, ute av syn og tilgjengelig strøm kreves. Vanlig enkeltaktiv hendelse, dagsbudsjett, hvile og tredagers gjentakelsesvern beholdes i produksjon.

Manuell preferanse og hendelsesstyrt energisering lagres separat. Original OFF-entry og fysisk undersøkt ON-readback kan gjenfinnes i sakens arkiv. Continue gjentar ikke effekten; ny manuell bruk nullstiller den uten å slette funnene. Storys manus og lysregler beholdes. Teksten oppgir klokkede journaloppføringer uten å konkludere om en aktør eller fremmed opprinnelse.

Det store skiltet nærmest veien var originale scenedeler26/27/28, ikke de importerte motelBlade/motelNeon som allerede var skjult. Nærdelene og fjernkopi1252 er pensjonert med synlighet/kollisjon. Det eksisterende innkjøringsskiltet og fotoankeret beholdes. Emissivt nattlys forsterkes2,6 ganger og følger dagsmasken; eksisterende detaljerte glødebokstaver/geometri og alle LOD-er beholdes. Finere neonstil og lesbarhet holdes til videre visuell vurdering i TODO.

## Prøving og grenser

- Editorbygg og Linux-pakke PASS;1993 pakkede selvtester PASS.
- Reaktiv spillerreise PASS,22 kontroller/3 fulle disk-/Flow-gjenopprettinger i editor og pakke: ekte ray/OFF, faktisk gange gjennom østutgang og nord-/veststi, vanlig60-sekunders kandidatalder, retur, ekte EXAMINE, arkiv og nytt manuelt reset.
- Testen bruker en tydelig initial approachplassering og utsetter bare konkurrerende tilfeldig valg (`nextAt=activeTime+600`) i en isolert fixture. Force velger hendelsen uten å omgå dens input-, alders-, fraværs- eller synlighetskrav. Naturlig utvalgsfrekvens, menneskelig input og en hel natt hevdes ikke.
- Motellets native audit PASS med81 deler og beholdt historie/foto/TV/biler. Pakkede natt- og skumringsprøver fra motsatte veiretninger består; bildene er gjennomsett. Utviklingsbildene før/etter bruker samme dagkamera, men er ikke to frosne pakkebygg.

[Dag før](SaroReactiveMotel-2026-11-02/motel-before.png), [dag etter](SaroReactiveMotel-2026-11-02/motel-after.png), [pakket natt](SaroReactiveMotel-2026-11-02/motel-night.png), [pakket skumring fra motsatt retning](SaroReactiveMotel-2026-11-02/motel-dusk.png), [kort prøveoppsummering](SaroReactiveMotel-2026-11-02/summary.json).

## Åpne feil

WORLD-04 ble ikke gjenskapt i seks stabile korridor-/OPS-/klokke-/Rest-bilder. Nedre vegg, backingbokser, mesh, bounds og materialer er uendret; faktisk PC-bruk og begge klokker ved UI-kontrollen er prøvd. Dette er ingen bevist veggretting. Neste avgrensning er en faktisk ute→sørdør→korridor-overgang uten lasting.

WORLD-01 ble heller ikke gjenskapt med to statiske kamerafixtures ved målt returområde. Den ene brukte produksjonens førerøyeberegning, men viste ikke kabinen eller den bevegelige handoffen. Ingen feilflate er identifisert, og det gamle returbildets svarte striper holdes åpne. De private nærliggende vegglagringene er ikke lagt i Git.

Prosjektbibliotekets bloom-kort gjaldt Three.js og var ikke kompatibelt bevis for Unreal-feilene. Egne eksisterende regissør-, lys-, lagrings-, gang- og arkivsystemer er gjenbrukt. Nattens samlede disk-/kopikontroll gjenbrukes; lett df viste158GB før installasjon. Ingen ny rydderunde eller arbeidskopi. Gjeldende kandidat/cache beholdes til videre arbeid og neste samlede kjøring.
