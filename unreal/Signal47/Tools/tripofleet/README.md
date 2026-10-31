# Eksisterende Tripo-modeller til UFO-/bilprøven

**Status:** Gjeldende. **Gjelder:** UE5.8.4, Linux, private Tripo-kilder. **Sist kontrollert:**31. oktober2026.

Klargjør Toms teksturerte GLB-originaler i Blender og skriver tre målte statiske LOD-er for Unreal. Skriptet lager ingen modell, hjul, kupé, kollisjon eller spillinteraksjon. Sammenbygde hjul blir ikke skåret løs eller erstattet med falske deler. Råmodell, avledede filer og den private konfigurasjonen skal ligge utenfor offentlig Git.

**Mens Tom spiller: bare kildearbeid og `--validate-only`.** Blender, import i Unreal, bygg og grafiske prøver venter. Kjør én tung jobb om gangen gjennom prosjektets vanlige vakt. Skriptet starter aldri motoren selv.

## Privat konfigurasjon

Lag `sources.json` utenfor checkout. Dette er et format-eksempel; sti, mål og retning må velges etter faktisk kildeinspeksjon. Eksempelmålene er ikke en måling av Toms jeep.

```json
{
  "version": 1,
  "output_root": "/private/tripo-fleet/prepared",
  "models": [
    {
      "id": "classic_jeep",
      "name": "SM_TripoFleet_ClassicJeep",
      "label": "Tom's classic jeep",
      "source_glb": "/private/tripo-fleet/raw/classic-jeep.glb",
      "source_forward": "-Y",
      "source_up": "+Z",
      "target_dimensions_m": [1.85, 4.0, 1.8],
      "fit": "uniform",
      "fit_axis": "length",
      "pivot": "ground",
      "lod0_max_triangles": 65000,
      "lod_ratios": [1.0, 0.5, 0.2],
      "lod_bounds_tolerance": 0.08
    }
  ]
}
```

`source_forward`/`source_up` beskriver geometrien **etter vanlig glTF-import til Blender**, med ortogonale akser fra `+X`, `-X`, `+Y`, `-Y`, `+Z`, `-Z`. De angir en fysisk retning; skriptet gjetter ikke front fra symmetriske UFO-bounds. `target_dimensions_m` er ønsket `[bredde, lengde, høyde]`. Standard `fit: uniform` bruker bare `fit_axis` (`width`, `length`, `height` eller `longest`) for skalafaktoren og bevarer proporsjonene. Faktiske øvrige mål føres i kontrakten. `fit: exact` er et uttrykkelig valg om ulik skalering langs aksene.

`pivot: ground` legger hele modellens laveste geometri på Z=0 og sentrerer XY, mens `center` sentrerer alle tre akser. Ground-pivot er geometrisk; den beviser ikke hjulkontakt på terreng. UFO kan bruke center. Samme normalisering og pivot brukes på alle LOD-er. `lod_bounds_tolerance` tillater maksimalt oppgitt andel av LOD0-utstrekningen per bounds-side, med 1cm gulv. For stor silhuettendring avvises og krever høyere forholdstall eller manuell vurdering.

Valgfri `lod0_max_triangles` setter et uttrykkelig budsjett for den klargjorte LOD0-kopien. Uten feltet beholdes originalt trekanttall i LOD0. Eksempelets 65 000 er et prøvebudsjett, ikke godkjent kvalitet. Når råmodellen overstiger budsjettet, reduseres bare arbeidskopien med Decimate etter at `original.blend` er lagret. Faktiske trekanter telles etter reduksjon og validering; ved behov tillates én målt korreksjon før for høyt antall avvises. LOD1/2 lages deretter uavhengig fra den ferdige LOD0-kopien, etter `lod_ratios`. Dermed betyr `[1,.5,.2]` prosent av den klargjorte LOD0, ikke av råmodellens million trekanter. Samme bounds-/UV-/materialkontroll gjelder også reduksjonen til LOD0. `lod0_preparation` i modellkontrakten viser kildeantall, faktisk LOD0-antall, budsjett og eventuelle reduksjonstrinn. Original PBR og høyoppløst `.blend` bevares.

ID er unikt, maksimalt 40 ASCII-tegn og starter med liten bokstav. Navn starter med `SM_`, er unikt og maksimalt 63 ASCII-tegn. Råkilden skal ikke ligge under `output_root`. En eksisterende `original.blend` gjenbrukes uten overskriving når kildehash stemmer; endret råmodell krever ny privat output-mappe.

## Kjøring

Lett kontroll leser GLB-header, materialsemantikk, innbakte bildereferanser og konfigurasjon. Den importerer verken `bpy` eller `numpy` og oppretter ingen output-filer:

```sh
python3 prepare.py --sources /private/tripo-fleet/sources.json --validate-only
```

Når spilløkten er ferdig, kjør den vanlige Blender-vakten rundt:

```sh
blender -b -t 2 --python prepare.py -- --sources /private/tripo-fleet/sources.json
```

`--only classic_jeep,police_car` velger konfigurerte ID-er. Ingen nedlasting, betaling, nettverkskall, ny Blender-installasjon eller automatisk Unreal-kjøring følger med.

## Bevarte kilder, materialer og eksport

Hver modell får følgende filer under `<output_root>/<id>/`:

- `original.blend`: importert original med originale navn, hierarki, UV-er, materialnoder og pakkede bilder. Lagres før normalisering og reduksjon; rå GLB endres aldri.
- `original-source.json`: kobling til råfil, Blender-versjon og originalobjekter. Dette er metadata for å beskytte den redigerbare originalen mot feil kilde ved gjenkjøring.
- `prepared.blend`: separat normalisert modell og tre LOD-objekter. LOD1/2 er skjult i forhåndsvisning; originalen ligger i sin egen fil.
- `exports/<name>.fbx`, `exports/<name>_LOD1.fbx`, `exports/<name>_LOD2.fbx`: samme nulltransformasjon, materialrekkefølge, UV-er og koordinatramme.
- `textures/`: bytekopier av de opprinnelige innebygde PNG-/JPEG-bildene, med uendret oppløsning og uten ny komprimering, grønn-invertering, baking eller ny UV-projeksjon.
- `materials.json`: originale glTF-faktorer og semantiske teksturreferanser. BaseColor-faktor er lineær. Pakket MR bruker G=roughness og B=metallic, hver med opprinnelig faktor. AO bruker R og sin styrke. Emisjon har eget kart, faktor og styrke. Normaler er merket `OpenGL`; eventuell grønn-invertering til Unreal skjer bare i den native importeren. MASK/BLEND, alpha cutoff, alpha og two-sided bevares i sidecar.
- `model_contract.json`: faktisk trekanttall, bounds, mål, UV-/materialnavn, brukte materialer og 12 asymmetriske geometriprober per eksportert LOD. LOD-er må gi positive og strengt fallende trekanttall.

Originalt materiale i `.blend` beholder glTF-nodene. Sidecar er nødvendig fordi FBX alene ikke overfører glTF-PBR-semantikken pålitelig. UV-transformer, alternative tekstur-UV-er, materialutvidelser som transmission/clearcoat, teksturkomprimeringsutvidelser, vertexfargemultiplikasjon, rigg og animasjon avvises med konkret feil. De blir ikke lydløst flatet ut. Understøttet materialutvidelse er `KHR_materials_emissive_strength`. Importerte, teksturløse materialfaktorer er tillatt når geometrien har original UV0; manglende materialer/UV-er avvises.

Alle LOD-er måles **etter** faktisk reduksjon, `mesh.validate` og fjerning av ubrukte kontrollpunkter, ikke fra ønsket prosent. Oppryddingen bruker `delete_loose` med sletting av flater av, og krever uendret antall overflatepolygoner. Geometriprobene velges bare blant polygonenes faktiske hjørneindekser, slik at et løst punkt som ikke kan tegnes i Unreal ikke blir brukt som renderbevis. Normale meshmodifikatorer evalueres til egne kopier, kildemeshene endres ikke. Reduksjonen er Blender Decimate; ingen automatisk reparasjon av silhuetten, nye interne deler eller topologisk hjuldeling påstås.

Jeepens faktiske seter og kupé er beholdt. `glass_regions` angir inspiserte vindusflater i den ferdig normaliserte Blender-rammen. Bare hele rutetriangler får egen materialslot med tynn alpha-rute, 12% opasitet og lav ruhet. Det er et separat rutemateriale, uten temporal maskering av karosseriet. Det er ikke volumetrisk brytningsglass. Original kroppstekstur, UV, normalmap og opaque-lakk endres ikke. `lamp_regions` markerer bare eksisterende front-/baklykter med R/G-hjørnefarger; valgfri `ellipse_xz` avgrenser runde linser. Native materialet legger de styrte lampeparameterne til den opprinnelige emisjonsgrenen. Ingen helbil-emisjon brukes. De små, faktisk fargelagte LOD0-linseflatene beholdes identisk i lavere LOD-er mens resten reduseres; trekanttall og materialslotter måles etter sammenføyningen.

`preview.py` gir enkle CPU/PBR-bilder av råkilde eller ferdig modell, med originale materialer og opptil 480px som standard. Kjør gjennom den samme motorvakten; det er en tung jobb. For Jeep kan `--stage prepared --only classic_jeep --views overview,front,side,driver` kontrollere den faktiske, konfigurerte førerplassen. Råkilde viser front -Y etter glTF-import; ferdig eksport har front +Y for å passe prosjektets målte FBX-konvertering.

`split_patrol_door.py` kjøres etter `prepare.py`, før native import. Den skiller ut de inspiserte ytre førerdørflatene fra hver faktisk politi-LOD med samme profil og hengsel. Karosseri og dør er en komplementær deling av de opprinnelige flatene, med kilde-UV/materialer; ingen dørinnmat modelleres. Dørens bakside bruker samme kildemateriale tosidig. Original GLB og `original.blend` forblir uendret. Åpen dør må vurderes visuelt i Unreal fordi Tripo-kilden ikke har en egen forfatterrigget dør.

## Kontrakt mot native import

Toppen `<output_root>/manifest.json` har `version`, `coordinates` og `models`. Hver modellrad har `id`, `name`, absolutt `output_dir`, relative `contract`/`materials`, `source_glb`, `source_sha256` og `status`. Modellkontrakten har `parts[meshnavn]` med `asset_folder`, LOD0-målinger og `lods` med nøyaktig LOD0, 1, 2. `fbx` i hver LOD-rad er relativ til modellmappen. `materials` er alle bevarte slotter; `used_materials` er bare de som faktisk har flater i det nivået. Manifestet publiserer modellen først når alle tre eksportene er ferdige.

Blender: meter, X=bredde, Y=lengde, Z=opp, front=+Y. FBX: `axis_forward=-Y`, `axis_up=Z`, metriske enheter. Følg eksisterende `fieldintegration/import_recipe.py`: legacy FBX, `force_front_x_axis`, scene-/enhetskonvertering og import-yaw +90°. Den etablerte native konverteringen er `UEcm=(Bx,-By,Bz)*100`, altså front -Y i Unreal og front -Z i spillets Three-koordinater. Alle tre LOD-er skal gå gjennom samme importer. Ikke gjenbruk en annen aksekonvertering bare fordi materialene ser riktige ut.

Spillets verdenskoordinater bruker derimot `S47Three::ToUE(X,Y,Z)=(100X,100Z,100Y)`. FBX-lokalaksenes fortegn skal ikke brukes på verdensfotavtrykk eller sceneankre. En ferdig mesh har dermed runtime Three=`(Bx,Bz,-By)` før eventuell verdensrotasjon.

Generert kontraktstatus er `PREPARED_NATIVE_UNVERIFIED`. Native import må kontrollere **hvert faktisk render-LOD** mot egne bounds/prober og egne trekanttall. Endelig visuell prøve må dekke front, side, terrengkontakt, teksturer, LOD-skifter og eventuelle cockpit-/interaksjonsankre før en spillmodell erstattes. Disse skriptene endrer ingen eksisterende spillanker, plassering, bilinnmat eller gameplay. Lisens-/publiseringsvurdering av de faktiske Tripo-kildene og derivatene tilhører leveransen; råfiler og Blender-/FBX-/teksturderivater holdes private.

## Gjenbruk og prøvestatus

Bruker prosjektets målte FBX-oppsett fra `fieldintegration/import_recipe.py` og flåte-/PBR-prinsippene i `sarofleet/build_fleet.py` samt originalbevaringen i `triponpcs/prepare_roswell.py`. Bibliotekssøk fant [lod-pipeline](https://github.com/arjun988/blender-skills/blob/8f778d2405a214b508d4c7d80742be8e43acdd52/.claude/skills/lod-pipeline/SKILL.md): separat LOD0-grunnlag, stabile materialslotter og silhuettkontroll er tilpasset. Referansen er katalogisert som UNVERIFIED for vår stack; ingen MCP-installasjon eller fremmed skriptkode er kopiert.

Fem faktiske Tripo-kilder er eksportert med Blender og importert til UE5.8.4: jeep, trekkvogn, politibil, trekant-UFO og tallerken. Samlet originalgeometri 3 769 015 trekanter er redusert til 364 631 i LOD0, med to ytterligere avstandsnivåer. Native import, tre LOD-er, kilde-PBR og faktiske Vulkan-bilder består. Politidøren er separat geometri med bevart hengsel. Runtime bruker jeep på serviceetappene, original truck i sideparkeringen, trekkvogn ved diner og ny politibil. De to UFO-ene kan vises/fjernes under F10 «Tripo craft preview».

Jeepens originale hjul er sammenbygget med resten av modellen og ruller ennå ikke separat. Ingen feil hjulsett er lagt utenpå. Kjørbar prøve, faktisk førerutsikt og levende instrumenter er sett; naturlig menneskelig kjørefølelse gjenstår å vurdere. Private native assets finnes under `Content/Licensed/TripoFleet`; andre arbeidskopier må få denne lokale mappen før de pakker, ellers bruker runtime eksisterende reservekjøretøy. Råfiler, Blender-kopier og teksturer forblir private.
