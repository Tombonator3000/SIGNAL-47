# Blender, Tripo og Unreal: material- og lysarbeidsflyt

**Status:** Gjeldende verktøy. **Gjelder:** originale produksjonsskript for Blender 5.2.2 og UE 5.8.4; eksplisitt valgte egne/eide modeller. **Sist kontrollert:** 31. oktober 2026.

Dette er kjørbare verktøy for Toms bestilling etter [Yokoharas demonstrasjon](https://www.youtube.com/watch?v=kCuoHpv-uZY), ikke forfatterens upubliserte skript. Arbeidsdelingen er generering/gjenbruk, lokal bearbeiding i Blender og native materialer/lys i Unreal. Preview-verktøyene skriver egne prøvescener. Produksjonsintegrasjonen under kobler valgte materialer og lys til den faktiske spillverdenen; installert spill følger separat leveringsstatus i [STATUS](../../../../Docs/Active/STATUS.md).

## Produksjon: SARO og Sierra Motor Court

[build_production.py](build_production.py) lager 17 native materialvarianter fra faktiske kildegrafer under `/Game/Signal47/Art/LookdevProduction`. De er med i kilde og cook-oppsettet. Én 128² lineær maske gir forsiktig UV-basert slitasje og glansvariasjon; dette er ikke geometrisk kantdeteksjon. Originale teksturer, normalgrafer, UV-er og geometri beholdes. Kontorets motellatlas beholder BaseColor helt, slik at etiketter og trykk ikke recoloreres. CreamEnamel-normalen, som fire Office-grafer deler, følger med både som uendret kildeasset og én egen produksjonskopi. Dermed fungerer både spillet og regenerering fra en ren klon.

[lookdev-bindings.json](../../Config/Props/lookdev-bindings.json) velger eksakte kildematerialer og meshstier. `S47Lookdev` setter varianter på de aktuelle slottene, også når SARO-rom opprettes senere. Eksisterende dynamiske materialer hoppes over, slik at aktive strøm-, skjerm- og fargedrivere beholder pekerne sine. Room6s levende glass, tekst/skjermer og emissive materialer er unntatt. Manglende variant beholder originalmaterialet.

Bindingsreglene omfatter også kontorets eksplisitte CPU-mesh. Renderkopier laget av `S47SaroLegacyCleanup` følger en registrert opprinnelig assetsti; et generelt navn på en midlertidig mesh er ikke nok til å bli valgt. De to lookdev-konfigurasjonene leses av sine egne systemer og inngår ikke i rekvisittplasseringens skjema-/selvtest.

[lookdev-environment.json](../../Config/Props/lookdev-environment.json) gir separate dag-/nattverdier til eksisterende himmel, sol, eksponering og native tåke. Profilen gjelder nær SARO/motellet med myk geografisk overgang. Solretning og vær kommer fortsatt fra spillets klokke. Brukerens eksponering legges til, og tåke Off/Low/High beholder sine kvalitetsgrenser. FSSS brukes bare ved natt og tåke satt til High (Ultra-preset); vanlig High-preset bruker fortsatt sin lettere tåke. Ingen ekstra himmel- eller tåkeaktør opprettes. SARO-veggarmaturer har korrigert lysretning og kildestørrelse, med samme candela, rekkevidde og strømkurser.

Regenerering gjøres i et ledig, reservert motorvindu med prosjektets eksisterende `unreal_guard.py`, ved å kjøre `UnrealEditor-Cmd <uproject> -run=pythonscript -script=<absolutt sti til build_production.py> -unattended -nullrhi`. Rapporten går til prosjektets private `Saved/LookdevProduction/build-report.json`, eller `S47_LOOKDEV_PRODUCTION_REPORT`. Godkjente bindingsrader vedlikeholdes eksplisitt i konfigurasjonen; generatoren endrer ikke spillets konfigurasjon eller verdensplasseringer.

`-S47LookdevOff` gir samme spill med opprinnelige materialer, miljøverdier og vegglampeoptikk for før/etter. Opt-in `S47GraphicsReview` skriver native material-/miljøavlesning sammen med bildet. Dette er utviklingsverktøy, ikke nye innstillinger i spillerens meny.

## Filer

| Skript | Funksjon |
| --- | --- |
| [materials.py](materials.py) | Eksakt objekt-/materialslot, private materialkopier, selektiv roughness/metallic, bevart farge-/normalgraf, maskert overflateslitasje, bump og glass. |
| [lighting.py](lighting.py) | Egen dag-/nattrigg, sky illumination, sol-/måneretning, eksponering, praktiske lys og avgrenset volumetric scattering/fog. |
| [baking.py](baking.py) | Baker den faktiske materialgrafens base color/roughness/metallic/normal til eksisterende UV-er; bruker prosjektets eksisterende shaderkartlegging i `interiors/build_owned.py`. |
| [pipeline.py](pipeline.py) | Import av Tripo-GLB eller Blender-fil, objektinventar, kildevern, prøvescene, før/etter, lagring i ny Blender-fil og valgfri FBX. |
| [launch.py](launch.py) | Starter Blender gjennom prosjektets eksisterende vern mot samtidige motor-/spilljobber og timeout. |
| [unreal_setup.py](unreal_setup.py) | Leser manifest, lager først importplan, deretter valgfritt native PBR-/glassmaterialer og et separat lys-/tåkekart i egen Lookdev-mappe. |
| [build_production.py](build_production.py) | Lager avgrensede SARO-/motellvarianter med opprinnelige grafer, kildevern og eksplisitte bindingsrader til runtime. |
| [example.json](example.json) | Redigerbar materialoppskrift med separate dag-/nattprofiler og uttrykkelige Unreal-lysverdier. |

Ingen MCP, API-kreditter eller ny Tripo-generering kreves for å behandle en eksisterende modell. Originalmodeller, UV-er, rigger, transformasjoner og personlige Saved bevares. Private preview-resultater ligger utenfor Git; de eksplisitt integrerte produksjonsmaterialene versjoneres med spillet. Preview-skriptene avviser eksisterende resultatmappe fremfor å overskrive et tidligere arbeid.

## Kjør prøvescenen først

Fra repoets rot, med ledig motorvindu:

```sh
python3 unreal/Signal47/Tools/lookdev/launch.py -- \
  --demo --config unreal/Signal47/Tools/lookdev/example.json \
  --output "$HOME/scratch-bm/Signal47-lookdev-my-day" --profile day \
  --before-after --bake --bake-size 1024 --export-fbx --self-test

python3 unreal/Signal47/Tools/lookdev/launch.py -- \
  --demo --config unreal/Signal47/Tools/lookdev/example.json \
  --output "$HOME/scratch-bm/Signal47-lookdev-my-night" --profile night --preview
```

Dette bruker en liten original prøvescene, ikke modeller som er produsert for spillet. Resultatet er `before.png`/`after.png`, ny `.blend`, manifest, teksturkart ved `--bake` og FBX ved `--export-fbx`. Kameraet er det samme før/etter. For en kildefil må et eksisterende kamera være valgt for preview; verktøyet flytter ikke produksjonskameraet. Cycles CPU med fire tråder og lavt sampleantall er standard for raske prøver.

## Bruk en Tripo-/Blender-modell

1. Eksporter den riktige eksisterende Tripo-modellen som GLB med teksturer, eller bruk en kopi av original `.blend`. Bevar råeksport og eventuell originalrigg. Ingen endring av eide modeller i offentlig Git.
2. Kjør `--input /absolutt/modell.glb --inventory-only` i stedet for `--demo`, med ny resultatmappe. `inventory.json` viser faktiske objektnavn, dimensjoner, UV-er og materialslotter.
3. Kopier eksempeloppskriften til et privat arbeidsområde. Bytt `materials[*].object` og `slot` til de faktiske målene. Ikke bruk en global roughness-/metallendring for alle objekter. Bruk `refine.mask_image` for eksempelvis eksponert folie; `channel` velger R/G/B/A. Masker lastes som private Non-Color-bilder.
4. Kjør oppskriften på originalfilen til en ny resultatmappe med `--bake --export-fbx`. Den nye Blender-filen beholder redigerbare noder. Den kan referere til originalens eksterne teksturer; bevar disse ved flytting av arbeidsområdet.
5. Kontroller den konkrete modellen og alle LOD-er i Unreal før den bytter ut et fungerende spillobjekt. Dette verktøyet utfører ikke automatisk LOD-generering, retopologi, ny rigging eller flytting av spillankre.

Oppskriften krever meterbasert Blender-scene (`scale_length=1`). Enheter må klargjøres uttrykkelig på en separat kildekopi. UV-baking bruker eksisterende aktiv UV-kanal innenfor 0..1. Overlappende UV-er/UDIM trenger en egen atlasplan; overlapp blir ikke automatisk rettet eller erklært feilfri. Avanserte mixed-/gruppeshadere som materialforbedringen ikke forstår avvises fremfor å bli ødelagt. Opaque baking gjenbruker den mer omfattende eksisterende shaderkartleggingen.

## Materialer og shaders

`kind: surface` beholder originalens teksturkoblinger med mindre en oppgitt roughness-/metallicverdi erstatter akkurat den kanalen. Med maske blandes ny verdi bare inn i det valgte området. Slitasje er en deterministisk prosedyre med farge-/ruhetsvariasjon og mikrobump over den opprinnelige normalen. Dette er ujevn overflateslitasje, ikke en påstand om geometrisk beregnet kantavskalling. `wear.mask_image` begrenser slitasjen til en kunstnerstyrt maske. Skala og bumpavstand er i meter.

`kind: glass` har `mode: pane` for en tynn glassflate med Fresnel/transparens/refleksjon, og `mode: solid` for transmisjon, IOR og valgfri volumabsorpsjon i faktisk lukket geometri. `thickness` er eksportinformasjon; det lager ikke tykkelse på en flat modell. Pane får ikke falsk volumabsorpsjon. Glass trenger egen materialtolkning i spillmotoren og bakes ikke til et ugjennomsiktig PBR-kart.

Hver materialregel lager en separat objektbundet kopi. Originalmaterialet beholdes, og endret oppskrift bygges fra den bevarte grunnversjonen i stedet for å legge stadig flere slitasjenoder over hverandre. Ingen mesh-, normal-, UV- eller riggoperasjon inngår i materialpasset.

## Himmel, lys, tåke og natt

`lighting.day` bruker fysisk skytekstur, samlet solretning og egne eksponeringsverdier. Solretningen er azimut med klokka fra Blender +Y/nord og høydevinkel i grader. Blender 5.2s skyegenskaper kontrolleres ved kjøring; en manglende egenskap gir feil, ikke stille reserveadferd.

`lighting.night` bruker en separat svak verdensbelysning, måne og navngitte lokale AREA/POINT/SPOT-lys. Daglyset gjenbrukes ikke som natt med bare lavere eksponering. Profilebytte skjuler den andre administrerte riggen. Eksisterende kildelys beholdes; `mute_existing_lights: true` er et uttrykkelig og reverserbart valg for en separat lookdev-fil.

Tåken er et avgrenset volum med `Volume Scatter`, anisotropi og valgfri absorpsjon. Volumet har ingen ugjennomsiktig Surface-forbindelse og merkes som ikke vanlig eksportgeometri. Bruk moderate tettheter. Blender har W/W·m⁻² for disse lampene; Unreal krever egne lux-/lumenverdier. Eksempelverdiene er utgangspunkt for prøvescenen, ikke godkjent lysregi for hele spillet.

## Native Unreal-oppsett

En FBX overfører ikke disse prosedyrenodene, verdenslyset eller volumet. Fire bakte PBR-kart overføres separat. Normalene er DirectX tangent/Y-minus, fargekart er sRGB, data-/normalkart er Non-Color. Manifestet er sannheten for teksturroller; filnavn gjettes ikke.

```sh
# Bare plan, ingen editor/import/endring:
python3 unreal/Signal47/Tools/lookdev/unreal_setup.py \
  --manifest /privat/resultat/manifest.json --profile night
```

I et separat editor-/commandlet-vindu kjøres skriptet med `S47_LOOKDEV_MANIFEST` satt til manifestets absolutte sti. `S47_LOOKDEV_APPLY=1` tillater import til `/Game/Signal47/Lookdev/<name>`. `S47_LOOKDEV_PREVIEW=1` lager/åpner kun det egne `Maps/Preview_day` eller `Preview_night`; lagre andre editorkart først. `S47_LOOKDEV_PROFILE=day|night` velger profilen. Uten APPLY er det bare en plan. Ingen automatisk materialtilordning til spillets meshes. FBX bruker midlertidige eksportkopier; manifestets `fbx_objects` og FBX-egenskapen `s47_source_object` kobler eksportnavn til opprinnelig objekt/slot.

Adapteren lager native ugjennomsiktige PBR-materialer og glass med Fresnel, roughness, tint og IOR. Solid-glassets absorpsjon er en uttrykkelig referansetykkelse-tilnærming, ikke identisk Blender-transport. Den separate miljøprøven bruker SkyAtmosphere, SkyLight, DirectionalLight, praktiske lys, manuell eksponering og ExponentialHeightFog med volumetrisk scattering. Unreal-tåken er ikke den samme avgrensede Blender-boksen; utseende og kostnad må vurderes i Unreal. Mangel på uttrykkelige native lys-/tåkeverdier blokkerer preview i stedet for å gjette enhetskonvertering. Ubakte prosedyrematerialer blokkerer import fremfor å bli flate reservefarger.

Spillets godkjente himmel, tidsstyring, SARO-strøm, nattlys og kvalitetsvalg skal fortsatt styre den faktiske verdenen. Ved produksjonsintegrasjon må relevante materialslotter og eksisterende lysdrivere kobles til det nye oppsettet. Ikke legg denne previewriggen oppå spillets egen sol/himmel og kall det integrert.

## Kontroll og kilder

Blender 5.2.2 er prøvd med materialbevaring, gjentatt kjøring, dag/natt-bytte, åtte 512² PBR-kart, FBX og samme-kamera før/etter samt nattpreview. Native UE 5.8.4-NullRHI-prøve har opprettet fire materialer, åtte teksturer og egne dag-/nattkart uten editorfeil. En eksisterende H31-Tripo-GLB er prøvd gjennom import, materialpass, fire kart og FBX med bevart geometri.36 faktiske Blender-materialkontroller og ni rene importplankontroller består. NullRHI er ingen grafisk spillprøve. Gjenstående visuell spillaksept oppdateres i [STATUS](../../../../Docs/Active/STATUS.md). Rene importplankontroller kjøres med `python3 -m unittest discover -s unreal/Signal47/Tools/lookdev -p 'test_*.py'` fra repoets rot. Ingen C++-endring eller ny spillpakke inngår.

Prosjektbiblioteket ble søkt først. `arjun988/blender-skills` (festet `8f778d24`, UNVERIFIED i biblioteket) ga referanser for arbeidsdeling; ingen skill-/MCP-installasjon. Bibliotekets dokumenterte Cycles CPU-metode og prosjektets eksisterende `interiors/build_owned.py`, FBX-aksevalg og `Scripts/unreal_guard.py` er gjenbrukt. [Blenders Principled-dokumentasjon](https://docs.blender.org/manual/en/latest/render/shader_nodes/shader/principled.html), [skytekstur](https://docs.blender.org/manual/en/latest/render/shader_nodes/textures/sky.html), [Volume Scatter](https://docs.blender.org/manual/en/latest/render/shader_nodes/shader/volume_scatter.html) og [Epics volumtåke](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-fog-in-unreal-engine) forklarer forskjellene mellom DCC og runtime. API-er er kontrollert mot installert Blender/UE, ikke bare pseudokode fra videoen.
