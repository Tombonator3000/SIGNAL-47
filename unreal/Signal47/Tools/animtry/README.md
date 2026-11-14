# Tools/animtry: Free100-klipp på byfolket (ANIM-TRY-20261112, → M4, M1)

**Status:** Kjørt for deputy (walk, idle, run, sit) 13. november; walk og idle sett i spillet bak `-S47Free100=walk,idle`, venter på Toms godkjenning · **Gjelder:** UE 5.8.4, Linux, Free100Animations på Tripo-byfolket og MetaHuman-servitrisen · **Sist kontrollert:** 13. november 2026

Første prøve etter [den faste regelen for animasjon](../../../../AGENTS.md) (Tom 12. november): gange, tomgang, jogg og sitting for mennesker skal komme fra ekte bevegelse, ikke håndlaget nøkkelramme. Free100Animations er første kilde. Her tas walk, idle, run (jogg) og sit fra UE5-mannekengen over til våre skjeletter med Unreals IK Retargeter, og spillet kan bruke dem på én figur av gangen uten ny mekanikk.

Lisens: Tom 12. november: fri til bruk, laget av en privatperson på Epic-innhold (Manny/Quinn). Kan brukes i spillet og pakken. Råfiler og importerte uassets ligger bare i `Content/Licensed/Free100`, som Git ignorerer (`unreal/.gitignore`). Ingen lisensierte filer ligger i denne mappa, bare skript, navn og måltall.

## Første kjøring (13. november 2026, deputy)

- Import og retarget går gjennom i 5.8.4. Kroken er bygget inn (`FS47ClipBank::For` med foretrukket prefiks, `FS47TripoNpcs`), og `S47CreaturePreview` har fått `-S47CreatureStyle=npc:<rolle>` med `-S47CreatureCam=front|side|top`: stå 2 s, gå 8 m i 1,25 m/s, stå 3 s. Uten `-S47Free100=` brukes Tripo-klippene som før.
- Fire ting måtte rettes i `retarget.py` (alle står forklart i koden):
  1. Retargeteren skriver klippet for en root uten skinnets enhet og dreining (Tripo-root: skala 100, 90°). Bekkenet skrives derfor om under root i hvile for alle klipp (`to_skin_root`); ellers sto idle 96 m opp i lufta.
  2. Standardstabelens «Run IK Rig» har ingen op som mater målene; den dro kroppen 20° forover med tærne i været. Den er slått av (`S47_ANIMTRY_IK=1` beholder den). Føttene plantes av spillet.
  3. Bare armer og bein rettes inn mot kildens hvilepose. Rygg, nakke og hode beholder Tripo-figurens egen holdning (ellers 17° lut mot kildens 8°).
  4. Malens gange er rask (3,08 m/s, 231 cm per steg-par). Spilt etter distanse i 1,25 m/s ble den et sakte utfall med dype knær. `shorten_stride` gjør stegene 0,6 ganger så lange (`selection.json` `stride`): 277 cm per runde, 1,85 m/s i klippet, 1,9 steg/s i 1,25 m/s (Tripo-gangen: 2,06).
- `S47_ANIMTRY_FORCE=1` overskriver klipp; rigger og retargeter fra en tidligere kjøring gjenbrukes (slett fila for hånd for å lage dem på nytt). Sletting og ny opprettelse i samme prosess virker ikke uten bruker.
- Målt i spillet (side, 8 m): plantet fot flytter seg 0,0–1,4 cm i jevn gange (Tripo-gangen 0,1–0,75 cm); første steg fra stillstand 4,3 cm (Tripo 4,7 cm). Spillets egen plantefeil 0,00 cm (Tripo 1,58 cm). Hoftehøyde 89,6–95,6 cm i gange.
- Ikke gjort: `run` er 6,2 m/s i klippet (sprint, ikke jogg) og er ikke sett; `sit` er ikke sett (deputy sitter ikke; prøv på patron). Idle står med malens forskjøvne føtter og litt lut (10°).

## Kilden

`~/Assets/signal47-free/_inbox/free100animations-20261111/` har zip-fila, `unpacked/` (pakket ut med `python3 -I` og stikontroll, ingenting kjørt) og `CATALOG.md`/`catalog.json` (alle 222 animasjons- og meshpakker, hva de er og om de passer 1986).

- Zip-fila er et helt UE-prosjekt (`EngineAssociation: 5.8`). De 105 egne klippene under `/Game/ThirdPerson/Anims/100/` er lagret av UE 5.8. Epics malinnhold under `/Game/Characters/Mannequins/` er lagret av UE 5.6. Alt er på `SK_Mannequin`.
- **Gange, jogg og tomgang er Epics malklipp** (`MF_Unarmed_Walk_Fwd`, `MF_Unarmed_Jog_Fwd`, `MM_Idle`), ikke blant de 105. Sitting er `AS_SittingOnBench`. Servitrisen kan senere få `AS_BartenderCleanCounter`, `CleanMug` og `PourDrink`.
- Pakkestiene må flyttes til `/Game/Licensed/Free100`. Det gjøres i et kasserbart 5.8-prosjekt, som i `Tools/cosmosshop`. Da fikser redigereren referansene, og hovedprosjektet får aldri `/Game/Characters` eller `/Game/ThirdPerson`.

## Skjelettene våre

| Figur | Mesh | Skjelett | Hvordan den animeres i dag | Retarget |
|---|---|---|---|---|
| Byfolket: deputy (sheriffkontoret), attendant, souvenir_man, waitress-reserven, patron, nora, earl, radio_woman | `/Game/Licensed/TripoNPC/Rigged/SK_Tripo_<rolle>` | Eget per rolle, 23 hovedbein: `root`, `pelvis`, `spine_01–03`, `neck_01`, `head`, armer, bein til `ball_l/r` (Mixamo-rigg fra Tripo, omdøpt til UE5-navn av `triponpcs/prepare_rigged_townsfolk.py`) | `FS47TripoNpcs` → `FS47TripoCreature` (UPoseableMeshComponent) → `FS47ClipBank`/`FS47ClipMotion` (S47CreatureClips): klippene samples bein for bein, gangen spilles etter hvor langt roten faktisk går (lest fra **bekkenets** forflytning i klippet), føttene plantes. Klippene `A_Tripo_<rolle>_{idle,walk,sit,relax,look_around}` er Tripo-klipp retargetet i Blender fra `nordic.glb`. Bevegelsen styres av `S47TownDay` (1,25 m/s). | Trenger IK-retarget: tre ryggvirvler og én nakke mot mannekengens fem og to. Samme kjedenavn, andre bein (`tripo23`). |
| Servitrisen ved disken (MetaHuman) | `/Game/MetaHumans/MHC_Waitress/Body/SKM_MHC_Waitress_BodyMesh` | `metahuman_base_skel`: samme navn og kjede som UE5-mannekengen (`spine_01–05`, `neck_01–02`, `root`) | `S47Waitress` → `US47PersonComponent` → egen `US47PersonAnim` (native AnimInstance): en idle-løkke (`AS_Waitress_Idle`), blikk, blunk og pust; ansiktet kopierer kroppen (`ABP_Face_Waitress`). Har en ubrukt **Gesture**-plass over idle. Står stille, går ikke. | Nesten identitet (`mannequin`). Raskeste vei til et ekte klipp. |
| Lille Roswell og Roswell 2 | `/Game/Licensed/TripoNPC/Roswell/SK_Tripo_Roswell(2)` | Samme 23-beins oppsett (Roswell 1 uten `root`) | Roswell 2: egne Tripo-klipp i `FS47ClipBank` (`A_Roswell2_*`: walk, sneak, run, climb, fall osv.) + `FS47RoswellMotion`. Roswell 1: prosedyrisk. | Ikke med nå: aliens følger designbibelen (få, rolige bevegelser), ikke menneskegange. |
| Dale, de høye/bleke, shadow_man, crawler | statiske bakes eller egne Tripo-kropper | – | – | Ikke med nå. |

Ingen av byfolket bruker AnimBlueprint; alt er native kode som sampler AnimSequence direkte. Derfor er kroken liten: et nytt klipp på samme skjelett og med samme navnemønster kan byttes inn uten ny mekanikk.

## Kjøring (Tom eller én motoragent, én tung jobb av gangen)

Fra hovedkopien `/home/tom/SIGNAL-47` (ikke en worktree: `Content/Licensed` er lokal), etter at denne grenen er slått sammen:

```sh
Q=~/.claude/skills/signal47-nattskift/scripts/q.py
cd ~/SIGNAL-47/unreal/Signal47
# 1. Flytt pakken til /Game/Licensed/Free100 i et kasserbart 5.8-prosjekt, kontroller, kopier inn (ca. 0,25 GB, 2 x ~2 min)
python3 $Q bash Tools/animtry/import.sh
# 2. IK Rigs, IK Retargeter og klipp for deputy (standard: walk, idle, run, sit), deretter kontroll i ny prosess
python3 $Q ~/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd $PWD/Signal47.uproject \
  -run=pythonscript -script=$PWD/Tools/animtry/retarget.py -unattended -nullrhi -nosplash -NoSound -NoLiveCoding
#    servitrisen etterpå: S47_ANIMTRY_TARGETS=waitress_mh S47_ANIMTRY_EXTRAS=1 (idle + tre diskgester)
#    flere: S47_ANIMTRY_TARGETS=attendant,souvenir_man,... ; ny runde: S47_ANIMTRY_FORCE=1
# 3. Se i spillet (kroken er bygget inn): -S47CreaturePreview -S47Case=0 -S47CreatureStyle=npc:deputy -S47CreatureCam=side -S47Free100=walk,idle
#    (med -S47CreatureCapture=/tmp/<ny mappe> -S47CreatureCaptureSeconds=11.4 for bilder)
```

Rapportene `verification.json` (import) og `retarget-report.json` (retarget) har bare navn og tall og kan legges i Git. Se på dem før spillprøven: walk 0,6–2,2 m/s, hver fot plantet 25–75 % av syklusen, idle under 5 cm forflytning, sit senker hoftene 25–70 cm.

## Hva skriptene gjør

- `import.sh` kopierer bare mappene i `selection.json` `copy` (mesh, skjelett, materialer, idle/walk/jog og de 105; ingen kontrollrigger, kart eller blueprints) til et nytt kasserbart prosjekt, kjører `migrate.py` (import, så verify i ny prosess) og kopierer resultatet til `Content/Licensed/Free100`. Stopper hvis Git ville sett filene, hvis en motorjobb kjører (`unreal_guard.py`), hvis det blir under 100 GB ledig, eller hvis `Content/Licensed/Free100` eller arbeidsmappa finnes fra før.
- `migrate.py` nekter å kjøre i `Signal47.uproject`. Den flytter mappene uten redirectorer, lagrer i 5.8-format og leser lengde, bilder, bildefrekvens, rotbevegelse og hvor langt root, bekken og føtter flytter seg i hvert valgt klipp.
- `retarget.py` krever `Signal47.uproject` og skriver bare under `/Game/Licensed/Free100` (`Rigs/`, `Retargeted/<rolle>/`). Før lagring nekter den hvis en annen pakke er endret. Den lager IK Rig for kilde og mål med like kjedenavn (Spine, Neck, Head, Left/RightClavicle, Left/RightArm, Left/RightLeg, bekkenet som retarget-rot, fot- og håndmål, FBIK-løser når motoren gir den), IK Retargeter med standard op-stabel, kjeder koblet etter navn og målposen automatisk rettet inn mot kildens (T-pose mot A-pose). Ett batch-retarget per klipp, med navnet `A_F100_<rolle>_<klipp>` som `FS47ClipBank` leser.
- Gange og jogg: klippspilleren leser forflytningen fra **bekkenet**, ikke fra root-beinet. Derfor flytter `retarget.py` all forflytning i root-sporet inn i bekkenet og holder root i hvile. Går klippet på stedet (malklippene gjør trolig det), legges forflytningen inn igjen fra føttene: den plantede fotens fart bakover er gangfarten. Da står en plantet fot stille i klippets eget rom, slik `ReadGait` venter.
- Kroken (bygget inn 13. november): `FS47ClipBank::For` får et valgfritt foretrukket prefiks og en liste klippnavn. `FS47TripoNpcs` sender `/Game/Licensed/Free100/Retargeted/<rolle>/A_F100_<rolle>_` og listen fra `-S47Free100=walk,idle`. Uten flagget, eller der klippet mangler, brukes Tripo-klippet som før. Loggen sier hvilke klipp som kom fra Free100. Ingen ny mekanikk: `S47TownDay` flytter figuren som før, `FS47ClipMotion` spiller gangen etter faktisk avstand og planter føttene. Når Tom har godkjent et klipp, blir det standard (listen i koden) og flagget går bort.
- Servitrisen (MetaHuman) er ikke med i diffen. Neste steg der er å la `S47Waitress` laste `A_F100_waitress_mh_idle` i stedet for `AS_Waitress_Idle` når flagget har `idle`, og senere legge en diskgest i den ubrukte `Gesture`-plassen i `US47PersonAnim` med lav vekt og sjelden.

## Prøveplan

Én figur og ett klipp om gangen (regel 6), opptil fire kritikkrunder med bilder og video til Tom.

1. **Figur: deputy, klipp: walk.** Han går ruten sin i `S47TownDay` ved sheriffkontoret om dagen, og har fra før både Tripo-walk og stående idle å sammenligne med.
2. **Før:** samme sekvens uten flagget. **Etter:** `-S47Free100=walk`. Samme frø, samme klokkeslett og samme kamera begge ganger.
3. **Video fra tre vinkler** (front, side, ovenfra; fotkontakt synlig), 6–10 s hver, med spillets egen bildeopptaker. Enklest: utvid `S47CreaturePreview` med `-S47CreatureStyle=npc:deputy` (samme fast plassering i dagslys, en rett strekning på 8 m fram og tilbake i 1,25 m/s, så stillestående 3 s), og ta opp med `-S47CreatureCapture=<dir> -S47CreatureCaptureSeconds=10` og en kameraparameter for vinkelen. Ellers tre `-S47ShotStills`-serier i byen.
4. **Fotgliding:** les `retarget-report.json` (stance-andel per fot, laveste fot over bakken). I spillet: ta bilder hver 0,1 s fra siden mens foten står plantet; foten skal ikke flytte seg mer enn 2 cm før den løftes, og aldri gå gjennom bakken. Loggen `S47: Tripo deputy on its own rig: walk … m/s a cycle of … cm` skal vise Free100-syklusen (ca. 120–160 cm).
5. **Sjekkliste (regel 4):** ingen knekk i skuldre og hender (3 ryggvirvler mot 5), løkkepunkt uten hopp, riktig takt mot 1,25 m/s, ingen skala på beina, samme ramme i editor og spill.
6. **I spillsekvens:** Freeform dag 1, gå forbi sheriffkontoret mens deputy går til bilen; se også at han står (idle) og at overgangen gange → stå glir (0,3 s fade).
7. Deretter idle, så sit på patron (sitter ved disken), så servitrisen (MetaHuman idle). Hver ny figur og hvert klipp får egen runde.

## Filer

- `selection.json`: hvilke mapper, klipp, ekstraklipp og målskjeletter.
- `import.sh`, `migrate.py`: pakken inn i `Content/Licensed/Free100`.
- `retarget.py`: IK Rigs, IK Retargeter, klipp, gangfiks og rapport.
- `verification.json`, `retarget-report.json`: lages av kjøringene.
