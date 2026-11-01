# Privat Tripo-pumpe til Caliche

Gjenbruker prosjektets `tripofleet`-funksjoner uten å endre Jeep-/fleet-filene. GLB, teksturer, Blender-filer, FBX, manifest og native kandidater holdes privat. Skriptene starter ikke Blender eller Unreal selv. Kjør tunge jobber enkeltvis gjennom prosjektets reservasjon, med minst 100 GiB ledig etter store operasjoner.

## Rekkefølge

1. `probe.py` kjøres i Blender med `--input /private/pump.glb --out /private/probe`. Skriver originale mål, materialer, redigerbar kildekopi og fire 512px sidebilder. `--resume` bruker den lagrede kopien. Bruk `--python-exit-code 1` på Blender.
2. Lag privat `sources.json` etter formatet i `../tripofleet/README.md`. Bruk én modell med id `caliche_regular_pump`, navn `SM_Tripo_CalicheRegularPump`, og eksplisitte `source_forward`/`source_up`. Legg til `station_source` (original egen stasjons-`.blend`) og `station_placement` (dens `placement.json`).
3. Lett kontroll: `python3 prepare.py --sources /private/sources.json --validate-only`.
4. Under reservasjon: `blender -b -t 2 --python-exit-code 1 --python prepare.py -- --sources /private/sources.json --stage all`. `pump` og `station` kan kjøres hver for seg. Originale PBR-kart, UV-er og høyoppløst kilde bevares. Reduserte LOD-er måles etter faktisk reduksjon.
5. Native editorjobb med GeometryScripting: sett `S47_PUMP_MANIFEST=/private/prepared/manifest.json`, `S47_PUMP_APPLY=1`, valgfritt `S47_PUMP_STAGE=pump|station|all`, og kjør `import_assets.py`. Ingen original asset endres.

Den inspiserte pumpen er Z-opp og har front mot Blender -Y. Høyde-fit 2,10m, groundpivot og tre LOD-er gir samme koordinatramme i alle avstander. Native plassering har yaw 90 grader etter fleet-importens målte koordinatkonvertering.

`prepare.py` klipper bare REGULAR-pumpen ut av en kopi av den egenlagde Station-meshen. Kun hele flater innenfor et smalt målt volum fjernes. Betong/øykant beskyttes, pivot og fullstendige stasjonsbounds må være identiske; de separate dør-, skilt- og lysdelene røres ikke. Kandidatens eventuelle fjern-LOD-er kommer fra den klippede meshen. Native import måler rendergeometri/bounds/trekanttall og gjenbruker originalens materialer etter slotnavn.

Runtime beholder eksisterende `town:pumps`, `ff:pump`, parkeringsavstand, tank, betaling og saveformat. Bare Freeform bytter til kandidatene. Normal stopp gjenoppretter original stasjonsmesh og dens materialbindinger. `TripoPumpLoaded()` krever de to faktiske kandidatassetene, slik at fallback ikke kan godkjennes som ny modell i native prøve. Native nær-/fjernbilder og pumpebruk er separat fra Blender-proben.
