# SARO serviceflåte

F9 har kilde-/FBX- og native importkontroll. Blender-eksteriøret er visuelt vurdert; pakket aksept føres i PRODUCTION_MODELS.md. Root kjører én tung jobb av gangen.

Raphael Bagets C10 er valgt etter faktisk inspeksjon av glTF-geometrien, 78242 tri før tilpasning. Den gir runde fendere, grill, støtfangere, speil, stepside-lasteplan og separate hjul. Fairheaven LT80 og El Alamo78 erstatter henholdsvis sedan og pickup foran bygg2. Alle tre lokale kilder er CC BY4.0; kreditering og originalarkiver beholdes. Råmodeller og avledede Blender-/FBX-/GLB-/teksturfiler går til `S47_FLEET_WORK` utenfor offentlig Git. Bare generatorer og egne runtime-tilpasninger ligger her.

C10 roteres fra forberedt glTF med front−X til Three-front−Z: `(-srcZ,srcY,srcX)`, skalert med `2.06/2.1205409469`, forskjøvet +.30m langs ThreeZ. Blender er meter/Zopp og `Three=(Bx,Bz,-By)`. Importeren bruker etablert FBX-løype med +90° importrotasjon, bekreftes med asymmetrisk akseprobe før native plassering. Karosseri innen nåværende 2.10×5.50m servicekollider. Ratt, kluster/radioflater, nåler, advarsler, kompass og nøkler bruker eksisterende levende noder/ankre. Resten av den gamle cab-geometrien skjules når hele det nye settet finnes. Manglende modellsett beholder gammel bil komplett.

Nytt eget innhold: formet dash, innfelte instrumentrammer, benk med sømmer, dørtrekk, sveiver, håndtak, søyletrekk, rattstamme og SARO/FIELD SERVICE07-merking. Bare blå karosseriflater omlakkeres; krom, gummi og kildens PBR beholdes. Urelatert drikke-/bilmerking fjernes eller dekkes med egen merking. Dette må kontrolleres i de faktiske første Blender-/spillbildene.

`build_fleet.py` skriver 3 eksporterte LOD per del, portable teksturer, `.blend`, `.glb` og `model_contract.json` med faktiske tall. `verify_fleet.py` gjenimporterer alle FBX og kontrollerer antall, UV, materialplasser, nullareal, bounds og pivot. `source-verification.json` må være PASS. `import_assets.py` bruker eksisterende material-/FBX-funksjoner og importerer bare flåten; krever ferskt sidecar-/eksportsett. Sidecars bevarer glTF-pakket G=roughness/B=metallic og OpenGL-normaler får korrekt native grønn-invertering.

Originalt dash/merking/cab/linsedeler og materialer: `/Game/SaroFleet`. Tredjepartsderivater: `/Game/Free/ServiceFleet`. Ingen ny ContentJSON. `FS47Truck::Bind` installerer biltilpasningen. Root kobler `FS47FleetParked::Build(Web)` etter Props og SAROsite; returnert shared pointer holdes av Web. Den bytter bare g454s bilkropper, bevarer øvrig dressing, bakkeflekker, hjulstopp og gamle kolliderfotavtrykk. Den personlige Fairheaven-stasjonsvognen endres ikke.

Egen offentlig addon-kilde lagres av `export_own_addons.py` i en separat fersk Blender-prosess etter at den samlede flåten er lagret og verifisert. Den fjerner alle C10-/parkerte bilobjekter og ubrukte avhengigheter, kontrollerer bare egne materialer/teksturer og lagrer `Saro07_Addons.blend` med fire redigerbare originalsamlinger. Fire egne FBX og en GLB legges i `exports/`.

Kjør først egne teksturer med bundled Python, så Blender med `-b -t2 --python-exit-code1 --python build_fleet.py`, deretter tilsvarende `verify_fleet.py`. Alle jobber må gå gjennom root sin strict guard. Ingen eksport eller import er kjørt av denne kildeskrivingen.

Egne originale PBR-PNG er levert i textures/ og pakket med relative stier i Saro07_Addons.blend. libraries.write ble erstattet med normal lagring i en separat prosess etter konkret Blender-krasj. mesh.validate og målrettet topologiopprydding kjøres før eksport og opptelling. Godkjenning krever faktiske source- og native rapporter, ikke bare prosessens exitkode. Bruk FBX-importflagget `-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0`.
