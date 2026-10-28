> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Unreal-ferdigheter, Nanite og MetaHuman-mannekenger

Toms bestilling 28. oktober 2026: undersøk ferdigheter som kan forbedre FX, grafikk, lys, lyd og skygger i SIGNAL / 47, spesielt Nanite til omgivelsene. Første personpass skal bruke MetaHumans som stående/sittende mannekenger i stedet for eksisterende lavpolyfigurer.

Status: kilder, lisens, prosjektkode og lokale karakterressurser er undersøkt. Tre avgrensede subagenter har dekket Nanite, lys/FX og mannequin-ankere. Denne leveransen registrerer funn og en konkret produksjonskontrakt. Ingen ny ferdighet er installert, ingen motorinnstilling er endret, og ingen nye mannekenger er produsert eller plassert ennå.

Målmiljøet er faktisk UE **5.8.4**, Linux, Vulkan SM6. Godkjent terreng/himmel, eksisterende filmprofiler, servitrisen, dialog, personlige lagringer og historiens tilstedeværelsesregler er føringer for videre arbeid.

## 1. Utvalgte ferdigheter

Prosjektbiblioteket ble søkt først. Kildene nedenfor er gjennomgått i relevante deler og sammenholdt med lokal kode og Epic-dokumentasjon. Ferdighetene er faglige referanser, ikke ferdig dokumentert ytelsesgevinst eller nye assets.

| Ferdighet | Konkret nytte her | Versjon, lisens og teststatus |
| --- | --- | --- |
| [scenario-unreal-world-building](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/game-engines/unreal/scenario-unreal-world-building/SKILL.md) | Del solide miljømodeller fra glass, bevegelse, gameplay og småvegetasjon; velg Nanite/instanser/LOD etter innhold. | UE5.8, MIT. `ue_world.py` er uttrykkelig ikke kjørt i ekte Unreal; syntetiske tester beviser ikke integrasjon. |
| [ue-materials-rendering](https://github.com/quodsoler/unreal-engine-skills/blob/f3742d7b688690810df369802b90430324e380b9/skills/ue-materials-rendering/SKILL.md) | Nanite-assetdata, materialskiller, VSM, Lumen og native C++-koblinger. Skiller editorinnstillinger fra pakket runtime. | UE5.8, MIT. Utvalgte Nanite-API-er er kontrollert mot lokale 5.8.4-headere; ingen motor-/ytelsestest av skillen her. |
| [scenario-unreal-lighting-rendering](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/game-engines/unreal/scenario-unreal-lighting-rendering/SKILL.md) | Lys fra riktig sted i lampehuset, riktig rekkevidde, temperatur, skygge og volumetrisk bidrag. Prioriter mottak, telefonvegg, trafo og byskilt. | UE5.8, MIT. `ue_light.py` er ikke kjørt i Unreal; testene bruker fake/offline miljø. |
| [scenario-unreal-vfx](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/game-engines/unreal/scenario-unreal-vfx/SKILL.md) | Stedlige gnister, støv og meteor-/UFO-effekter med tydelig hierarki, bounds, levetid og skalerbarhet. | MIT. Motorhelpers er UNVERIFIED; oppgitte offline-tester er ikke en ekte Unreal-prøve. |
| [ue-audio-system](https://github.com/quodsoler/unreal-engine-skills/blob/f3742d7b688690810df369802b90430324e380b9/skills/ue-audio-system/SKILL.md) | Demping med avstand, blokkering gjennom vegger, romklang, lydprioritet og forankring ved radio/TV/trafo. | UE5.8, MIT. Lokale headere bekrefter utvalgte attenuation-, filter-, fade- og submix-API-er. Må tilpasses dagens egen lydgraf. Ingen runtimeprøve. |
| [scenario-unreal-performance](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/game-engines/unreal/scenario-unreal-performance/SKILL.md) | Skill CPU/GPU, tekstur-/RT-minne, shaderstopp og geometri før tiltak. Én målrettet sammenligning for en konkret endring. | UE5.8, MIT. `ue_perf.py` har kun syntetiske/offline-bevis, ikke målinger fra spillet. |
| [ue-niagara-vfx](https://github.com/kevinpbuckley/unreal-engine-skills/blob/b22b11d0a46bac2d55eae8d3273d2ef22c08a95d/skills/core/ue-niagara-vfx/SKILL.md) og [ue-niagara-sequenced-effects](https://github.com/kevinpbuckley/unreal-engine-skills/blob/b22b11d0a46bac2d55eae8d3273d2ef22c08a95d/skills/core/ue-niagara-sequenced-effects/SKILL.md) | C++/Niagara-kontrakt, User Parameters, start/stopp, festing og samordnet regi etter faktisk spillklokke. | Mål UE5.8, ingen entydig lisensfil funnet. Les som faglig referanse; ikke kopier/installér kode som om den var MIT. Ingen runtimebevis her. |

Festede revisjoner: Scenario Labs `f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b`, quodsoler `f3742d7b688690810df369802b90430324e380b9`, Kevin Buckley `b22b11d0a46bac2d55eae8d3273d2ef22c08a95d`. Quodsolers faktiske LICENSE er MIT. Relevant kildekopi, lisens og kontrollsummer er bevart i `/home/tom/scratch-bm/Signal47-unreal-skills-20261028/sources/`. Ingen hel samling er installert eller gjennomgått som én bevist løsning.

## 2. Nanite til omgivelsene

### Faktisk utgangspunkt

Nanite er allerede aktivert i [DefaultEngine.ini](../../../unreal/Signal47/Config/DefaultEngine.ini), sammen med Virtual Shadow Maps, Lumen/HWRT, distance fields og Vulkan SM6. Det er ikke det samme som at alle meshes bruker Nanite.

Fire tette rutenaturmodeller bruker allerede Nanite: klippen `SM_RN_Cliff_A`, Boulder A/B og Outcrop A. Klippen har 1 998 242 kildetrekanter. Se [selection.json](../../../unreal/Signal47/Tools/routenature/selection.json). [Rutenaturimporten](../../../unreal/Signal47/Tools/routenature/import_assets.py) bevarer Nanite-rastergeometri og har en separat lettere RT-representasjon.

Importløypene for gammel Web/Geo, TownIntegration, MotelV2, PhotoLab, SubstationDetail/SubstationSite og Caliche-vegetasjon har derimot bevisst non-Nanite-oppsett. En global bryter gjør ikke disse importene om. Motellet har dessuten en resident-LOD-/RT-kontrakt i [native.py](../../../unreal/Signal47/Tools/motelv2/native.py) som må gjennomgås ved eventuell endring.

### Kandidater og avgrensning

| Innhold | Anbefaling |
| --- | --- |
| Store klipper, steiner og tette solide ruiner | Første referanse og målrettede videre kandidater. Bevar eksisterende godkjent terrengform og plassering. |
| TB2s ugjennomsiktige murverk, gesimser, tak og tredetaljer | God kandidat etter at TB2 faktisk er importert. Hold glass og interaktive deler separat. Nanite alene leverer ikke de manglende nye bymodellene. |
| Motellets solide bygningsdeler | Mulig kandidat når reell nærgeometri gir nytte. Rett først feil gulvnivå, svevende ting, overlapp og kollisjon fra [Toms feilrapport](../../Active/Reports/PLAYTEST_TODO_2026-10-28.md). |
| Oppgraderte trafoskap, isolatorer og fundamenter | Aktuelt for detaljerte solide deler. Skill brytere, dører, ledninger og kollisjon fra fast miljø. |
| Gress, sage, billboards og smågrus | Behold dagens billige instanser/LOD først. Masked er støttet, men Nanite er ingen automatisk gevinst for tynt overlappende løv. Nanite Foliage er Experimental i målversjonens dokumentasjon. |
| Glass, vann og gjennomsiktige effekter | Behold passende material-/effektløype. Skill solide TV-/lampekabinetter fra glass, skjerm og glød. |

En pilot må bevare hensiktsmessig RT-/Lumen-fallback. Den ordinære RenderData-LOD0 på et Nanite-asset er fallback, ikke kildens fulle trekanttall. Validatorer som forutsetter vanlig LOD-geometri må derfor oppdateres samtidig. `HasValidNaniteData()` er et faktisk tilgjengelig runtime-API i lokale headere; editor-only-spørringer må ikke brukes i pakket spill.

Nanite og VSM passer sammen for store solide skyggekastere. Bevegelig WPO kan invalidere skyggecache. Ikke slå på `r.RayTracing.Nanite.Mode=1` globalt som første tiltak; native Nanite-RT er et separat eksperiment. Nanite erstatter ikke bedre modellering, troverdig montering eller riktig gameplay-kollisjon.

Primærkilder: [Nanite tekniske detaljer](https://dev.epicgames.com/documentation/unreal-engine/nanite-technical-details?lang=en-US), [VSM](https://dev.epicgames.com/documentation/unreal-engine/virtual-shadow-maps-in-unreal-engine), [Nanite Foliage](https://dev.epicgames.com/documentation/unreal-engine/nanite-foliage), [Linux-renderkrav](https://dev.epicgames.com/documentation/unreal-engine/linux-development-requirements-for-unreal-engine?lang=en-US). Linux-støtte krever riktig SM6/Vulkan-funksjonsstøtte; det er ikke løfte om støtte på alle eldre maskiner.

## 3. Lys, skygger, FX og lyd som sitter i miljøet

1. **Armatur og lyskilde skal stemme.** Eksisterende `S47Lighting.cpp`, `S47FixtureLighting.cpp` og `S47ExteriorLighting.cpp` har allerede native kilder og kvalitetsvalg. Rett doble mottakslamper og feil feste-/lyspunkter før nye generelle lyskilder legges til.
2. **Skyggekostnad styres lokalt.** Se på rekkevidde, antall skyggekastende lys og småobjekter før global kvalitet senkes. Bevar store lysblokkere og sammenheng mellom Nanite, VSM og RT-fallback.
3. **FX følger spillhendelse og overflate.** Niagara-effekter må ha rett feste, bounds, avstand og pause-/strømtilstand. Historiekritiske signaler skal være lesbare også når dekor-FX reduseres. Trafoens lysbue finnes allerede med rolig variant, Low/High og målrettet PSO-forhåndslasting; ny undersøkelse skal ikke bygge denne på nytt som om den manglet.
4. **Lyd trenger korrekt kilde og rom.** Prioriter svak trafohum ved skap, telefon/TV/radio ved de faktiske objektene, og demping/filter gjennom stengte dører og vegger. Romklang og ute-/inneovergang skal støtte stedets størrelse og materialer. Bruk avstands-/stemmebudsjett så lavt prioriterte bakgrunnslyder ikke konkurrerer med dialog.

Dagens lyd er en egen WebAudio-lignende graf som ender i én synth-komponent, se [S47WebAudio.h](../../../unreal/Signal47/Source/Signal47/Public/S47WebAudio.h). Den har egen romlig panning og avstandsdemping i [S47WebAudio.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47WebAudio.cpp), ikke en separat native `UAudioComponent` for hvert objekt. Derfor kan ikke en generell `USoundAttenuation`/AudioVolume-oppskrift limes inn og forventes å påvirke hver kilde. En senere lydrettelse må enten tilpasse kildefiltrering i dagens graf eller flytte utvalgte miljølyder til native komponenter uten dobbel avspilling eller endring av dialogklokken.

Dagens `M_WebTone` erstatter også motorens tonemapper. Eksponerings-/grading-råd må tilpasses den kjeden, ikke legges dobbelt oppå den. Se [forrige referansevurdering](../../Active/Reports/PLAYTEST_TODO_2026-10-28.md).

Primærkilder: [Sound Attenuation](https://dev.epicgames.com/documentation/unreal-engine/sound-attenuation-in-unreal-engine?lang=en-US), [Audio Volumes](https://dev.epicgames.com/documentation/en-us/unreal-engine/audio-volumes-in-unreal-engine), [Niagara Effect Types](https://dev.epicgames.com/community/learning/knowledge-base/LJnb/unreal-engine-niagara-scalability-effect-types).

## 4. Første MetaHuman-pass: mannekenger

Toms føring er at de bare skal stå eller sitte på stedene sine. Førstepasset skal erstatte eksisterende lavpoly-kropper med noen få troverdige MetaHuman-varianter, uten ny AI, vandring eller full talerigging for disse bakgrunnsfigurene.

### Eksisterende personer som faktisk kan erstattes

| Figur | Eksisterende modellanker | Første positur | Bevares |
| --- | --- | --- | --- |
| Kvinnen ved vaskeriet | `world.town.woman` | Sittende på benken, radio i fanget | `world.town.proxies.woman`, `radio_woman`, radioen og historiens tilstedeværelse. |
| Mannen ved huset | `world.town.smoker` | Stående mot gaten, tilpasset eksisterende trapp/porch | `world.town.proxies.smoker`, `souvenir_man`, sigarett-/porch-glød og timer. |
| Lastebilsjåføren i diner | `world.diner.objs.driver` | Sittende ved disken | `world.diner.proxies.driver`, riktig stol-/setehøyde og eksisterende samhandling. |

Servitrisen er allerede MetaHuman med fungerende tale og beholdes i egen `FS47Waitress`-løype. Dale og Kinski er fortsatt stemmer, ikke eksisterende lavpoly-kropper. Sheriff/vindusbilde og motellmottak får ikke oppfunnet en fysisk NPC-plassering i dette passet.

**Ressurser som faktisk finnes:** Én ferdig karakter, `MHC_Waitress`, finnes i hovedkopien, arbeidskopien og privat staging. Kropp, ansikt, bob-hårkort og uniform er tilgjengelige. Ingen ferdig mannlig MetaHuman eller passende mannsklær ble funnet. Radio-kvinnen kan bruke den kvinnelige kilden som prototypegrunnlag, men trenger egen passende kles-/hårvariant; sjåfør og røykende mann trenger en ny mannlig preset før baking. Servitrisens modell skal ikke endres for å lage kopier.

### Produksjonskontrakt

- Lag separate, redigerbare stående og sittende positurer fra passende MetaHuman-kilder. Bevar Creator-karakter og skeleton privat så figurene kan animeres senere.
- Bak positurene til statiske kropp-/hode-/hår-/klesmesher med delte spillmaterialer og noen få hensiktsmessige LOD-er. Hold teksturminne nede; statisk kropp alene fjerner ikke store teksturer.
- Ikke bruk kontinuerlig RigLogic, AnimBP, skinning, AI eller strandsimulering for disse mannekengene. Frosne skeletal components er kun en mulig prototype, ikke ønsket sluttløype for stille figurer.
- Forankre via eksisterende `FS47Obj::WorldPosition()` og koordinatkonverteringen i `S47Three`. Stående føtter og sittende bekken/sete er ulike ankere. Ikke legg alle kroppene på en hardkodet gulv-Z.
- Nye modeller må følge `FS47Town::SetPeople` sin synlighet og proxytilgjengelighet. Fjern bare gammel kroppsgeometri; ikke radio, props eller interaksjons-ID-er. Sjåførens gamle separate hode må ikke bli liggende igjen.
- Bruk passende klær for New Mexico 1986, med noen få delte variasjoner. Ikke ta servitriseuniformen som generell bekledning til alle.
- Lisensierte MetaHuman-kropper, DNA, teksturer og deriverte meshes blir i ignorerte/private mapper. Offentlig repo får egne oppskrifter, plassering, manifest og renderte bilder med kreditering.

[Maskinlesbar plan](../../../unreal/Signal47/Tools/metahuman/mannequins-plan.json) fører de tre ankerne, positurene, kildestatus og vernekontraktene. Den er produksjonsdata, ikke en innlastet runtime-erstatning. Ingen av de tre nye mannekengene er bakt eller installert i denne undersøkelsen.

Epics [Skeletal Mesh to Static Mesh Conversion](https://dev.epicgames.com/documentation/en-us/unreal-engine/skeletal-mesh-to-static-mesh-conversion-in-unreal-engine) beskriver nettopp å lagre en posert karakter som statisk mesh. Bruk en kopi/egen pose så den opprinnelige animasjonen ikke ødelegges. [MetaHuman Assembly](https://dev.epicgames.com/documentation/metahuman/assembly?lang=en-US) skiller UE Cine fra UE Optimized og gir grunnlag for minne-/materialvalg før baking.

## 5. Rekkefølge og relevant kontroll

1. Følg opp de rapporterte plasserings-/kollisjonsfeilene og den manglende TB2-integrasjonen. Det gir synlig gevinst som ingen rendererbryter erstatter.
2. Produser mannequin-pakken: radio-kvinne først fra tilgjengelig prototypekilde, deretter en mannlig grunnvariant til sjåfør og røykende mann. Kontroller positur og setekontakt på de faktiske stedene, samt gammel/ny synlighet og proxy.
3. Prøv Nanite på ett faktisk oppgradert miljøsett med tette solide deler. Sammenlign samme utsnitt, skygger, fallback og minne. Bevar dagens vegetasjon og terrengform.
4. Bruk ferdighetene målrettet til lys-/lydkontakt og deretter stedlige FX, innenfor eksisterende grafikkvalg.

Ingen nye fullmatriser eller motorjobber trengs for dette kildenotatet. Videre produksjon følger Toms risikobaserte regel: relevante raske kontroller og én målrettet prøve per endret del, samlet leveranse før eventuell nødvendig full kontroll ved installasjon/release. Ikke bruk skillenes mange syntetiske tester som krav om hundrevis av nye spillkjøringer.
