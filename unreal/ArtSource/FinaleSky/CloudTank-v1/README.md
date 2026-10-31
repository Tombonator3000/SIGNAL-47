# Egen cloud tank-kilde, første fluidpass

Redigerbar Blender-kilde og de tre faktiske 128³/8-bit-atlasene fra ramme48/72/96 er bevart her. Hele OpenVDB-cachen og flyttallsdataene bevares lokalt med råmanifestet; de er ikke del av dette portable delsettet. Gjenbygg med `Tools/finalesky/bake_clouds.py` ved128,96rammer og `--picks 48 72 96`. Native import bruker `atlas.json` og PNG-filene. Projeksjonsbildene er datavisninger, ikke Unreal-bilder.

Dette erstatter ingen tidligere geometri eller NativeArchive. Nye fluidvarianter får egen kildemappe.
