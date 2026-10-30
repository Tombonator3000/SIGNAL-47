# SARO-karmer

Kildeoppskrift for to originale karmer: malt stål til service/fotolab og tre til arkivet. Ingen ny modell av dørbladet lages. Eksisterende `SM_Office_Door` gjenbrukes senere med måltilpasning og materialoverstyring i runtime.

`build_frames.py` har en ren `append_frame(box, cyl, materials, origin, width, height, style)`-funksjon som kan importeres i Office-byggeren uten scenenullstilling eller eksport. Helperne har samme Three-koordinater og signatur som Office: `(x, høyde, z)` i meter. Materialordboken bruker originale `M_Office_*`-navn.

Standalone, først når foreldretasken har frigitt det tunge vinduet:

```sh
blender -b -Y -t 2 --python unreal/ArtSource/DoorSet/build_frames.py
```

`-Y` deaktiverer automatisk kjøring av innhold i den eksisterende Office-filen. Kilden leser bare materialer fra `../Office/Signal47_Operations.blend`; den kjører ikke Office-byggeren og lager ikke nye teksturer. `.blend`, to FBX-er og `model_contract.json` skrives her etter faktisk kjøring. Arkivvarianten er skjult i sin egen samling i kildescenen fordi begge prototypene har samme riktige origin.

## Passform

- Hengselsiden er `x=0`, gulv-origin er `(0,0,0)`, og lukket bladmidtplans er `z=0`. Prototypen er én meter bred og 2,07 meter høy. Bladets underside står 12 mm over gulvet.
- Side- og toppklaring er 4 mm. Terskeltopp er 8 mm over gulv, 4 mm under bladet. Stopplist og pakning har faktisk profilgeometri; kontaktflaten beregnes fra bladkantens bakside pluss 2 mm.
- Eksisterende Office-kjerne er 55 mm tykk og kantlippingen rekker 32 mm bak midtplanet. For nominell 45 mm kjerne brukes `leaf_depth_scale=45/55`; faktisk kantbakside blir 26,1818 mm og stopkontakt 28,1818 mm.
- Tre hengselspor er åpne rundt eksisterende 13 mm hengselradier. Låssiden har grunn innfelling for bladets eksisterende låsplate og et dypere sluttstykkehull, separat metallkant og inntrukket mørk bakvegg.
- `FrameScaleX=aktuell bladbredde/1.0`, `FrameScaleY=aktuell bladhøyde/2.07`. I UE svarer dette til mesh-skala `(FrameScaleX,1,FrameScaleY)`. Runtime plasserer det eksisterende sentrerte dørbladet en halv bredde og en halv høyde pluss gulvklaringen fra hengsel-origin.
- Runtime eier endelig plassering, hengselretning, leaf-dybdeskala, tekstplater, interaksjonsproxy og eksisterende kollisjons-/gangsoner. Nye karmer har dekorativ kollisjon av. Ingen dørlås, historieport eller fremkallingsmekanikk endres her.

## Native import

`unreal/Signal47/Tools/interiors/import_doorframes.py` bruker eksisterende målt FBX-oppskrift i `fieldintegration/import_recipe.py`, tre LOD og dekorativ kollisjon av. Utdata er `/Game/Signal47/Art/Doors`. Alle materialspor bindes til gjeldende `/Game/Signal47/Art/Office/Materials`; ingen materialgrafer eller teksturer kopieres eller reimporteres. Bruk prosjektets eksisterende guard, legacy-FBX-flagget og én motorjobb. `-S47DoorFramesVerifyOnly` kontrollerer lagrede native filer uten reimport.

Gjenbruk: egne Office-formhelpers, Office-materialer og validert native FBX-/LOD-løype. Prosjektbibliotekets `arjun988/blender-skills`-profil er lest som metodebakgrunn og er UNVERIFIED mot UE 5.8.4. Ingen tredjepartskode, MCP-installasjon eller betalt API er brukt.

Kildeoppskriften er skrevet uten å starte Blender eller Unreal. Faktisk modellbygging, native import, åpning/lukking og visuell aksept føres av foreldretasken etter kjøring.
