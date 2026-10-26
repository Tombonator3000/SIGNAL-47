# Caliche: plakater, reklame og hverdagsting

Produksjonspakke til SIGNAL / 47, New Mexico, april 1986. [Katalogen](catalog.json) spesifiserer 48 originale, flate trykkflater som JPG: filmplakater, butikkreklame, emballasjetrykk, blanketter, kvitteringer, skilt og små hverdagslapper. De er laget for senere import og plassering. Pakken leverer ikke nye 3D-modeller, materialbindinger, interaksjoner eller innhold som allerede er satt inn i spillet.

De [31 tidligere Dale- og arkivdokumentene](../Documents/README.md) er en egen pakke. Denne katalogen erstatter ingen av dem og oppretter ingen nye hovedrutebevis eller oppdrag.

De 48 JPG-finalene bruker 23 425 648 byte, mot 149 498 003 byte for PNG-originalene. Det er 84,3 prosent mindre. Filene er mellom 267 399 og 658 959 byte, med kvalitet 82, 84 eller 86. Teknisk kontroll av alle 48 finaler og uavhengig visuell nærlesing av 96 katalogpunkter består. Kontrollen er bundet til de leverte bildehashene.

## Innhold og planlagt plassering

| Filer | Gruppe | Bruk |
|---|---|---|
| 01 til 06 | Seks originale kinoplakater | To utvendige plakatskap og fire foajérammer ved Plaza. |
| 07 til 08 | Billettluke og gamle billetter | Avskjedsark og en løs, gammel kinobillett. |
| 09 til 20 | Lokalreklame og menyer | Mesa Diner, eksisterende bensinstasjon, fôrbutikk, vaskeri, motell, suvenirbutikk, radio og kirke. |
| 21 til 30 | Emballasje og småtrykksaker | Sekketrykk, produktetiketter, fyrstikkbok, takeaway-trykk, matpapir og vanlig musikkassettkort. |
| 31 til 36 | Kvitteringer og hverdagspapirer | Motellblanketter, inerte kvitteringer, postkort og et veikartomslag. |
| 37 til 45 | Arbeidsplass og service | Ordinære SARO-skilt, tomme arbeidsblanketter, lageretikett, trafostasjonsskilt og telefonkort. |
| 46 til 48 | Nabolag og hverdag | Fiktiv savnet-hund-lapp, gammelt loppemarkedsoppslag og usignert handleliste. |

Filmplakatene er *The Desert Sky*, *Friday Matinee*, *Dust County*, *The Night Coach*, *Summer at Blue Lake* og *Visitors from Mars*. De bruker original fiktiv illustrasjon, uten kopierte filmplakater, virkelige skuespillerportretter eller kjente kunstnersignaturer. De to første beholder eksisterende Plaza-overskrifter. De fire øvrige titlene og deres korte slaglinjer er ny bakgrunnsfiksjon.

Nøyaktig tekst, designretning, foreslått plassering, kildereferanser og konkrete kontrollpunkter står per bilde i katalogen. `physical_size_m` angir omtrentlig bredde og høyde på den flate trykkflaten. Dette er importveiledning, ikke mål på en ferdig tredimensjonal gjenstand.

## Kilder og kanon

Eksisterende stedsnavn, priser og skilttekster følger [Unreal-byens interaksjonstekster](../../Signal47/Source/Signal47/Private/S47Caliche.cpp), [SARO- og motellbeskrivelsene](../../Signal47/Source/Signal47/Private/S47Looks.cpp), [bymodellenes trykkatlas](../../Signal47/Tools/townbuildings/make_textures.py), [Mesa Diners atlas](../../Signal47/Tools/mesadiner/make_textures.py) og [suvenirbutikkens vareetiketter](../../Signal47/Tools/calichestreet/souvenir/make_merch_labels.py). Trafostasjonens navn og faretekst følger [dens eksisterende atlas](../../Signal47/Tools/substation/make_textures.py). Katalogens `source_refs` avgrenser hva hver kilde faktisk underbygger.

Plaza er stengt. «FOR 31 YEARS» er den eksisterende avskjedsteksten, og gir ikke et kjent nedleggelsesår. Ingen av plakatene etablerer et slikt år eller en gjenåpning. Caliche Chapel beholder «SUNDAY SERVICE 10 AM». Vaskerireklamen angir ingen maskinantall. Radioannonsen gjelder 1420 AM, adskilt fra observatoriets signal på 1420.405 MHz.

Mesa Motor Oil, Mesa Blend, Sundown Cola, Blue Mesa, Desert Salt og Longwatch er originale fiktive produktmerker for denne produksjonen. De etablerer ingen ny dagligvarebutikk, jernvarebutikk eller forretningsadresse. CALICHE SERVICE brukes som generisk serviceoverskrift ved den eksisterende bensinstasjonen.

Kvitteringene dokumenterer ingen faktisk spillerhendelse. Navne-, dato- og feilfelt på blanketter forblir tomme. Veikartbildet er bare et omslag, og erstatter ikke det aktive servicekartet. Den usignerte handlelisten tilskrives ikke Dale. Loppemarkedsoppslaget gjelder lørdag 12. april 1986, dagen før spillnatten, uten trykt årstall eller ny aktivitet.

## Filer og kontrollgrunnlag

- [GALLERI.html](GALLERI.html) er et lokalt galleri med gruppefilter, fullstørrelseslenker og katalogens trykktekst. Det fungerer uten nett.
- [OVERSIKT.jpg](OVERSIKT.jpg) er en merket miniatyroversikt. Den er et produksjonsverktøy, ikke en ekstra rekvisittflate.
- [catalog.json](catalog.json) inneholder de 48 oppføringene og deres bilde-/genereringsmetadata.
- [generation.json](generation.json) lagrer fulle bildeprompter fra innebygd imagegen, kildebasenavn, kildehasher, dimensjoner, faktisk JPEG-kvalitet, bytes og slutthasher. Den lagrer ikke absolutte lokale kildestier.
- [manual-review.json](manual-review.json) dokumenterer faktisk visuell vurdering av hvert katalogpunkt, bundet til den vurderte JPG-hashen med `reviewed_jpg_sha256`.
- [verification.json](verification.json) skiller teknisk filkontroll fra manuell vurdering. Manglende manuell vurdering står åpen. OCR er ikke en eksakt tekstgodkjenning.

Les de endelige manifestene og kontrollfilene for faktisk leveringsstatus. En godkjent filtype bekrefter ikke automatisk lesbarhet, kanon eller utseende.

## Konvertering og verifisering

Kjør fra reporoten. Python med Pillow må være tilgjengelig. [prepare_pack.py](Tools/prepare_pack.py) gjenbruker [Documents-konverteringen](../Documents/Tools/prepare_assets.py), den eksisterende JPEG-/review-kontrollen og felles galleribygger.

```sh
python3 unreal/ArtSource/EnvironmentGraphics/Tools/prepare_pack.py \
  --inputs /absolutt/sti/utenfor/repo/inputs.json

python3 unreal/ArtSource/EnvironmentGraphics/Tools/prepare_pack.py \
  --verify \
  --review-file unreal/ArtSource/EnvironmentGraphics/manual-review.json
```

Innputtfilen ligger utenfor repoet og har formen `{"assets":[{"id":"CINEMA_DESERT_SKY","prompt":"Full bildeprompt","source_path":"/absolutt/sti/til/original.png"}]}`. Full konvertering krever alle 48 oppføringene. `--partial` kan brukes under produksjon til både konvertering og verifisering. Alle oppgitte kilder valideres, og de etterspurte bildene kodes før første filskriving. Manglende eller korrupte innputtfiler skal derfor ikke overskrive deler av leveransen.

Finalene under `jpg/` er RGB JPEG med sRGB-profil, progressiv lagring, optimalisering og 4:4:4-fargesampling. Langkanten er maksimalt 2048 piksler og hver finalfil maksimalt 650 KiB. Kvaliteten starter på 86 og prøver 84, 82, 80 og 78 ved behov. Faktiske verdier lagres i manifestet. Bildene oppskaleres ikke. Miniatyroversikten er en separat sammensatt oversikt.

Konverteringen leser den stabile, gyldige [sRGB.icc](../Documents/Tools/sRGB.icc), hentet uendret fra den tidligere leveransens første JPG. Dermed gir samme kilde og innstillinger stabile JPEG-bytes på tvers av prosesser. PNG-originalene og innputtfilene med lokale stier beholdes utenfor Git. Ingen finalbilder beskjæres, tegnes over eller tekstredigeres under konverteringen.

En manuell reviewrad bruker `id`, `reviewed_jpg_sha256`, `method: "visual inspection"` og `checks` med den nøyaktige `key_checks`-teksten, `status` og `notes_nb`. Byttes bildet, må den nye hashen vurderes. Gamle kontroller skal ikke automatisk godkjenne nye bildefiler.

## Senere import i Unreal

Importer grafikken som farget Base Color-tekstur med sRGB på. Behold bildets sideforhold og kontroller `physical_size_m` mot den faktiske målflaten. De to utvendige kinoplakatenes målflate er omtrent 1,13 × 1,77 meter; de fire foajéflatene omtrent 1,15 × 1,70 meter. Katalogen beskriver de øvrige foreslåtte trykkflatene.

Legg UV-er slik at hele tekstblokken og papirmarginen beholdes. Unngå strekking, speilvending og trimming gjennom ord eller priser. Kontroller typografi ved vanlig spillerhøyde, skrå betraktningsvinkel og relevante mipnivåer før bindingen godkjennes.

Mesh, papir- eller metalltykkelse, små kantrundinger, glass, separate roughness-/normalkart og fysisk plassering må fortsatt produseres eller tilpasses ved integrasjon. Prøv lys og materialer i den faktiske scenen. Denne bildepakkens produksjonskontroll er ikke en runtimeprøve av slike senere endringer.
