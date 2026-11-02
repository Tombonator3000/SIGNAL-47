# Alderete-gård i Freeform

**Status:** Kilde implementert og prøvd i editor; pakking/levering føres i STATUS. **Gjelder:** FARM-01, UE5.8.4/Linux. **Sist kontrollert:** 2. november 2026.

`S47FarmWorld` bruker valgt session mode som eneste layoutkilde. World og Flow rekonsilierer før lagret spillerpose valideres; Story bruker original gårdsplassering og stengt grind. Freeform flytter den eksisterende husgruppen310m vest og de to eksisterende kuhjordene med road-lokal forskyvning(92,0,-316). Husenes høyde flyttes stivt per struktur etter samme terrenggrunnlag som bil og gange. Ingen nye lagringsfelt innføres.

Gårdsgruppen består av de opprinnelige300 trekantene i g910–912. De blandede kildemodellene flyttes aldri samlet. Den faktiske tubulargrinden er192 trekanter i hver av de to områdekopiene; bare disse skilles ut og dreies. Freeform får avgrenset kjøre-/gangadgang, synlig grus, beitegjerde og trau. Storys port, ku-/ninth-/fotoankre beholdes. Skur-/husinteriører er fortsatt bakgrunnsgeometri.

## Klargjøring av lokal assetcache

Genererte Web-assets inngår ikke i Git. I et reservert og vernet editorvindu kjøres først eksisterende `../highwayranch/prepare_cpu.py` for g910/g911/g912, deretter `prepare_cpu.py` her for nøyaktig g678. Hjelperne beholder CPU-indekser og kontrollerer uendrede LOD-antall, materialer, RT, Nanite og kollisjonsmetadata; de importerer eller modellerer ingenting. Bevar originale filer før klargjøring. Begge cachekilder må være klargjort før cook; én ekstra arbeidskopi skal ikke opprettes.

`-S47Title -S47Freeform -S47FarmWorldTest -UserDir=/tmp/<egen-profil>/` bruker én tydelig initial veikjøretøy-/lastfixture, deretter ordinær kjøring og gange gjennom porten, full disk/Fortsett, cargo-ray/Use og retur. Story/Freeform-sammenligningen skjer separat etter fysisk reise. `-S47FarmWorldAssetsOnly` hopper over ruten og er bare kort pakket ressurs-/renderkontroll. Resultatene er utviklerstyrt API-prøve, ikke menneskelig spillaksept. Se gjeldende STATUS for faktisk installasjon.

## Fri sikt og veibane

Bare faktisk målte steininstanser med hele bounding box inn i RanchZ-veibanen flyttes til grøftekanten i Freeform, med terrengjustert høyde og bevart rotasjon/skala/LOD. Story gjenoppretter de originale instansmatrisene. Native prøve2. november identifiserte nøyaktig én: `props:rocksEast_1`, instans4, `namaqualand_boulder_03`, roadX29,936–35,526/Z193,543–198,555. Den står nå utenfor veibanen ved Z181,963–186,974. Faktisk komponent har NoCollision og ingen authored bil-/gangboks; rettingen rydder en visuell steinsperre som tidligere kunne gjennomkjøres. Ressurs-/renderprøven måler alle180 instanser i14 familier og prøver Story/Freeform-retur uten å gjenta helreisen.
