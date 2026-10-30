# SARO-kontor, gang, arkiv og fotolab

**Status:** Under arbeid, målrettet kandidat akseptert. **Gjelder:** kilde `95807f4f`, UE 5.8.4/Linux. **Sist kontrollert:** 30. oktober 2026.

Dales kontor har en fysisk forskningsvegg med ni ferdige bildeflater, variert hylleinnhold, lagdelte mapper og fire originale Vintage-rekvisitter. Gangen har 1,73 m klar gangsone mot tidligere 1,30 m, og Operations-fronten er flyttet 0,50 m for en større møteplass. Veggpaneler, lister, kabelkanaler, takgrid, armaturer, karmer og bevegelige dørblader er samordnet med de eksisterende lys- og dørdriverne. En faktisk lukkehandling avvises mens spillerkapselen står i fløyens sveip.

De seks interaktive skrivebordsarkene, to øvrige papirflater og deres ID-er og lesestatus er bevart. Eldre posisjoner som nå klipper ny geometri migreres avgrenset; faktisk Writer, ny Reader, tittel og Continue er kontrollert i fjorten isolerte runder over sju fixtures. Dette er API-/Walker-styrt motorkontroll, ikke menneskelig spilling.

Arkivet har åpne kartonger, mapper, variasjon og tydelige arbeidsområder. Elleve originale kilder er beholdt ved samme historieankre. Fotolaben har foldede kanter, rør, armaturdetaljer, linse/belg, flaskelokk og egne overflater. Normalbelysningen er økt fra 45 til 450 lumen i den eksisterende takarmaturen for rommets omtrent 41 m². Rødt lys, printlys, halvspenning, blackout og strømrestore er kontrollert separat.

## Faktiske kontroller

| Omfang | Resultat |
| --- | --- |
| Office og karmer | Native/ferskleser av alle åtte Office-deler og to karmer. R16 oppdaterer bare ny Dressing og Cork; øvrige bindingsidentiteter beholdes. |
| Vintage-originaler | Fem native modeller, 376 importkontroller og 296 i uavhengig fersk prosess. Fire plasseres i kontoret; bokhyllen er kildebeholdning. Full finite geometri-/UV-/orienteringssammenligning; kjøpte bytes blir private. |
| Fysisk kontor/gang, R14 | 180 kontroller, 21,086 m og 16 disk/full-Flow-restores, med alle fire originalrekvisitter faktisk plassert. |
| Seks skrivebordsark, R14 | 580 kontroller og fem diskforløp. |
| Arkiv, R17 | Alle 33 faktiske renderbuffere kontrollerer posisjonsgrenser, topologi og UV i separat fersk prosess. |
| Trågflater, R18 | Originale røde/svarte toppflater var koplanare. Separat egen klone hever kun 54 source-vertexinstanser ved tolv hjørner 1 mm. Full normalisert MeshDescription og orienterte renderhjørner med UV/normal/farge er identiske med originalen. Original query-body, komponent, materialer og fotopapir ved 1,003 m beholdes. |
| Faste bilder | R14 Low42, R15 High42, R16 High42 og R18 arkiv6/lab9. Utvalgte før/etter er gjennomsett. High gir indirekte Lumen-lys; Low har mørkere tak. |
| Endelig leveranse | Pakking og én samlet matrise er startet. Lokal installasjon og kontroll av installert payload/Saved gjenstår. Ingen offentlig release. |

[Hashbundet målrettet kvittering](SARO_INTERIORS_2026-10-30/targeted-receipt.json). Rå feilforsøk er beholdt; R16s tekniske PASS var ikke visuell aksept: granskingen oppdaget arkivets roterte LOD1/2 og trågflimmer. R17 tvang samme legacy-FBX-transformasjon for alle eksplisitte LOD-er. R18 bekreftet trågklonen i ny prosess og nye bilder. Ingen feilrapport er omskrevet til PASS.

## Samme High-kamera før og etter

Arkiv, skumring: [før](SARO_INTERIORS_2026-10-30/archive-before.png), [etter](SARO_INTERIORS_2026-10-30/archive-after.png). Kameraplassering, FOV og grafikkinnstillinger er de samme.

Fotolab, skumring: [før](SARO_INTERIORS_2026-10-30/lab-before.png), [etter](SARO_INTERIORS_2026-10-30/lab-after.png). Normalmodus er i begge tilfeller på; endret armaturflux er del av den viste forbedringen.

[Nytt kontor](SARO_INTERIORS_2026-10-30/office-after.png) og [ny forbindelsesgang](SARO_INTERIORS_2026-10-30/corridor-after.png). Disse oversiktskameraene har ikke et tilsvarende opprinnelig førbilde; de omtales derfor ikke som et målt før/etter-par.

## Kilder og grenser

Eide Vintage-originaler brukes med egne 2k PBR-baker. Old West Learning og JustB Desert er vurdert selektivt; ingen generell ruinestil er innført. Ingen Tripo-generering, kredittbruk, kjøp eller separat betalt API er brukt i dette miljøpasset. Native kjøpte modell-/teksturbytes ligger utenfor Git under Licensed. Dale er fortsatt reserve, og ingen fysisk Dale-scene er laget.

Prosjektbibliotekets løsning `bilder-for-og-etter-med-worktree` ga prinsippet for faste kameraer; dens opprinnelige PASS gjaldt web, og Unreal-beviset her er separat. Eksisterende Office-/fieldintegration-importoppskrifter og installert UE5.8.4-kilde er brukt for faktisk import/LOD/lys. Ingen ekstern UNVERIFIED skill er installert.

Godkjent himmel/terreng og MetaHuman-servitrisen er beholdt. Menneskelig input, lytting og helspill-FPS er UNVERIFIED. Utviklerbyggenes allerede eksisterende ray-tracing-residency-varsel står i råbildene; det er ikke skjult gjennom en global budsjettendring. Installert spill er fortsatt den tidligere kilden `3dcba420` inntil separat leveringskvittering bekrefter noe annet.
