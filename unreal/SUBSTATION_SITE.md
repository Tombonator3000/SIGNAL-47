# Trafostasjonens omgivelser

Tom ba om å fortsette modellarbeidet etter PR150. Dette passet knytter det originale anlegget til stedet med tre kraftmaster, fysisk luftlinje og et vedlikeholdsskur med arbeidsrekvisitter. Ferdig inspeksjonsforløp, port og servicefront beholdes.

## Plassering

Samme Three-root(7890,-1.10,536.20) og yaw180 som trafokitet. De nye mastene står i worldThree[7890,-1.340012,558], [7890,-1.328311,590] og [7890,-1.401798,622]. Høydene kommer fra faktiske terrengtrekanter i originalgeo_007.glb/g873, ikke et generelt høydeestimat. Ingen veikryssing eller ny telefonlinjekobling. Nærmeste gamle telefonmaster vedworldX8015 beholdes.

Ledningene går fra faktiske busstubber B[x=-1.04,0,1.04; y4.65; z7.85] til nye trefasefester. Første spenn har.65m nedheng, de videre32m-spennene.9m. Stolpefot går litt ned i bakken, og bare små fotavtrykk blokkerer gange. Hele ledningsnettets AABB skal ikke bli en usynlig vegg.

Skuret står i Blenderlokalt[6.5,4.9,0] på eksisterende grusbed, med dør mot−Y. Det står innenfor gjerdet med over1m fri klaring til busstativet og uten å dekke trafotank, gate, servicepanel eller rampe. Det er et låst vedlikeholdsskur i den stengte delen, uten ny falsk inngang eller ny oppdragsport.

Ingen ny kulvert legges under flat servicevei uten grøft. Eksisterende kulvert ved worldX7986.2/Z520 er beholdt. Dette er et avgrenset originalmodellpass, ingen migrering eller lisensasset-import.

## Status

Kilde `3f3687d5`, kandidatP1: 222 faktiske kildekontroller,29 native mål-/LOD-/materialkontroller, editorbygg, Linux-pakking og1383 selvtester PASS. Tre LOD-er har25 666 /14 342 /8652 trekanter. Sju korte pakkede scenarioer består: normal inspeksjon med fysisk gange/ray/journal/service/lampe/diskreload, dag, skumring, nær lykt, fjern lykt, skur og gammelt kontrollrom. Alle sju spillerbilder er vurdert, og modellhjelperen har også vurdert de to nye dagbildene. Ingen kollisjons- eller plasseringsfeil funnet. Skuret står i dyp skygge, så rustnyansene er tydeligere i DCC-forhåndsvisningen. Hele110-runden, ny PSO-matrise og ny menneskelig kontrollerprøve er ikke kjørt. Merge/installasjon gjenstår ved dette kontrollpunktet. Kontrollene begrenses til det nye kitet, dag/skumring/natt og eksisterende inspeksjon med lagre/last, pluss et lite gammelt utvalg.

Prosjektbiblioteket er søkt etter Blender/Unreal og kraftlinjer. Bare generelle uverifiserte profilerx-2108195576206488058 ogx-2111849852350894319 var relevante; ingen kode eller geometri derfra brukes. Prosjektets egen målte FBX/PBR/importløype fra Tools/substation videreføres.

## Nærlys

Etter rå nattbildet fra forrige levering er bare den native lommelyktspotten justert. Den finner nærmeste faktisk synlige flate langs strålen, uten å ignorere hele Web-Owner eller dempe ut fra skjulte arealer. Under3.5m gir en jevnt filtrert avstandsrespons omtrent34lux i sentrum; fra3.5m beholdes420cd. Dette kompenserer for fast bildeeksponering, uten ny elektrisk funksjon, endret oppdragsintensitet, flimring, lagring eller globale filmparametere. Første editorprøve mot skapet viste1.472m treff/74.309cd og lesbar tekst. Pakket P1 viser samme1.472m treff på faktisk skap, gain.17693 og74.309cd; ved5.280m er gain1 og420cd. Begge beholder ThreeIntensity64. Teksten er lesbar der forrige nærbilde var utvasket. Rawlogger, bilder og pakkehash er referert i Tools/substationsite/acceptance.json.

Blender-rust ble revidert én gang fra småflekket til større avlange felt. Første kart/render er bevart lokalt under ~/scratch-bm/Signal47-substationsite/first-confetti-rust. To UV-endekapper på kryssarmen er rettet før den faktiske eksportkontrollen; testen er ikke slakket.

En lokal samlet redigerbar Blender-scene er også laget fra begge originale prosjektene. Den har preview-grunn/lys og er et modellarbeidsdokument, ikke et runtime-nivå. De to separate originalprosjektene beholdes i repoet.
