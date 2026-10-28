# Caliche, seks originale bygningsfamilier

## TB2, 27. til 28. oktober 2026: fem mer detaljerte bygninger

Sheriffkontoret, vaskeriet, fôrbutikken, kinoen og kirken er bygget på nytt med langt mer detalj, etter konseptarkene i `unreal/Concepts/Caliche-2026-10-23` og fotoreferansene i `ArtSource/References/NewMexico-1980s-2026-10-27`. Motellet og rom 6 er uendret TB1; Codex lager et nytt motell separat (Sierra Motor Court). `tb2/TB1_mot_TB2.jpg` viser før og etter.

| Bygning | Kilde | LOD0 trekanter (Shell, Glass, Interior, Fixtures) | Viktigst nytt |
|---|---|---|---|
| Sheriff | `tb2/sheriff.py` | 9 691, 100, 7 560, 1 734 | varm puss med avrundede hjørner, trinnet takkant, SHERIFF med stjerne, gitter, overdekket inngang, radiomast, kjøleanlegg |
| Laundry | `tb2/laundry.py` | 9 935, 10, 21 552, 2 572 | blokkmur i relieff, falmet LAUNDROMAT, butikkvinduer med maskiner, OPEN i neon, tørketrommelventiler, telefonkiosk |
| Feed | `tb2/feed.py` | 24 026, 14, 22 464, 5 954 | falsk front i tre trinn, malt FEED & SEED, rusten markise, lasterampe med sekker, lagerhall i bølgeblikk |
| Cinema | `tb2/cinema.py` | 9 922, 50, 3 426, 7 534 | art deco-fasade, PLAZA-skilt i neon, marquee med 85 pærer, billettluke, foajé med kiosk |
| Church | `tb2/church.py` | 20 182, 136, 8 552, 5 584 | panelbord, bratt spontak, klokketårn med klokke, spisse vinduer som gløder, inngangsparti, innredet skip |

Fotavtrykk, vegghøyder, dører og vinduer fra `design.py` er uendret, og dørene er fortsatt lukket. Under 2,3 m stikker ingenting mer enn 0,6 m ut fra veggen, bortsett fra trinn under 0,2 m og fôrbutikkens eksisterende lasterampe på 0,95 m, som gulvet inne står på. Spillets kollisjon og adgang er derfor de samme.

Nye materialer ligger i `tb2/materials_<bygning>.json`, heter `M_Town_<Bygning>...` og bruker bare de eksisterende teksturene i `textures/`. `build_townbuildings.py`, `check_source_plan.py`, `verify_source.py` og `tb2/preview.py` leser dem i tillegg til `materials.json`. `tb2/<bygning>_NOTAT.md` beskriver hver bygning med svakheter.

Forhåndsvisning av én bygning (Cycles, fem faste kameraer):

```sh
blender -b -t 3 --python-exit-code 1 --python tb2/preview.py -- Cinema tb2 /tmp/cinema 0 street,oblique,door,roof,dusk
```

Faktisk kontroll: `check_source_plan.py` består (170 678 / 108 201 / 54 449 trekanter i LOD0/1/2 for alle 28 deler), `build_townbuildings.py` består, og `verify_source.py` består med 966 av 966 kontroller mot de lagrede FBX-, GLB- og Blender-filene. Motellfilene er de samme som før.

Ikke gjort: nativeimport, nye native materialer for radene i `tb2/`, ny `assembly.json`, pakket spill og bilder fra spillet. Lys i Unreal må settes som ekte lyskilder ved lampene; glødestyrkene er justert for Cycles og må justeres i spillet.


TB1 er generert som redigerbare Blender-originaler med tre egne geometriske LOD-er, fysisk meterskala og ni delte originale 2048-PBR-sett. Siste faktiske kildekontroll består 793 sjekker. Native import, belysning, kontakt med bakken og visuell konseptlikhet må godkjennes fra den pakkede kandidaten hos integrator. En vellykket kildetest er ikke visuell godkjenning.

| Blender-original | Innhold | Deler |
|---|---|---:|
| `Sheriff.blend` | Mineralfasade med faktiske åpninger, dyp sprossevegg, resepsjon, skrivebord, skrivemaskin, telefon, fylkeskart med fysiske nåler, arkiv og bakrom | 4 |
| `Laundry.blend` | Slitt blå/krem fasade, vinduer med faktisk romdybde, tolv myntvaskere, tre tørkere, kontrollpaneler, åpne trådkurver, sammenleggingsbord og arbeidsbenk | 4 |
| `Feed.blend` | Pusset falsk front, bølgeblikkmarkise, varelager, hyller, galvanspann, bulende sydde sekker, disk, lastelem og verktøy | 4 |
| `Cinema.blend` | Trinnvis Art Deco-fasade, fysisk marquee, skilt, billettluke med senket skodde, plakatkasser, foajé og liten kiosk. Gardin avslutter mot ubrukt sal | 4 |
| `Church.blend` | Kledd trekirke med saltak, spisse vinduer, åpent klokketårn og hul klokke, inngang, åtte par benker, alter og sakristi | 4 |
| `Motel.blend` | Åtteroms motellfløy, resepsjon og kontor i scenen `Motel_Court`. Rom 6, møbler og komplett lite bad i egen scene `Room6` | 8 |

Samlet eksport er 65 154 / 48 610 / 27 450 trekanter i LOD0/1/2. `model_contract.json` er autoritet for hver del, faktisk eksportert geometri, materialplasser, bounds og de ulike verdensankrene. Alle delpivoter er lokale nullpunkt, med `[0,0,0]` ekstra deloffset. Rom 6 har et annet verdensanker enn motellgården, og skal aldri plasseres med gårdens anker.

Blender er meter og Z opp. Verdensmapping er `Three=(rootX+Bx, rootY+Bz, rootZ-By)`, yaw 0. Legacy FBX bruker `axis_forward=-Y`, `axis_up=Z`, `FBX_SCALE_UNITS`. Native import må bruke samme målte policy som Mesa Diner: `force_front_x_axis=True`, `MATCH_UP_FORWARD_AXES`, importyaw 90. `exports/axis_probe.fbx` har senter Blender `(2,4,6)` og mål `(.2,.4,.6)`; målt importmål er UE `(200,-400,600)` cm og `(20,40,60)` cm.

`Shell` har ekte åpninger og veggtykkelse. `Glass` er egne flater, `Interior` har rommenes faktiske dybde, og `Fixtures` lar integrator styre lys uten å endre metall, tre og puss. Modellene utvider ikke spillerens adgang til opprinnelig stengte bygg. Opprinnelige gangsoner, kjøregulv, NPC-er, fotoankre, medieskjermer, dokumenter og dynamiske rekvisitter beholdes.

Motell og Rom 6 bruker eksisterende egen seng, native KELVORA-TV-er og originale telefoner, lamper, dokumenter, dører og Nora ved de eksisterende ankrene. Disse er ikke eksportert en gang til i dette settet. De fullstendige sceneobjektene finnes dermed først etter runtime-integrasjonen. Rom 6 har en fysisk modell av badet bak den eksisterende stengte baderomsdøren, med vask, toalett, dusj og gardin. Adgangsregelen beholdes.

Alle ni PBR-familier er originale og prosedyregenererte. Materialenes `texture_tint_linear` skal multipliseres med teksturens RGB i native import, som i Blender-kilden. Normalbilder er DirectX; Blender-grafen vender bare grønnkanalen for korrekt lokal forhåndsvisning. Brukt teksturdata er pakket inn i hver familieoriginal, i tillegg til relative referanser under `textures/`. Blender-originalene er selvstendige å redigere; gjenbygging av Python-kilden leser våre egne hjelpere fra `../substation/meshkit.py` og `../substation/make_textures.py`.

Root kjører tungarbeidet sekvensielt i sitt eksklusive vindu:

```sh
python make_textures.py
blender -b -t 2 --python build_townbuildings.py
blender -b -t 2 --python verify_source.py
```

Python må ha numpy og PIL. `python3 check_source_plan.py` er den lette geometriske kontrollen uten Blender. `verify_source.py` leser de faktiske 84 FBX-filene, 28 GLB-delene, asymmetrisk akseprobe og alle seks lagrede Blender-originaler. Den kontrollerer eksakte trekanttall/materialplasser, nullpivot, meterbounds, UV-areal, sveist geometri og byteidentiske pakkede teksturer. Bilder og native tester er en egen kontroll.

`runtime_replacement.json` foreslår eksakte gamle bladnoder og partielle klipp i uforanderlig CORE-manifest. En hel motellforelder skal ikke skjules. Gamle sammenslåtte materialmesher inneholder også den opprinnelige parkeringsplassen, bilen, bassenget, lyktestolper, planter og ismaskin. Disse er eksplisitt bevart eller avgrenset. Foreslåtte klipp er kildeaudit, og er ikke utført av dette skriptet.

Første GLB-forsøk for Rom 6 ble avvist fordi Blender eksporterte den andre scenens siste utvalg. `use_active_scene=True` retter dette. FBX-geometrien var korrekt. Motellgangen ble deretter rettet til de tre eksakte eksisterende, kant-i-kant gulvplatene. Den nyeste rapporten kontrollerer denne versjonen; tidligere logger beholdes av root i produksjonsarbeidsroten.
