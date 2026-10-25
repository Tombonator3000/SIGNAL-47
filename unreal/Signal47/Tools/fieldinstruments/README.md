# STATION 01 feltinstrumenter

Originale nærmodeller for feltreisen i kapittel 3. Generatoren lager 13 deler med tre manuelt detaljerte LOD-er, FBX og GLB på alle nivåer, samlet GLB og portabel Blender-kilde. Ingen Blender-, UE- eller rendringsjobb kjøres av delagenten.

Kjør `make_textures.py` med Python og Pillow, deretter `build_fieldinstruments.py` i Blender med to tråder og `-- --manifest <manifest.json>`. Rotagenten bestemmer det sekvensielle maskinvinduet.

## Bevarte handlinger

Transitens sokkel og optiske retning bevarer E09A-funnets plassering. Feltlampens mast, hode, lampeglass og blackout-lokk er separate. Lokket festes lokalt ved null til `world.site.objs.lampHood`; det animeres av eksisterende `SetLampCovered`, Three rotX 0 eller 2.1. Ingen rotasjon er bakt i den eksporterte delen.

Telefonens hengende earpiece festes lokalt ved null til `world.site.objs.fieldPhoneHandset`. Telefonkassen er en egen modell ved den opprinnelige vestveggen. Ringing, opphenting og historieforløp beholdes.

Receiverens skala/S-meter ligger i en egen del og må drives av `world.site.m.dial`; lampeglasset må drives av `world.site.m.fieldBulb`. De genererte materialene er bare for kildescenen frem til denne native bindingen. Den nye skalaens egen trykktekstur må beholdes når lys/farge endres.

Benken har samme x-utstrekning `[-12.95,-9.95]`, Three z `[1.5,2.22]` og topphøyde `.8` som før. Det er en faktisk åpning for den eksisterende animerte skuffen. `drawer` og `drawerInside` skal beholdes. Ingen fast ny skuffefront dekker bevisboken.

## Integrasjonskontrakt

`model_contract.json` skrives av Blender med målte bounds, trekanttall, materialslots, LOD-statistikk, interaksjonshåndtak og bevarte pivoter. Alle eksporterte objekter står ved null og har skala 1. Blender Z er opp. Native import skal følge samme `MATCH_UP_FORWARD_AXES` og yaw90 som eksisterende modellpakker, til Unreal `(100x,-100y,100z)`.

Ikke skjul hele `world.site.group`, `.st`, `.interior`, hytteinteriør eller en hel original materialgruppe. Originalscene har statisk sammenslåtte geometrier, og originale materialmesher kan inneholde flere interaktive bevis. Bruk avgrenset geometriretire i `retire_volumes.json`, gjennomgå rammebilder ved foting A, marker A/B og kabel før aksept. Disse volumene er forslag som må kontrolleres mot faktiske triangeler, ikke bekreftet eksportfiltrering.

Kanoniske bevistekster og dokumentpaneler i `S47Chapter3` beholdes. Trykket på nærmodellene er en fysisk illustrasjon av de eksisterende postene. EvidenceProps er kosmetiske film-/kassett-/konvoluttrekvisitter, ingen ny opptaker eller ny innsamling.

## Kilde og status

Alle geometrier og kart er originale. `shapes.py` er tilpasset prosjektets egen `substation/meshkit.py`. Biblioteksøk ga ingen relevant dokumentert modellgenerator. Profilen `ahujasid/mcp-for-blender` er undersøkt, men omhandler en annen, historisk MCP-installasjon og gjenbrukes ikke. Den etablerte lokale Python/Blender-løypa brukes.

Dette er authoring-kilden. Blender-eksport, native import, visuell vurdering og pakket spilltest må være dokumentert av rotagenten før modellen kalles levert i spillet.
