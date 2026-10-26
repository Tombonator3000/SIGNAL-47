# SC01: originale vindusressurser

Redigerbar Blender-kilde for blek hånd/underarm, utvendige glasspor og en profilert stållamell. Alt er ny original geometri uten eksterne modeller eller referansebilder. `build_scene.py` bygger og eksporterer kildene deterministisk. `model_contract.json` binder FBX-hasher, materialspor, trekanttall og metriske grenser.

Native import bruker prosjektets eksisterende FBX/PBR-oppskrift. Integrasjonseieren kjører `import_assets.py` i UnrealEditor-Cmd når ingen andre motor-/spilljobber kjører. Bare `/Game/Signal47/HorrorSC01` skrives. Ikke kjør import parallelt med bygg eller tester.

Spillgeometrien ligger ved østre nordrute i kontrollrommet. Glasset står ved z=-4,56; håndroten beveger seg fra x=4,26 til 5,98 utenfor glasset ved z=-4,69 og forsvinner bak eksisterende østre veggpilar. Sporene står fysisk på utsiden ved z=-4,58. Skjermen har 26 solide lameller ved z=-4,34, ikke en usynlig synssperre. Lampen utenfor glasset har egen lavspenningsbryter og kobles ikke til mottaker, K3 eller byforsyning.

Sporene kontrolleres med faktisk sikt fra trygg side og to tilstander på en separat ekte romlampe. Å se gjennom et lesepanel gir ingen observasjon. `FS47HorrorVisual` produserer bare lokale verdensfakta; historiekoordinatoren eier scenetekst og progresjon.

Blender-eksport er utført. Native import, pakket kjøring og pikselkontroll får egne faktiske kvitteringer i SC01-leveransen; kildegeometri alene attesterer ingen spilltest.
