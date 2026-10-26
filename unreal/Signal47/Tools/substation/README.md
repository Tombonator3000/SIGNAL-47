# Caliche Junction Substation

Originalt modulært Blender-anlegg for SIGNAL / 47, New Mexico 1986. Alle modeller, mineral-/metallkart og fiktive engelske skilt er laget for prosjektet. Kildebyggingen er fullført; uavhengig eksportkontroll og native visuell aksept må leses i de aktuelle rapportene. Et Blender-bilde er en kildepreview, ikke spillbevis.

Oljetransformatoren har avrundet tank, boltet lokk, separate kjøleplater med luftgap, tre høye porselensbushinger, tre mindre frontbushinger, toppterminaler, kobberledere, horisontal oljebeholder, avlufter, rør, ventiler, analoge instrumenter og stålunderstell. Bakre galvanisert bussramme, separat kontrollskap utenfor låst inngjerding, diamantnetting i faktisk geometri og stort skilt over porten gir anlegget tydelig fysisk form. Dette er en original fiktiv spillmodell, ikke en installasjonstegning.

Alle mål er meter, Blender Z opp, servicefront mot lokal negativ Y. Stålunderstellet når Z0; tankens høyeste terminal når omtrent5,97m og busslederne omtrent7,85m. Kildegeometrien ligger i hver dels lokale pivotrom. De fleste deler har sitepivot0; portbladene, hendelen og lampen har egne normaliserte pivoter. `site_local_offset_m` legges til én gang ved plassering. Den samlede Blend/GLB-kilden plasserer delene med disse offsettene; de enkelte FBX-/GLB-filene har objektposisjon0.

Runtimekontrakten er låst til Three root `[7890,-1.10,536.20]` og yaw180: `worldThree=(7890-bx,-1.10+bz,536.20+by)`. Native integrasjonen eier kollisjon, ganggulv og interaksjoner. Ingen Blender-collider blir automatisk eksportert. Inngjerdingen ligger ved Y−7,2 foran med4,4m lukket portåpning. Hovedgrusen er20×16m, topp−0,08 og begravd skjørt til−0,65. Kontrollplinthens topp er0 ved X[4,1;5,9],Y[−9,25;−7,75]. Apronen X[3,2;6,8],Y[−14;−9,25] stiger lineært fra−0,25 til0. Disse toppene er autoritet for runtimeagentens eksplisitte gangflater.

Ti eksportdeler:

| Del | Pivotforskyvning i site, meter | Formål |
| --- | --- | --- |
| SM_Substation_Transformer | 0,0,0 | Tank, kjølere, porselen, ventiler |
| SM_Substation_Buswork | 0,0,0 | Bakre stålportal og faseledere |
| SM_Substation_Control | 0,0,0 | Eksternt skap, fronttrykk og lampehus |
| SM_Substation_StatusLamp | 4,62;−8,925;1,70 | Bare amberlinsen, M_Substation_LampAmber |
| SM_Substation_Lever | 5,52;−8,94;1,15 | Håndtak med lokal X som rotasjonsakse |
| SM_Substation_Fence | 0,0,0 | Inngjerding, navneskilt og advarsler |
| SM_Substation_Gate_L | −2,2;−7,2;0 | Hengselpivot, lokal Z |
| SM_Substation_Gate_R | 2,2;−7,2;0 | Hengselpivot, lokal Z |
| SM_Substation_Pad | 0,0,0 | Fundament, kontrollplinth og apron |
| SM_Substation_Gravel | 0,0,0 | Grusbed og lave steinsilhuetter |

Kontrollens daterte serviceark ligger ved Blender `[4.83,-8.937,.77]`, størrelse0,50×0,25m, vendt−Y. Lampens metallbezel ligger i Control; StatusLamp inneholder bare linsen og kan få egen MID. Skapets trykk inneholder identitet, status og et datert inspeksjonskort, uten driftsinstruksjoner.

`materials.json` gir navngitte materialrader med lineær grunnfarge, metall, ruhet, emisjon, normalstyrke og relative PBR-baner. Fem originale kartfamilier har2048px grunnfarge og1024px normal/ruhet. Normalkartene er DirectX, med grønnkanalen snudd i Blender-materialet. Skiltatlaset er originalt trykk, rasterisert med systemets DejaVu Sans/Bold; ingen fontfil eller tredjepartsmodell er kopiert inn.

`model_contract.json` gir `parts[SM]` med `bounds_local_m=[[min],[max]]`, LOD0 `triangles`, ordnede `materials`, `used_materials`, `site_local_offset_m`, `origin=[0,0,0]` og `lod_triangles=[LOD0,LOD1,LOD2]`. `lods['0'/'1'/'2'][SM]` har hver LODs eksakte grenser og materialer. Alle deler har tre egne FBX-er; LOD0 har separat GLB og samlet GLB-backup. FBX bruker samme verifiserte−Y/Z og FBX_SCALE_UNITS-løype som radiostasjonen. Native force_front_x_axis, MATCH_UP_FORWARD_AXES og importyaw90 gir Blender(x,y,z)→UE(100x,−100y,100z). `axis_probe.fbx` har sentrum(2,4,6)m og størrelse(0,2;0,4;0,6)m; forventet native sentrum er(200,−400,600)cm.

Bare etter eksplisitt tungjobbslot:

```sh
cd unreal/Signal47/Tools/substation
python3 run_source.py
```

Løypen kjører originalteksturer, Blender-bygg og uavhengig Blender-tilbakeimport sekvensielt gjennom prosjektets owned guard, maks to Blender-tråder. Den starter ikke Unreal eller rendering. `build_substation.py` pakker alle aktuelle bilder, bevarer en skjult redigerbar originalsamling og navngitte komponentgrupper, sveiser sammenfallende vertices med1µm terskel og beholder UV-/materialdata. Dette gir faktisk avrundede porselensnormaler, samtidig som plateflater og endekapper beholder hardkant. `verify_substation.py` kontrollerer30 faktiske LOD-FBX-er, proben, ti binære GLB-er, original Blend, pakkede bildebyteverdier og fysisk delte/smidige porselensnormaler. Ingen native eller visuell PASS skal utledes fra kildekontrollen alene.

Prosjektbiblioteket er søkt etter transformator/Blender/Unreal og deretter Blender. Bare en historisk MCP-profil var relevant som referanse; den hadde ingen ny verifisert modell-/eksportløype og er ikke gjenbrukt. Arbeidsløypen følger prosjektets eksisterende `radioarray/station/build_station.py`, radioarray-import, Caliche-import og egen gas-station-meshkode. Ingen eksterne kjøpsassets eller nye kjøretidsavhengigheter inngår.
