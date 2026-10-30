# SARO-rekvisitter, 1986

**Status:** Gjeldende. **Gjelder:** UE5.8.4 på Linux, native SARO-detaljer. **Sist kontrollert:** 30. oktober 2026.

Visuell føring: [SARO i 1986](../../../../Docs/Design/SARO_1986.md). Egne kilder ligger i `unreal/ArtSource/Saro1986`; telefon- og Tripo-kilder ligger privat utenfor repoet. Ingen modell- eller teksturdata fra `Content/Free` eller `Content/Licensed` committes.

Prosjektbibliotekets profil `x-2111849852350894319` beskriver Blender for enkel geometri og Tripo for sammensatte rekvisitter. Den er uprøvd som implementering og brukes bare som arbeidsreferanse. Importen gjenbruker prosjektets faktisk prøvde `Tools/fieldintegration/import_recipe.py`, med legacy FBX, metriske akser, PBR og `NoCollision`.

Kjør `rx3.py` og `period_props.py` med Blender i prosjektets sekvensielle tungvindu. `period_props.py -- --jacket` regenererer bare egne klær; telefonen klargjøres fra den allerede anskaffede WE500-kilden og deles i kropp og håndsett med spiralledning. Originalkilden endres ikke.

Importer med `-run=pythonscript -script=Tools/saro1986/import_assets.py`, `-nullrhi` og `-ExecCmds="Interchange.FeatureFlags.Import.FBX 0"`. `-S47Saro1986Kind=props` importerer RX3, jakke, telefon og de utvalgte CPU-dataene. `rx3`, `jacket`, `phone` og `legacy` velger ett trinn. `fixtures` bruker den private klargjøringen angitt i `S47_TRIPO_FIXTURE_PREPARED`.

Armaturet klargjøres fra den nyeste eksporten som faktisk vises i Tripo, ikke fra en eldre nedlastet variant. `prepare_fixture.py -- --source /privat/siste.glb --target-dir /privat/klargjort --max-triangles 20000 --lens-mode tubes` bevarer kilden og atlasets UV. Produksjonsvarianten er Tripo H3.1/P1.0 med 18 748 trekanter i huset og 184 i to rette T12-glassrør. Originalens metriske størrelse normaliseres til 1,30 × 0,36 × 0,08 m med toppsentrum som feste; glassrørene stikker 1,6 cm under huset. Metallhuset har ingen emissiv flate; lysfeltene følger de eksisterende elektriske driverne. Originaler, PBR-teksturer og redigerbar Blender-fil beholdes privat.

Den godkjente `SM_Office_Room` beholdes byteidentisk. En separat CPU-kopi gir en transient renderkopi som kan fjerne bare de gamle gangarmaturdelene. Legacy-klipping krever navngitt rot/geometri, skarpe volum og forventet triangelantall; alle valg kontrolleres før noe byttes. Plakatflytting beholder originalgrafikk, UV og materialer. Spillbare foreldre, kollisjoner og lagret tilstand brukes videre.

`-S47Saro1986Off` slår av detaljmodulene. Egne avknapper finnes for RX3, perioderekvisitter og armaturer. Nærkameraene heter `interior-phone-detail`, `interior-jacket-detail`, `interior-rx3-detail` og `interior-fixture-detail` i `interiors`-suiten. Kameraer er visuell kontroll, ikke bevis på menneskelig input.
