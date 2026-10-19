# Runde 16: bearbeiding og kilder

To valgte malte paneler fra innebygd `image_gen.imagegen`, begge med `transparent_background=true`. Originalene under `sources/` er bevart byte for byte. Begge er 2172×724 RGBA, ikke native 8K. `prompts.json` har hele verktøybestillingen og referanser. Vestpanelet brukte østpanelet som stilreferanse. De fire konseptene er inspisert visuelt; ingen piksler fra konseptene er klippet inn.

## Sammenstilling

Østpanelet fyller slutt-x 0..4095, vestpanelet 4096..8191. `processing.json` inneholder kontrollpunktene for den stykkevis lineære omfordelingen av native azimut. Lange østmessa slutter ved65°, den isolerte butten flyttes fra native134,0884° til130°, og vestveggen ligger250..310°. Sluttaksen er jevn: asimut = x ×360/8192. Ingen perspektivbøying i sluttbåndet.

Kildenes terrengkontur hentes fra tre sammenhengende alfaverdier≥160, medianvindu5 og Gauss sigma0,6. Native baseline er y579. Den opprinnelige konturens steinformer og malte klippebånd beholdes gjennom koordinatprøvetaking, mens høydene skaleres til de bestilte gradene. Sektortak og korte skuldre er oppført i JSON. Nord/sør har26px tak før kantutjevning; soloppgangssektoren har9px; mindre nordvestmesaer44px. De små åpningene og radiene står i JSON. En glatt kubisk faktor senker ryggen til baseline i disse åpningene.

Over horisonten prøvetas native y som579 + (slutt-y −768) × native_relief/slutt_relief. Under horisonten går native579..723 lineært til slutt768..1023. Dermed brukes hele den malte sletta uten gjentatte bunnrader. RGB prøvetas bilineært og minst2px innenfor native landkontur, så native alfakant ikke trekker inn fargede restpiksler.

Begge joins ligger i lave rygger, ved nord x8191/0 og sør4095/4096.192px på hver side blandes parvis mot motsatt side med vekt0,5 × (1 − avstand/191)². RGB og kontur bruker samme blanding. Det legges ingen doble mesaer oppå hverandre. Etter lys og8-bits avrunding settes de to sentrale pikselkolonnene til sitt felles gjennomsnitt. Dette fjerner1-nivås kvantiseringsavvik. Nabotekstur beholdes. Begge joins er RGBA-identiske.

## Maske og lys

Den felles konturen finnes i `silhouette_y.csv`. Alfa er255 × clamp((y − kontur_y +1,25)/1,25,0,1), avrundet til8bit. Alle rader fra y768 er eksplisitt alfa255. Alt over kanten er alfa0; RGB i helt transparent luft nulles. Dette gir høyst2 delvis transparente kantpiksler per kolonne, uten gjennomsiktig land, skyer, stjerner eller luftdis. Samme `shared_alpha.png` settes på begge sluttfiler. `stitched_neutral.png` bevarer det felles omprøvde maleriet før lyssetting.

Natt: kilde-RGB multipliseres kanalvis med(0,13;0,14;0,15), og(4;5;7) legges til. Bakken mørkner svakt nedover. Lav bakrygg får litt mer blått. En smal kjølig kant langs høyere klipper går mot RGB(41;47;55). Åtte svake ranchpunkter på2×2 eller3×2px legges inn som fargekorreksjon på sletta, uten bygningsformer eller glød. Eksakte koordinater og farger er i JSON.

Grålysning: samme nøytrale maleri, samme kontur og pikselmaske. Vestvekt = (1 + cos(asimut −270°))/2; østvekt =1 − vestvekt. Kanalvektene går kontinuerlig fra(0,43;0,30;0,40) i øst til(0,60;0,58;0,63) i vest, med(4;4;6) lagt til. Lysere vestvendte detaljer blandes svakt mot rosa(139;96;108); lav østlig landrygg får varm atmosfærefarge(103;67;72). Alt er begrenset til dekkende land. Ingen farget dis er lagt i lufta. Ranchpunktene legges ikke på grålyset.

Eksport: PNG RGBA8 med innebygd sRGB ICC (`srgb.icc`, Pillow/LittleCMS). Python3.14.4, Pillow12.3.0, NumPy2.5.4 og SciPy1.18.1 fra et eksisterende miljø. Ingen pakker er installert, og ingen kodefiler er opprettet eller endret i repoet.

## Kontrollbilder

- `horizon_night_seam_center.png` og `horizon_dawn_seam_center.png`: full8192×1024 RGBA, rullet4096px. Nordskjøten står midt i bildet.
- `contact_sheet.png`:2048×512, natt over grålysning, transparente felt vist på sjakkbrett. Sjakkbrettet finnes bare i QA-bildet.
- `land_detail_contact.png`:2048×208, de nederste416 radene av hvert bånd forminsket. Natt over grålysning. Ensfargede kontrollmatter ligger bak landets alfa og er ikke en del av assetene.
- `seam_detail.png`:1536×640. Nord til venstre, sør til høyre; natt øverst, grålysning nederst. Hvert utsnitt er768×320 ved1:1. Kun kontrollmatter i lufta.
- `east_0_180_source_detail.jpg` og `west_180_360_source_detail.jpg`: utsnitt av generatorkildene, lysnet til konturinspeksjon.

`verification_initial.json` bevarer første FAIL med for høye lave rygger og1-nivås grålyssøm. `verification_before_ground_fix.json` bevarer neste rapport, der en vilkårlig grense på4 RGB-nivåer fanget normal6-nivås nabotekstur bort fra sømmen. Sluttkontrollen måler den faktiske skjøten mot lokal tekstur. `rejected_stretched_ground_contact.png` viser den tidlige bunnstrekkfeilen som er korrigert i sluttfilene. Disse er historiske kontrollbevis, ikke leverte bildevarianter.

Ingen runtimeinnbygging, WebP-kopi, bygg, spilltest, commit eller push inngår. Skrivingen i repoet er begrenset til de to bestilte mappene. Samtidige kodeendringer fra annet arbeid er observert og bevart, se `verification.json`.
