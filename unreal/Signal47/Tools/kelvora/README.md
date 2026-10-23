# KELVORA i Unreal

Egne modeller etter Toms referansebilder. TV på motellkontorets bokhylle og i rom 6, RANGER 640 på arkivskapet i SAROs kontrollrom. Blender-kilder og eksporterte originaler bevares i `~/Desktop/Retro TV og radio`. Ingen kjøpte modellfiler er kopiert inn i denne leveransen.

`S47RetroProps.cpp` legger rekvisittene under de eksisterende sceneankrene. Kontor-TV og rom 6 får hver sin dynamiske instans av KELVORAs skjermmateriale, med starttilstanden fra den gamle skjermen. De eksisterende materialhåndtakene peker på de nye instansene. Skjermen er unlit slik at apparatets eget romlys ikke reflekteres som en hvit flekk over filmen. Kabinettet beholder ekte PBR-lys og skygger. De eksisterende filmtegnerne bruker samme materialhåndtak som før, og lyd, strøm, av/på, 47-sekundershendelsen og lagring drives av de eksisterende modulene. Piloten følger skjermens strøm. Nye kabinetter svarer på bruksporing med de opprinnelige ID-ene.

## Assets og gjenbygging

`Content/Kelvora` inneholder originale UE 5.8.4-mesher, tre detaljnivåer, kollisjon, PBR-materialer og teksturer. Modellene har meter som kildeenhet, cm i Unreal, pivot midt under og front +X. Skjermen bruker materialplass 1 og lineær UV0. Skjermmaterialet korrigerer FBX-importens V-retning før spillets eksisterende EmU/EmV-matrise, slik at både film og det lagrede TV-bildet står riktig vei. **CRT-flatens normal må vende ut mot fronten.** Dette sjekkes i Blender-eksporttesten; feilen med innovervendt CRT er rettet i både Blender, FBX, GLB og alle LOD-er.

`import_kelvora.py` kjøres med UE Python. `S47_KELVORA_SOURCE` velger kildemappen, standard er leveransen på skrivebordet. Skriptet bygger `/Game/Kelvora` fra FBX og PBR-kart; kun egne ressurser under denne stien endres.

Den gamle kontor-TV-en er flettet sammen med kontoret. `office_without_tv.py EKSPORTMAPPE UT` klipper kun trekanter med alle tre punkter innenfor det gamle apparatets lokale volum. Original eksport og webkilder endres ikke. Den originale skjermen er en hel, separat mesh som skjules. Hyllas større trekanter bevares. `import_office.py` importerer resultatets tre klippede mesher; `S47_KELVORA_OFFICE` velger UT, standard `~/scratch-models/kelvora-office`. Kopier `office-patches.json` til `Content/Kelvora` etter importen. Originalmeshenes materialer blir sittende på komponentene.

Ved ny verdenseksport må kontorpatchene regenereres fra den nye eksporten. Ikke bruk gamle g-ID-er mot en ny scene.

`DefaultGame.ini` koker `/Game/Kelvora` og stager patchoversikten. FBX- og Blender-kilder på skrivebordet er fortsatt redigerbare. Filmene ligger i den eksisterende lokale `WebData/tv`-pakken og blir tegnet som levende film, ingen filmrute er bakt inn i modellteksturene.

## Målrettede prøver

Bygg editor-målet og den pakkede Linux-utgaven. Kjør eksisterende `screenshots.sh`-funksjon `shot` med selvtest, radio, kontorets `programme-near`, `frame` og rom 6 `-S47Ch4Drive=tv`. Se bevis og rapport i `~/scratch-bm/Signal47-kelvora`. Ikke start en tung Unreal-jobb mens en annen agent bygger, koker eller kjører spillet.
