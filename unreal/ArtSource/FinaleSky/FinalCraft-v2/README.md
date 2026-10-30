# Eget fartøy, revision 2

Aktiv geometri for `/Game/FinaleSky/SM_S47_FinalCraft`, med revision `S47-FINALE-SKY-2`. [FinalCraft-v2.blend](FinalCraft-v2.blend) inneholder 59 redigerbare originaldeler og tre sammensatte LOD-er. [manifest.json](manifest.json) er det uendrede råuttrekket fra kildebygget, med faktiske generator-/geometrireferanser og FBX-er under `exports/`.

Nativeimporten har tre egne LOD-er med 13 396 / 8 228 / 5 300 triangler og de fire eksisterende `MI_S47_FinalHull/Ribs/Recess/Light`. Kollisjon og Nanite er av. Den lave yttersilhuetten beholdes; undersiden har tre industribånd og åtte små kantlys.

[build_craft_v2.py](../../../Signal47/Tools/finalesky/build_craft_v2.py) bygger [geometry_v2.py](../../../Signal47/Tools/finalesky/geometry_v2.py) til en fersk outputmappe. [import_craft_v2.py](../../../Signal47/Tools/finalesky/import_craft_v2.py) oppdaterer bare fartøyets eget mesh og bruker den eksisterende FBX-importhjelperen. Blender-kilden har meter/Z opp og FBX-eksport−Y/Z; nativeaksen og UV-oppskriften er bevart fra den etablerte løypen.

Den gamle generatoren, `source-contract.json`, `VerifiedSource` og `NativeArchive` er separate, bevarte originaler. Se [arbeidsløypen](../../../Signal47/Tools/finalesky/README.md) for videre iterering.
