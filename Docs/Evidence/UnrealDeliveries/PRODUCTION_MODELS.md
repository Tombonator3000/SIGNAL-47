> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Gjennomføringsplan og modellresultat: SARO og Caliche

Bestilt 25. oktober 2026. Testet spillkilde `aefd18157be7502ac98d3e1695b4c4fcdd7d26a0`, kandidat `M6`. Akseptbeviset er `Signal47/Tools/townintegration/acceptance.json`. PR155 er flettet i da4e2394 og samme testede M6 er installert: 40 identiske payloadfiler og 46 byteidentiske Saved-filer.

## Rekkefølge og resultat

1. SARO strøm: 2 hovedmaster og 1 service-/trafostolpe innen 100 m, mot 12 tidligere master. Luftlinjen ender før antennefeltet. Feltlys, meterflate, gjerder, takutstyr og historieobjekter beholdes.
2. Bygg 2: skilt på sammenhengende vegg, inntrukket glass og fysisk romdybde. De 16 gamle vindusrammene/persiennene fjernes presist fra g454: 13550 minus 976 bil- og 1872 vindustrekanter gir 10702 beholdte. I tillegg fjernes 488 trekanter fra de to gamle grimeplanene i g455, slik at gamle vindusåpninger i grimen ikke lager lyse rektangler på den nye veggen. Øvrige 1224 grime-trekanter beholdes med opprinnelig atlas. Beholdte attributter/UV/indekser er kontrollert; sammenslåtte foreldre skjules ikke. To varme skiltlys ved eksisterende lampeankre gir nattlesbarhet og følger opprinnelig SARO-strøm/blackout.
3. SARO07: lokalt lisensiert C10 med eget førermiljø og SARO-merking, levende instrumenter/radio, lys og animerte hjul. To parkerte periodemodeller erstatter gamle bilkropper. Sju materialrader bruker nå faktiske BaseColor-kart/faktorer i stedet for feilaktige specularF0-kart; geometri og materialslots er bevart. Personlig stasjonsvogn beholdes. Site/Fleet installeres bare som et komplett sett.
4. Sheriffkontor, motell/resepsjon/Rom 6/bad, vaskeri, fôrbutikk, Plaza-kino med foajé og kirke: 6 originale Blender-familier, 28 deler og 84 geometriske LOD-er. Trekantantall LOD0/1/2: 65154/48610/27450. Mesa Diner, bensinstasjon, suvenirbutikk og 1420 AM beholdes.
5. Eksport, native lesing, pakket funksjon og bilder er egne bevis. Standardinstallasjon: PR155 er flettet i da4e2394 og samme testede M6 er installert: 40 identiske payloadfiler og 46 byteidentiske Saved-filer.

| Leveranse | Kilde/native | Avgrenset pakket kontroll | Dag/skumring/natt |
|---|---|---|---|
| SARO strøm/fasade | PASS | PASS | PASS |
| SARO07 og to ansattbiler | PASS | PASS | PASS |
| Sheriffkontor | PASS | PASS | PASS |
| Motell/resepsjon/Rom 6/bad | PASS | PASS | PASS |
| Vaskeri og fôrbutikk | PASS | PASS | PASS |
| Kinofoajé og kirke | PASS | PASS | PASS |

## Kontroll og avviste forsøk

M6 har 1400 selvtester og 10 ulike målrettede pakkede forløp PASS. 39 faste kamerastillinger ved spillerhøyde og 44 bildehash er registrert og vurdert. De konkrete forløpene er room6-views, saro-views, town-views, motel-views, cab-drive, parking, room6-table, motel-tv, town-phone, motel-roof. Eksisterende dører, gangsoner, telefoner, TV-er, dokumenter, Nora og historieporter beholdes.

Faktiske native kontrollantall fra akseptbeviset: saro: 13, saro_all_lod_geometry: 63, town_atlas: 0, saro_stain_trim: 11, sky_capture_graph: 14, fleet_material_roles: 7, fleet: 92, town: 113, town_all_lod_geometry: 588, town_trim: 109, room_trim: 74. 9 kopierte Blender-originaler er åpnet og kontrollert med pakkede/relative teksturer; 411 kildefiler er kopiert med hash. Avviste kandidater: M1, M2, M3, M4, M5. Deres årsaker og råbevis står i akseptfilen. Tidligere importfeil viser hvorfor LOD0-bounds og trekanttall alene ikke er tilstrekkelig kontroll av LOD1/2.

## GPU-status og øvrige avgrensninger

Følgende er hentet fra sluttbevisets `limits`. Et PASS for de valgte forløpene betyr ikke at en tidligere GPU-feil er rettet:

- Faste kamerastillinger, native API-forløp og ordinære kjøre-/gangkontrollere er prøvd. Fysisk Xbox, full gjennomspilling og lydlytting er ikke verifisert.
- Den opprinnelige skycapture-shaderen utløste gjentatt Vulkan device-lost ved badekameraet. En avgrenset shadergren for refleksjonsopptak består isolert editor-A/B og dette pakkede utvalget. Selve shader-/driverårsaken er ikke bevist, og hele ruten er ikke stabilitetsmålt. Capture utelater fin panorama-/Melkevei-detalj og bruker HoleSky-kalibrert nattenergi; hele finite-hovedshaderen og kvaliteten på volumskyene er bevart.
- Full 110-scenariorunde og PSO-matrise er ikke kjørt. Ingen ny ytelsesgaranti.
- Det eksisterende varselet om raytracing-minne er fortsatt synlig. Renderer og minnebudsjett er ikke overstyrt globalt.
- Adgangsreglene til stengte bygg beholdes. Modellerte interiører gir ingen ny historieadgang.

## Kilder, arbeidsdeling og leveranse

Avgrensede hjelpere skrev Site/strøm, Fleet og Town-kilder. Root integrerte og kjørte tunge jobber sekvensielt med strict unreal_guard. Prosjektbibliotekets Blender/Unreal-profiler var generelle UNVERIFIED-referanser; prosjektets etablerte FBX-/PBR-løype ble brukt. Ingen ny plugin eller betalt modellanskaffelse.

PNMs [Electric Service Guide](https://www.pnm.com/esg-construction-drawings) og [RUS-tegninger fra 1986](https://www.rd.usda.gov/sites/default/files/50-4-specifications-and-drawings-for-distribution-line-construction-Nov.-20-1986.pdf) er visuelle referanser. Stolpetallet er et målt layoutvalg.

Egne kilder: `Tools/sarosite`, `Tools/townbuildings`, `Tools/sarofleet`. Fullstendige lisensierte biler og derivater ligger utenfor offentlig Git, med kreditering og lisenser. Leveringsmappe: `/home/tom/Desktop/Signal47 modeller 25okt`. Bevisrot: `/home/tom/scratch-bm/Signal47-production-models`. Bilde-/kildefiler er knyttet til den testede kandidaten gjennom akseptfilens hash.

## Faktisk levering, 2026-10-25 kl. 02:13 CET

PR155 er flettet i da4e2394 og samme testede M6 er installert: 40 identiske payloadfiler og 46 byteidentiske Saved-filer. Verifisert sikkerhetskopi: `/home/tom/scratch-bm/Signal47-production-models/installation-M6-20261025T010934Z-db509ecb/payload-backup`. Bevis: `/home/tom/scratch-bm/Signal47-production-models/installation-M6-20261025T010934Z-db509ecb/result.json`. Alle1225 kilde-/assetinput matcher kandidaten. Ingen ny normal spillerøkt er startet. Galleri og ni Blender-originaler ligger i `/home/tom/Desktop/Signal47 modeller 25okt`.
