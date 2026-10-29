# Originale editorbilder for Plaza-plakatene

**Status:** Under arbeid. Avgrenset editorprøve PASS; pakket kontroll UNVERIFIED. **Gjelder:** To utvendige Plaza-plakater, UE 5.8.4 / Linux Vulkan, kilde `05a49e8a90f2122f0e27f34f5251205fdb1567b9`. **Sist kontrollert:** 29. oktober 2026.

[Leveranserapporten](../../../Docs/Evidence/UnrealDeliveries/CINEMA_POSTERS_2026-10-29.md) beskriver omfang, avslag, rettinger og bevisgrenser. [Portabel editorkvittering](editor-r3-receipt.json) binder kilder, fem native assets, 293 bevarte originalfiler, guardresultater, alle ni lysbilder og begge fysiske rutene. Bilder nedenfor er byteoriginale PNG-er fra faktisk Unreal-editorruntime, uten beskjæring, retusjering eller fjernet UI.

| Vedlegg | Hva bildet viser |
| --- | --- |
| [Dagsoversikt](day-cinema.png) | Eksisterende stengt kino og begge utvendige plakatskap. |
| [The Desert Sky, dag](day-cinema-poster-left.png) | Venstre motiv, tittel, orientering og innskrevet 2:3-format bak eksisterende glass. |
| [Friday Matinee, dag](day-cinema-poster-right.png) | Høyre motiv og tittel i samme eksisterende rammeverk. |
| [The Desert Sky, natt](night-cinema-poster-left.png) | Faktisk mørk nattvisning uten ny lamperegi. |
| [Ordinær Closed-interaksjon](plaza-closed.png) | Normal fotrutes faktiske Player.OnUse, «E THE PLAZA» og den bevarte Closed-teksten. |

De fire miljøvedleggene kommer fra produksjonsserie R3; Closed kommer fra normal fysisk kontrakt R1. Alle ni dag-/skumrings-/nattbilder er gjennomsett. Dusk og øvrige bilder er beholdt privat med SHA/bytebinding i kvitteringen. Normalruten består 87 kontroller og 37,384956 m; Off består 69 og 37,403985 m. Begge inkluderer full isolert disk/title/Continue. Den fysiske ruten bruker ett erklært kapittel2-fotutgangspunkt; eksisterende kapittel2-/02:55-UI i Closed-bildet er bevart. Statiske kamerafixtures, API-styrt gange og menneskelig spillinput holdes atskilt.

Den eksisterende RT-resident-advarselen på 83,942 MiB av 400 MiB står i originalbildene. Nattmotivet er mørkt. Maskintest er ikke menneskelig lesbarhets- eller ytelsesaksept. Pakket kandidat planlegges sammen med HUM01; ingen installasjon, offentlig release eller ferdigmelding av hele ART01 følger av disse editorbevisene.
