# SARO bygg 2 og serviceforsyning

Originale redigerbare kilder for SIGNAL / 47. Nettleserversjonen er kun målegrunnlag. Ingen kjøpte modeller eller brukerbilder er brukt i dette settet. Den eksisterende lisensierte stolpemodellen gjenbrukes av HighwayPower, og distribueres ikke som del av dette Blender-settet.

## Innhold og koordinater

`SARO_Site.blend` har tre originale deler med fysisk inntrukne vinduer og grunne, synlige kontorrom. Ingen nye spillerrom eller handlinger er innført. `SARO_Site.glb` er samlet forhåndsvisning; `exports/` inneholder pivotlokale FBX med tre forfattede LOD-nivåer, samt GLB og en asymmetrisk akseprøve. Hele scenen bruker sceneplasseringene i `site_geometry.PARTS`. Hver FBX har lokalt nullpunkt; offset skal bare påføres én gang.

Blender `(x,y,z)` blir Unreal `(x,-y,z)*100`. Runtime bruker Three med Y opp: `part.root + (bx,bz,-by)`. Rootene er fasade/glass `[29.6,-0.6,0]` og serviceutstyr `[3.6,-0.6,18.6]`, alle uten ekstra rotasjon. Taket møter den bevarte gesimsen ved worldY3.6. Glass ligger 0.27m bak murflaten. SARO-navnet står på ubrutt mur; kontorvinduene har faktiske åpninger.

`placement_audit.json` dokumenterer originale g439/g440/g441/g452, seks nøyaktige kolliderføtter og faktiske reflektortreff fra det gamle nordlige luftstrekket. Beholdt hovedlinje slutter ved den naturlige hovedstolpen `[-9,-0.6,19.739557342938497]` før antennefeltet. Ett 12.651m serviceavgreningsspenn fører til trafostolpen `[3.6,-0.6,18.6]`, deretter til original weatherhead `[1.6,3.55,10.86]`. Feltets eksisterende kabelgrøft/underjordiske fordeling beholdes. Innen 100m horisontalradius fra SARO-root er det to hovedmaster og én service/trafo, til sammen tre forsyningsmaster. Feltlysmaster er bevart og inngår ikke i dette antallet. Hovedlinjen har 82 master globalt; øvrige avgreninger til ranch/motell/trafoanlegget ligger sør og beholdes.

## Reprodusering, sekvensielt av ansvarlig hovedagent

Ingen av skriptene starter Unreal eller andre prosesser selv. Maskinens eksisterende eksklusivitetsguard brukes rundt hvert tyngre trinn. Ingen samtidige Blender-/Unreal-jobber.

```sh
PY=/home/tom/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3
"$PY" Tools/sarosite/make_textures.py
blender --background --threads 2 --python-exit-code 1 --python Tools/sarosite/build_site.py
blender --background --threads 2 --python-exit-code 1 --python Tools/sarosite/verify_source.py
"$PY" Tools/sarosite/audit_layout.py
```

`audit_layout.py --manifest ... --geo-dir ...` lar målegrunnlaget velges eksplisitt. Dette lette skriptet trenger numpy og kjører ikke DCC. Texturegeneratoren trenger numpy/Pillow/installerte DejaVu Sans-fonter. Fonter er ikke kopiert inn. Bitmapene og PBR-kartene er originale, prosedyregenererte og beskrevet i `texture_provenance.json`. Pakkede bilder og relative søskenbaner gjør selve Blender-fila flyttbar sammen med `textures/`.

I Unreal-editorens Python-prosess settes `S47_SAROSITE_REPORT` til en ny rapportfil før `import_assets.py`, og til en annen ny rapportfil før `verify_assets.py` i en fersk editorprosess. Importøren skriver kun `/Game/SaroSite`. Bruk samme eksisterende headless editor-/guardkjøring som MesaDiner. Nativeimporten sjekker akser/pivot, materialkoblinger og tre forfattede LOD-nivåer; fresh verifier kontrollerer faktisk lagrede assets mot `model_contract.json`. Runtime legger ingen JSON-avhengighet til innlasting.

## Runtime-integrasjon og avgrensninger

`FS47SaroSite::Build(Web)` etter `BuildHighwayPower()` og før `BuildHighwayLandscape()`, `Update()` etter area-enter og per tick. `Audit(TArray<FString>&)` returnerer faktisk mesh/root/synlighetsstatus, pensjonerte kollisjonsflater og lokale forsyningstall. Root eier Web-hook, cook-konfigurasjon og nativekjøring. `-S47SaroSiteOff` beholder hele originale bygg- og Grounds-kit. Den separate HighwayPower-linjen kan fortsatt eksistere, som tidligere; dette er ikke en atomisk reversering av hele hovedveinettet.

Byggutskiftingen krever alle tre native-modeller med eksakte LOD-/bounds-/materialkontrakter, fire originale utility-geometrier, original vingeboks/skilt og alle seks kolliderføtter. Først da pensjoneres bare disse seks draw-komponentene og de seks gamle ±0.25m-føttene. Ny .24m-trafofot fra HighwayPower bevares. Gammel meter-glassflate g442, feltlys, gjerdetråder g451, gesimser/ventilasjon og alle story-/interaksjonsankre bevares. Nye vinduer blokkerer ingen ray; vegger/utstyr bruker Visibility og lar den separate interaksjonskanalen være fri. Eksisterende plint, gulv og gangregler er uendret.

Kilde/FBX/native/visuell aksept er separate bevis. Et kilde-PASS hevder ikke at en ny pakke er testet eller installert. Faktisk fasade- og nattlesbarhet vurderes i pakket spill av hovedagenten.

## Gamle vindusformer i grime, separat indekskutt

M4 viste lyse rektangler selv etter at de gamle rammene og persiennene i g454 var fjernet. `dressSaro.ts` hadde også lagt transparent vegggrime 12mm foran den gamle fasaden, med hull for de åtte tidligere vinduene på hver langside. Disse hullene eksponerte lys puss på den nye veggen og så ut som gamle vinduer.

`prepare_stain_trim.py --export EXPORT` leser g455 fra original `geo_003.glb` og skriver `legacy_stains/trim_geo_003.glb`, `proof.json` og `stain-patches.json`. Bare trekanter som i sin helhet ligger på ett av de gamle fasadeplanene Z±5.512, innen X20.1..39.1/Y−.7..3.7, fjernes. Sør246+nord242 gir 488 fjernede og 1224 beholdte trekanter. Utfilen er lest tilbake: opprinnelig attributtbuffer er byteidentisk og beholdt indeksrekkefølge er eksakt. Andre vegger, plint- og bakkegrime beholdes. Denne modellen er et separat kildekutt; `SARO_Site.blend` og dens tre eksportdeler er uendret.

Root kjører `import_stain_trim.py` i eksisterende guard-editorløype med `S47_SARO_STAIN_REPORT=nyrapport.json`, eventuelt `S47_SARO_STAIN_VERIFY_ONLY=1` i fersk prosess. Standardkilde er søskenmappa `legacy_stains/`, alternativt `S47_SARO_STAIN_SOURCE`. Bare den nye modellen `/Game/SaroSite/Trim/StaticMeshes/g455` importeres. Faktisk lagret trekanttall, ett LOD/slot, FP32-UV og metric bounds kontrolleres. Runtime krever hele den gamle g455/1712-kontrakten og ny1224-kontrakt før atomisk utskifting, og binder samme originale atlasmateriale tilbake. NativeAudit kontrollerer den faktiske meshreferansen og at atlasmaterialet er uendret. Ingen ny fysisk eller fortellermessig funksjon er innført.
