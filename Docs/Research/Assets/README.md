# Kjøpte biblioteker: lyd og 3D

**Status: Referanse** · **Gjelder:** hva Toms to kjøpte biblioteker kan brukes til i SIGNAL / 47, og reglene for å ta noe inn. Hva som faktisk er bygget, står i [STATUS](../../Active/STATUS.md) og [TODO](../../Active/TODO.md). **Sist kontrollert:** 10. november 2026.

| Bibliotek | Hvor | Oversikt | Kontrollert hvordan |
|---|---|---|---|
| Ultimate SFX for Games ($25), Function Loops: 2335 lydfiler, 7,3 timer | `~/Desktop/SIGNAL 47/bt25_ultimatesfxforgames_softwarebundle/` (samme pakke som `~/Assets/ultimate-sfx/`) | [Lydbibliotek](LYD_ULTIMATE_SFX_2026-11-10.md), måletabell [sfx_catalog.csv](sfx_catalog.csv) | Hver fil målt (lengde, nivå, spekter, anslag, løkkbarhet). Ingenting er hørt av et menneske. |
| Cosmos by Leartes: 78 eide oppføringer og én måned abonnement | Toms konto på cosmos.leartesstudios.com | [Cosmos](COSMOS_LEARTES_2026-11-10.md) | Offentlige sider, Fab og Gumroad. Ingenting lastet ned eller prøvd i Unreal 5.8. |

Måletabellen lages på nytt med `python3 Tools/sfx-catalog/analyze.py` (trenger ffmpeg, numpy og scipy) og oversikten med `python3 Tools/sfx-catalog/make_md.py`. Annen plassering av biblioteket: `S47_SFX_LIB=<mappe>`.

## Regler før noe tas inn

- **Ingen kjøpte filer i Git.** Lyd går inn gjennom den private klonen (`web/src/assets/licensed/tools/build_sfx.py`, så `Scripts/import-audio.sh`) og havner i `Content/Licensed/`, som Git ignorerer. Cosmos-innhold legges under `Content/Licensed/` på samme måte. Begge lisensene forbyr å gjøre råfilene tilgjengelige for andre.
- **Cosmos forbyr bruk som treningsdata eller «reference material» for AI-verktøy.** Cosmos-modeller og -bilder skal derfor ikke mates inn i bilde-til-3D, bildegenerering eller som referansebilder til en agent. Vanlig bruk som assets i Unreal er dekket.
- **Lydlisensen er ikke bekreftet mot kilden.** Biblioteket har ingen lisensfil. Vilkårene vi bygger på, står bare i vårt eget notat i den private klonen (`LICENSES.md`). Tom bekrefter mot Function Loops' EULA før spillet deles.
- **Cosmos-lisensen etter abonnementet er uavklart.** Last ned det som trengs i løpet av måneden, ta vare på kvittering, og spør Leartes skriftlig før lansering. Environments og tools er ikke med i abonnementet.
- Et klipp er ikke i spillet før det står i `sfx/MANIFEST.md` i den private klonen med kilde, utsnitt og bruk.

## Hva lyden gir oss

| Behov i spillet | Beste kilde i biblioteket | Merknad |
|---|---|---|
| Skrekkmøter: en grunn som ligger under, og et støt idet noe blir sett | Horror Ambience 18 og 20 (mørkest), 26, 07, 11 som grunn; Horror Screamers 27, 17, 30, 26 (harde) og 21, 02, 22 (mykere) som støt | Screamerne er 16 s med anslaget i de første 0,1 s: klippes til 1 til 3 s. Grunnene er bare 16 s: to i ulik fase eller kryssfade. |
| Store himmelhendelser | Orchestral Braams 01, 06, 09; Orchestral Impacts 03, 07, 28; ekte stigninger i ABYSS Effect Loops (Shadow 12, 17, 01) | Impacts er nesten ren sub: trenger et lysere lag for små høyttalere. |
| TV og CRT som reagerer på noe over huset (FIX10-5) | Glitches 09, 22, 27, 12 (snø og knitring), 03 og 26 (brum), 25 (bildet faller ut) | Spillet bruker allerede steking som radiosus og mikrobølge som TV-brum. |
| Terminaler og kontrollrom (FIX10-1) | Ui Clicks 02, 04, 05, 10, 18, 20 (tørre og mørke); korte pip fra Checkpoints 06, 12, 03 | 8-Bit og Ui Games Interface er for blanke for 1986. |
| Lytting på arrayet | Ambience Space & Planet 29, 23, 12 (dype og jevne), 30 og 15 (kommer og går) | Nr. 04 er allerede i spillet som «pressure». |
| Diner, motell, dører, brytere | Doors, Household Foley & Devices, Kitchen, Foley | 13 av de 17 klippene spillet bruker i dag, kommer herfra. |
| Varsler og sirener | Alarms & Sirens 01, 28, 20 | 11,5 s med utklang, ikke løkker. Svært høyt nivå. |
| Bilradio og jukeboks sent på kvelden | 1984 Retro Music Production Bundle (darkwave, retrowave, Neon Highway) | Moderne synthwave i 80-tallsstil, ikke autentisk 1986. Ingen country, rock eller jazz i pakken. |
| Adaptiv mørk musikk i lag | ABYSS – Dark Cinematic: 100 BPM, 8 takter = 19,2 s, toneart i filnavnet (D# og G# lar seg stable) | Den eneste delen som er laget for å legges lag på lag i takt. |

Ubrukelig her: Adventure, War Strategy, Shooter Radio Commands og bossreplikkene (engelske stemmer), Guns, Medieval Fighting, Magic Spells, Bluster Shots, Modern Music Kits, pluginen DrumKits.

## Hva Cosmos gir oss

| Sted eller behov | Eid pakke | Vurdering |
|---|---|---|
| Kirken i Caliche | Colonial Convent Church (merket UE 5.0 til 5.8) | Bruk direkte; kandidat til hele bygget. |
| Kirkegård | Gravsteiner og jerngjerde fra Fantasy Gothic Castle; benker og lysestaker fra Gothic Cathedral | Løft enkeltdeler, velg slitte og enkle. |
| SARO-kontor, arkiv, kommunekontor | 1900s Office | Treverk og rekvisitter; for gammelt som helhet. |
| Diner og kafé | Diner Booth Seating, Coffee Shop | Båser direkte; velg bort moderne espressodetaljer. |
| Menn fra staten, deputy | 1950s Businessman / Mafia Character, Police Character | Riggede figurer på Unreal-skjelettet. |
| Adobehus, skur, trailere | Spanish Cottage, Middle Eastern Town, Favela (bølgeblikk, ledninger, vanntanker) | Enkeltdeler. |
| Gård, lekeplass, forfall | Medieval Russian Village (gjerder, brønn), Post Apocalyptic Playground, post-apo-pakkene | Enkeltdeler. |
| Røyk, støv, tåke over playaen | VFX Smoke, VFX Explosion | Bruk direkte; prøves i 5.8. |
| Skritt og bevegelse | SFX Character Footsteps og Movement | Innholdet er ikke bekreftet; se i pakken. |

Biler: trolig for nye for 1986, bare som bakgrunn. Fantasy, cyberpunk og tegneseriestil blir liggende. Cosmos har ikke kraftlinjer, parabolantenner, 80-talls pickup, CRT-skjermer, kassettspiller, telefonkiosk, generator, motell eller UFO-effekter; de løses som før.

**Hentes i abonnementsmåneden** (3D-assets, lyd, VFX og materialer er med): lydpakkene Post Apo Diner SFX og SFX Retro Police Station, VFX-biblioteket (støv, røyk, gnister, elektrisitet), Vintage-kategorien (kassetter, TV-bord, telefon, radioklokke) og materialer (sand, adobe, asfalt, rust). Environments koster ekstra: The Gas Station, Rino's Diner og Western Town er de tre som ville gitt mest.

## Tatt inn så langt

| Dato | Hva | Hvor i spillet |
|---|---|---|
| Før 10. november | 17 klipp fra lydbiblioteket (`lic:`-lydene: dører, brytere, nøkler, radiosus, TV-brum, «pressure») | Se `sfx/MANIFEST.md` i den private klonen. |

## Åpne spørsmål til Tom

1. Bekrefte lydlisensen mot Function Loops' EULA (spillbygg med redigerte klipp, ikke råfiler).
2. Hva abonnementet faktisk gir (Individual eller Studio, kreditter per asset), og om nedlastet innhold er lisensiert etter måneden.
3. Hva som er innløst fra Humble-nivåene (Cosmos-siden for innløsning og Humble-biblioteket).
4. Om Rocket-pluginen virker på Unreal 5.8 under Linux; ellers zip fra nettsiden og manuell kopiering.
