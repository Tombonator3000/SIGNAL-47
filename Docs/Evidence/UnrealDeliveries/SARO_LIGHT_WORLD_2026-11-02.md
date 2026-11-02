# Andre nattleveranse: rombrytere, respons og Station 01

**Status:** Referanse. **Gjelder:** Unreal Linux/Vulkan, runtime `f96349fd66288aee96deedd3aa36a433e53a0a62`, [PR327](https://github.com/Tombonator3000/SIGNAL-47/pull/327), main-fletting `32ab029d`. **Sist kontrollert:** 2. november 2026.

Installert kl.01:10 CET. Standardstarteren bruker denne pakken. Alle164 personlige Saved-elementer er kontrollert uendret før og etter installasjon og isolert Fortsett. Forrige payload og Saved er bevart privat; ingen offentlig spillrelease.

## Endringene

- Fire fysiske veggbrytere styrer kontrollrom, korridor, arkiv og operasjonskontor. Valgene lagres separat fra strømforsyning og hendelsesflimmer. Gamle saker starter med lys på. CRT-er og instrumenter fungerer med taklyset av; bord-/vinduslys beholder egne drivere. Samme system brukes i Story og Freeform.
- Gasstasjonens canopy/skilt og Props-styrte fasader får samme dagslysmaske på emissive materialer og faktiske lys. Upstream strøm og flimmer beholdes. Øvrige native TownBuildings-/MotelV2-dekormaterialer trenger fortsatt presis ute-/inneklassifisering; LIGHT-01 står delvis åpen.
- Freeforms motorlyd følger faktiske encoderendringer. En akseptert trackingkommando gir relérespons, og gjennomført fysisk ut-og-tilbake-test gir én lokal kvittering. Pause, retur og Continue utløser ingen falsk servicekvittering. Arkivet viser jobbid/original, kontroll, markering, konklusjon og innlevering fra eksisterende lagret jobbtilstand.
- Det delte landskapet følger nå Station 01s faktiske kvartdreining, høyde og kamerakoordinater. Freeform-gangporten er tilgjengelig; Story beholder kapittelporten. Fem native hut-deler med tre LOD-er ble avlest og bygget besøkt.
- Fremste dekorative dobbeltgjerde med egne tråder er fjernet. Tilsiktet gjerdelinje og strømledninger er bevart.
- Eksisterende blå telefonmodell brukes ved bensinstasjonens opprinnelige telefonanker. Bare målte24+24+2 legacytrekanter er pensjonert. CPU-data beholdes på de tre faktiske CalicheHomes-sluttassetene; geometri, slotter og kollisjons-/lyd-/historieankre er ellers bevart. Riktig sluttmodell ble kontrollert i pakket spill.

## Prøving

| Kontroll | Resultat og avgrensning |
| --- | --- |
| Editorbygg og Linux-pakke | PASS. Én tung jobb om gangen under produksjonsreservasjon og guard. |
| Pakkede selvtester | PASS,1982 kontroller. |
| Fire rombrytere | PASS,42 kontroller både i editor og pakke: faktiske øyeray/OnUse, native lys/CRT, separat supply Cut/Restore-fixture, full JSON/Flow og gamle saves uten ny nøkkel. Romplassering er eksplisitt; ikke en naturlig gåtur eller egen Story-gjennomspilling. |
| Service/utstyrsrespons | PASS,53 kontroller og3 Flow-restores. Faktisk S-03-bevegelse, engangsrespons, ny tracking, pause og gjenopptak. Menneskelig lydvurdering gjenstår. |
| Sammenhengende stationreise | PASS,14 kontroller,2,88km reell autopilot-/Drive-bevegelse, fire faktiske områdeoverganger, vanlig Walker-input gjennom port og hut, disk/Flow-Continue og retur. Ingen destinasjonsteleport. Maks målt kanonisk framesteg5,11m; ikke en menneskelig kjøreprøve. |
| Pakkede telefon-/landskapsressurser | PASS,tre målrettede kontroller. |
| Dag/natt | Pakkede bilder fra samme syntetiske posisjon kl.12/23 er gjennomsett. |
| Installert Fortsett | PASS fra isolert kopi av personlige lagringer; originalene er uendret. |

## Bilder og åpne funn

Bryterbildene viser på/av i samme kandidat, med samme kamera før og etter faktisk bryterbruk. De er ikke en sammenligning av to spillversjoner.

| Sted | På/dag | Av/natt |
| --- | --- | --- |
| Kontrollrom | [Taklys på](SaroLightWorld-2026-11-02/control-on.png) | [Taklys av](SaroLightWorld-2026-11-02/control-off.png) |
| Operasjonskontor | [Taklys på](SaroLightWorld-2026-11-02/office-on.png) | [Taklys av](SaroLightWorld-2026-11-02/office-off.png) |
| Bensinstasjon | [Dag](SaroLightWorld-2026-11-02/gas-day.png) | [Natt](SaroLightWorld-2026-11-02/gas-night.png) |

[Stationgård](SaroLightWorld-2026-11-02/station-yard.png), [inne i hut](SaroLightWorld-2026-11-02/hut-inside.png), [retur etter områdeskifte](SaroLightWorld-2026-11-02/road-return-after.png), [kort maskinlesbar oppsummering](SaroLightWorld-2026-11-02/summary.json).

WORLD-01 er fortsatt synlig som svarte striper i returbildet. Stationens visuelle polering er ikke ferdigmeldt. WORLD-04 er ikke gjenskapt: bilde16 peker mot nedre Records_front_1_lower_paint i authoredRoom og bakenforliggende partitionRecords-bokser; første klokkeprøve var delvis forstyrret av sen armaturutskifting, mens feltprøven sto ute. Dette er avgrensning, ikke bevist årsak eller retting. Alderete-gård, motellskilt/-vegger, full EVIDENCE-01-reise og REACTIVE-01 står fortsatt i TODO.

Prosjektbiblioteket ga ingen verifisert runtime-løsning for disse konkrete feilene. Egne eksisterende lys-, lyd-, Drive-, Walker-, lagrings- og asset-systemer er gjenbrukt. Samlet disk-/kopikontroll fra22:20 er gjenbrukt; lett df før store operasjoner viste minst167GB ledig. Ingen ny rydderunde; dagens kandidat/cache beholdes til videre arbeid og neste samlede kjøring.
