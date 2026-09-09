# Blender refinement queue

The browser prototype models were converted to OBJ so the project has real source geometry instead of losing the prototype work.

Refine in this order:
1. `SM_CRT_Terminal_A` — bevel housing, recessed glass, proper keyboard caps, vents, rear cable.
2. `SM_ReceiverRack_A` — separate module faceplates, meters, knobs, labels, vent perforation, cable exits.
3. `SM_ControlDesk_A` — 1980s formica/steel construction, drawers, cable grommets, edge wear.
4. `SM_DeskPhone_A` — cradle, detachable handset, coiled cable, button legends.
5. `SM_DotMatrixPrinter_A` — tractor feed, paper stack, print head path, top cover.
6. `SM_Mug_Intact_A` / `SM_Mug_Broken_A` — story prop; improve silhouette and shard correspondence.
7. `SM_RadioDish_Bowl_A` / `SM_RadioDish_Pedestal_A` — truss back, azimuth/elevation joints, feed support, ladder/service detail.

`Blender/Source/build_signal47_prototype_assets.py` can be run inside Blender 5.x to create a starting `.blend` containing all ported models with light bevel treatment. It has not been executed here because this runtime has no Blender binary.
