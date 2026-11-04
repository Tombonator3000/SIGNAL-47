# EASTWING-FOTO-TEX: produksjonskilder

BaseColor lages med den innebygde bildegenereringen etter Toms bestilling og godkjente østfløyreferanser. Eksakte prompter og valgte genererte originaler registreres ved leveransen. Bildene er originale syntetiske materialflater, ikke fotograferte eller målte materialskann.

`../photo_texture_tools.py` lager bare lineære datakart. Det endrer aldri BaseColor. Materialstyrte, begrensede mikrodetaljer og lokale maskeringer gir et estimert høydefelt. Brede fargevariasjoner i betong, olje, dekkspor og maling blir ikke direkte til høyde. Garasjens kjente sagfuger har egne smale fordypninger. Kremmalt mur bruker lokal fugekontrast; denne tolkningen må vurderes visuelt ved integrasjon.

Høyde lagres først som 16-bit PNG i `height/`. Tilhørende JSON oppgir offset og spenn i meter: `h = offset + sample/65535 × scale`. NormalGL avledes deretter fra den faktiske lagrede PNG-en og fysisk UV-skala. Bildekoordinatens radretning peker ned, derfor brukes `n = normalize(-dh/dU, +dh/drow, 1)`. En rampekontroll bekrefter OpenGL Y+. Unreal-integrasjonen må bruke riktig grønnkanalkonvensjon; Claude eier materialimporten.

Roughness er lineær 8-bit gråtone uten sRGB-metadata. NormalGL er lineær RGB uten sRGB-metadata. Normalene har ikke innbakt lys. Roughness har egne materialnivåer og slitasjemasker, ikke bare invertert BaseColor. Sømløse datakart bruker periodisk filtrering og kanttilpasning; garasjekartet bruker ikke periodisk wrapping. Gjentakelse av albedo og faktisk utseende i motoren er separate kontroller.

Kjør fra repoets rot med ferdig dimensjonerte BaseColor-filer:

```sh
uv run --offline --no-project --with numpy --with pillow --with scipy python unreal/ArtSource/EastWing/Props/Extras/photo_texture_tools.py --self-check unreal/ArtSource/EastWing/Props/Extras/textures/EW_PolishedConcrete_BaseColor.png
```

Flere filbaner kan oppgis i samme kjøring. Verktøyet krever 2048 × 2048, eller 2048 × 4096 for garasjen. Det utfører ingen Unreal-import, modellendring eller bygging. Høyde og metadata er redigerbare produksjonskilder og trenger ikke importeres som ekstra runtimekart.

## Leveranse 4. november 2026

11 materialsett (33 PNG) i `../textures/` og fire RGBA-dekaler i `../../../Decals/textures/`. Filnavn følger EASTWING-FOTO-TEX. Gulvet er polert betong, ikke VCT. BaseColor har sRGB-chunk; Roughness og NormalGL er lineære datakart. Garasjen er unik 2048 × 4096 over 6,7 × 10,5 m; de gule linjene ligger ved omtrent 29 % og 83 % av bredden. Øvrige sett er 2048 × 2048. Polert betong bruker 2 m, blokkvegg 1,6 m (fire blokker bredt og åtte skift høyt), andre flater 0,5 m. Bøkefibre og stålbørsting følger U.

`generation.json` inneholder de eksakte promptene, utvalgte bildekilder og filbaner. Innebygd ImageGen ble brukt, ikke API/CLI-fallback. Originalene i `generated/` er 1254 × 1254 for kvadratiske bilder, 887 × 1774 for garasje og 2172 × 724 for brede dekaler. FFmpeg Lanczos klargjorde de bestilte leveringsmålene; dette er oppskalering, ikke ekstra målt detalj. Originale genererte PNG-er er bevart uendret. Gulvstøvet fikk én ImageGen-korreksjon fra nesten hvitt til dempet sandfarge.

Rask kontroll: 37/37 PNG-er har riktig størrelse og kanaler, 11 normalsett har normaliserte vektorer innen 8-bit-kvantiseringsfeil, og Y+-rampekontroll består. Dekalenes dekning/faderetning er kontrollert og sammensatt over grå bakgrunn. 2 × 2 gjentakelse er visuelt kontrollert. Mønstrene fortsetter over tilegrensene, men bildegenereringens kantpiksler er ikke identiske; særlig blokkfugen og treets kant bør ses tett på i motoren. Det er ikke gjort en påstand om matematisk identiske albedokanter eller native visuell godkjenning.

Claude eier import og utseende i Unreal. NormalGL krever riktig grønnkanalhåndtering for Unreals DirectX-normaler. Rustfritt stål trenger metallisk materialrespons fra eksisterende materialoppsett. Ingen S47EastWing-kilde, `build_extras.py`, modell eller uasset er endret av denne leveransen. Ingen motorbygg eller installasjon er kjørt.

Prosjektbiblioteket: `ressurser/x-2105024134073946209.md` ble lest som generell ImageGen-referanse. Den er UNVERIFIED og gjelder Three.js; ingen kode eller motorintegrasjon ble gjenbrukt. Tidligere lokale teksturer som ble erstattet, inkludert pågående polert betong/garasjeruhet, er sikret i `/home/tom/scratch-bm/eastwing-foto-tex-before-20261104/`; denne lille sikkerhetskopien kan vurderes ved neste samlede rydding etter Claudes integrasjon.
