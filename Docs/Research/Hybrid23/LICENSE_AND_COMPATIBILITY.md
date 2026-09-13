# Hybrid23 — lisens og kompatibilitet

**UnitySplats kan brukes og distribueres i et kommersielt spill uten pluginavgift eller royalty etter den undersøkte MIT-lisensen.** Behold copyright, lisensvilkår og tredjepartsnotiser sammen med kilde og distribuert spiller. Dette gjelder programvaren; det gir ikke automatisk rett til bilder, genererte verdener, treningsdata eller betalte tjenester som brukes til å lage innholdet. [Faktisk lisens ved undersøkt revisjon](https://github.com/arloopa/UnitySplats/blob/6c0258189a2b124af1282fa9236fd9b6637f1a1a/LICENSE.md).

## Undersøkte filer og rettigheter

Den lokale klonen `Artifacts/Hybrid23/Upstream/UnitySplats` var uten endringer ved lisenskontrollen. Revisjonen er `6c0258189a2b124af1282fa9236fd9b6637f1a1a`; `package.json` identifiserer pakken som `com.arloopa.unitysplats` versjon **1.2.0**, Unity **6000.0** eller nyere. Det er revisjonen, ikke bare versjonsnummeret eller en flyttbar `main`-gren, som identifiserer koden i prøven.

| Del | Kontrollert grunnlag | Praktisk betydning |
| --- | --- | --- |
| UnitySplats | MIT, ARLOOPA 2026. Faktisk LICENSE og Third Party Notices lest og kopiert. | Kommersiell bruk, modifikasjon og distribusjon er tillatt med bevarte notiser. Ingen kildepubliseringsplikt følger av MIT. |
| Avledet kode | Pakken navngir gsplat-unity, PlayCanvas, UnityGaussianSplatting, GPUSorting, Spark, SPZ og ZstdSharp med MIT og opphav. GPUSorting-shaderen inneholder også egen full MIT-tekst. | Behold hele tredjepartsoversikten og filenes eksisterende copyright. |
| Unity.WebP | Lokal klone på `9818db6327e09d399bf4fb84da4ed19f03b565b4`, tag **0.3.22**, MIT for Eunpyoung Kims wrapper. | Separat pakkeavhengighet; dens lisens følger også eventuell spillerpakke. |
| libwebp | Unity.WebPs faktiske THIRD PARTY NOTICES inneholder BSD-3-Clause for Google. | Behold copyright, betingelser og ansvarsfraskrivelse ved binærdistribusjon; opphavets navn kan ikke brukes som produktanbefaling. |
| ZstdSharp.dll | Bundlet DLL identisk byte for byte med `lib/netstandard2.1/ZstdSharp.dll` i **ZstdSharp.Port 0.8.8** fra NuGet. MIT bekreftet i pakkemetadata og ved den oppgitte kilderevisjonen. | Egen original lisenskopi og opphav bevares, selv om prøveinnholdet bruker PLY i stedet for SPZ. |
| System.Runtime.CompilerServices.Unsafe.dll | Bundlet DLL identisk med `lib/netstandard2.0/System.Runtime.CompilerServices.Unsafe.dll` i Microsofts **6.1.0**-pakke. Microsofts metadata oppgir MIT. | Denne DLL-en var ikke navngitt separat i UnitySplats-notisene. Et tillegg med Microsoft/.NET-opphav og faktisk kildelisens er bevart her. |

Primærkilder: [UnitySplats tredjepartsnotiser](https://github.com/arloopa/UnitySplats/blob/6c0258189a2b124af1282fa9236fd9b6637f1a1a/Third%20Party%20Notices.md), [Unity.WebP lisens](https://github.com/netpyoung/unity.webp/blob/9818db6327e09d399bf4fb84da4ed19f03b565b4/unity_project/Assets/unity.webp/LICENSE.md), [libwebp-notis](https://github.com/netpyoung/unity.webp/blob/9818db6327e09d399bf4fb84da4ed19f03b565b4/unity_project/Assets/unity.webp/THIRD%20PARTY%20NOTICES.md), [Microsoft NuGet 6.1.0](https://www.nuget.org/packages/System.Runtime.CompilerServices.Unsafe/6.1.0), [ZstdSharp-lisens ved DLL-ens revisjon](https://github.com/oleg-st/ZstdSharp/blob/2cd0c019693bc786a5fe5c3be94e107b24e7267e/LICENSE).

Originale lisensfiler og supplerende DLL-notis ligger i [License/](License/). [PROVENANCE.json](License/PROVENANCE.json) registrerer kildeadresser, filhash, offisielle NuGet-pakkehash, nøyaktig samsvarende DLL-stier og original pakkemetadata. NuGet-arkivene ble lest for kontroll; ingen av disse pakkene ble installert av lisensgjennomgangen. Kopiene endrer ikke upstream-koden.

## Separat vurdering av innhold og tjenester

Pluginens MIT-lisens er ingen gratisplan for Marble, Magnific, bildeverktøy eller Unity. Slike tjenester kan ha egne abonnement, kreditter, eksportbegrensninger og rettigheter til generert innhold. Gjeldende vilkår og den faktisk brukte kontoplanen må være dokumentert for tjenesten som brukes i prøven. Ingen ny betaling eller kontoavtale er inngått som del av denne lisenskontrollen.

Et eget Blender-/Unity-miljø omgjort til splats beholder sitt eksisterende opphav. Et AI-generert bilde, en tjenestegenerert verden og en rekonstruksjon av et fotografi trenger hver sin innholdsproveniens. Å levere disse filene i PLY, SPZ eller GLB gir ikke nye rettigheter i seg selv. Lisenser til eventuelle modellvekter eller treningsverktøy er også separate fra visningspluginen. Ikke bruk en lisens for en åpen renderer som dokumentasjon på rettigheter til alle eksempelverdener.

## Teknisk relevans for SIGNAL / 47

UnitySplats oppgir Linux med Vulkan og OpenGL Core, GPU-sortering der GPU-en støtter den og CPU-sortering ellers. Unity 6000.0–6000.3 støttes med både URP RenderGraph og kompatibilitetsmodus. MSAA er oppgitt støttet. Dette er konkrete forskjeller fra aras-p-prøven; de er **ikke** allerede bestått på vår Intel/Mesa-maskin bare fordi de står i dokumentasjonen. [Pinnet README](https://github.com/arloopa/UnitySplats/blob/6c0258189a2b124af1282fa9236fd9b6637f1a1a/README.md).

Det finnes faktisk kode for proxybasert belysning: en justert mesh med vanlig Lit-materiale tegnes til et separat lysbilde som modulerer splatfargen. Den trenger tilsvarende geometri, kameratilpasning og ekstra rendering. Dette er en tilnærming til dynamisk lys; det gjør ikke splats til vanlige fysiske mesh-overflater. Standardshaderen skriver fremdeles ikke egen dybde. [Proxykomponent](https://github.com/arloopa/UnitySplats/blob/6c0258189a2b124af1282fa9236fd9b6637f1a1a/Runtime/GsplatProxyRelighting.cs), [shader](https://github.com/arloopa/UnitySplats/blob/6c0258189a2b124af1282fa9236fd9b6637f1a1a/Runtime/Shaders/Gsplat.shader).

De tidligere Splat21-resultatene gjelder fortsatt: aras-p/OpenGL feilet, og Vulkan hadde synlig blandingsfeil mot et vanlig objekt. World Labs' anbefalte fork retter Marble-import og sortering mellom flere splatobjekter; dette beviser ikke at vår blandingsfeil er løst. [World Labs’ Unity-veiledning](https://docs.worldlabs.ai/marble/export/gaussian-splat/unity).

## Prioriterte godkjenningsporter

1. **Identitet og ressurser:** Pin begge plugin-revisjoner, behold lokal tilpasning som egen diff, registrer faktisk Unity-/URP-/GPU-/API-konfigurasjon og håndhev minne-/tidsgrenser. Ett Unity-løp av gangen; første rendererfeil skal stanse den berørte prøven.
2. **Overdekking:** Sammenlign splats med/uten ugjennomsiktig forgrunn, bevegelig dør og kamera fra flere vinkler. Kontroller kanter og dybde, også med prosjektets MSAA-/HDR-innstillinger. Splat21s eksisterende feil er en regresjonsprøve, ikke en lukket port.
3. **Lys og kamera:** Samme proxy og kamerastilling med lampe av/på, dokumentert lysforskjell, ingen stale lysmap ved kamerabytte, og riktig bilde i spillerens fotoeksponering. Et penere lysbilde alene beviser ikke korrekt lommelykt eller skygge.
4. **Spillgeometri:** Eksisterende mesh styrer målestokk, gangflate, hindringer, dører og interaksjoner. En splats avgrensningsboks er ikke presis kollisjon. Bevar spillkonstanter og originalfoto.
5. **Levering:** Registrer splatkilde, eventuell tjenesteplan og eksportvilkår; legg lisensmappen med en distribuert eksperimentpakke. Samme kilde-/payloadhash skal følge faktisk kjøring, bilder og konklusjon. Skill generert konsept, render av egen geometri og reell bilde-til-verden-generering.

Lisensgjennomgangen har ikke startet Unity eller godkjent en ny grafikkombinasjon. Faktiske Hybrid23-resultater må leses i prøvens egen rapport; teknisk PASS kan ikke utledes fra denne lisensvurderingen.
