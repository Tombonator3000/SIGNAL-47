# Toms Tripo-modeller fra 9. november (TRIPO-NEW-20261108)

**Status:** Gjeldende. **Gjelder:** UE 5.8.4, Linux, private Tripo-kilder (betalt plan). **Sist kontrollert:** 9. november 2026.

Ni modeller fra Toms skrivebord: `Biggie.glb`, `Elvis.glb`, `caddilac.glb`, `camper.glb`, `cosmic.glb`, `nordic.glb`, `zet.glb`, `walkie talkie 3d model.glb` og `pakke tripo3d-1.glb`. De er laget på Toms betalte Tripo-plan og skal **aldri i Git**. Råfilene er kopiert med `SHA256SUMS` til `~/Documents/ChatGPT/Signal 47/modeller/tripo-new-2026-11-09/raw`. Alt som lages, ligger i samme mappe under `prepared/` og `logs/`, og de native assetene ligger i `/Game/Licensed/TripoNew` (ignorert). `/Game/Licensed` kokes allerede i sin helhet (`DefaultGame.ini`).

Bruk aldri noe fra `~/Desktop/Gard-modeller-Blender` eller `Gard-modeller.blend` (uttrekk fra et annet spill).

## Kjøring

Alle steg går gjennom reservasjonen, én tung jobb om gangen. Blender teller som tung jobb:

```sh
Q=~/.claude/skills/signal47-nattskift/scripts/q.py
python3 $Q python3 run.py prepare --previews      # Blender: statiske FBX, atlas, manifest, bilder
python3 $Q python3 run.py eyes zet                # measure_eyes.py på zet sin _eyes.blend (også nordic, cosmic)
python3 $Q python3 run.py import                  # triponpcs/import_assets.py til /Game/Licensed/TripoNew, mot hovedprosjektet
```

`prepare.py` følger `triponpcs/prepare.py`. En rigget figur bakes i første bilde av sitt eget `idle`-klipp og skaleres til høyden sin, med fotpivot under bekkenet. Kjøretøy og rekvisitter skaleres til lengde eller høyde, med pivot midt på bakken. FBX-oppskriften er den samme, så i Unreal vender en figur mot +Y (Three +Z), og et kjøretøy har fronten mot -Y (Three -Z), som TripoNPC-figurene og TripoFleet-bilene. Bobilen og cabrioleten er gjort 14 og 12 prosent smalere, fordi Tripo laget dem for brede for lengden.

| Fil | Asset | Mål |
| --- | --- | --- |
| Elvis.glb | SM_Tripo_Singer | 1,83 m |
| Biggie.glb | SM_Tripo_Bigfoot | 2,45 m |
| zet.glb | SM_Tripo_Zet | 1,40 m med hatt |
| nordic.glb | SM_Tripo_Nordic | 1,96 m |
| cosmic.glb | SM_Tripo_Cosmic | 2,30 m |
| camper.glb | SM_Tripo_Motorhome | 7,3 x 2,9 x 3,2 m |
| caddilac.glb | SM_Tripo_PinkConvertible | 5,7 x 2,1 x 1,6 m |
| walkie talkie 3d model.glb | SM_Tripo_Walkie | 0,34 m med antenne |
| pakke tripo3d-1.glb | SM_Tripo_Parcel | 0,31 m, uten tekstur |

Navnene på de fem første er de som `S47Hap` i `S47Happenings.cpp` venter på. Katalogen i spillet er `S47TripoNew` (stier, mål og stand-in når modellen mangler). Øynene til Zet, Nordic og Cosmic er målt med `triponpcs/measure_eyes.py` på `<navn>_eyes.blend`. Den fila har den bakte kroppen og et ettbeins armatur der hodet står i positur. `dot_world`, regnet om til `(100x, -100y, 100z)`, er punktet i meshens egne centimeter, ført i `S47EyeGlint` (`ForStatic`).

Alle fem figurene har Tripo-rigg (80 UE5-bein) med egne klipp (idle, walk, run, look_around osv.). Her brukes de statisk. En animert utgave kan lages med `triponpcs/prepare_pale_one.py` ved å legge til rader i `BODIES`.

## Merknader

- `pakke tripo3d-1.glb` er en grå, teksturløs modell som ligner en håndradio fra 80-tallet, ikke en pakke. Den er importert, men brukes ikke som postpakke før Tom har sett på den.
- Walkien har LCD og tastatur og ser yngre ut enn 1986. Tom avgjør.
