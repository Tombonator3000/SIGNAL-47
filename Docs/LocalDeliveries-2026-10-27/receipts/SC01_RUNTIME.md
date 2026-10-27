# SC01: Den første overflygningen

SC01s lokale, pakkede tekniske aksept består. Testet kilde er `dbae31775d3690383417c0f3652de16fb8ac00ca` på `codex/story-sc01`, etter [den vedtatte gjennomføringsplanen](HISTORIE_IMPLEMENTERINGSPLAN.md). [PR168](https://github.com/Tombonator3000/SIGNAL-47/pull/168) er flettet i `82f000547a6247ad6c920670860f7db5c6d922ac`. Endelig bevis og installatørens tørrkjøring består. Faktisk installasjon ble avvist før filbytte fordi brukeren startet installert NP1, PID 437278. Prosessen er ikke stoppet. NP1 er fortsatt standard; SC01-kvitteringen fullføres etter at spillet er lukket.

## Spillbar leveranse

Nye saker får innholdsrevisjon 1 med normalrunde, kamera og faktisk instant-back-referanse. Normalprofilen er 1419.900 MHz, az 042, gain 55 og båndbredde 48. Spilleren prøver begge eksisterende utganger, sammenligner referanselampen direkte og gjennom glass og bruker en fysisk opak skjerm. En bevegelig original hånd/underarm kommer frem utenfor det kjente vinduet, trekkes bak veggkanten og etterlater fysisk glasspor. Undersøkelsen og dispatchrapporten beskriver bare faktiske observasjoner. Himmellyset kan overses.

Den siste av 36 frie eksponeringer reserveres for den nødvendige referansen. Feil motiv gir ingen credit og bruker ikke reserven. De 35 foregående albumoppføringene bevares; prøvebudsjettets syntetiske oppføringer regnes ikke som fotografiske bevis. En tidligere intern sak som allerede mangler denne eksponeringen, beholder bilder og eksisterende progresjon uten oppdiktet referanse.

Lagring har versjonert scenario-, verdens- og manusstatus. Gamle saker og tidligere innholdsrevisjoner får ingen nye bakoverrettede krav. Fullført SC01 gjenåpnes ikke ved ny døråpning, og en oversett hånd gir ingen sett-kropp- eller rapportcredit. Ved produsert grense fortsetter saken gjennom eksisterende historie. H01 forblir åpent til SC02 er levert. Registeret har alle 79 manus-ID-er, men senere scenarioer har fortsatt åpne verdensprodusenter.

## Kontroller på samme pakke

| Kontroll | Faktisk resultat |
|---|---|
| Kilde og medier | 24 manuskontrakter, ni eksportkontroller, stemmekatalog og 14 isolerte installerprøver består |
| Pakket selvtest | 1 501 kontroller består |
| SC01 High og Low | 120 kontroller og 17 disk-/Flow-restores hver, 140,00 m native gange |
| Oversett hånd | 112 kontroller og 16 restores, uten oppdiktet kropp eller rapport |
| Ankomst | Både direkte og burger, faktisk kjøring/parkering og overlevering; én ankomst per sak |
| Eksisterende historie | Kontrollrom, T01 53/19, T02 65/15, Dale 191/5, LineLights 12/3, feltfoto/mørkerom og hele eksisterende finale består |
| Fersk vanlig Continue | Fem separate prosesser fra før normalrunden, midt i vindushendelsen, etter den, før readback og etter fullføring/gjenåpnet dør |
| Visuell kontroll | 32 originale PNG/JPEG-er er faktisk vurdert, inklusive High/Low, ankomster, Continue, feltprint og belyst bil nær/fjernt |

Totalt 21 isolerte pakkekjøringer er gjennomført, inklusive to belyste bilopptak etter utilstrekkelige nattbilder. Prøvene bruker native gange/Resolve, faktiske siktråler, ordinær fotografering og fokuserte Slate-knapper der kontrakten krever det. Forberedte startposisjoner og senere kapittelhopp er testfixtures. Dette er ikke en menneskelig gjennomspilling av hele natten.

Continue-kildene er faktiske FS47SaveStore-filer kopiert til egne profiler. Prosessene starter på tittelen med vanlig Continue, uten kapittel-, tilstands- eller kontraktdriver. Logg, skjermbilde og bevarte kildestorer kontrolleres. Bare uttrykkelig registrerte nye ordinære autosaves er bevis på fersk serialisert tilstand; uendrede inputfiler er ingen minnedump.

## Modeller, lyd og RT-minne

Original Blender, tre FBX-er og produksjonsskript beholdes. Native hånd har full LOD0 på 19 388 og redusert LOD1 på 5 816 trekanter, uten Nanite eller separat RT-proxy. Glasspor, skjermspile og fire materialer er bytebevart.

Den eksisterende krediterte Fairheaven SW84-fullbilen får ordinære LOD-er 19 461/5 837. Full LOD0s 25 919 kildeposisjoner, trekanter og grenser er hashkontrollert uendret. Åtte materialbindinger, 18 glTF-/buffer-/teksturfiler og 25 øvrige native assets er bevart. Body/dør og kreditering er uendret. Samlet cullingboks omfatter LOD1s avvik under 2 cm. Reproduserbar import er avgrenset til fullbilens eksakte asset.

Endelig måling viser 79,8043 MiB AlwaysResident RT-geometri i 197 oppvarmede samples, under eksisterende 80 MiB-varselgrense. Pool 400 MiB, prosentgrense 20 og synlige varsler er uendret. Dette måler den aktuelle residensprøven, ikke total VRAM eller generell ytelse. R1 målte 80,7112 MiB og R2/R3 80,0648 MiB; de opprinnelige FAIL-bevisene er beholdt.

Fem faktiske stemmetakes varer 35,07 sekunder i PCM16 mono 48 kHz. Alle fem fullfører telefonruting med positive source-/routed-samples i den pakkede High-prøven. Teknisk fil-, hash-, import- og avspillingskontroll består. Fire takes har eksakt ASR; ett egennavn gir SARO/Sorrow-avvik i streng ASR. Menneskelig lytting, stemmelikhet og emosjonell vurdering er UNVERIFIED.

Glassmerkene er svake ved bildets avstand og delvis dekket av interaksjonstekst. Toast dekker deler av hånden i enkelte bilder. Tidlige D01-telefonbilder er tatt mellom linjer; senere SC01-overlevering er lesbar. Fjernbilen er liten i bildet, så bare grov fjernvisning er visuelt attestert. Fysisk Xbox, menneskelig frykt/forståelse/tempo og ubrutt helnatt er egne åpne prøver.

## Kilde- og bevisbinding

Bevisrot: `/home/tom/scratch-bm/Signal47-sc01-20261026`.

- Kandidat: `candidate-r4/Linux`; 40 pakkefiler uten Saved.
- Binær SHA256: `ecc5038c5d1f94e1afe00a5cd14d41c5a6b1ed5bcbb0b62f7f40c63a9e76956e`.
- Payloadsnapshot SHA256: `6fbb03a9241828ac81cac09673eefdf601634e07ea15da7fff907817249c8af3`.
- `packaging-r4/proof-fragment.json` binder faktisk BuildCookRun og alle 14 007 frosne kilde-/Content-inputs før og etter bygg, også lokale genererte/lisensierte avhengigheter.
- `source-checks-r4.json`, `rt-memory-r4-final.json`, `media-runtime-r4.json`, `FINAL_VISUAL.json` og `continue-batch-1` fører de konkrete kontrollene og grensene.
- `runtime-profile-archive-r4/manifest.json` verifiserer permanente, byteidentiske kopier av 21 fullførte testprofiler, 677 filer. Tidligere editor-/R1-/R2-/R3-bevis og FAIL-resultater bevares separat.

Byggets to genererte FileOpenOrder-logger bevares ved siden av kvitteringen før de opprinnelige rene kildebytesene restaureres. Ingen spillkode eller pakkefil endres i denne etterkontrollen. Endringer etter den testede kildecommitten skal være dokumentasjon frem til installasjon.

## Levering venter på lukket spill

`delivery-r4/install-proof.json` har SHA256 `aac7334d1c1dd902e78cf37f6aea15f168d537c144678e2266f1a45318a167dd` og validerer 23 akseptrader, 21 runtimekjøringer og 14 007 kilde-/Content-inputs mot faktisk PR-head/core. Ingen CI-jobber var rapportert for eksakt head; dette er lokal aksept. `install-apply-r4.log` fører den trygge avvisningen av aktiv spillprosess, før backup/transaksjon eller filbytte.

Brukerens eventuelle nye lagringer fra den åpne NP1-økten må få en ny lesende baseline etter lukking og bevares ved installasjon. Den tidligere baselinen har 40 filer/57 noder og skal ikke skrives tilbake over nye brukerdata. Deretter kjøres installer, isolert standardstarter-smoke, full payload-/Saved-etterkontroll og canonical pruning. Dokumentasjonskvittering og SC02-produksjon er fortsatt ikke utført.
