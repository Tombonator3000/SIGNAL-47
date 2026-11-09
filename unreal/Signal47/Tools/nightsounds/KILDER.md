# Kilder: nattens lyder i Freeform

**Status:** Gjeldende. **Gjelder:** `clips/*.ogg`, lest av spillet fra `Content/Audio/Raw/night/` (SOUNDSCAPE-20261108, ANIMALS-20261108, WEEKLY-CASE-20261109). **Sist kontrollert:** 9. november 2026.

Alle opptakene er merket **CC0 1.0** på sin egen Freesound-side, kontrollert 8. november 2026. Siden viser lisenslenken til `creativecommons.org/publicdomain/zero/1.0` og ingen annen lisens. CC0 krever ingen kreditering, men opphavet føres her. Filene kan ligge i Git.

Det som ble klippet, er Freesounds offentlige forhåndsvisning i 128 kbps (`cdn.freesound.org/previews/...-hq.mp3`). Det er samme metode som for kameraets lukker og nattlydene fra 4. oktober, se `web/THIRD_PARTY_NOTICES.md`. Ingen innlogging ble brukt. `prepare.py` klipper hvert stykke etter `sources.json`, gjør det mono i 44,1 kHz og legger på et høypass under buldret. Den sterke delen blir jevnet til -18 dBFS RMS med topp under -1 dBFS, og stykket får korte inn- og uttoninger. Ledningslydene får de to siste sekundene tonet over de to første, så de kan gå i løkke uten skjøt.

| Klipp | Freesound | Opphavsperson | Tittel | Utsnitt |
|---|---|---|---|---|
| coyote0, coyote1 | [711262](https://freesound.org/people/buzzatsea/sounds/711262/) | buzzatsea | coyotes howling in santa fe | 3,5 til 19,5 s og 29,5 til 40 s |
| coyote2, coyote3 | [256533](https://freesound.org/people/dkaufman/sounds/256533/) | dkaufman | coyote barks and howls | 12 til 21 s og 30,5 til 38,5 s |
| coyote4 | [833772](https://freesound.org/people/darcydunes/sounds/833772/) | darcydunes | Coyote_02 | hele |
| train0 | [591873](https://freesound.org/people/FunWithSound/sounds/591873/) | FunWithSound | Train Horn 71 (Distant, Clean) | hele |
| train1 | [574424](https://freesound.org/people/TRP/sounds/574424/) | TRP | Train horn distant wind Irving Nature Park 190929 | 0 til 18 s |
| train2 | [593082](https://freesound.org/people/FunWithSound/sounds/593082/) | FunWithSound | Train Passing By 146 with Engine, Horn (Distant) | hele |
| mill0 til mill6 | [131924](https://freesound.org/people/felix.blume/sounds/131924/) | felix.blume | A windmill is squeaking alone in the desert (USA, Arizona) | sju knirk mellom 4 og 113 s |
| mill7 | [611149](https://freesound.org/people/LiftPizzas/sounds/611149/) | LiftPizzas | Windmill 01 | 2 til 9,8 s |
| wires0 | [829000](https://freesound.org/people/bruno.auzet/sounds/829000/) | bruno.auzet | gusts on overhead power lines | 60 til 88 s, løkke |
| wires1 | [147599](https://freesound.org/people/felix.blume/sounds/147599/) | felix.blume | Wind blowing and howling in an antenna and its wire on the top of the chilean cliff. | 32 til 62 s, løkke |
| flush0 | [574656](https://freesound.org/people/TRP/sounds/574656/) | TRP | Wing flaps doves fly away NOTL 181225 | 0,2 til 4,2 s |
| flush1 | [616623](https://freesound.org/people/TRP/sounds/616623/) | TRP | 121003 Pigeon flock fly away, wing flaps, Toronto | 0 til 6,8 s |

Fra før i prosjektet og brukt videre uendret: hundene (`dog0`, `dog1`), ugla, vinden og sirissene, alle CC0 og ført i `web/THIRD_PARTY_NOTICES.md`. Kveget (`cow_moo_0..2`) er fra BigSoundBank, merket CC0 og ført i `Docs/Research/FREE_ASSET_CATALOG.md`.

Ingen lyd er fra film, spill eller ukjent kilde, og ingen har ikke-kommersiell lisens. Utvalget er gjort etter beskrivelse og lydstyrkekurve, ikke etter lytting. Tom må høre om klippene passer.

## Fôrskjulet (WEEKLY-CASE-20261109, patch 2)

Lydene til ukesaken i fôrskjulet: slag i tre, spark mot døra, flis som revner, kjetting og hengelås, noe tungt inni og pust. De er lest av `S47ShedSound` (`shed*`-klippene) og hentet og kontrollert 9. november 2026 på samme måte som over: hver side viser bare CC0 1.0 (`creativecommons.org/publicdomain/zero/1.0`), og det som er klippet, er den offentlige HQ-forhåndsvisningen uten innlogging. Nedlastingene ligger utenfor repoet med kilde, lisens og sha256 i `KILDER.txt`. Hvert klipp er ett slag eller ett spark, siden spillet spiller ett om gangen: opptak med flere slag i ett er ikke brukt. Slag, spark og flis beholder anslaget (`fade_in` 2 til 4 ms). Pusten er klippet til løkke.

| Klipp | Freesound | Opphavsperson | Tittel | Utsnitt |
|---|---|---|---|---|
| shedknock0 | [637643](https://freesound.org/people/kyles/sounds/637643/) | kyles | knock and bang on wood doors various | 25,6 til 25,88 s |
| shedkick0, shedkick1 | [452609](https://freesound.org/people/kyles/sounds/452609/) | kyles | door wood old heavy kick open good impact 1 with rattle 1 without | 0 til 1,5 s og 2,12 til 3,55 s |
| shedsplinter0 | [723974](https://freesound.org/people/Saladsounds/sounds/723974/) | Saladsounds | Wooden Door breaks | 0 til 1,3 s |
| shedsplinter1 | [262538](https://freesound.org/people/Talitha5/sounds/262538/) | Talitha5 | Locked door being forced open | 0,05 til 1,85 s |
| shedsplinter2 | [160213](https://freesound.org/people/qubodup/sounds/160213/) | qubodup | Kicking/Forcing/Breaking Wooden Door | 0 til 1 s |
| shedsplinter3, shedsplinter4 | [452561](https://freesound.org/people/kyles/sounds/452561/) | kyles | axe chops through thin wood trailer rv wall tearing splinters | 0,25 til 2,1 s og 50,15 til 52,1 s |
| shedchain0, shedchain1, shedchain2 | [452576](https://freesound.org/people/kyles/sounds/452576/) | kyles | chain thick rattle hooked onto metal1 | 0 til 2,44 s og 3,45 til 6,68 s og 7,15 til 8,93 s |
| shedchain3 | [330635](https://freesound.org/people/leonelmail/sounds/330635/) | leonelmail | Padlock Chain Lock Unlock | 43,15 til 45,75 s |
| shedchain4 | [345070](https://freesound.org/people/metrostock99/sounds/345070/) | metrostock99 | CHAIN & LOCK ON FENCE SOUND | 0,4 til 3,9 s |
| shedheavy0, shedheavy1 | [438308](https://freesound.org/people/craigsmith/sounds/438308/) | craigsmith | G24-09-Falling on Wooden or Dirt Floor with Splinters | 8,55 til 9,9 s og 11,35 til 12,45 s |
| shedheavy2, shedheavy3 | [377016](https://freesound.org/people/Mrthenoronha/sounds/377016/) | Mrthenoronha | Dragging Furniture | 2,05 til 5,25 s og 17,05 til 21,05 s |
| shedheavy4 | [578493](https://freesound.org/people/PostProdDog/sounds/578493/) | PostProdDog | Dragging a body across a concrete floor 2 | 11,4 til 15,4 s |
| shedbreath0 | [574208](https://freesound.org/people/mucky_pete7/sounds/574208/) | mucky_pete7 | MONSTER BREATH 1 | 4,6 til 11,9 s |
| shedbreath1 | [470900](https://freesound.org/people/Zrte/sounds/470900/) | Zrte | Monster breathing | 0,5 til 7,5 s |
| shedbreath2 | [350414](https://freesound.org/people/bevibeldesign/sounds/350414/) | bevibeldesign | Animal Heavy Breathing | 3,04 til 9,1 s |
| shedbreath3 | [476032](https://freesound.org/people/Sheyvan/sounds/476032/) | Sheyvan | Heavy Dog Breathing | 12,9 til 18,7 s |

Utsnittene er funnet ved måling (RMS i korte vinduer og `silencedetect`), ikke ved lytting. Kjente svakheter: 578493 og 574208 er svake opptak, så bakgrunnsstøyen øker når de jevnes; 350414 har et jevnt støygulv; 470900 er trolig bearbeidet. Mangler klippene, lager `S47ShedSound` den samme typen lyd i koden. Tom må høre om de passer.

```sh
python3 -I Tools/nightsounds/prepare.py --originals DIR --only shed   # skjulets klipp på nytt fra DIR/<id>.mp3
```

## Bruk

Fra `unreal/Signal47`:

```sh
python3 -I Tools/nightsounds/prepare.py --check     # alle klippene er her
python3 -I Tools/nightsounds/prepare.py --stage     # Content/Audio/Raw/night/*.wav for spillet
python3 -I Tools/nightsounds/prepare.py --originals DIR   # lag klippene på nytt fra nedlastede forhåndsvisninger, DIR/<id>.mp3
```

`Scripts/import-audio.sh` kjører `--stage` etter de andre opptakene, og `Scripts/audio-raw.py` lar `Raw/night` stå. Uten filene lager spillet de samme lydene i koden, som før.
