# Gatelykt (cobra head), original Blender-kilde

Oppgave STREETLAMPS-20261108. En amerikansk gatelykt av typen «cobra head», slik den sto langs
riksveier og bygater i 1980-årene: støpt aluminiumshus, prismatisk dropp-refraktor under,
fotocelle med vridelås oppå og slipfitter bak, på en oppsvingt mastarm og en konisk galvanisert
stålstolpe. I tillegg finnes en kort arm for montering på en trestolpe (telefonstolpe).

**Kilde og lisens:** Dette er originalt arbeid laget for SIGNAL / 47. Geometrien er skrevet som
metrisk bpy-kode, og PBR-kartene er generert prosedyralt av `build_streetlamps.py`. Ingen
nedlastede modeller, bilder eller teksturer er brukt. Ekte armaturer fra perioden (GE M-400A og
American Electric 115, høytrykksnatrium) er bare brukt som beskrivelse av form og proporsjoner.
Ingen merkenavn, logoer eller tekst er modellert.

## Filer

- `build_streetlamps.py` skriver `SIGNAL47_StreetLamps.blend`, tolv FBX-filer i `Export/`
  (LOD0 og `_LOD1`/`_LOD2` for fire mesher), `textures/`, `materials.json` og `model_contract.json`.
  Kjør med Blender 5.2 og to CPU-tråder bak prosjektets jobbvern.
- `verify_source.py` åpner den lagrede fila på nytt, kontrollerer LOD-er, slots, lukket topologi
  med normaler som peker utover, pivoter og navngitte punkter, og leser FBX-ene tilbake. Resultatet
  skrives til `source-verification.json`.
- `preview.py` lager små Cycles-forhåndsvisninger i `Preview/`. Dette er Blender-bilder, ikke bilder
  fra spillet. Bakken, himmelen, solen og trestolpen er bare kontekst og eksporteres ikke.
- Import til Unreal gjøres med `Signal47/Tools/streetlamps/import_assets.py` til
  `/Game/Signal47/Art/StreetLamps`.

## Mesher og pivoter

Koordinater: Three `(x,y,z)` i meter med y opp, Blender `(x,-z,y)`, målt Unreal-import
`(x,z,y)*100`. Alle mesher har tre håndskrevne LOD-er i samme ramme og ingen ekstra
pivotkorreksjon. Eksakte mål, bounds og trekanttall står i `model_contract.json`.

| Mesh | Pivot | Retning |
|---|---|---|
| `SM_StreetLamp_Head` | senter av slipfitter-munningen bak, der armen går inn | nesen mot +X |
| `SM_StreetLamp_Arm` | stolpeaksen ved armens rot (senter av klammerbåndet) | armen mot +X, stiger mot +Y |
| `SM_StreetLamp_Pole` | midt under fotplata, y=0 er bakken | stolpen langs +Y |
| `SM_StreetLamp_WoodArm` | festepunktet på trestolpens side, i armens akse | armen mot +X |

Sammensetting: stolpen står i origo. Armen settes ved `(0, 7.60, 0)` og hodet ved armens endepunkt
`(1.80, 0.45, 0)` relativt til armen, med samme dreining om stolpeaksen. Da havner hodet ved
`(1.80, 8.05, 0)` og emitteren ved `(2.22, 7.877, 0)`. På trestolpen ligger hodet ved `(1.20, 0, 0)`
relativt til trearmen. Stolpeaksen ligger da `0.13 m` bak pivoten, for en stolpe med radius 0,13 m.

## Glass og lys

`M_StreetLamp_Lens` er en egen, ugjennomsiktig materialslot for den prismatiske glassbollen.
`EmissiveFactor` (lineær RGB) ganges med bollens grunnfargetekstur. Verdien `(0,0,0)` betyr slukket.
Høytrykksnatrium bør ligge rundt 2100 K, varm oransje. `TextureTint` har standardverdien `(1,1,1)`.

Bollens laveste ytre punkt (`lens_centre_three_m`) ligger ved `(0.42, -0.165, 0)` fra hodets pivot.
Normalen er `(0,-1,0)`. Emitteren ligger 8 mm lenger ned, ved `(0.42, -0.173, 0)`, slik at et
skyggekastende lys ikke blir skjult av glassmeshen. Bollens geometriske midtpunkt og
lysbuerørets plass i huset står også i kontrakten.

Normalteksturene følger DirectX. Blender-grafen vender den grønne kanalen for visning. Kildeskriptene
endrer ikke global eksponering, himmel eller lysstyring.

## Kollisjon

Ingen UCX er laget, fordi spillet bruker egne 2D-kolliderere. Importen følger ExteriorFixtures:
`auto_generate_collision=False` og complex-as-simple på body setup.
