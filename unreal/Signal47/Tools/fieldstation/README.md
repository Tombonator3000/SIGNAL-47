# STATION 01, original feltstasjon

FS1 erstatter den gamle, lille landmålerhytta med et originalt redigerbart modellsett. Kilden følger det faktiske rommet fra manifestet og S47Station.cpp: 4 x 3 meter innvendig, gulv y=0, vegger 2,6 meter, 20 cm veggtykkelse. Stasjonen ligger under `world.site.group`, med Three-rot `[0,0,8000]`. Den historiske TypeScript-kilden er bare lest som koordinatreferanse.

Dette er en modellkilde, ikke i seg selv en import, spilltest eller visuell godkjenning. `source_plan.json` beskriver den lette geometrikontrollen. `model_contract.json`, faktiske Blender-originaler og `verification.json` blir først skrevet når root kjører den sekvensielle DCC-løypa.

| Del | Innhold | Forelder i spillet |
| --- | --- | --- |
| `SM_FieldStation_Hut_Shell` | Pussvegg med ekte åpninger, takpapp med overlapp, brettede beslag, ovnsrør, serviceinntak, strømmåler, dyp vindusinnfatning, eget lesbart skilt, slitt terskel og begrenset kontaktgrus | `world.site.group` |
| `SM_FieldStation_Hut_Interior` | Kalkpuss, mørk brystning, listverk, separate gulvbord ved opprinnelig gulvhøyde, bjelker og gammel kabel med keramiske klips | `world.site.interior` |
| `SM_FieldStation_Hut_Glass` | Åtte tynne glassruter i de to faktiske veggåpningene | `world.site.group` |
| `SM_FieldStation_Hut_Props` | Metallskap med dører og beslag, original trestol, vegghylle og reservedelsbokser ved gamle møbelankre | `world.site.interior` |
| `SM_FieldStation_Hut_Door` | Eget dørblad med bordfuger, stiler, hengsler og håndtak, nullpunkt i hengselen | `world.site.objs.hutDoorLeaf` |

Døra eksporteres uten innbakt åpningsvinkel. Forelderen i spillet står lokalt Three `[-9.45,0,4.48]` og starter med rotY `-1.75`. Det eksisterende `SetHutDoor` driver både den nye døra og den opprinnelige stengekollisjonen. I den samlede Blender-originalen står døra åpen under en egen tydelig merket forhåndsvisningsforelder. Denne forelderen skal aldri importeres som en ekstra animasjon.

Sørvinduet beholder x `[-12.3,-11.3]`, y `[.95,1.85]`, vegg z `[4.5,4.7]`. Østvinduet beholder z `[2.3,3.3]`, y `[.95,1.85]`, vegg x `[-9,-8.8]`. Døråpningen beholder x `[-10.5,-9.4]` og høyde 2,0 meter. Verken skilt eller puss ligger bak rutene. Det utvendige skiltet sitter over døra ved det opprinnelige skiltankeret.

Benken, mottakeren, felttelefonen, røret, instrumentene, diagrammet, timingloggen og lampene tilhører den samordnede instrumentleveransen eller de bevarte interaksjonene. De blir ikke eksportert en gang til her. Skuffen og feltboken beholdes under de eksisterende dynamiske håndtakene. Stolens gamle lisensierte eller kodebygde erstatning må fjernes spesifikt når den nye stolen monteres; ikke skjul hele interiøret for å gjøre det.

Alle modeller bruker meter og Blender Z opp. Mapping er `Three=(rootX+Bx,rootY+Bz,rootZ-By)`. FBX følger den målte bygningsløypa med `axis_forward=-Y`, `axis_up=Z` og `FBX_SCALE_UNITS`. Native import bruker `force_front_x_axis=True`, `MATCH_UP_FORWARD_AXES` og importyaw 90. Den asymmetriske akseproben har senter `(2,4,6)` og størrelse `(.2,.4,.6)` i Blender; forventet native senter er UE `(200,-400,600)` cm.

`design.py` oppgir hvert anker og konservative dekningsvolumer. Disse volumene overlapper bevarte interaksjoner og er ikke tillatelse til å slette all gammel geometri i volumet. Root må klippe identifiserte gamle overflater eller bladgeometri og beholde alle proxies, collidere, fotoankre, dynamiske lamper, glødepunkter, dørforelder og telefonrørforelder. Ingen hel `world.site.group`, `st`, `inSt` eller `interior` skal skjules. Gamle instrumenter, benk, lampedeler og diagram skal heller ikke forsvinne som følge av et bredt materialfilter. Opprinnelige gangsoner og gulvautoritet endres ikke.

Materialene har eget prefiks `M_FieldStation_`. Seks originale prosedyregenererte PBR-familier fra vår eksisterende `townbuildings/textures` kopieres byteidentisk til denne kildemappa. Et nytt originalt skilteatlas genereres lokalt. Normalbilder er DirectX, mens Blender-grafen vender grønnkanalen kun i forhåndsvisningen. Materialets teksturtint skal multipliseres med Base Color-teksturen i native import. Alle brukte teksturer pakkes i `.blend` og beholder relative filreferanser. Ingen kjøpte eller eksterne modeller eller teksturer inngår. Bibliotekssøket ga bare en historisk Blender MCP-kandidat uten dokumentert kompatibilitet; ingen ekstern bro eller installasjon brukes.

Root kjører DCC og native arbeid i det eksklusive, sekvensielle vinduet:

```sh
python3 make_sources.py
python3 check_source_plan.py
blender -b -t 2 --python build_fieldstation.py
blender -b -t 2 --python verify_source.py
```

LOD0, LOD1 og LOD2 er egne geometriske varianter med bevart silhuett og åpninger. Hver del eksporteres i både FBX og GLB på alle tre nivåer. Den samlede GLB-en og `.blend` er en montert kildereferanse. Python-verifikasjonen av kilden tester bare geometri, UV, materialer, koordinater og LOD-budsjett. Blender-verifikasjonen leser faktiske eksportfiler og den lagrede originalen. Pakket spill skal deretter kontrollere terskel, lys gjennom glass, dørforløp, foto og feltoppgaver fra spillerhøyde. En godkjent modellkilde gir ingen ny historieadgang og er ingen påstand om konseptlikhet eller funksjon i spillet.
