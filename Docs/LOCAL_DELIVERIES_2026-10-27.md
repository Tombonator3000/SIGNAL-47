# Lokale leveranser samlet til GitHub, 27. oktober 2026

Samlingen omfatter ferdige lokale kilder, konsepter, bilder og leveringsrapporter. Kildegrenen er `codex/local-deliveries-design-bible-20261027`, basert på `main` ved `0ee7c248`. Den er en reposamling, ikke en ny nedlastbar spillutgave. Fletting til main føres separat etter prosjektets påkrevde skjermbildematrise.

## Designbibel og visuelle leveranser

- [42-siders designbibel som PDF](DesignBible-2026-10-27/SIGNAL-47-Designbibel.pdf), fra samtale `01a17d06-3c66-78f3-b21b-b784af2cc91a`.
- [Redigerbare kilder, kart, konsepter og proveniens](DesignBible-2026-10-27/README.md). Original PDF er byteidentisk; nye bygg skriver til egen mappe.
- [Andre lokale leveranser og filindeks](LocalDeliveries-2026-10-27/README.md): motel-/dinerkonsepter, UI-forslag, historiske runtimebilder, Blender-render og grafikk. Identiske filer peker til eksisterende repoarkiv.
- [Fotoreferansenes katalog](../unreal/ArtSource/References/NewMexico-1980s-2026-10-27/README.md). Originale tredjepartsfotografier beholdes lokalt.

Konsepter er produksjonsmål, ikke ferdig modellering. Motellets utvendige ombygging og nytt dinerinteriør er ikke implementert gjennom denne innsamlingen. Historiske skjermbilder og arbeidsmodeller er merket med sin faktiske opprinnelse.

## Spillkilder og kontrollert lokal levering

MetaHuman-servitrisens og atmosfærepassets egne kilder, verktøy og bilder er nå med i denne samlingen. [MetaHuman-rapporten](../unreal/METAHUMAN_WAITRESS.md) og [atmosfærerapporten](../unreal/ATMOSPHERE_2026-10-27.md) dokumenterer faktisk pakket spill, vurderte bilder, installert standard og begrensningene. Samlet atmosfære-R1 er lokalt levert med 1 593 selvtester og bevart MetaHuman, himmel og personlige lagringer. Arkiveringen endrer ingen installasjon.

Følgende tidligere leveranser finnes allerede i main: [daghimmel](../unreal/SKY.md), [nærdetaljer og armaturer](../unreal/NEAR_DETAILS_2026-10-26.md), [terreng og natur](../unreal/TERRAIN_ROUTE_2026-10-26.md), [ankomst og Dales kontor](../unreal/ARRIVAL_OFFICE_2026-10-26.md), [ku-modeller](../unreal/COW_MODELS.md), [dokumentgrafikk](../unreal/ArtSource/Documents/README.md) og [miljøgrafikk](../unreal/ArtSource/EnvironmentGraphics/README.md). Originale Blender-kilder og egne native assets er bevart på sine etablerte steder.

## Historieleveranser

Manglende NP1-, T02- og SC01-kvitteringer er samlet [som historiske tekster](LocalDeliveries-2026-10-27/README.md#historiske-leveringskvitteringer). SC01s ferdige kilde finnes allerede på GitHub ved [`dbae31775d3690383417c0f3652de16fb8ac00ca`](https://github.com/Tombonator3000/SIGNAL-47/commit/dbae31775d3690383417c0f3652de16fb8ac00ca), på grenene `codex/story-sc01` og `unreal-port-core`. [Kildeindeksen](LocalDeliveries-2026-10-27/receipts/SC01_INDEX.md) angir leveringsstatus og videre integrasjon.

SC01 er ikke integrert i dagens main-baserte terreng-/MetaHuman-/atmosfærebygg. Den eldre grenens verden eller kildefiler er ikke kopiert over nyere arbeid. SC02 er uferdig forberedelse og er ikke aktivert. Arkivering av en kvittering betyr ikke ny installasjon eller spilleraksept.

## Hva som bevares lokalt

Lisensierte MetaHuman-/Fab-originaler, private råopptak og kildebygg, vannmerkede tredjepartsreferanser, rå samtalearkiv, personlige Saved-filer og bygg-/cachelogger publiseres ikke. Egne native assets bruker Git LFS. En ren offentlig klon inneholder derfor ikke alt privat innhold som kreves for å gjenskape installert spill.

Manifestene kontrollerer kopier, kildehash og lokale avhengigheter. Ingen ny Unreal-, Blender- eller pakkejobb er nødvendig for denne innsamlingen. De tidligere avgrensede spilltestene beskrives i rapportene; full 110-scenario-matrise for samlet kilde er ikke hevdet utført.
