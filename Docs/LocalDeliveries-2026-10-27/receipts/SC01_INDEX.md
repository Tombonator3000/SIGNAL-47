# SC01: kilde- og kvitteringsindeks

Dette er en innsamling av ferdig leveringsdokumentasjon, uten runtime-integrasjon.

| Spor | Eksakt kilde |
|---|---|
| Fysisk SC01 og scenariointegrasjon | [f935b237](https://github.com/Tombonator3000/SIGNAL-47/commit/f935b237180af1cd5c1aca31cf683dcbc4338b82) |
| Endelig testet kilde, ordinære LOD-er og RT-residens | [dbae3177](https://github.com/Tombonator3000/SIGNAL-47/commit/dbae31775d3690383417c0f3652de16fb8ac00ca) |
| Pakket aksept og dokumenterte begrensninger | [ba1441bb](https://github.com/Tombonator3000/SIGNAL-47/commit/ba1441bbcc9c9ba47a46508ab3128409750cf741) |
| Siste lokale kvittering | `a383c4bafd55b8dbc1e4029ed6d0c963944750dd`, `codex/story-sc01-receipt` |
| Tidligere PR | [PR 168](https://github.com/Tombonator3000/SIGNAL-47/pull/168), historisk merge `82f000547a6247ad6c920670860f7db5c6d922ac` |
| Bevarte eksterne kildegrener | [codex/story-sc01](https://github.com/Tombonator3000/SIGNAL-47/tree/codex/story-sc01), [unreal-port-core](https://github.com/Tombonator3000/SIGNAL-47/tree/unreal-port-core) |

Ved innsamlingen er `dbae3177` nåbar fra begge de eksterne grenene, men ikke fra `origin/main` `0ee7c248`. Den gamle grenens komplette tre skal ikke erstatte nyere terreng, armaturer, MetaHuman eller atmosfære. En senere integrasjon må kombinere kildene og kontrollere samlet bygg før ny standardinstallasjon. Ingen eldre SC01-payload er installert her.

Den opprinnelige [SC01-kvitteringen](SC01_RUNTIME.md) registrerer 1 501 selvtester, High/Low med 120 kontroller hver, 21 isolerte pakkekjøringer, 32 vurderte originalbilder og endelig RT-residens 79,8043 MiB. Dette gjelder den identifiserte SC01-pakken, ikke dagens kombinerte standard. Installasjon ble avvist før filbytte da spilleren startet NP1. Menneskelig lytting, emosjonell stemmevurdering, fysisk Xbox og ubrutt helnatt er ikke attestert.

Kildeleveransen omfatter original `unreal/ArtSource/HorrorSC01/`, native egne `Content/Signal47/HorrorSC01/`, `S47Horror`, `S47HorrorPack`, `S47HorrorTest`, `S47HorrorVisual`, HorrorStorys register-/stemmeverktøy og versjonert lagring. Endelig RT-import følger `dbae3177`.

SC02s lokale `sc02-preflight.md` er merket `READ_ONLY_PREPARATION_NOT_PRODUCTION`. Den er ikke ferdig produksjon og er ikke aktivert. [Historieplanen](HISTORIE_IMPLEMENTERINGSPLAN.md) er plan, ikke bevis på ferdige SC02–SC13.
