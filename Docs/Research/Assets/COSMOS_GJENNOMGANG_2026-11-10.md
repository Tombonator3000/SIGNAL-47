# Cosmos og Desktop-pakker: gjennomgang for SIGNAL / 47 (D46-8)

**Dato:** 10. november 2026 · **Metode:** kun filnavn og mapper (`find`, `unzip -l`), og `.uproject`/`.uplugin` lest direkte fra zip-filene. Ingen bilder åpnet, ingen zip pakket ut, ingen nedlasting, ingen Unreal, ingen endringer i repoet. Filstørrelser er summert filstørrelse (ikke fysisk diskbruk).

**Lisens-påminnelse:** Cosmos forbyr bruk som treningsdata eller referansemateriale for AI-verktøy, og rå filer skal ikke ligge i det offentlige repoet. Kilde: `Docs/Research/Assets/COSMOS_LEARTES_2026-11-10.md`. Ingenting av dette skal inn i Git; bruk `Content/Licensed/`.

**Lisens:** Tom bekreftet 10. november 2026 at alt er kjøpt via Cosmos med utviklerlisens (også zip-filene på skrivebordet).

---

## 1. Inventar

| Sted | Innhold | Merknad |
|---|---|---|
| `Desktop/SIGNAL 47/Content from Cosmos/Smoke/` | Utpakket VFX Smoke-prosjekt (`SmokeBundle_VFX.uproject`, EngineAssociation 5.0), 61 filer: Rocket-røyk og -tåke (NS1 Smoke Chimney, Smoke Toxic, Fog), Showcase-kart, Cyberpunk-demomaterialer | Røyk er det mest brukbare her |
| `Desktop/SIGNAL 47/SFX from Cosmos/` | Footsteps 64 wav (betong, grus/tre/teppe; løp og gange), Footsteps-Crouch 38 wav, Landings 16 wav (betong, tre, jord, teppe) | Lyd, ikke målt i denne runden |
| `Desktop/SIGNAL 47/3D MODELLER/` | 324 filer totalt, herav 26 `.glb` i rot og mappa `Gard-modeller-Blender` | `.glb`-filene er ikke identifisert som Cosmos i oversikten (flere er Tripo-modeller ifølge TODO). Ikke vurdert ut over navn. Kandidater etter navn for jeep-saken (JEEP-FORM): `classic jeep 3d model.glb`, `military jeep 3d model.glb`; for sheriffbilen: `police car 3d model.glb`. Må åpnes i Blender før bruk |
| `Desktop/SIGNAL 47/Music/` | 2 mp3: menu og Track 1-Intro Drive | Ikke vurdert |
| `Desktop/SIGNAL 47/bt25_ultimatesfxforgames_softwarebundle/` | Ultimate SFX (Function Loops) | Egen gjennomgang: `Docs/Research/Assets/LYD_ULTIMATE_SFX_2026-11-10.md`. Ikke en del av denne rapporten |
| `Desktop/*.zip` (tabell under) | 13 zip-filer | Se seksjon 2 |

## 2. Zip-filer på skrivebordet

Størrelse: zip på disk / summert uutpakket størrelse fra `unzip -l`. Antall uasset/umap/fbx er fra listen.

| Zip | Zip / uutpakket | Filer | uasset / umap / fbx | Engine (.uproject/.uplugin) |
|---|---|---|---|---|
| AbandonedHospital | 978 MB / 1001 MB | 447 | 329 / 2 / 101 | 4.22 |
| AbandonedVillage | 2239 MB / 2332 MB | 582 | 542 / 4 / 0 | 5.0 |
| DemonicVillage | 1622 MB / 1892 MB | 6953 | 3654 / 9 / 0 | 5.0 |
| MilitaryBarriers | 257 MB / 259 MB | 113 | 108 / 1 / 0 | 4.22 |
| PostApocPlaygroundVol1 | 183 MB / 189 MB | 47 | 40 / 2 / 0 | 5.0 |
| PostApocPlaygroundVol3 | 248 MB / 252 MB | 67 | 60 / 2 / 0 | 5.0 |
| PostApocalypticCoffeeShop (prosjekt `Airport`) | 1864 MB / 1918 MB | 947 | 935 / 7 / 0 | 5.3 |
| SorcerersHut | 2762 MB / 2818 MB | 775 | 622 / 2 / 0 | 5.0 |
| SpanishCottage | 3842 MB / 4139 MB | 657 | 389 / 8 / 0 | 5.2 (plugin Water, WaterExtras aktivert) |
| VintageRoom | 4770 MB / 7311 MB | 573 | 400 / 2 / 48 | 4.19 |
| BrightEye 1.2.0 | 0,3 MB / 0,9 MB | 28 | C++ (21 .cpp/.h) | plugin, EngineVersion 5.8.0 |
| ProjectOptimizationToolkit 1.0 | 71 MB / 295 MB | 509 | C++ (396 .cpp/.h) | plugin, EngineVersion 5.8.0 |
| QAAgent 0.9.0-beta | 44 MB / 205 MB | 139 | C++ (57 .cpp/.h), 38 .obj | plugin, EngineVersion 5.8.0 |

Alle fire med `EngineAssociation` under 5.8 (4.19 til 5.3) må migreres og testes i 5.8 før bruk. Pakkene fra 4.x er de største risikoene.

---

## 3. Vurdering per pakke

Skala: **A** bruk direkte, **B** løft enkeltdeler, **C** la ligge.

| Pakke | Vurdering | Hva som passer SIGNAL / 47 | Hva som ikke passer |
|---|---|---|---|
| **VintageRoom** (4.19) | **A/B** | Beste kilde for 1980-tallsinteriør: kassettkasser (`MI_CasetteType01/02`), teppe, stoler, lysekrone, gardiner, askebeger, brødboks, kamera, bøker, malerier. Passer hjemmet til Dale, motellrom, SARO-kontor | Eldre stil innimellom (gjør en sjekk). 7,3 GB uutpakket, nesten bare teksturer. Ta ut enkeltmeshes, ikke `Showcase`- og `Overview`-kart |
| **Smoke** (Content from Cosmos, UE 5.0) | **A** | NS1 Smoke Chimney (SARO-generator, gården), NS1 SmokeToxic (brent ring, D45-28), NS1 Fog (tåke over playaen, T16-13), MI_NS1_Cloud | Cyberpunk-demomaterialer og Showcase-kart |
| **SpanishCottage** (5.2) | **A/B** | Adobehus med dører, vinduer og luker som egne BP-er (`BP_Door`, `BP_Windows`, `BP_Wheel`), landskapslag `Material_A–E`, `M_Cracks`, `M_Decal`, brann-SubUV (`M_Fire_SubUV`) og flare. Passer Caliche-hus, motell og skur | Vann-plugin er ikke nødvendig. Fallende løv (`Fallen_Leaves`) passer ikke ørken. 4,1 GB uutpakket |
| **AbandonedVillage** (5.0) | **B** | Forlatte hus i Caliche: `Structure` 100, `Roof` 28, `Props` 25, `Decals` 15, `MI_Cracked_01/02`, landskapslag for gjørme og gress. Forfallsdetaljer til gater og hus | Hele landskapet og `Overview`/`Showcase`/`Map_Dawn`-kartene. Vegetasjon er grønn og passer ikke ørken. Stor (2,3 GB) |
| **PostApocPlaygroundVol3** (5.0) | **B** | Kirke- eller skoleområde i Caliche: benk, skilt, basketballkurv, lekehus, karusell, huskestativ, `MI_House_01b` | Helt rusten/ødelagt utseende på noen deler; sjekk farger |
| **PostApocPlaygroundVol1** (5.0) | **B** | Lekeplassutstyr (jungle gym, sklie, sett 02 og 04), rust-materialer | Samme som Vol3; mer ødelagt |
| **MilitaryBarriers** (4.22) | **B** (allerede i bruk) | Trådgjerde (`BP_T_Wall_*_BarbedWire`), sandsekker/tønner (`BP_BasicSandBarriers`, `BP_BasicBarrel`). SARO-gjerdet (D45-14) bruker denne | Hindringsbane, militære barrierer. Pakken er ikke i Cosmos-oversikten: bekreft lisens før den blir liggende i `Content/Licensed/` |
| **AbandonedHospital** (4.22) | **B** | Institusjonsinteriør: sengen (`MI_Bed01`), toalettbås, vogn (`MI_Cart01`), knust speil, tørr vegetasjon (`MI_Branches`, `MI_Bark`). Kan gi SARO-gang eller en klinikk i Caliche | Stil er for forlatt-horror, og 4.22. 1 GB uutpakket. Kun enkeltmeshes |
| **DemonicVillage** (5.0) | **B** for gravsteiner og kirke; **C** for resten | Kirkegård (`Cemetery` 19): gravsteiner og gjerder kan gi kirkegården. Kirke (`Church` 37): bygningsdeler. Lysestaker (`BP_Candle`) | `Ritual`, `Hangman`, `Bones`, `BP_CorpseMovement`, `BP_Bone_Sway`, demonisk tone, `BPP_PLA_*`-hus. Pakken er 1,9 GB med 6953 filer; hent bare enkeltmeshes. `HDRIBackdrop`-pluginen er ikke nødvendig |
| **PostApocalypticCoffeeShop** (`Airport`, 5.3) | **B** | Diner- og motellinteriør: disk (`BP_Counter_01`), lamper (`BPP_Lamp_*`, `BPP_Shorter_Lamp_*`), luggage, dressing table. Passer motell og diner i Caliche | Forfall og plast-bokser (`BPP_Scatola_Plastica`), alkoholflasker. Sequences og sublevels er ikke nødvendige |
| **SorcerersHut** (5.0) | **B** for landskapslag og enkelte steiner; **C** for resten | Landskapslag `LL_Asphalt`, `LL_BrokenAsphalt`, `LL_Gravel`, `LL_Mud` for veikanter og grusvei. Steinmeshes (`Rock` 11). Motvekt for PT-ROCKS | Heksehytte-rekvisitter (bøker, flasker, lys, bein), `Cinematic`-sekvenser, `clarens_midday`-HDRI (dagslys, feil for natt). 2,8 GB |

### Plugins

| Plugin | Hva det er | Anbefaling |
|---|---|---|
| **BrightEye 1.2.0** (PullsarDev, 5.8.0) | Editor-only arbeidslys for å lyse opp mørke områder i viewporten uten å endre banen. `Category: Level Design`, modul `Editor`, tillatt på **Linux** | **Kan vurderes** etter gjennomgang av 21 kildefiler. Den rører ikke spillbanen. Ikke installert |
| **ProjectOptimizationToolkit 1.0** (Leartes, 5.8.0) | Editor-verktøy: asset-rydder, optimaliserer og avhengighetsanalyse. Modulene `POTCore`, `POTDepViz`, `POTCleaner` er **kun Win64** (`PlatformAllowList`) | **Ikke installer** nå. Ingen Linux-støtte i beskrivelsen, og en rydder som sletter assets er risikabel mot repoet og `Content/Licensed/`. Kan bare brukes på en kopi, og bare etter manuell gjennomgang |
| **QAAgent 0.9.0-beta** (Leartes Studio, 5.8.0) | Runtime- og editor-modul som samler krasj- og feilrapporter, legger til spillkontekst og sender analyse til en **separat QA-server**. Beta, kun `Win64`. Zip-filen inneholder ferdige Win64-byggfiler (`Intermediate`, `.obj`) | **Ikke installer.** Runtime-modulen er beregnet på spillet, og den sender data til en ekstern tjeneste, som krever egen gjennomgang av personvern og hva som sendes. Beta på Windows betyr at den ikke er testet på Linux |

---

## 4. Hva Tom bør laste ned neste fra Cosmos

Koblet til TODO-punktene som er åpne. Rekkefølgen er etter nytte og hva som er innen abonnementet.

**Innen abonnementsmåneden (3D, lyd, VFX og materialer er med; environments er ikke med):**
1. **VFX-biblioteket** (støv, røyk, gnister, elektrisitet): WEATHER-20261108 (støvstorm), FIX10-1/-2 (generatorgnister, transformator), D45-28 (brente ringer), T16-13.
2. **Materialer** (sand, adobe, asfalt, rust): ørkenbakke og veier i Caliche, PT-ROCKS-undergrunnen.
3. **Vintage-kategorien** (kassettspiller, TV-bord, telefon, radioklokke): TV-er (FIX10-5), telefonautomaten (PT-PAYPHONE), SARO-kontoret og motellrommet. Vintage-sidene viser 4 kreditter (`Docs/Research/Assets/COSMOS_LEARTES_2026-11-10.md`, avsnitt 6 punkt 7); sjekk kreditter før nedlasting.
4. **Post Apo Diner SFX** (fås via ticket) og **SFX Retro Police Station**: lyd til diner og sheriffkontor (TOWNSFOLK, TOWN-USE).

**Environments (kjøpes enkeltvis; prioritert etter nytte):**
6. **The Gas Station Environment** (91 til 93 meshes, UE 5.3 til 5.8, 50 x 50 m): bensinstasjonen i Caliche, PT-PAYPHONE. Størst gevinst.
7. **Wasteland** (168 meshes, tagger `pole` og `fence`): kan gi kraftstolper og gjerder for STATION 01 og trafoen. Ikke bekreftet at stolpene finnes. Kjøp først etter å ha sett innholdslisten på Cosmos.
8. **Western Town / Village Pack** (ca. 10 bygg): Caliche-gaten, TOWN-USE, butikker og hus.
9. **Rino's Diner Environment** eller **Post-Apo American Diner**: diner-erstatning hvis den nåværende ikke holder.
10. **Auto Repair Garage** (`sb-auto-repair-garage`): verkstedet for T16-20 (crafting og verksted) og SARO-garasjen.
11. **Retro Police Station Environment** (Gumroad, `RetroPoliceEnvironment`): sheriffkontor og cellen. Passer SHERIFF-CAR-CHOICE og NPC-DAY.
12. **Post Apocalyptic Desert Environment** (87 meshes): ørken, stein og kanskje kjøretøy. Ubekreftet innhold; sjekk før kjøp.
13. **Vegetation Pack** (45 meshes): ørkenbusker, bare hvis innholdet er tørt. Ellers la ligge.

**Ikke på Cosmos (per oversikten), må løses utenfra:** kraftlinjer og stolper (hvis Wasteland ikke har dem), radioteleskop og parabolantenner, 70/80-tallspickup, CRT-skjerm, kassettopptaker, telefonkiosk, generator, motell, tilhenger, åker, mesa-stein, UFO-stråle.

---

## 5. Åpne punkter for Tom

- Bekreft at AbandonedHospital, AbandonedVillage, DemonicVillage, MilitaryBarriers, SorcerersHut og VintageRoom er kjøpt på Cosmos (utgiver og lisensbevis). SARO-gjerdet (D45-14) bruker MilitaryBarriers allerede.
- Cosmos-lisensen etter abonnementet er fortsatt uavklart (`COSMOS_LEARTES_2026-11-10.md`, avsnitt 7). Last ned det som trengs i måneden, og ta vare på kvittering.
- Migrering: VintageRoom og AbandonedHospital er 4.19 og 4.22. Test i 5.8 før noe går inn i spillet.
- Kun Tom avgjør om BrightEye skal installeres. ProjectOptimizationToolkit og QAAgent anbefales ikke installert nå.
