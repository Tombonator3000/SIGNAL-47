# Gjennomføringsplan: SARO og resterende Blender-modeller

Start: 2026-10-25 kl. 00:20. Gren `codex/saro-production-models`, fra `cfadc1c0`.

## Avtalt omfang og rekkefølge

1. SARO strømnett: kartlegg faktiske stolper/spenn og skill strømfordeling fra feltlys. Ett hovednett langs veien, nødvendig serviceavgrening og trafo. Fjern eksakte duplikater, tilhørende tråder og kollidere sammen.
2. SARO bygg 2: erstatt falske vindusflater med vindusnisjer/glass og synlig dybde. Flytt fasadeskiltet til en solid veggflate. Bevar originale interaksjoner, siktlinjer og gangsoner.
3. Kjøretøy: bruk detaljert, lovlig tilgjengelig tidsriktig bilmodell som SARO 07, med egen SARO-logo og tilpasset førermiljø/dashbord. Bevar instrumenter, radio, lys, hjul, Xbox-kjøring og inn-/utstigning. Bytt enkle parkerte biler med realistiske periodemodeller.
4. Resterende modellprioritering fra CALICHE_CONCEPT_PASS.md: sheriffkontor; motell/resepsjon/Rom 6/bad; vaskeri og fôrbutikk; Plaza-kino med foajé; kirke med enkelt interiør. Mesa Diner, bensinstasjon, suvenirbutikk og radio er allerede levert og beholdes.
5. Sekvensiell eksport/import og målrettet pakket kontroll. Faktiske bilder fra spillerhøyde i dag/skumring/natt, og korte prøver av berørte interaksjoner, lagring og bilflyt. Ikke full verdens-/PSO-kø. Flett og installer kun testet kandidat med backup og uendrede Saved-data.

## Arbeidsdeling

| Eier | Filer/område | Status |
|---|---|---|
| station_runtime | SaroSite, HighwayPower, Tools/sarosite | Kartlegging og implementering startet |
| radio_model | Truck, EveningVisual ved behov, Fleet, Tools/sarofleet | Modellvalg og tilpasning startet |
| dish_model | Tools/townbuildings, seks originale modellsett | Ankre og generatorer forberedes |
| root | Plan, integrasjon, maskinkø, kontroll, levering | Aktiv |

Hjelpere skriver kilde og modellgeneratorer parallelt. Bare root starter tunge Blender-/Unreal-jobber, med unreal_guard. Modellkilder og import forberedes parallelt; hele SARO- og bysettet kontrolleres i samme sluttkandidat. Modellfiler er ikke ferdige før eksport, Unreal-import og faktisk bilde-/funksjonskontroll består.

## Kilder og avgrensninger

Prosjektbibliotekets søk og fallback er utført. De to Blender/Unreal-arbeidsflytprofilene er UNVERIFIED, ikke gjenbrukt kode. Den refererte Blender-eksportskillen er lest som kontrollreferanse; eksisterende, prosjektprøvd FBX-/akse-/roundtrip-løype brukes. Ingen ny plugin eller betalt modellanskaffelse.

PNMs [Electric Service Guide-tegninger](https://www.pnm.com/esg-construction-drawings) og [RUS-tegninger fra 1986](https://www.rd.usda.gov/sites/default/files/50-4-specifications-and-drawings-for-distribution-line-construction-Nov.-20-1986.pdf) brukes som visuelle referanser for en sammenhengende strømtilførsel. Det endelige stolpetallet velges fra spillets målte layout, ikke fra et påstått universelt antall for observatorier.

## Leveringsoversikt

| Del | Modell/kode | Import | Pakket funksjon | Visuell kontroll | Levert |
|---|---|---|---|---|---|
| SARO strøm/fasade | Kilde PASS | Native PASS | Gjenstår | Gjenstår | Gjenstår |
| SARO 07 og parkerte biler | Pågår | Gjenstår | Gjenstår | Gjenstår | Gjenstår |
| Sheriffkontor | Kilde PASS | Native PASS | Gjenstår | Gjenstår | Gjenstår |
| Motell | Kilde PASS | Native PASS | Gjenstår | Gjenstår | Gjenstår |
| Vaskeri/fôrbutikk | Kilde PASS | Native PASS | Gjenstår | Gjenstår | Gjenstår |
| Kino | Kilde PASS | Native PASS | Gjenstår | Gjenstår | Gjenstår |
| Kirke | Kilde PASS | Native PASS | Gjenstår | Gjenstår | Gjenstår |

## Konkret resultat før pakket kontroll

SARO får to hovedmaster og én service-/trafostolpe innen 100m, mot tolv overlappende eldre master. Luftlinjen ender før antennefeltet; faktiske reflektorkrysninger er fjernet. Bygg 2 har fysisk inntrukne vinduer og et skilt på hel mur.

Seks bygningsfamilier består av 28 originale modeller og tre LOD-er, 65154/48610/27450 trekanter. Presise sammensatte kutt erstatter gammel statisk geometri. TV, telefon, levende lamper, Nora, dokumenter, dører og opprinnelige spillgrenser beholdes. Kinoen har modellert foajé og lukket sal; modellen innfører ingen ny adgang.

SARO 07 bruker den lokalt lisensierte C10-modellen med originalt tilpasset førermiljø, SARO-merking og animerte hjul. Ansattbilene bruker Fairheaven LT80 og El Alamo78. Kreditering er bevart i CREDITS.txt. Fullstendige lisensierte derivater ligger lokalt utenfor offentlig Git; egne addonmodeller ligger i Tools/sarofleet.

Første ukokte bilde bekrefter faktisk romdybde gjennom sheriffvinduet, men er ikke pakket aksept. Eksisterende RT-minnevarsel er fortsatt synlig, uten endret globalt budsjett. Sluttstatus og begrensninger føres etter pakket kontroll.
