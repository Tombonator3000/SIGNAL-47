# Radio-kvinnens CPU-forberedelse

Root kjører `prepare_cpu.py` i prosjektets eksisterende editor gjennom `unreal_guard.py require-idle` og egen tungreservasjon. Ingen jobb startes av dette kildebidraget. Prosjektet bruker eksisterende Python-editorinngang `-ExecutePythonScript=<absolutt sti til prepare_cpu.py>`; øvrig editorstarter og logg eies av root.

Før mutasjon kreves den festede WebData-manifesten og originale `geo_008.glb`, n1484/g1031/m641 under n1480, én faktisk LOD med 108 vertexer/36 trekanter, original `slot0`-materialbinding, RT på og Nanite av. Bare `allow_cpu_access` lagres. Alle `g1031.*`-filer kopieres og SHA256-kontrolleres i en ny privat backupmappe først. Tidligere backup overskrives aldri; ingen automatisk tilbakeføring.

Standard privat resultat er `/home/tom/scratch-bm/Signal47-mannequins-20261029/private/cpu-r1/{original/,cpu-report.json}`. Velg ny kjøring med `S47_RADIOWOMAN_CPU_DIR`; den må ligge under `S47_RADIOWOMAN_PRIVATE_ROOT` (standard den private mappen over) og utenfor Unreal-prosjektet. `S47_RADIOWOMAN_SOURCE_GLB` kan peke på en annen byteidentisk kildekopi. PASS-markør: `S47_RADIO_WOMAN_CPU_PASS`.

Runtime bruker den faktiske CPU-geometrien til å beholde de 12 radiotrekantene i g1031 og trekke fra de 24 skotrekantene. Originalt metallpanel g1032 beholdes helt. Pakkede vertex-/normal-/UV-/fargebuffere og originale materialpekere kopieres uten ny kvantisering. Den avledede radiovisningen får ingen kollisjon fra den gamle skoens BodySetup; proxy, radioens metallpanel og benk beholder sine eksisterende funksjoner. Bare de fire rene kroppsbarna skjules etter komplett preflight og ressursinitialisering.

Kandidat velges før verdensbygging med `-S47RadioWomanAsset=/Game/MetaHumans/Mannequins/RadioWoman/BakeR1/SM_RadioWoman_Seated` eller `s47.RadioWoman.Asset`; `-S47RadioWomanOff` beholder hele originalen. Manglende CPU-data, kandidat, skjelettavledet statisk LOD-inventar eller fysisk benkesete gir hel originalfallback. Faktiske native pose-, hånd/radio-, fot-, LOD-, interaksjons-, nattregel- og disk/Continue-bevis er **UNVERIFIED** inntil roots målrettede motorprøve; strukturell CPU-PASS er bare et internt delsteg.
