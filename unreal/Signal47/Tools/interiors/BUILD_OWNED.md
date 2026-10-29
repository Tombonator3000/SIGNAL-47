# Private originalmodeller for interiørpasset

**Status:** Under arbeid. **Gjelder:** Fire valgte Vintage-originaler. **Sist kontrollert:** 29. oktober 2026.

`owned_sources.py` validerer originalfiler, historisk inventar og lisensnotis uten motorjobb. Første Blender-kjøring R1 stoppet før første ferdige objekt fordi en støvgruppe tok imot shaderen gjennom `NodeGroupInput`. R1-rapport og logg er bevart privat. Oppskriften følger nå denne inngangen til riktig ytre gruppesocket gjennom eksplisitte gruppegrenser og interface-ID-er. Rettet Blender-/bake-/FBX-resultat, native og visuell aksept er fortsatt UNVERIFIED inntil ny kjøring.

Kjør først vanlig Python:

```sh
python3 unreal/Signal47/Tools/interiors/owned_sources.py
```

Neste kommando er selve Blender-jobben og skal bare kjøres gjennom prosjektets eksisterende reservasjon/guard, når andre tunge jobber og Toms spilløkt er avsluttet:

```sh
blender --background --factory-startup -Y --python unreal/Signal47/Tools/interiors/build_owned.py -- --output /home/tom/scratch-bm/Signal47-interiors-20261029/owned-bake-r1
```

Målmappen må være tom og ligge utenfor Git-arbeidskopier og originalpakken. Ingen API, kjøp, prosessoppstart eller motorimport finnes i oppskriften. Originalene lagres aldri tilbake.

Oppskriften appender bare de fire navngitte objektene og sjekker de faktiske objektene og teksturene igjen. Originale UV-sett, inkludert eskens `Overlay`, brukes uendret på en evaluert kildekopi. En separat triangulert målkopi får egen atlas-UV. Det gjøres ingen automatisk decimering av disse til sammen omtrent 19 000 kildetrekantene.

Basefarge, ruhet og metallandel hentes fra de opprinnelige Principled-inputene via emisjon i egne materialkopier. Dermed påvirker ingen scenelamper eller separat AO-multiplikasjon basefargen. Vektede shaderblandinger og nøstede shadergrupper beholdes. Normalpasset bruker originalgrafen med bump og baker DirectX-tangentnormaler. Ukjente shadergrener, alpha og transmisjon stopper med konkret feil; de flates ikke stille ut.

Private resultater: `Export/*.fbx`, fire teksturer per objekt, `materials.json`, `model_contract.json` og hashbundet `build-report.json`. FBX bruker samme metriske akser som eksisterende `fieldintegration/import_recipe.py`. Bottom-centre-pivot er utledet fra den eksporterte geometrien. Kassetten beholder originalens størrelse inntil fysisk mål er kontrollert.

**Native avhengighet:** Atlasmaterialet trenger `textures.metallic`. Eksisterende `fieldintegration/import_recipe.py` støtter bare metallandel som konstant. Native import må eksplisitt støtte dette teksturkallet før disse materialradene brukes; ellers blir kabelens plast og krom feil. `required_native_features` oppgir kravet i begge kontraktene. Normalstyrke er satt til 1 for den ekte bakeresultatet, med DirectX-normal og ingen grønnkanalvending. Native import og spillaksept er separate, gjenstående steg.

Prinsippene for append og pivot kommer fra prosjektets historiske `web/src/assets/licensed/tools/build_models.py`. Den gamle web-oppskriftens diffuse/AO-baking, metallandel 0 og forenklede GLB-er gjenbrukes ikke som produksjonsresultat.
