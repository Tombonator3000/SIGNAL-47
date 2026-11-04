# EastWing bygningsskall

Status: Generert i Blender og native-importert 3. november 2026. Alle åtte assets/24LOD-er består importkontrollen; runtime montering og visuell kontroll pågår.

`layout.py` bevarer blokkmodellens veggplan x20,6–38,6 og z−5,5–5,5, gulv y0, tak y3,32 og de seks opprinnelige gang-/portåpningene. En ny østserviceinngang følger nå planens krav, uten å flytte måleren ved z3,4. `build_shell.py` lager en redigerbar Blender-scene, tre eksplisitte detaljnivåer per asset, materialmanifest og segmentert UCX. Skriptet starter ingen motor, import eller rendering selv.

Kjør `python3 layout.py` for lett kontroll av kildeplanen. Root kjører Blender-skriptet under prosjektets `production_lease` og `require-idle`. Standardutdata er `exports/`; `S47_EASTWING_SHELL_OUT` kan velge en annen utmappe. `shell-manifest.json` viser leverte filer og ankre etter faktisk generering. Kjøring og native/visuell kontroll gjenstår. Etter generering validerer `python3 import_unreal.py` alle FBX-hasher, materialer, native trekantforventning og lokale/globale ankre. Root kan deretter kjøre `python3 run_unreal_import.py --apply --output /privat/ny/importmappe`. Wrapperen tar lease, krever ledig motor og bevarer 100 GB diskreserve. `--source` velger en alternativ generert mappe.

## Koordinater og montering

Blender-metre er `(spill.x, -spill.z, spill.y)`. Prosjektets målte FBX-importoppskrift gir Unreal-centimeter `(100*Blender.x, -100*Blender.y, 100*Blender.z)`. FBX bruker `FBX_SCALE_UNITS`, forward `-Y`, up `Z`. Ingen ekstra meterskala eller automatisk verdensplassering skal legges til.

| Asset | Montering |
| --- | --- |
| `SM_EW_Shell` | Global origin. Vegger med åpninger, sokkel, gulv og synlige ramper. |
| `SM_EW_Ceiling` | Global origin. Interiørtak og takribber. Det eksisterende delte yttertaket bevares. |
| `SM_EW_Trim` | Global origin. Karmer, vindusposter, persienner, gulvlister, ledningsrør, bokser, baldakin, portskinner og pullerter. |
| `SM_EW_WindowGlass` | Global origin. Separate nordruter; native glassmateriale og ingen brukskollisjon. |
| `SM_EW_SignBoard` | Global origin. Grønn skiltplate med kant og egen `EW_Shell_SignFace`, UV0 innen 0–1 på forsiden. Eksisterende skiltkunst kobles senere. |
| `SM_EW_PersonnelDoor` | Lokal mesh rundt spillanker `[21.3, 0, 5.5]`. Manifestet oppgir åpen rotasjon `[0, pi/2, 0]` inn i laben. Root eier faktisk bevegelse/låsing. |
| `SM_EW_RollUpDoor` | Lokal mesh rundt øvre portanker `[35.4, 3, 5.5]`. Kilden har separate lameller; eksporten er en lukket gardin. Åpning/rulling og kollisjonsbytte må implementeres før montering i den åpne Freeform-porten. |
| `SM_EW_ServiceDoor` | Lokal mesh rundt nytt østanker `[38.6, 0, 1.2]`, åpen rotasjon `[0, -pi/2, 0]` inn mot garasjen. Åpningen er z1,20–2,30 og høyde2,15m, med separat rampe x38,6–40,2 og bredde z1,0–2,5. |

Hver `.asset.json` gir lokale Blender-bounds, globale spill-bounds, `anchor_three_m`, materialrekkefølge og egne bounds/tall for hvert LOD. Statiske assets skal ikke sentreres igjen. Native import bruker `/Game/EastWing/Shell`, egen eierskapsmetadata og gjenbruker eksisterende Props-overflater uten å endre dem. Glass og skiltflate har egne materialslotter. Importøren kontrollerer hver LODs rendergeometri, trekantantall, akser, materialseksjoner og segmentert UCX. Dørmeshen skal bare ha sitt eksplisitte anker lagt til, én gang.

## Kilder og detaljnivåer

Åpne `EastWing_Shell.blend` direkte. Hvert asset har en egen `_EDITABLE`-samling; komponentene, UV-ene og bevelmodifikatorene beholdes. FBX er evaluert, sammenslått og triangulert. LOD1 reduserer bevel- og sylinderdetalj, LOD2 fjerner bevels og små rørklammer. Primære vegger, åpninger, ruter, portlameller og skiltgrenser beholdes. Alle LOD-er har samme materialslotter og pivot.

Modellene er ny prosjektlaget geometri. Eksport-, UV- og materialmetoden gjenbruker `Props/prop_common.py` og `Signal47/Tools/sarosite/build_site.py`. Bibliotekprofilene AnyScene og bpy-dev/blender-mcp er lest som metodegrunnlag, ikke installert eller kopiert som kode. AnyScene er ikke Unreal-verifisert; Blender MCP-profilens historiske testresultat beviser ikke denne modellens kvalitet.

De seks konseptbildene er sett. Skallet følger felles trekk: puss ute, kremfarget mur inne, grønne metallkarmer, nordvinduer med persienner, synlig elektrisk føring, port og grå betonggulv. Blender-kilden bruker eksisterende Props-materialsett med fysisk UV-skala. Native import erstatter puss og betong med lokale fotobaserte CC0-sett som beskrevet nedenfor; øvrige overflater beholdes. Det er ikke hevdet fotorealistisk resultat før faktisk material-/lyskontroll i Unreal.

## Kollisjon og åpne avklaringer

UCX er egne konvekse vegg-/gulv-/rampesegmenter, ikke en omsluttende bygningsboks. Kildeplanen kontrollerer hele volumet i sju passasjer. Personaldør og port har egne kollisjonsvolumer som bare gjelder lukket posisjon. Dekor og vinduer skal ikke overta interaksjonsproksyer eller blokkere gangåpninger. Root eier gang-/kjørekollisjon og native prøve med Jeepen.

Rampene følger eksisterende `AS47Props::AddFloor`: y0 ved fasaden og y−0,576 ved enden, garasje 4m og personaladkomst 1,6m. Dette gjør det synlige gulvet samsvarende med gangplanet. Østserviceinngangen er ny: den gamle utsiden hadde bare måler/kabel. Skallet splitter østveggen og UCX, og root må speile åpningen, låsing og gulvhelningen i runtime. Delt tak, takutstyr, meter og gammel dressing fjernes ikke av dette skriptet.

Glassuttrykk, strømstyrt lys, signalkunst, dør-/portfunksjon, gjenimport, native materialer og samlet visuelt resultat skal kontrolleres ved integrasjonen. En generert fil eller en godkjent kildeplan lukker ikke disse punktene. Native import rapporterer `PASS_NATIVE_SHELL_IMPORT` ved teknisk kontroll, aldri ferdig gameplay eller visuelt godkjent resultat.


## Fotobaserte native overflater

`import_unreal.py` bruker de allerede installerte 2K-originalene direkte fra
`Unity/Assets/Signal47/Art/Visual10/Textures/`; ingen nedlasting eller ekstra kildekopi.
`Unity/Assets/Signal47/Art/Visual10/source-manifest.json` oppgir for hver fil
SHA-256, forfatter, lisens og originaladresse. Importplanen validerer alle seks
filer mot manifestet og krever `CC0-1.0` og `2k` før motorimport.

| Native materialslot | Fotokilde | Opphav / lisens | UV-multiplikator |
| --- | --- | --- | --- |
| `EW_Shell_Stucco` | [painted_plaster_wall](https://polyhaven.com/a/painted_plaster_wall) | Amal Kumar, CC0-1.0 | 1 |
| `EW_Shell_Concrete` | [concrete_floor_02](https://polyhaven.com/a/concrete_floor_02) | Rob Tuytel, CC0-1.0 | 2 |
| `EW_Shell_Plinth` | Samme betongsett | Rob Tuytel, CC0-1.0 | 2 |

Begge Poly Haven-kildene oppgir 2 × 2 meter fysisk størrelse (kontrollert mot
primærsidene av koordinerende agent). Eksisterende FBX-UV er meter delt på
2 for puss og på 4 for betong/sokkel; materialets UV-multiplikator er derfor
henholdsvis 1 og 2. Dette gir én repetisjon per 2 meter uten nye UV-er eller FBX.
Diffuse importeres som sRGB, roughness som lineær data og `nor_gl` som normalmap.
Etter basisoppskriften setter importøren faktisk `flip_green_channel=True`,
leser verdien tilbake og lagrer teksturen for OpenGL→Unreal-normalretning.
Ingen metallkart brukes; metallic er null. Tak og øvrige materialer beholdes.

Disse tre materialene opprettes/oppdateres under `/Game/EastWing/Shell/Materials`
med `S47Owner=S47EastWingShell_v1`; eksisterende delte Props-materialer endres
ikke. Materialene får `S47License` og full `S47SourceProvenance` fra manifestet.
Importresultatet dokumenterer kilde, UV-skala og normalretning. Eksisterende
uvedkommende native assets avvises av samme eierskapskontroll som tidligere.

Materialrevisjonen har bestått vanlig Python-syntakskontroll, kildeplan for
8 assets / 24 FBX og kontroll av de tre erstatningsoppskriftene. Native reimport
og visuell vurdering må utføres separat; tidligere Shell-r1-PASS gjelder de
opprinnelige materialene. Biblioteksøket viste AnyScene som metodegrunnlag,
fortsatt UNVERIFIED for Unreal; denne endringen gjenbruker prosjektets målte
`fieldintegration/import_recipe.py` og Props-importørens normalhåndtering.

`EW_Shell_Masonry` bruker nå det lokale CC0-kildesettet i
`Textures/ConcreteBlockWall/`, med manifestvalidering av lisens, kartroller,
oppløsning, størrelse og SHA-256. UV-multiplikator 1,2 gir 2 m repetisjon fra
opprinnelig 2,4 m UV. Eget Shell-materiale kobler desaturert diffuse gjennom
`(0.8 + 0.25 * diffuse) * cream(.68,.63,.49)` til BaseColor, roughness gjennom
`0.70 + 0.15 * R`, og GL-normal med grønnkanalflipp og styrke 0,3.
Dette er en kremmalt tilpasning av værslitt blokk; primærkilden viser et
referansefoto, men oppgir ikke eksplisitt fotogrammetri. Native import/visuell
kontroll gjenstår; ren Python-kildeplan består 8 modeller / 24 FBX.

## Flytting av tre gamle tønner ved personalrampen

`legacy_access_recipe.py` beskriver bare de tre opprinnelige tønnene og deres
kontaktflater. `build_legacy_access.py` leser den hashfestede originale
`/home/tom/scratch-bm/Signal47-web/geo/geo_003.glb`, bevarer originalfilen og
anvender de etablerte bil-/vindus-/forplasskuttene før flytting på −2,6 m i spill-X.
Dette gir sentre x18,7 / 19,35 / 19,7, med uendret høyde og z.

- `SM_EW_AccessDressing`: 13 550 opprinnelige triangler → 10 256 etter eksisterende
  kutt (976 bil, 1 872 vindu, 230 vegetasjon, 144 hjulrester) og 72 for det gamle
  nedløpsrøret på østveggen, som krysset den nye servicedøren (Claude 4. november). Akkurat 648 beholdte
  triangler flyttes, 216 per tønne. Øvrige 9 608 beholdte triangler er uendret.
- `SM_EW_AccessStains`: 1 712 → 1 220 etter eksisterende 488 veggskittkutt og fire
  triangler med rennestripe under det fjernede røret.
  Bare seks kontaktflatetriangler flyttes; øvrige 1 218 beholdes uendret.
- Begge får tre identiske authored LOD-er. Ingen ekstra forenkling eller ny UV.
  Kilden krever uendret hjørneposisjon, UV, winding og materialindeks for alle
  urørte trekanter; flyttede vertexer kan ikke deles med urørte trekanter.

Root kjører Blender under produksjonslease med `S47_EASTWING_ACCESS_OUT` satt til
en privat genereringsmappe og `--python build_legacy_access.py`. Ingen motor
startes av kildekoden. `import_legacy_access.py` validerer manifest/FBX-hasher i
vanlig Python; med `S47_EASTWING_ACCESS_APPLY=1` inne i en allerede sikret Unreal
importeres de to nye assetene under `/Game/EastWing/Access`. Sett også
`S47_EASTWING_ACCESS_REPORT` til en privat JSON-resultatfil. Importen kontrollerer
3 LOD-er, eksakte trekanttall og bounds/akser, beholder FP32-UV/CPU-tilgang og
merker derivatene med egen `S47EastWingAccess_v1`-eier og kildebevis. Den overskriver
ikke de gamle fallback-assetene eller materialene.

Runtime må montere derivatene etter eksisterende Fleet-forplasstrim og koble
hver komponents opprinnelige atlas tilbake på materialslot 0. Oppdater de to
meshnavn-auditene i Fleet/SaroSite, behold 10 328-trekantkravet. De tre opprinnelige
walkingrectene har ingen ID: match alle fire grenser fra `old_walking_feet` med
toleranse, krev nøyaktig tre treff og flytt bare min/max-X. Ikke bruk gamle
arrayindekser etter øvrig collideropprydding. Flytteplanen og nye rects ligger i
`access-manifest.json`. Original drive-obstacle-liste har ikke disse tre rectene.
Native import og visuell kontroll av personadkomsten gjenstår til faktisk kjøring.

Reproduksjon: velg for eksempel privat OUT
`/home/tom/scratch-bm/Signal47-eastwing-props-20261103/access-r1` og sett
`S47_EASTWING_ACCESS_OUT` eksplisitt i begge prosesser. Skriptene har med hensikt
ingen implisitt outputmappe. Kjør først Blender-generatoren under lease, deretter
`python3 import_legacy_access.py` for kildeplan og til slutt importøren i root sitt
sikrede Unreal-vindu med APPLY/REPORT-variablene over. Kilde-SHA er
`fd60fa957655d289cd05909020f2f39f85e8a8e9692f3c402d9418f9b3d1a5f9`.
Manifestet gir seks FBX-hasher, bevaringssignaturer og eksakte før/etter-tall.
Runtime `RelocateAccessDressing()` forhåndskontrollerer begge derivater, de gamle
10328/1224-meshene, atlasene og nøyaktig ett treff per gammel walkingrect før
begge meshene og de tre rectene byttes samlet. Ved manglende forutsetninger
beholdes gammel plassering. Verifiser suksessloggen og den faktiske frie rampen;
at skallet er bygget beviser ikke at denne separate flyttingen lyktes.
