# Caliche, fire eksisterende bolighus

ENV-02 for Unreal. Status ved kildeforfatting: **ren Python-preflight PASS; Blender, faktisk eksport, native, runtime, bilder og pakke UNVERIFIED**. `source-preflight.json` er kildebevis, ikke en kjørt DCC- eller spilltest. Root kjører DCC og motor sekvensielt gjennom prosjektets guard.

Husene er de fire vanlige boligene i `web/src/world/Town.ts:759`, brukt som historisk målereferanse. Dette endrer ikke nettversjonen. Originalt manifest er låst til SHA-256 `69fcf95db3adf8eff76274f59b03f94e662936a147f1ee7389630c1a8a0925c9`. `source-contract.json` er forfattet autoritet for mål, ankre, åpninger og bevaring; `source-plan.json` er den lette geometriplanen før Blender.

| Hus | Verdensanker, Three meter | Front | Dørens Three-Z | Vinduets Three-Z | Uttrykk |
|---|---|---|---:|---:|---|
| H01 | `(8028.5, 7.3141970634, 2133)` | mot −X, vegg X8024 | 2136.4 | 2131.2 | tan puss, trekarmer, synlige viga-ender og trehette |
| H02 | `(8028.25, 8.0030660629, 2156.5)` | mot −X, vegg X8024.5 | 2159.4 | 2155.2 | krempuss, falmede salviegrønne skodder og trehette |
| H03 | `(8029.5, 8.8187494278, 2185)` | mot −X, vegg X8025 | 2181.6 | 2186.6 | tan puss, mørke karmer, slitt blikkhette |
| H04 | `(7976.5, 11.2685537338, 2241)` | mot +X, vegg X7981 | 2244.4 | 2239.2 | krempuss, falmede blå karmer og trehette |

Fotavtrykkene er de opprinnelige 9×10, 7,5×9, 9×10 og 9×10 meterne. Sokkelens nedre kant følger målt gammel veggbunn i hvert hus; gulv- og dørankrene flyttes ikke for å få modellen til å passe terrenget. Pussens tan/krem-farger bruker de opprinnelige sRGB-verdiene `(185,143,104)` og `(220,201,166)`. Rundede mineralhjørner, dype smyg, karmer med beslag, betongsåler, takkant med dryppnese, veggventil og små avgrensede reparasjoner gir lesbar fysisk detalj uten ny dekor som dekker de gamle objektene.

## Deler, bevaring og integrasjonskontrakt

Hvert hus gir `SM_Homes_H01_Shell`, `SM_Homes_H01_Roof`, `SM_Homes_H01_Fixtures`, tilsvarende gjennom H04: **tolv native deler**, med tre håndforfattede LOD-er. Alle har identitetstransform og lokalt nullpunkt. `Three=(rootX+Bx, rootY+Bz, rootZ-By)`, runtime yaw0. Eksporten bruker prosjektets målte `-Y` frem, `Z` opp, `FBX_SCALE_UNITS`; native må bruke `force_front_x_axis=True`, `MATCH_UP_FORWARD_AXES`, yaw90 og kontrollere den medfølgende asymmetriske akseproben.

Det nye skallet har faktiske veggåpninger, uten boks eller ny glassflate foran gamle bilder. Tre TV-/kjøkkenpaneler, proxyer, TV2-flipp og H02s mørke rute beholdes. Det er en dyp mørk vindusnisje bak den eksisterende flaten. Det nye settet inneholder ingen TV, kjøkkenbilde, dørblad eller ny adgang.

De fire opprinnelige dørholderne (1436/1439/1442/1445) med blad, egen trekarm og eksisterende lisensierte erstatning beholdes samlet. De målte bladene er 0,95×2,10 m. Det nye mursmyget er 1,17 m bredt og åpner til 2,26 m, rundt den eksisterende karmens 1,11 m og topp 2,18 m. Muroverdekningen slutter 2,30 m; H04s eksisterende lyspære begynner 2,32 m. Ny porch-hette har underkant 2,67 m, sidebeslag og ingen stolper foran røykeren. Terskelen ligger bare på innsiden av veggplanet og møter det beholdte trinnet.

Gamle stoops, postkasser, biler, røykeren, lyspærer, rør, målere, antenner, kjølere og takventiler beholdes. Takmembranens topp ligger fortsatt ved lokal Z3,04, slik at originalt takutstyr beholder kontakt. Ingen ukjent rekvisitt skjules. Nye detaljer ligger på vegg og takkant, uten nye kollidere i gang-/kjørefeltet. Originale analytiske spiller-/bilkollidere og gangsoner er fortsatt autoritet; nye mesh kan gi kompleks synlighetskollisjon, med RT og tre LOD-er bevart, Nanite av.

Den første integrasjonsmålingen telte 578 gamle strukturtrekanter. Avtalt kutt er **528** i fem sammenslåtte geometrier: 192 vindusramme-, 48 tak-, 240 vegg- og 48 grunnmurtrekanter. Differansen er de **48 beholdte trinntrekantene og H02s to mørke glasstrekanter**. Ingen hel Town-forelder skjules. `Tools/calichehomes_integration/` eier de presise kuttene og skiltarbeidet; `Tools/calichehomes_native/` eier import og faktisk native-kontroll. TB2-kildene, de fem landemerkene og motellets kildepakke endres ikke.

## Materialer og manifest

Egne materialnavn har prefiks `M_Homes_`. `materials.json` genereres med `source_material=M_Town_*`, `base_color_linear`, `texture_tint_linear`, `roughness`, `metallic`, `specular`, `normal_strength` og `normal_convention=DirectX`. PBR-bildene leses fra eksisterende originale `Tools/townbuildings/textures/`, med relative filreferanser. De kopieres ikke til en ny teksturfamilie og endres ikke. Brukte piksler pakkes også byteidentisk i hvert hus sin redigerbare Blender-fil. Bare Blender-grafen vender normalbildets grønnkanal for sin lokale OpenGL-visning.

Kanonisk generert manifest er `manifest.json`. `model_contract.json` er en byteidentisk kopi for leseren som først ble avtalt. Begge inneholder faktisk eksporterte data etter vellykket kildebygg, med følgende grensesnitt:

- `source_revision`, `source_contract_sha256`, `materials_manifest_sha256`, `source_inputs_sha256` og konkrete eksportfilhasher.
- `parts[name]`: `house`, `chunk`, `site_world_three_m`, `parent_handle=world.town.group`, `offset=[0,0,0]`, `bounds_local_m`, `vertices`, `triangles`, `materials`, `lod_triangles`, `native_policy`.
- `parts[name].lods`: tre dict-rader med `triangles`, `vertices`, `bounds_local_m`, `materials`, `fbx`. De samme radene kan slås opp i `lods[str(lod)][name]`.
- `houses`: fire `.blend`-filer, scener, redigerbare originalsamlinger og ankre. Ingen verdenskoordinater på flere kilometer er bakt inn i meshene.

## Kjøring og bevisgrense

Lett kontroll, uten Blender eller motor, fra repoets rot:

```sh
python3 unreal/Signal47/Tools/calichehomes/check_source.py \
  --export /home/tom/scratch-bm/Signal47-web \
  --report unreal/ArtSource/CalicheHomes/source-preflight.json \
  --plan unreal/ArtSource/CalicheHomes/source-plan.json
```

Faktisk primitivresultat er **8636 / 5932 / 3880** trekanter i LOD0/1/2 og 501/393/294 lukkede solider. Hver solid kontrolleres separat for to motsatt rettede bruk av hver posisjonssveiset kant og positivt signert volum; en positiv sum kan dermed ikke skjule én innadvendt boks. Alle profiler er kontrollert konvekse før eksplisitt triangulering. Alle trekanter har minst 5e−9 m² areal og ikke kollapset UV. 24 åpningstester dekker hvert hus sin dør/karm og rute i alle LOD-er. Ankre og bladstørrelser er sammenlignet med de faktiske uendrede GLB-dataene. Dette er kildegeometri, ikke en eksportmåling.

Root kjører så følgende to **separate** Blender-prosesser i ledige guardvinduer. Rapportmappen må finnes:

```sh
blender -b -t 2 --python-exit-code 1 \
  --python unreal/Signal47/Tools/calichehomes/build_source.py
blender -b -t 2 --python-exit-code 1 \
  --python unreal/Signal47/Tools/calichehomes/verify_source.py \
  -- --report /home/tom/scratch-bm/Signal47-caliche-homes-20261029/source-verification-r1.json
```

Forventet output: fire `.blend`-originaler med separat redigerbar kopi av hver del, fire familie-GLB-er, 36 FBX-LOD-er, tolv del-GLB-er, én FBX-akseprobe, materialmanifest og to identiske kontraktfiler. Kildebygget bruker eksplisitte trekanter og eksakt posisjonssveising med UV per hjørne; ingen remeshing eller automatisk LOD-redusering.

`verify_source.py` leser de faktiske eksportene og åpner hver lagret Blender-fil i den nye prosessen. FBX kontrolleres mot hver orienterte kildetrekant, UV-hjørne og materiale, slik at snudde flater ikke kan bestå ved bare riktige bounds. Den kontrollerer også meter/akse, LOD-tall, slots, normallengde, GLB-geometri, lagret topologi/UV/smoothing, separate originaler og pakkede pikselbytes. Alle delte kildefiler og teksturer bindes med hash før og etter. Native import, terrengkontakt, gamle objekters synlighet, dørenes faktiske innstikk, sammenhengende Caliche-rute, kvalitetsnivåer og pakke må fortsatt bevises av integrator.

Gjenbruk: prosjektets faktisk prøvde TownBuildings/TB2-eksport og materialskjema, pluss egne `substation/meshkit.py`-primitiver. Prosjektbibliotekets Blender-export- og Unreal-automation-profiler ble lest; de er merket UNVERIFIED for reell Unreal-kjøring og er derfor bare referanse, ikke testbevis eller erstatning for prosjektets importkontrakt.
