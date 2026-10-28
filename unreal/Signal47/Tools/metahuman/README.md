# MetaHuman-servitrisen

Gjeldende runtime ligger i `Source/Signal47`, ikke i scratch-mappens gamle `runtime`-utkast. Se [leveringsrapporten](../../../../Docs/Evidence/UnrealDeliveries/METAHUMAN_WAITRESS.md) før nye jobber.

- `prepare_runtime.py` importerer den originale uniformen på eksisterende kroppsskjelett og kontrollerer alle 43 ansiktstakes. Miljøvariablene `S47_UNIFORM_FBX` og `S47_METAHUMAN_REPORT` peker til private arbeidsfiler.
- `prepare_hair.py` lager et vanlig maskert, tosidig materiale for frisyrenes eksisterende hårkort. Materialet bruker UV0 og coverage-atlasets alfa. Det bruker ikke Groom-spesifikke vertex-attributter.
- `ArtSource/WaitressUniform/build_uniform.py` er gjeldende Blender-forfatterkilde. Eksakt MetaHuman-kropp, eksportert skjelett, FBX og fitting-scene er private kilder.
- `s47mh.py`, `mh_remote.py`, `phase.py` og shellskriptene er bevart historisk bootstrap-/Creator-verktøy. De starter ikke automatisk igjen etter Codex-overtakelsen.
- `integrate.py --apply` er sperret: den gamle løypa overskrev korrigert runtime, uniform og rapport fra utdatert staging. Den er kun bevart for lesende historikk. `uniform.py` er også en eldre kilde, ikke gjeldende uniform.

Tunge jobber kjøres sekvensielt med prosjektets `unreal_guard.py`. Lisensiert innhold blir i ignorert `Content/MetaHumans`; bare egen kode, dokumentasjon og eventuelle runtime-bilder hører hjemme i offentlig Git. Den eksisterende enkle figuren beholdes dersom nødvendige lokale assets ikke finnes.
