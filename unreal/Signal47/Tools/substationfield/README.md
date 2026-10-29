# Trafostasjonens feltutstyr og bakke

**Status:** Native kandidat under kontroll. **Gjelder:** FIX-07, Unreal 5.8.4. **Sist kontrollert:** 29. oktober 2026.

Claude har forfattet fire egne modeller: referansestolpe, testskap, kulvert og sammenhengende gradering ved servicefeltet. Blender-kilden er `Substation_Field.blend`, med tre FBX-LOD-er i `exports/`, modellkontrakt og gjenbyggbare Python-kilder her. `Preview/` inneholder Blender-bilder, ikke runtimebevis. Geometrien er original; eksisterende materialer fra prosjektets trafopakke brukes med sine uendrede krediteringer.

Modellene følger de målte bakkepunktene i `ground_probe.json`. De erstatter bare synlige forgjengere. T01/T02 beholder eksisterende interaksjonsproxies, kollidere, foto-occludere og lagringsdata. Manglende eller foreldede native modeller beholder hele den tidligere visningen. De nye modellene har ingen spiller- eller brukskollisjon.

Bygg, import og grafiske prøver kjøres enkeltvis gjennom prosjektets `unreal_guard.py require-idle` og en eid produksjonsreservasjon. Fra denne mappen brukes Blender `--background --python build_field.py`, deretter `preview.py` for kildebilder. `import_assets.py` kjøres som Unreal Python-commandlet med `S47_SUBSTATIONFIELD_REPORT` til en ny rapportfil. Den skriver bare `/Game/SubstationField`; materialforeldrene leses fra eksisterende `/Game/Substation`.

Fersk kontroll av alle faktiske lagrede LOD-vertices bruker `Tools/townintegration/verify_lod_geometry.py` med `S47_GEOMETRY_SOURCE_DIR` til denne mappen, `S47_GEOMETRY_ASSET_ROOT=/Game/SubstationField` og `S47_TOWN_LOD_REPORT` til en ny fil. Ingen import eller lagring inngår i denne kontrollen.

Native kandidat, testet kilde, pakket bygg og installasjon føres separat i leveringsrapporten. Cook-konfigurasjonen inkluderer eksplisitt `/Game/SubstationField`; dette er ikke i seg selv pakket aksept. Ingen installasjon eller offentlig release inngår i FIX-07-leveringen.
