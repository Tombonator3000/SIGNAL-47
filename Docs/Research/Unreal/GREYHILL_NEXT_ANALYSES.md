> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Flere Greyhill-systemer som kan undersøkes for Signal / 47

28. oktober 2026. Tillegg til [jaktanalysen](GREYHILL_RESEARCH.md). Prioriteringen gjelder analyse og egne implementeringer i Signal 47. Ingen av funksjonene nedenfor er innført i Signal 47 av dette oppdraget.

## Anbefalt rekkefølge

| Prioritet | System og konkret kilde | Hva som er bekreftet statisk | Nytten for Signal 47 | Neste undersøkelse |
|---|---|---|---|---|
| 1 | Dører og sperringer, `Door_Universal_BP` | Funksjonene `DoorOpenEvent`, `DoorCloseEvent`, `DoorRattling`, `NeedsBreak`, `AnAlienIsBreaking` og lås-/alienflagg finnes i dekodet bytekode. En bane sjekker overlapp, KO, lås og lukket tilstand før dørhandling. | Dører som kjøper forståelig tid i SC04 og SC11, med lydvarsel før inntrenging. | Følg én full bane fra lukket dør til fienden har passert. Kartlegg kollisjon, bruddteller, avbrudd og lagring. |
| 2 | Innendørs og utendørs lyd, `House_Sound_Changer_BP` | Overlap for spilleren krysstoner to vindlag med `FadeIn`/`FadeOut` over to sekunder. Det finnes også en felles uttoning. | La spilleren høre overgangen til dekning og bruke skritt og dørlyd til å lese søket. | Undersøk samtidige soner, retur over terskelen og om lokal fiendelyd fortsatt er lesbar. Sammenlign med eksisterende atmosfæresystem. |
| 3 | Gjenstandsinspeksjon, `Inspect_Item_BP` | Inspeksjonen bytter hvem som mottar input, viser inspeksjons-HUD, har akser for rotasjon og egne hendelser for ulike gjenstander. Avslutning gir input tilbake til spilleren. | Fysiske bevis, lesbar bakside og apparatbetjening uten tap av kontroll etter et panel. | Følg inn/ut, pause, fangst og lasting. Avklar om objektet flyttes eller kopieres, og hva som skjer med verden under inspeksjon. |
| 4 | UFO-landing og bortføring, `UFO_Landing_Field_BP` og `Game_Level` | Landing har adskilte bevegelser, lys, dør og ventetider. `Preacher_Abduction`, biplan-hendelser og andre bortføringsankre finnes i kartets bytekode. | En tydelig kjede av varsling, lys, fysisk handling og etterspor for SC03 og senere opptrapping. | Rekonstruer ett helt møte med startvilkår, spillertilgang, mulige avbrudd, slutt og gjenopptak etter lasting. Ingen påstand om at Greyhill-sekvensen er dynamisk. |
| 5 | Lagring og gjenopptak, `Game_Level.SaveGame` og `LoadGame` | Kartet serialiserer mange eksplisitte verdenstilstander, blant annet alien-synlighet, helse/død og dør-/oppgaveflagg. | Bevis og dører skal være konsistente når en jakt lastes eller gjentas. | Sammenlign lagrede felt med aktive tilstander, tidsstyrte hendelser og inputlåser. Kontroller hva som faktisk gjenopptas, og hva som startes på nytt. |
| 6 | Found footage-modus, `FF_MODE_BP` | Aktivering setter kameraformat til omtrent 1,33, slår på et postprosessvolum, legger til en sound mix og starter støylyd. Deaktivering går til omtrent 1,7778, slår av volumet, stopper støyen og bruker `ClearSoundMixModifiers`. | En konkret referanse til den eksisterende [found footage-planen](../../Design/Proposals/FOUND_FOOTAGE_FORSLAG.md), med samordnet bilde og lyd. | Materialet bak postprosessen er ikke undersøkt. Undersøk hvilke effekter det har og hvordan HUD/fotografier behandles. I egen løsning må deaktivert modus bevare andre lydmikser; ikke kopier en global tømming ukritisk. |
| 7 | Lommelykt og håndtering, `BP_Player` | Variabelen `is_Cranking_?`, opptrapping/nedtrapping av `Flashlight_Intensity`, synlig lys og kollisjonskonus er dekodet. Dette er ikke alene bevis for AI-ens lysdeteksjon. | Troverdig apparatrespons og lesbart sviktvarsel. | Følg lysstyrke mot animasjon, input, hiding og eventuelle overlapp. Hold bildeeffekt, strømtilstand og fiendens persepsjon som tre adskilte kontrakter. Ingen ny batteriøkonomi er besluttet. |
| 8 | Alienens bevegelse og animasjon, `Alien_AnimBP` og animasjonsmontasjer | AnimBlueprint og animasjoner for gange, løp, leting, treff og dør finnes i pakkelisten. De er ikke med i de 111 dekodede filene i første analyse. | Avdekke hva egen alien trenger for at overgang mellom vandring, søk og jakt skal bli lesbar. | Hent og les animasjonsgraf, blend spaces, montasjenotifikasjoner og forholdet mellom faktisk fart og animasjon. Bruk funnene til egne animasjoner. |

De fem første systemene er mest direkte knyttet til de fire avtalte jaktsekvensene. Found footage kan studeres som et parallelt, avgrenset referansespor til planen som allerede ligger i repoet.

## Nye konkrete funn i dette tillegget

I `ExecuteUbergraph_House_Sound_Changer_BP` går inngang via offset 511 til uttoning av utelyd ved 586 og innfading av innelyd ved 634. Utgang via 688 gjør motsatt ved 763 og 816. Det er et konkret soneskifte, ikke i seg selv et generelt system for lydokklusjon gjennom vilkårlige vegger.

I `ExecuteUbergraph_FF_MODE_BP` slår offset 729 på postprosessvolumet; 836 setter format til omtrent 1,33; 895 legger på lydmiksen og 915 spiller støyen. Deaktivering slår av volumet ved 1074 og går til banen som setter omtrent 1,7778 ved 89, tømmer lydmikser ved 148 og stopper støyen ved 159. Disse funksjonene bekrefter modusbryteren, men beviser ikke shaderens VHS-/CRT-effekter eller lesbarheten i spill.

I `ExecuteUbergraph_Inspect_Item_BP` setter offset 185 inspeksjonsflagget. Avslutningsbanen deaktiverer objektets input ved 406, gjenaktiverer spillerens ved 455 og nullstiller inspeksjon ved 500. Dette er nyttige kontrollpunkter for en senere analyse av avbrudd og fastlåst input.

Adressene er desimale Kismet-bytekodeposisjoner i den navngitte funksjonen, ikke maskinkodeadresser. Disse tre observasjonene er kontrollert mot de allerede dekodede filene. Ingen ny gjennomspilling eller full tilstandsanalyse av alle systemene er gjort.

## Hva vi kan hente ut videre

Vi kan hente funksjonsoversikter, tilstander, tidsrekkefølge, samspill med input og lagring, parameterverdier og konkrete feilsituasjoner som egne beskrivelser og testkrav. Full JSON, bytekode og originale assets beholdes i det private lokale uttrekket. Det offentlige repoet får egne analyser og egne implementeringer av mekanikkene.

Dørpassering under jakt er nå fulgt i en [detaljert kodeanalyse](GREYHILL_DOOR_CODE_ANALYSIS.md), med lyd, kollisjon og lagring. Den separate C++-referansemodellen er kompilert; 16 scenarioer er skrevet og 0 kjørt fordi Windows nekter native start. Modellen er ikke integrert i Signal 47. En framtidig praktisk prøve må dekke hele SC04-handlingen.
