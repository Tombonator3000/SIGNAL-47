# Environment27 — faktisk prøve og grenser

## Spillerreisen som skal bevares

Fortsett en historisk lagring, fullfør arkivets reisemål, reis til STATION 01, gå mellom instrumentene og inn i brakka. Undersøk fastmerket og den lokale lampen i begge rekkefølger, eksponer to faktiske feltfoto, les tidsloggen, reis tilbake og framkall filmene. Lagre og gjenoppta både på feltet og på SARO. Manglende foto og mislykket eksport skal kunne repareres uten tap av framdrift.

`run-station26-checks.sh` kjører denne reisen via spillets API og CharacterController i egne profiler. Environment27 legger til kontroll av synlig lampeemisjon og nøyaktig gjenoppretting av SAROs lys/tåke etter retur. Dette er ikke ekte tastatur-/museinput eller en blind forståelsestest.

## Miljø og ytelse

`run-environment27-checks.sh --preview` gjenopptar en ferdig feltlagring gjennom faktisk Continue og tar seks uredigerte spillerbilder. Full kjøring tar bildene først, varmer opp i 12 sekunder og måler deretter fem kamerabevegelser på 24 sekunder hver. Alle Update-intervaller beholdes, også lange enkeltbilder; skjermbildefangst og lagringsskriving inngår ikke i den målte runden. Kamera/HUD styres av prøven, mens miljøet kjører i det vanlige release-bygget.

60-FPS-målet vurderes separat fra scenariotestens PASS: p95 ≤ 16,67 ms og p99 < 20 ms. Maskin, oppløsning, grafikk-API, fokus, kvalitetsnivå og bildegrense følger målingen. Denne runden dekker feltets visuelle belastning, ikke full spilløkt, menyskifter, alle maskiner eller ren GPU-tid.

## Pakke og direkte feltprøve

Den samme kilden pakkes med kilde-, bygg- og filhash. Nyutpakket starter prøves separat; originalfoto, filrettigheter og standardstarter kontrolleres. `Start-STATION01.sh` oppretter en egen vedvarende prøveprofil med to historiske foto og uutførte P06–P09. Brukeren velger CONTINUE CHECKPOINT i den vanlige menyen. Eksisterende prøveprofil overskrives ikke. En API-kjøring av denne konkrete startlagringen kontrollerer at ekte Continue åpner feltet; faktisk menyinput er fortsatt en separat prøve.

## Visuell vurdering

Originale Unity-bilder og faktiske eksporterte JPEG vurderes ved ankomst, fastmerke, lampe, kabel, fasade og arbeidsbenk. Blender-render og designbibelens konsept er egne kildetyper. Godkjent forbedring krever leselige oppgaver og troverdig skala, sammenhengende terreng/materialer og fri gangrute. En slik godkjenning er ikke en påstand om fotorealisme eller ferdig 5–6-timersspill.

Åpne brukerprøver: vanlig tastatur/mus, menyfokus, blind forståelse/tidsbruk og subjektiv lydmiks. Resultater og eventuelle ytelsesavvik står i `Evidence/verification.json`.
