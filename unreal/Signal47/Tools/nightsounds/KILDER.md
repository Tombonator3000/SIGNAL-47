# Kilder: nattens lyder i Freeform

**Status:** Gjeldende. **Gjelder:** `clips/*.ogg`, lest av spillet fra `Content/Audio/Raw/night/` (SOUNDSCAPE-20261108, ANIMALS-20261108). **Sist kontrollert:** 8. november 2026.

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

## Bruk

Fra `unreal/Signal47`:

```sh
python3 -I Tools/nightsounds/prepare.py --check     # alle klippene er her
python3 -I Tools/nightsounds/prepare.py --stage     # Content/Audio/Raw/night/*.wav for spillet
python3 -I Tools/nightsounds/prepare.py --originals DIR   # lag klippene på nytt fra nedlastede forhåndsvisninger, DIR/<id>.mp3
```

`Scripts/import-audio.sh` kjører `--stage` etter de andre opptakene, og `Scripts/audio-raw.py` lar `Raw/night` stå. Uten filene lager spillet de samme lydene i koden, som før.
