# Forslag til forbedringer

Skrevet 3. oktober 2026, etter at kapittel 1 ("The Second Exposure") kom inn i nettversjonen. Punktene står i den rekkefølgen jeg ville tatt dem. Det som krever en beslutning fra Tom, står først, fordi det styrer det meste av resten.

En kortere versjon som kan deles, med en statuskolonne for beslutningene, en testliste og en grafikktabell for ChatGPT, ligger i Claude Docs: https://claude.ai/artifact/XQhxbdHbroo7gx7GuytZmK (privat til Tom deler den).

## Mer spenning, skrekk, romvesen og mysterier, 20. oktober (Tom spurte)

Tom 20. oktober: mer spennende, mer skrekk, mer alien horror, mere mysterier. Forslagene kom fra fem vinkler (uhygge, romvesen, mysterier, innsats og kroker), og to dommere gikk gjennom dem: én så på hva spilleren får, én på om det lar seg bygge og passer med kanon. Det under bruker rettelsene deres, og det som bøyer eller bryter en regel, står for seg under Valg for Tom. Fire av valgene er bygget 20. oktober, og det samme er første del av rekkefølgens punkt 1: én modul for linjene på bakken og en logg over hva som ble sett (bygget 20. oktober, PR #121), punkt 3 med lite grep 3: Tomás' feltbok og konvolutten for plate 4 (bygget 20. oktober, PR #122), punkt 1: ett trykk for hele natta og papp-romvesenet som blir en refleksstolpe (bygget 20. oktober, PR #123), punkt 2: advarselen som alltid varer 47 sekunder (bygget 20. oktober, PR #125), punkt 11 med lite grep 4: de negative nettene, Earls spole fra 1981 og sjåførens klokke (bygget 20. oktober, PR #124), punkt 7: kontortelefonen som spiller deg av (bygget 20. oktober, PR #126), og punkt 4 med små grep 1 og 2: reléet som går først, gården som holder pusten og 02:29, når SARO blir mørkt og Hondo skriver (bygget 20. oktober, PR #__N__). Det som er bygget, er merket under; resten er ikke bygget.

### Det jeg ville bygget først

Rangert. Alt her passer med reglene i `memory.md` og `HENDELSER.md`, med unntakene som er nevnt.

1. **Bygget 20. oktober, PR #123 (`story/curve.ts`; stolpen i `core/cutouts.ts`, `postStep` i `main.ts`). Ett trykk for hele natta, og papp-romvesenet får en slutt.** Etter Ward er natta bare natt i ti minutter. Så hoper tegnene seg opp (sirissene tynnes, lykta hakker, et rop bak S-11), toppen kommer, og så 8 til 12 minutter der ingenting kan skje. Papp-romvesenet kommer høyst tre ganger. Den tredje gangen står formen ytterst i frontlysene, og når lysene når fram, er det en refleksstolpe: "The same shape. There was nothing there when I got to it."
   - Hvorfor: `HENDELSER.md` §4 beskriver trykket, men koden har fire flate tidtakere, og rop og papp har ikke tak (pappen heller ingen grygrense, `main.ts:1508`). Det blir tapet, eller komikk. Høyst én ekstra 47-hendelse per kapittel.
   - Pass på: taket snur Toms ønske fra 18. oktober (`core/cutouts.ts`), så det er hans valg.
   - Kap. 1 til 5. Bygger på trykket i `abduction.ts`, `main.ts:1491-1530`, de fire tidtakerne og `Ambience.hush`. Middels til stor.
   - Slik det ble bygget: én kurve fra kapittel 1 til 5 som nattens ting spør før de starter (`curve.may`), regnet i spilletid og ikke på nattklokka (tapt tid og kjøring flytter klokka). Ti minutter ren natt fra 02:16, når kapittel 1 slipper spilleren ut, ikke fra Ward: samtalen hans åpner kapittel 2, så kapittel 1 ville ellers stått uten. Så oppbyggingen, rundt 6,5 minutter tidlig i natta og under 3 mot 05:29: sirissene faller ut én og én, hunder og ugle langt borte kommer sjeldnere, lykta hakker, i bilen blinker dashbordet og frontlysene eller radioen glir under suset. Fra en tredjedel en lyd ute i mørket og papp-romvesenet utenfor blikket, fra halvveis én av hendelsene (skritt, telefonen, døra, lampene, bilen på riksveien). Toppen er det eneste stedet glimt, varsel og 47-hendelser kan komme (stillheten, bulletinen fra 1947, første samtale i telefonautomaten, motell-tv-en; høyst én i hvert kapittel). Et glimt som aldri blir sett, teller ikke, og toppen går videre. Inne venter toppen; etter fem minutter ute uten noe gir den etter for 3 til 4 minutter ro. Etter toppen 8 til 12 minutter der ingenting kan starte, og sirissene kommer tilbake. Tegnene venter mens noe annet går (ellers druknet bulletinen i suset), og én ting om gangen står: `oneAtATime` i `main.ts` stopper hver ny start. Papp-romvesenet har ikke tak, etter Toms ønske fra 18. oktober, men kommer bare i oppbyggingen, ett eller to hver gang, og er borte ved toppen. Stolpen kommer én gang: i fart på riksveien i en oppbygging, etter at pappen er sett to ganger. Formen står ved en ekte refleksstolpe 150 til 210 m fram, er tydelig pappen i frontlysene ved rundt 30 m, så blinker lysene, og det er bare stolpen. Notatet: «HH:MM. *place*. The same shape. There was nothing there when I got to it.» Ikke styrt av kurven: ansiktet på filmen (sett stort teller det som toppen), telefonautomatens andre samtale etter rom 6, og det som bare blir funnet. Skrittene på arkivtaket kan fortsatt komme i kapittel 2s siste stille øyeblikk før samtalen med Nora, og bankene fra feltboka spør kurven som en hendelse. Lagres med saken (`GameState.curve`), vises i `?debug` og i utviklermenyen («The night's curve»). Ingen nye stemmer.

2. **Bygget 20. oktober, PR #125 (`story/abduction.ts`: `WARN`, `BEAT`, `warnClock`; antennene i `story/dishesFollow.ts`, kurven i `story/curve.ts`). Advarselen varer alltid 47 sekunder.** Når advarselen begynner, går klokka i hjørnet over til sekunder, slik stasjonsklokka gjorde 02:13. Ved :16 et pip og skrittene blir borte, ved :31 trykktonen, ved :40 dør lykta slik K3 slo ut. Den som husker prologen, vet at det er sju sekunder igjen, og løper.
   - Hvorfor: nedtellingen er det spilleren har lært med kroppen. I dag varer advarselen 20 til 47 s (`abduction.ts:152`). Det er Toms godkjente spesifikasjon og bryter ikke R3, men en fast lengde gjør bortføringen til et løp, og sekunder er det tydeligste varselet på telefon.
   - Kap. 1 til 4. Bygger på `warning()` (lykta slukker allerede ved 0,85 av lengden, altså 40 av 47) og `clockText`. Liten.
   - Slik det ble bygget: takten er prologens sekunder etter at linja døde (`Prologue.startCountdown`). Ved :00 går hjørneklokka over til minuttet varselet begynte og varselets egne sekunder i amber, som konsollens `T+ 00:00:ss` (for eksempel `02:58:00`), og sirissene stilner. Ved :16 prologens pip (52 Hz, med et lite pip over som en telefonhøyttaler spiller), og skrittene blir borte. Ved :31 trykktonen og det kjøpte trykket på én gang, og lykta begynner å hakke. Ved :40 dør lykta med K3-klikket og kommer ikke på igjen: sju sekunder mørke. Ved :47 lyset; klokka står på :47 gjennom tagningen og viser nattas tid igjen når du våkner. Unna kommer man som før: løper du ved pipet, rekker du motellkontoret fra 70 m; løper du først ved :31, kommer lyset 25 m fra døra; fra kanten av plassen holder sju sekunder. Antennene (PR #120) går på takten: den første ved :17,5, de ytterste med trykket ved :31, og 026 / 32 etterpå som før.
   - Endret fra forslaget: ved :31 kom skriveren i prologen; her er det trykktonen, som forslaget sa. Uten den kjøpte mappa (Pages) ligger tonen under det en telefonhøyttaler spiller, så da legges tre svake overtoner over (222, 296, 369 Hz). Man kan ikke lagre mens varselet og tagningen går («Not right now. Let the moment finish first.»), så lagring og Continue ikke blir en vei ut av løpet, og varselet stopper helt når man går til tittelskjermen (`Abduction.halt`).
   - Tatt med samtidig, fra kritikken av PR #123 (motell-tv-en og første samtale i telefonautomaten ble ofte borte): der en av de to 47-hendelsene venter på stedet (motellkontoret med bilen på plassen, eller ved telefonautomaten 02:40 til 04:45), går resten av oppbyggingen på høyst 40 s, også inne, og toppen holdes for den (`CurveIn.waiting`, `curve.pulled`). Mens det skynder seg, glipper testbildet (bildet ruller, tonen vakler, brummen kommer under), og bjella i telefonautomaten klinger av seg selv. Blir spilleren stående 5 s i en ro der, beholder roen bare de første tre minuttene. Det bøyer «8 til 12 minutter ro», men bare på de to stedene, og Tom bør nikke til det. Ingenting kommer i en ro, oppå noe annet eller som en ekstra 47 nummer to i et kapittel.

3. **Bygget 20. oktober, PR #122 (`story/fieldBook.ts`, `ui/fieldBookView.ts`, skuffen i `world/Station01.ts`). Tomás' feltbok: hans juli er i natt.** I hytteskuffen på STATION 01 ligger "T. VEGA / FIELD BOOK 3 / JUNE-JULY 47": kyrne vendt mot stasjonen, tre bank i taket på bilen, lampene som gikk ut med hundene, en fylkesbil med motoren i gang og ingen i. Siste side: "Three on plate 4. There were two of us." En tråd til ditt eget notat holder: "He wrote it in July 1947. I saw it tonight." Det du ikke har sett ennå, ser du etter på veien tilbake.
   - Hvorfor: det største grepet mot at hendelsene aldri når saken. Spilleren finner R5 selv, og resten av natta blir forventning uten at noe jager. Bygger spor 10 (ingen "plate 4" i `src`).
   - Kap. 3. Bygger på `Chapter3.ts` og `boardRules.ts`; hver hendelse trenger et kort knyttet til nøkkelen den har (`ev:`, `ne:`, `mark:`). Stor.
   - Slik det ble bygget: skuffen under benken, under lampa, fra kapittel 3; ni sider, juni (oppmålingen, «Told her it was a mistake», som brevet i kapittel 4) og juli 1 til 7 (lysene langs B, kyrne, tre bank, lampene og hundene i Caliche, fylkesbilen). Det spilleren har sett, står med spillerens eget klokkeslett, fryst ved første lesing (R4). Kyrne står mot az 026, ikke mot stasjonen (`pasture.ts`), så boka har «Took a bearing on them: 026». Lysene langs B er lagt til, fordi nesten alle ser dem på vei ut. På veien tilbake kommer det boka har og spilleren ikke har sett: kyrne snur når spilleren er langt unna, lysene kan komme nordover, og tre bank i taket rundt 20 s ut fra porten, gjennom nattregissøren og bare etter 30 s uten noe annet (én ting om gangen). Lampene og bilen ligger i Caliche og står som «In his book. Not yet in mine.» til de skjer. Ingen ny 47, ingen nye stemmer.

4. **Bygget 20. oktober, PR #__N__ (`story/saroDark.ts`, `world/saroPower.ts`; koblingen i `main.ts`, `darkIn`). 02:29: SARO blir mørkt, og Hondo skriver.** Mens spilleren er i labben, klakker K3 i det tomme kontrollrommet. Førti sekunder senere går alle lys på SARO ut i 47 sekunder, også mørkeromslampa og de røde lampene på antennene, og en liten silhuett står ytterst på plassen. Når strømmen kommer, går telexen: "FM HONDO VALLEY RADIO STN / RANGE SOLUTION NEGATIVE. REQUEST PROCEDURE."
   - Hvorfor: labhalvdelen av kap. 1 får en topp, og reléregelen kommer igjen når spilleren vet hva klakket betyr. Det retter også en blindvei: Ward sier "I read the telex. All of it." (`Chapter5.ts:364`), men telexen finnes ikke. Den er spesifisert i `KAPITLER.md` (The Second Exposure, Nytt).
   - Pass på: K3 kan stå åpent fra nedtellingen (`ns.k3Open`), og et bilde i fremkalleren må ikke ødelegges.
   - Kap. 1, 02:29 til 02:40. Bygger på `telex()`, `tripK3()` og lampene i `ServiceYard.ts`. Middels.
   - Slik det ble bygget: fra 02:29 i labben, 02:31:30 i gården og 02:36 i kontrollrommet, aldri etter 02:40, og bare når ingenting annet går (`oneAtATime`); det venter til det som går, er ferdig. K3 klakker i det tomme kontrollrommet, dempet gjennom veggen, og hjørneklokka går over til sekunder i amber. Ved :31 synker alt et øyeblikk, ved :40 går alt lys som er SARO sitt ut (`world/saroPower.ts`): mørkeromslampene og glasset deres, DARKROOM-lampa, natriumlampene i gården og feltet, kontrollrommet, annekset, S-03-skjermen, brusautomaten og de røde lampene på alle 27 antennene, og malingen som ikke tar imot lys (sjablongene, B-12-skiltet, gjerdet og porten, bokstavene på fløyen), dempes til rundt en tredel med lampene og kommer tilbake som før. Summingen og romsuset dør, og et pust etter natta selv. Øynene venner seg sakte til stjernelyset; i labben ser man etter seks sekunder bare døråpningen, grå av stjernelys. Riksveiens gatelys og motellet lyser videre: det er bare SARO. I mørket står en liten skikkelse helt stille, svart mot stjernene, først på taket (nordøsthjørnet av kontrollromstaket, rett over rommet der K3 slo ut, eller nordvesthjørnet av østfløyen) der det kan ses over takkanten, etter 12 s også på tomta mot riksveien. Den kommer utenfor midten av blikket og blir stående når man ser på den; den er borte ved zoom, med lommelykta på den, når man går helt bort og når lyset kommer. Etter 47 s slår det inne inn igjen med én gang (lysrørene i et fast mønster med to korte utfall, rett på med «Flashing images: slow fades»), og natriumlampene varmes opp over 28 s. Så går telexen, laget i kode med motor og anslag, og Hondo Valley står på rullen ordrett etter `KAPITLER.md`, stemplet med minuttet klakket kom (normalt 0229). `SHOW LOG` får bruddet, returen og `TELEX RECEIVED  HONDO VALLEY`. Første lesing gir notatet «Somebody else got one tonight». K3 står fortsatt åpent. Lagring er sperret mens det går; våtbenken, B-12 og kapittelbildene venter på lyset («Too dark ... wait for the light»), og kapittel 1s slutt venter også. Lagres som `NightShiftState.hondo`; loggen over hva som ble sett får `blackout` og skikkelsen (`figure, dark`). Utviklermenyen: «SARO dark».
   - Endret ved byggingen: K3 står åpent i de fleste netter (ingenting ber spilleren slå det inn i kapittel 1), så «reléet» slår ut likevel, og loggen skriver `K3 OPEN` en gang til uten `K3 CLOSED` imellom: et avtrykk av utslaget, ikke reléet (R5). «With nothing left to trip. It tripped anyway.» Skikkelsen står på taket mot stjernene og ikke ytterst på plassen: svart mot svart syntes ikke, og på telefon var den 12 punkter høy. Den står 5 til 30 m unna og blir stående når man ser på den; det bøyer to regler i `HENDELSER.md` (punkt 12). Et bilde i fremkalleren blir ikke ødelagt: kjemien går videre i mørket. Har natta aldri blitt mørk (kapittel 1 ferdig før 02:29, eller spilleren borte fra SARO til 02:40), står telexen likevel på rullen fra 02:29, slik `KAPITLER.md` først hadde det, så Wards «I read the telex. All of it.» alltid har en telex; i kapittel 2 venter den på minuttet sitt. Natriumlampene som varmes opp sakte, er lagt til. Mørket er kapittel 1s ene 47: kurven holder av plassen til det har kommet eller tiden er gått (`keep`), og det lander toppen. Ingen nye stemmer; notatene sier bare det Reyes så.

5. **Kapittel 2 får én topp i arkivet.** Rundt 03:13 skriver samtaleskriveren ved veksleren "IN TRK1 EXT 201" av seg selv, og 47 sekunder senere ringer veggtelefonen: kontrollrommet 02:13, skriveren, et dunk, et gisp, koppen. Rekker spilleren å legge inn bank 3 før :47, skriver konsollen samme løsning med det gamle klokkeslettet; ellers kommer arket blankt. Bak 1947-boksen står spole 047 ("NOT FOR CATALOGUE"): 4/7, en sveiv, et klipp, 47 sekunder ingenting, et relé, -32 LY.
   - Hvorfor: kap. 2 er uten spenning, og telefonen kommer ellers ikke igjen før THE EVENT. Et avtrykk av 02:13 holder R5, og strimmelen viser "02:13 IN EXT 201 FROM EXT 201". Spolen fullfører spor 6 og betaler seg mot FRAME 04, E10 og Noras tilståelse.
   - Rettelse: stempelet er det spilleren fikk (`ns.solveAt`), ikke 02:13:47.
   - Kap. 2. Bygger på `recPhone` i `Annex.ts`, `resetK3`, SHOW TAPES og `Decoder.ts`. Stor. Den sterkere varianten er valg 1 under.

6. **Den siste mila.** Forbi sjumilsstolpen åpner CB-en seg selv: en motor som din, liten, som hoster og dør, mens din egen går. Førtisju sekunder senere er bilen på C, og din motor dør. "Turn the key" kommer opp som hver gang i natt; solenoiden klikker fire, en pause, sju, og den starter aldri. Døra: "The handle moves. The door does not."
   - Hvorfor: kap. 6 er ni minutter med alt slått av. Varselet 47 s før er prologens grammatikk (R3), og nøkkelen som reddet spilleren hele natta, tas bort; svaret i 4/7 er av samme slag som walkien (R5).
   - Avveid mellom dommerne: lyden går gjennom CB-en, så bilradioen fortsatt er første slag i THE EVENT, og CB-en spiller bare motoren, uten klikkene, så 4/7 er spart til 05:29. Utløses på avstand, fordi klokka kryper.
   - Kap. 6. Bygger på `Chapter6.ts`, `core/cb.ts` (bare stemmer i dag) og `walkie.ts`; `event()` trenger egen nøkkelmelding og `can-out` for touch. Middels.

7. **Bygget 20. oktober, PR #126 (`story/officePhone.ts`; koblingen i `main.ts`, `officeIn`). Kontortelefonen spiller deg av.** Første gang du kommer inn på motellkontoret, ringer sveivetelefonen. I røret, smalt: planker som knirker, hyttedøra, bærebølgen og to lukkere med samme mellomrom som FRAME 03 og 04. "The office line to the hut. Someone walking on the boards. A shutter, then another. I took two frames out there."
   - Hvorfor: telefonen er koblet til STATION 01, men bare en rekvisitt (`Chapter4.ts:374`). Linja gjentar det som ble registrert, og nå er spilleren på den. Den forbereder P12 og 05:29:47, og alle spillere når den.
   - Kap. 4, før rom 6. Bygger på `s.f3` og `s.f4` i `Chapter3.ts`, `phoneLine` og sveiveringen. Middels.
   - Endret ved byggingen: den ringer når spilleren kommer til fots inn på kontoret, ut på motellplassen eller innen 22 m fra døra, ikke bare inne, fordi kontoret er valgfritt og hver vei til rom 6 krysser plassen; på avstand sier teksten at det ringer i kontoret. Bjella er laget i kode (den kjøpte pakken har ingen telefonbjelle): to gonger og en kneppert i ujevne støt, som når noen sveiver, høyst 36 s. Ingen snakker. På linja, under bærebølgen fra mottakeren i hytta: skritt på hytteplankene (de samme opptakene som spillerens egne føtter), døra, en lukker og «Another.» med samme mellomrom som FRAME 03 og 04 (over 8 s trekkes det sammen, høyst 14 s), så hyttebjella, skritt bort til telefonen, røret som løftes, Noras to første replikker svakt fra øretelefonen (hennes opptak, ellers bare tekst), noen som lytter, og røret som legges på. Rekkefølgen er den som skjedde ved hytta: bjella ringte etter begge bildene. Notatet ender «That was me, at HH:MM. The receiver out there gives back T. Vega's correction from 1947. This line has kept me.»; «his correction since 1947» ville røpet P12. Et kort på bevisbordet, «THE MOTEL OFFICE, TONIGHT», med tråder til FRAME 03, FRAME 04, mottakeren og telefonen i rom 6. Én gang i natta (`ev:office`), bare når ingenting annet går (`oneAtATime`), og ingenting starter mens den ringer, er av røret og 10 s etter. Kurven styrer den ikke, men den lander toppen. Uten bildene (utviklermenyens kapittel 4) bare skrittene, døra og samtalen. Ingen ny 47, ingen nye stemmer og ikke noe merke på kartet (kontorlinja er ikke A, B eller C).

8. **Fire, og ikke ennå sju: bilen på B.** Fra kap. 2, første gang du kjører over B, går radioen til bærebølge et sekund, motoren feiler, og lampene blinker fire ganger. Etter rom 6 dør motoren, OIL og VOLTS lyser, og den tar på andre forsøk og blinker sju. Kompasset på dashbordet står på 026 over B, og på den gamle veien glir det mot 026 fra mil 6.
   - Hvorfor: en stige mot 05:29 og tegnene i bilen fra `HENDELSER.md` §8 for dem som kjører overalt, med `drive/trouble.ts`, som står ubrukt. Den lærer nøkkelen som punkt 6 tar bort. Blinkene teller opp her; fargene i frontlysene er spart til 05:29.
   - Forutsetning: én plassering av B. Lysene svinger ved m 330 (`lineLights.ts`), kartnotatet sier "just south of the motel" (`main.ts:1435`), og STATION 01 ligger rundt 840 m sør for SARO.
   - **Forutsetningen er på plass (bygget 20. oktober, PR #121):** B går fra transitten gjennom B-12-vingen på SARO-tunet og krysser riksveien ved avkjøringen, 23 m sør for tomta (`story/lines.ts`, `CROSSINGS`). Selve stigen i bilen er ikke bygget.
   - Kap. 2 til 5. Bygger på `trouble.ts` (blinkingen finnes bare i `Chapter6.event`) og `drive/Truck.ts`. Middels.

9. **Bortføringen får følger.** Ved STATION 01 må alle gå. Har ingen tatt spilleren ennå, starter advarselen på vei fra hytta etter Noras samtale. Senere går noen i SARO-jakke dine siste 47 sekunder der du ble tatt: stopper der du stoppet, løper det samme snublende løpet, står stille og begynner på nytt. Nora: "You have been out on the line. One side of your face." Nestlederen har tørket blod under nesa og har ikke merket det.
   - Hvorfor: "minst én før kapittel 5" er bare et trykkpåslag i dag (`abduction.ts:147`). Opptaket viser R5 på spillerens egen kropp, og merket gjør tiden borte ekte.
   - Pass på: `calm()` trenger 18 minutter før 04:35, så samtalen må være over rundt 04:17; det er et varsel, ikke en garanti. Noras "Tomás came back from it like that in June" er ny bakgrunn Tom må godta.
   - Kap. 3 til 5. Bygger på `record()` og `recordsStep` (en sti på 47 s per sted), P10 i `Chapter4.ts` og `caliche.ts`. Middels.

10. **Tatt i bilen.** 02:40, sørover. Der B krysser, glir radioen til bærebølge, motoren dør og frontlysene synker. Vrir du raskt, tar den; løper du nordover langs skulderen, kommer du også fri. Ellers tre bank i taket, hvitt lys ovenfra, svart, og 03:19 sitter du 300 m lenger fram med nesa mot nord og radioen på 1090: "The odometer reads 31.4 miles more than it did at SARO."
    - Hvorfor: i dag skjer bortføringer bare til fots, men `HENDELSER.md:176` sier "ute eller i bilen". Telleren er Hill-sporet i 1986-maskinvare. Dashbordet blir bare mørkt.
    - Rettelser: `failFirst` gir høyst ett mislykket forsøk, og `wake()` går ut fra at spilleren er til fots, så bilen må flyttes.
    - Kap. 1 til 4. Bygger på `trouble.ts`, `World.keyStall`, `abduction.ts` og `abductIn` (krever i dag `!world.driving`). Stor.

11. **Bygget 20. oktober, PR #124 (`story/lostNights.ts`, `core/reelTape.ts`; Earl i `story/caliche.ts`, sjåføren i `story/Chapter5.ts`). De negative nettene er Caliches tapte netter.** På NIGHT SHIFT-korken holder en tråd fra permens -38 10/19/85 til nestlederens grønne nål: "The night SARO printed -38, the attendant lost two hours on the old road." Når 1983 og 1981 er fylt inn, kommer et spørsmål uten svar: "-39 tonight. Who loses time tonight?" Gir du Earl datoen før 04:00, spiller han spolen fra september 1981: gutten sier nattens setninger ord for ord, med tomrom der Earl svarte i natt. "I answered him tonight. He wasn't answering me."
    - Hvorfor: det binder vitnene til forløperne og gjør vitnekortene til et resonnement; en levende mann svarte et opptak (R5). Allerede sådd i `caliche.ts:295-299` og `336-353`, og i permens 09/22/81 01:52:07.
    - Kap. 2 og senere. Bygger på en ny `ns`-regelbok i `boardRules.ts`, `BOY_CALL`, noen Earl-klipp og mørkeromsloggen fra `KAPITLER.md`. Middels.
    - Endret ved byggingen: et nytt kort, «WHAT THEY TOLD ME», har det Earl, kvinnen på benken og betjenten selv sa, fordi nestlederen er ute fra 02:45 til 04:30 og spørsmålet ellers ikke kunne nås i kap. 2 og 3. «I answered him tonight» sies av Earl, ikke av spilleren. Hullene på båndet er like lange som Earls svar i natt (3,61 og 1,12 s), og natta utenfor blir stille i dem. Mørkeromsloggen finnes bare i `KAPITLER.md` og er ikke brukt. Earls nye replikker har ikke opptak («No recording for this reply»).

12. **Sett arrayen tilbake. Den går tilbake.** Ward: "Put the array back on the schedule and tell me it stays there." Spilleren skriver ARRAY RESUME, og alle 27 svinger mot 042. Førtisju sekunder senere blinker de røde lampene fire og sju, og alle svinger tilbake til 026 med MOTION NO COMMAND i loggen. Hver gang.
    - Hvorfor: nattevakten mislykkes med den virkelige jobben, med egne hender (R1). Det bygger S-03-historikken, og Ward kan si i kap. 5: "You tried it three times. It went back three times."
    - Kap. 2 til 4, etter FRAME 02. Bygger på `OpsTerminal`, `Dish.point` og `story/field.ts`. Middels.

### Valg for Tom: dristigere ideer som endrer reglene

Disse trenger Toms beslutning. Hver bryter eller bøyer noe som står fast. Tom valgte 2, 3, 5 og skrittene på taket 20. oktober («Spenning og skummelt slår faste regler»); reglene som er løftet, står i `HENDELSER.md` punkt 12.

1. **Telefonen fra framtida en gang til.** Arkivtelefonen spiller rommet før det skjer: et dunk gjennom betong, en skuff som ruller ut. Ved 47 s starter alle 27 drivmotorer, skuffen med spole 047 slår opp, og antennene har vippet ned mot horisonten langs 026. Bryter: telefonen fra framtida hører til prologen, og antennene forlater el 32. Gir det sterkeste skremmet kap. 2 kan få, men prologtelefonen mister noe. Erstatter punkt 5.
2. **Bygget 20. oktober (den fulle versjonen, `story/dishesFollow.ts`). Antennene senker seg mot deg.** Under en advarsel i feltet kommer fatene ned og følger deg. Bryter R5 ("nothing in the world acts because the player is there", `HENDELSER.md:42`) og az 026 / el 32. Varianten som passer: fatene svinger én gang, mot gresset der du kommer til å våkne (en forløper, R3).
3. **Bygget 20. oktober (`story/abduction.ts`, omrisset bøyd i `core/roadside.ts`). Noe står over deg når du våkner.** Halvannet sekund øverst i bildet: et lite hode bøyd over deg, 3 m unna, borte når blikket løfter seg. Bryter: aldri nærmere enn rundt 40 m (`HENDELSER.md:84`). Ikke ansikt, ingen bevegelse, ingen lyd. Malt omriss, ikke boksfiguren. Passer i stedet: de tre på åsen.
4. **Den tredje før rulleteksten.** (a) Et svakt reflekterende bånd ute langs C på FRAME 03, og Nora snur bildet uten et ord; (b) noen i SARO-jakke 9 m foran bilen 05:29:10, med båndet i frontlysenes farger; (c) ingenting. Bøyer: bildet etter rulleteksten er eneste avsløring, og det tomme setet pekes aldri på (`HISTORIE.md:118-122`). (a) er mildest; båndet legges inn når lukkeren går (R4).
5. **Bygget 20. oktober (`core/filmFace.ts`: siden av hodet og ett øye, resten utenfor bildet eller i mørket). Ett ansikt før 05:29.** I kanten av et fritt bilde i kap. 4, uskarpt nær linsa: siden av et blekt hode og ett svart øye i blitsen. Bryter: ansiktet er spart til THE EVENT (`HENDELSER.md:313`). Gir det mest direkte svaret på "mer alien horror", men FLASH blir gjenkjennelse. Den ene dommeren mente prisen er for høy. Mildest: halve ansiktet.
6. **Ingen stjerner der antennene peker.** En rund flekk uten stjerner ved az 026, el 32 fra 02:15, og det siste bildet i uttrekket har samme hull. Bøyer "the world is honest" (`SPILLDESIGN.md:60`) og kan se ut som en grafikkfeil. Billig i `Sky.ts`. Hold det svakt, og la det ikke vokse.

Også et valg, **bygget 20. oktober** (`story/roofSteps.ts`, `world/roofFrost.ts`): lette, for korte skritt over arkivtaket i kap. 2, med rim på takaggregatet etterpå. Billig, men bryter "De lager ingen lyd, ikke engang skritt" (`HENDELSER.md:84`).

### Små grep

1. **Bygget 20. oktober, PR #__N__ (`core/ambience.ts`, `breath`). Gården holder pusten.** 02:16, første gang ut: helt stille, så kommer sirissene én og én over 47 s. `Prologue.ts:998`, `Ambience.hush`.
   - Slik det ble bygget: første gang spilleren går ut i kapittel 1, uten vind og siriser: «Not a sound out here. Not even the crickets.» Sirissene kommer én og én over 47 s, vinden og sirissuset til slutt, og musikken holdes igjen så lenge. Det hører til kurvens ti rene minutter og er ingen hendelse: ingenting venter på det, kurven går videre, og det skjer bare én gang.
2. **Bygget 20. oktober, PR #__N__ (`story/Prologue.ts`, `relayFirst`, `lightsDip`). Reléet går først.** 23:52 slår K3 ut før skippet, og 40 s senere dipper lyset. 40 s er servicekortets ("TRIPPED 40 SEC BEFORE OUTAGE"); i nedtellingen er det 7 s. Tom velger.
   - Slik det ble bygget: når konsollen er lukket og sveipet går av seg selv, slår K3 ut etter 7 s med det vanlige klakket («A clack from the rack. Bank 3 has dropped out again.»). 40 s etter, på loggens klokke, synker lyset i 0,7 s, én gang og tilbake: `LINE POWER  DIP 0.7 S` i loggen og «The lights dip. Forty seconds after the relay.» Notatboka knytter det til servicekortet: «it trips before, not after». Så skippet til 02:13 som før; står spaken fortsatt ute, slår skippet den inn, og det står i loggen. Fra 02:13 vet spilleren grammatikken: klakk, førti sekunder, så skjer noe.
3. **Bygget 20. oktober, PR #122 (med feltboka). Konvolutten mot lampa.** "Plate 4. 1 print. 35c." Mot lampa: tre små skikkelser. La "There were two of us" være Tomás' setning. Den forseglede konvolutten på motellkontorets disk er blitt fotobutikkonvolutten fra Roswell, og kan holdes mot lampa fra kapittel 1; en tråd går til bokas siste side.
4. **Bygget 20. oktober, PR #124 (med punkt 11). Sjåførens klokke.** Den står på 11:58, mot permens 10/19/85 23:58:31. En slepelapp fra bensinstasjonen kan bære de tapte timene.
   - Endret ved byggingen: klokka er sjåførens i dineren, ikke betjentens; betjenten slepte bilen hans, og sjåføren husker ingen sleping («Nobody towed me. She came back on by herself.»). Gjennomslaget av slepelappen er datert 10-20-85 (utrykning 1:45, inn 4:25, 2,7 timer), og feltet for hvem som ringte er tomt.
5. **Morgenen holder pusten.** 05:13 i dineren stopper fuglene og kjøleren. "There. Some mornings about now." ("Every morning" strider mot `Chapter5.ts:426`.)
6. **Kortet ender på en ring.** Telefonen ringer under CHAPTER TWO. Oppsummeringene står, fordi stemmene er nøklet på teksten.
7. **Linjene møtes.** Dra linjene videre på kartet. Notatet i `main.ts:1435` sier allerede "through STATION 01" og må skrives om. **Notatet og B-streken er rettet (bygget 20. oktober, PR #121):** "across the highway at the turn-off to SARO, and on to the south-west, through STATION 01", og streken går gjennom avkjøringen. Linjene er ikke dratt videre.
8. **En tråd du ikke husker.** Etter en bortføring: en blyanttråd med "same" i din hånd, aldri på et P-spørsmål.

### Forslag til rekkefølge

1. Én modul for A, B og C på bakken, så punkt 1 og 2, og en teller over hva som ble sett. **Modulen og telleren er bygget 20. oktober, PR #121** (`story/lines.ts`, `story/seen.ts`), punkt 1 er bygget 20. oktober, PR #123 (`story/curve.ts`), og punkt 2 er bygget 20. oktober, PR #125 (`story/abduction.ts`).
2. Små grep 1 og 2, Hondo (4) og arkivet (5, eller valg 1). **Små grep 1 og 2 og Hondo er bygget 20. oktober, PR #__N__.**
3. Bilen: 8, så 10, så 6. Stigen lærer nøkkelen som 05:29 tar bort.
4. Saken: 3 og 11, konvolutten og klokka. **3 og konvolutten er bygget 20. oktober, PR #122**; 11 og klokka er bygget 20. oktober, PR #124.
5. 9, 7 og 12. **7 er bygget 20. oktober, PR #126.**
6. Den niende kua, blodløs ved B-fløyen på STATION 01 (570 m fra beitet), passer når som helst i kap. 3, men er kroppsskrekk Tom bør se først.

Tell 47-ene: dette legger til minst fem. (To er bygget, PR #__N__: gården som holder pusten, som ikke er noen hendelse, og mørket 02:29, som er kapittel 1s ene 47.) Motor, stillhet, 4/7 og pust høres allerede i walkien, telefonautomaten og THE EVENT; derfor ingen klikk på CB-en i punkt 6.

### Hull

Ingen foreslo måling: en logg over hva som skjedde og ble sett, og spilletid per kapittel, så man vet om glimtene sees på telefon. (Loggen er bygget 20. oktober, PR #121: `story/seen.ts`, vist med `?debug`. Spilletid per kapittel er ikke bygget.) Ingenting sier hva den som slår av Missing time får i stedet. Lydslagene trenger teksting, nye linjer trenger stemmer, og hvert engangsslag trenger en `townSeen`-nøkkel og oppdaterte `tools/*.py`. Fortsatt tomt: mil 0 til 6 på den gamle veien og Caliche etter 04:50 (hytta på STATION 01 har fått feltboka, PR #122). Ikke tatt opp: glimtet bak labbens glass, figuren blant kyrne, "STA 01 / B", nettverkstelexen (SITE 03, SITE 11) og en lydflate fra den kjøpte pakken som følger trykket.

## Forbedringer, 16. oktober kveld (Tom spurte)

Det jeg tror gir mest for innsatsen står øverst. Ingenting her er bygget.

1. **Volum per kanal.** Det finnes bare ett volum. Musikken går nå nesten hele tiden, og radioen, lydene i mørket og motoren kjemper om plassen. Egne glidere for musikk, radio og lyder i pausemenyen, lagret i innstillingene. Liten jobb.
2. **Gjør veien til motellet til et sted.** 650 m til fots er rundt seks minutter langs mørk asfalt. Steg 0 av hendelsesregissøren (`HENDELSER.md`) passer akkurat her: stillheten som faller, lommelykta som hakker, ett glimt ute på sletta, telefonlinja som summer, et billys langt i sør som aldri kommer fram. Det er også den billigste prøven på retning A før byen bygges.
3. **Notatbokkartet viser feil side.** Servicekartet (E14) har STATION 01 øst for riksveien, men i spillet ligger stasjonen 600 m vest. Kartoppgaven i dineren bygger på servicekartet, så det bør stå. Notatbokkartet med krysset der spilleren er, viser nå at man kjører østover når man kjører vestover. Forslag: notatbokkartet tegnes etter verden slik den er (spilleren tegner fra hukommelsen), servicekartet beholdes.
4. **CB-radioen i bilen.** Den er modellert, men gjør ingenting. Nora kan kalle opp på den mens spilleren kjører til motellet i kapittel 4, og senere andre, med mumling og tekst som bilradioen. Det gir kjøreturene innhold uten nye områder.
5. **En lydside i utviklermenyen.** Mye er UNVERIFIED på ekte høyttalere (radioen, nasjonalsangen, lydene i mørket, motoren, musikken). En side som spiller hver lyd etter tur med knappene «for høyt», «for lavt» og «bra», og lagrer svarene, gjør det raskt for Tom å sjekke alt på en gang og gir meg noe konkret å justere etter.
6. **Raskere testrunde.** Hele leveranselista tar nå tre til fire timer. En kort runde på 15 til 20 minutter (prologen, én kjøretur, motellet, radioen, lagring) for små endringer, og hele lista før hver PR, gir hyppigere leveranser.
7. **Lastebilen lagres der den står.** Nå lagres bare stedet (SARO, motellet, stasjonen, dineren). Står den på veiskulderen, er den tilbake på SARO etter lasting.
8. **Hint første gang.** Det er mange taster nå (E, F, R, M, I, C, Tab, høyre musknapp). En kontrollside i notatboka, og hvert hint bare første gang det gjelder, i stedet for lange meldinger hver gang.
9. **Mobil.** SAROs område har nå motellet og 800 m vei i tillegg. Må måles på ekte telefon med `?debug`. Faller det, kan motellets detaljer skjules langt unna (bare masse og lys).
10. **Caliche, steg 1** når steg 0 er prøvd: én gate ved dineren med noen opplyste vinduer, bensinstasjonen og telefonautomaten, før folka. (Bygget 17. oktober, `world/Town.ts`.)

Min anbefaling: 1 og 3 først (små og tydelige), så 2 og 4 sammen, fordi de begge gjør veien mellom stedene levende.

## Et sted der rare ting skjer: døgn, en søvnig småby, Roswell og Twin Peaks (16. oktober)

Tom 16. oktober: gjøre SIGNAL / 47 mer til et knutepunkt der merkelige ting skjer, med døgnsyklus, en liten søvnig småby og tilfeldige skumle ting. Tenk Roswell-romvesener og Twin Peaks. Han ba om forslag. Ingenting under er bygd eller avgjort.

### Det som står fast, og det som er åpent

Står fast: én slutt, THE EVENT 05:29 der C krysser den gamle veien. Fenomenet følger referansene (R1): 1947-linjene A, B og C, az 026, 47 sekunder, 1420 MHz, fire og sju. Et avtrykk gjentar det som ble registrert og kan ikke svare (R5), så ingenting jager spilleren og ingenting svarer. Ingen monstre, og ansiktet som kan minne om en Grey, er omtrent tre bilder i FLASH. Høyst tre hint om andre steder (Hondo Valley, Site 03, Site 11), og de er brukt. Hendelsene kan gi hint i hovedsaken (Tom 15. oktober).

Åpent: om spillet fortsatt er én natt. Det er det største valget her, fordi en ekte døgnsyklus ikke får plass i en natt som går fra 23:41 til 05:29.

### Kjernen

Du er nattevakten ved en søvnig ørkenby nær Roswell, der alle har hørt om 1947 og få vil snakke om det. Du kjører rundt om natta, legger merke til ting som gjentar seg, skriver dem ned, og ser at de ligger på en linje.

Et eksempel. Klokka er 02:40, og du tar omveien gjennom byen. Trafikklyset blinker gult over et tomt kryss. Telefonautomaten utenfor bensinstasjonen ringer idet du går forbi. Du tar røret: skriveren i kontrollrommet, linje for linje, den samme utskriften du fikk 02:13. Notatboka får et funn med klokke og sted. På kartet ligger det nå tre merker: kyrne på beitet som sto vendt mot nord-nordøst, gatelysene som gikk ut ett og ett, og telefonen. De ligger på en rett strek, og den streken har du sett før.

Det er ikke skrekk fordi noe jager deg. Det er skrekk fordi alt er vanlig, bortsett fra én ting, og den ene tingen henger sammen med resten.

### Fem retninger

**A. Natten blir større (én natt, byen om natta)**

Natten er som i dag. Byen ligger ved krysset der dineren står, og dineren blir det nordligste huset i hovedgata. Mellom kapitlene kan spilleren kjøre dit. Klokka er mellom ett og fem: trafikklyset blinker gult, bensinstasjonen er åpen hele natta, sheriffens lille kontor har lys i vinduet, en liten AM-stasjon har en nattprater i studio, kinoen er stengt, og en suvenirbutikk har et håndmalt skilt med en flygende tallerken. Det rare følger linjene. Ingen døgnsyklus; natten går mot gry som i dag.

Nærmest kanon, og bruker det som finnes: lastebilen, veiens område, dineren, klokka, gryet og feilene med bilen. Byen klokka tre om natta er det mest Twin Peaks som finnes. Koster middels: ett nytt område med sammenslåtte fasader, tre eller fire folk og en styring av hendelsene. Svakhet: ingen døgnsyklus, og det var noe av det Tom ba om.

**B. Kvelden før (én kveld og natten)**

Nytt spill starter søndag 13. april rundt 17:30, i lastebilen på vei fra byen til skiftet. Sola går ned 18:27, det er blåtime til 18:53, og så kommer stjernene og kometen. Spilleren kan stoppe i byen mens butikkene er åpne: fylle bensin, spise i dineren, møte Nora på motellet som den hyggelige eieren hun er for alle andre, hilse på nestlederen hos sheriffen. Klokka 23:30 tar Dale imot spilleren på SARO, og Night Shift starter som i dag. Senere i natt er den samme byen stengt, og noe er galt med den.

Gir overgangen fra dag til natt, som er den fineste delen av et døgn, og Twin Peaks-kontrasten: de samme folka og stedene, varme om kvelden og tomme klokka tre. Den som har drukket kaffe med Nora før hen vet hvem hun er, kjenner rom 6 hardere. Dale blir en man har møtt, ikke bare en logg. Koster mye: kveldslys på SARO, veien og byen (alt er laget for natt), folk med replikker, og åpningen endres, så telefonen 23:41 ikke lenger er det første som skjer. Svakhet: tregere start. Kvelden må være kort, 15 til 25 minutter, og kunne hoppes over («Start at the shift»).

**C. Dagene før (flere dager, som Firewatch)**

Torsdag 10. til søndag 13. april som klippede dager. Hver dag har to eller tre biter: ettermiddag i byen, kveld, en kort natt, og et dagskort imellom. Uka har ekte ting å bruke: Masters på radioen torsdag til søndag (Nicklaus vinner søndag, og radioen i dineren nevner det mandag morgen), og Halleys komet nærmest jorda 11. april, så fredag kveld er kometkveld i byen. Det rare øker dag for dag, og reléet K3 som «tripped again» i Dales logg kan slå ut mens spilleren er der.

Gir ekte døgn og plass til en by med vaner. Koster svært mye: dagslys viser lavpolystilen mest, folk trenger dagsplaner, lagringen må tåle dager. Det er i praksis et nytt spill foran det som finnes. Svakhet: natten mister noe av tyngden hvis fire netter før den også har vært rare.

**D. Netter i byen (frispill etter slutten)**

Etter rulleteksten låses et eget valg opp på tittelskjermen: en vilkårlig natt i byen, uten historie. Døgnet går (for eksempel ett døgn på 48 minutter), byen har sine vaner, og hendelsene kommer tilfeldig etter de samme reglene. Ingen ny kanon: det er utenfor historien, slik mange spill har et frispill etterpå.

Gir knutepunktet og døgnsyklusen uten å røre den ene natten. Koster mye av samme grunn som C (dagslys), og det er lite å gjøre i det uten oppgaver. Gir mest mening hvis B eller C allerede har bygd byen i dagslys.

**E. Natten går igjen (det dristige)**

Etter rulleteksten begynner natten på nytt 23:41. Spilleren er nå en referanse, og et avtrykk gjentar det som ble registrert (R5). Samme natt med små forskjeller: folk sier de samme setningene ord for ord, stemmen til Tomás på linja har ett ord til, det står tre kopper på disken i dineren. Spilleren vet hva som kommer og kan være der det skjer.

Henger bedre sammen med fortellingen enn noe av det andre. Men den svekker den ene, korte slutten, og trenger et godt svar på hvorfor man spiller igjen. Parkert.

### Det rare: hvordan det virker

En liten styring (en «hendelsesregissør») med en pott av hendelser. Hver hendelse har et sted, en linje (A, B eller C), et tidsvindu og et kapittel den tidligst kan skje i. Den skjer én gang, og det går minst ti minutter mellom to. Tidlig i natten er hendelsene stille og lette å bortforklare; utover natta blir de mer fysiske.

Reglene, så det ikke blir billige skvetter:

- Hver hendelse er en gjentakelse av noe som ble registrert (en lyd, et lys, en setning) eller en forstyrrelse i instrumentene. Ingenting reagerer på spilleren, følger etter eller svarer. Regissøren sørger for at spilleren er i nærheten når noe skjer, men i verden skjer det ikke fordi hen er der.
- Den skjer på eller nær en linje, ofte i 47 sekunder.
- Skrekken kommer av at noe mangler: stillhet, mørke, at ting står helt stille. Ikke av at noe hopper fram.

Når spilleren ser en hendelse, får notatboka et funn med klokke og sted, og kartet får et merke. Noen ting synes bare på film, som merket ved B-12. Tre merker på samme linje får linja til å tegne seg på kartet, og det er hintet. Feilene med lastebilen (`drive/trouble.ts`) får bare skje tilfeldig nær linjene, oftere jo nærmere C. Lommelykta flimrer nær hendelsene, som foreslått 15. oktober.

Hendelser i og ved byen, i tillegg til de ti fra 15. oktober:

1. Trafikklyset blinker gult som vanlig, og i 47 sekunder blinker det fire og sju.
2. Telefonautomaten ved bensinstasjonen ringer. I røret: skriveren i kontrollrommet fra 02:13. Før 02:13 er det noe som ikke har skjedd ennå.
3. Betjenten på bensinstasjonen sier den samme setningen to ganger med to timers mellomrom, ord for ord, med den samme hostingen midt i. Han husker ikke den første gangen.
4. Gatelysene går ut ett og ett langs en linje over byen, og tennes igjen i samme rekkefølge.
5. Nattpraten på AM-stasjonen: en innringer, en gutt, beskriver lyset over mesaen som om det er i kveld. Etterpå sier verten at linja var død da han tok den.
6. Alle hundene i byen gjør på en gang, og stopper på en gang.
7. Kyrne på beitet står vendt mot az 026, helt stille, også når lastebilen kjører forbi med lysene rett på dem.
8. Sirissene stopper i 47 sekunder. Bare vinden.
9. Bilradioen: en nyhetssending fra juli 1947 om en «flying disc» funnet på en ranch, i 47 sekunder, før natten 1986 kommer tilbake.
10. Frontlys som kommer imot på riksveien og aldri kommer fram. I speilet er det ingen.
11. Tv-en i resepsjonen på motellet: testbildet, så bærebølgen i 47 sekunder.
12. Papp-romvesenet fra suvenirbutikken står ute i veikanten i frontlysene, langt fra butikken. Neste gang står det i vinduet igjen. Det må ha vært noen unger.

### Byen

Arbeidsnavn: **Caliche** (navnet på det harde kalklaget i ørkenjorda). Andre muligheter: Dos Mesas, Piñon Wells. Skiltet ved innkjøringen er på engelsk i spillet: `CALICHE / POP. 412`.

- **Hvor:** ved krysset der den gamle Roswell-veien går ut fra riksveien, med Mesa Diner som nordligste hus. Da ligger alt på én vei: SARO, motellet 700 m sør (avgjort 15. oktober), byen med dineren, og den gamle veien østover derfra.
- **Hovedgata:** 300 til 400 m. Dineren (finnes), bensinstasjon med telefonautomat, sheriffens lille kontor, en AM-stasjon i en betongkloss med mast, en stengt kino, en kirke, et vaskeri som er åpent hele natta, en fôrbutikk, suvenirbutikken og noen hus med tv-skjær i vinduene. Ingen ekte merkenavn. AM-stasjonen kan ligge på 1420 kHz, et ekko av 1420 MHz som ingen i spillet peker på; kallesignalet må sjekkes så det ikke tilhører en ekte stasjon.
- **Folka** (egne, ikke lånt fra Twin Peaks): servitrisen og sjåføren (finnes), Nora på motellet (finnes), nestlederen hos sheriffen som sitter alene på nattskift med et kart med knappenåler over meldinger om døde motorer, kyr og lys (han tror ikke på romvesener, han tror på kart), nattpraten på AM-stasjonen som spiller country og tar imot telefoner om lysene, kvinnen på benken utenfor vaskeriet med en transistorradio stilt mellom to stasjoner («It's louder tonight.»; hun hører bare etter, ikke noe mer), betjenten på bensinstasjonen, og suvenirmannen som selger «1947 debris» (aluminiumsfolie) og har levd av historien siden boka kom.
- **Ytelse:** ett område, fasadene slått sammen, høyst tre små rom (disken på bensinstasjonen, sheriffens kontor, studioet sett gjennom glass), detaljer bare nær spilleren. Budsjettet er 300 draw calls som ellers.

### Roswell i 1986

Roswell-historien fikk nytt liv rundt 1978 til 1980, da boka The Roswell Incident kom ut. I 1986 er den halvt glemt og halvt vekket: noen tjener litt på den, andre er lei (servitrisen, som allerede er i spillet). Basen ved Roswell ble lagt ned i 1967. Turismen og UFO-museet kom først tidlig på 1990-tallet. Caliche står altså like før bølgen, og ett håndmalt skilt er riktig, ikke en hel gate.

Romvesenene er derfor kultur og rykter i byen: plakater, unger med masker, suvenirmannens «vrakdeler», innringerne hos nattpraten. Ansiktet i FLASH er som i dag. Hvis Tom vil ha mer, er mitt forslag ett tvetydig glimt som bare synes på film: en liten skikkelse i kanten av frontlysene på ett bilde fra feltkameraet, aldri med øynene.

### Twin Peaks: det vi låner og det vi lar være

Låner: det vanlige og varme med noe galt under, dineren som hjertet i byen, faste folk med hver sin hemmelighet, langsom rytme og lange pauser, et trafikklys som henger i vinden, rolige synthflater (Scott Buckley, som spillet bruker fra før), og at byen er en annen klokka tre om natta.

Lar være: sitater, figurene, det røde rommet og kirsebærpaien. Vi har kaffe og grønn chili, antennene der Twin Peaks har skogen, og suset fra hydrogenlinja der de har uglene (uglene finnes allerede i naturlyden, og kan få stå i fred).

### Døgnsyklus: hvordan, hvis den kommer

- Ikke et døgn som går fritt i 24 timer. Mye av et døgn er kjedelig å spille, og fullt dagslys viser stilen på sitt svakeste. I stedet biter av døgnet som går sammenhengende inne i biten, slik Firewatch gjør det, for eksempel 17:30 til 23:41 i B.
- Teknisk finnes begynnelsen: `Sky.setClock` og `uDawn` flytter allerede himmelen mot gry. Det som mangler: solas plass og himmelens farger for ettermiddag og kveld, tåke og omgivelseslys som følger himmelen, natriumlamper og neon som tennes av en fotocelle når det mørkner, og på Ultra sola som lys med skygge.
- Solnedgang 13. april 1986 ved Roswell er 18:27 MST, og den borgerlige skumringen slutter 18:53 (regnet med NOAA-formelen; den gir også soloppgang 05:29 til 05:30 den 14., som stemmer med kanon).
- Klokka i fri bevegelse: natten har faste punkter (02:13, 05:29). Når spilleren kan ta omveier, går klokka i historiens tempo og blir stående rett før neste faste punkt, slik den allerede blir stående på 05:28:59 på den gamle veien. Da hopper aldri en omvei over historien.

### Innvendinger og hva som gjøres med dem

1. **En fri by tapper spenningen fra en stram natt.** Byen endrer seg med kapitlene (butikkene stenger, lys går ut, folk går hjem), notatboka har alltid ett tydelig mål, og hendelsene øker med natten. Prøves i testen under: går Tom seg bort, eller vil han tilbake til saken?
2. **Tilfeldige skvetter blir billige og bryter R1 og R5.** Hver hendelse er en gjentakelse på en linje med klokkeslett, skrekken kommer av noe som mangler, ingenting følger etter, og det går minst ti minutter mellom to.
3. **For mye romvesen dreper mysteriet.** Kultur og rykter, FLASH som i dag, høyst ett glimt på film.
4. **Dagslys koster og viser stilen.** Biter av døgnet, og kvelden først: lav sol, lange skygger og silhuetter tilgir mye.
5. **En by på mobil.** Ett område, sammenslåtte fasader, få rom, detaljer bare nær spilleren.
6. **Det blir Twin Peaks-kopi.** Egne folk, og observatørens arbeid (observer, dokumenter, sammenlign) som det man faktisk gjør.

### Min anbefaling

Mål: B, kvelden før og natten, med byen som knutepunkt. Det gir mest av døgnfølelsen for prisen, holder den ene natten og slutten, og gjør Nora og Dale til folk man har møtt. C og D bare hvis kveldslyset i B holder i lavpolystilen. E parkert.

Første steg er A, uten døgn: byen om natta og hendelsene. Den prøver det viktigste først, om det er gøy å legge merke til, skrive ned og koble rare ting, og alt den bygger, brukes i B.

**Første versjon (avgrenset):**

- Caliche som én gate ved dineren: dineren og seks til åtte fasader, blinkende trafikklys, bensinstasjonen med betjent og telefonautomat, vinduet hos sheriffen, AM-stasjonen med verten i vinduet, skiltet til suvenirbutikken.
- Hendelsesregissøren med fem hendelser: telefonen med skriveren, sirissene som stopper, kyrne mot az 026, gatelysene langs linja, bilradioen fra 1947. Tilfeldige feil med bilen nær linjene.
- Funn med klokke og sted, merker på kartet, og linja som tegner seg etter tre merker.
- Åpen fra kapittel 3 (når bilen kommer seg ut fra SARO med en grunn) til kapittel 5.
- Venter: døgnet, flere folk, flere rom, flere hendelser. Motellet 700 m sør og antennefeltet er egne punkter fra 15. oktober.
- Tester: ny `events.py` (hver hendelse skjer, gir funn og merke, kommer ikke igjen, ikke utenfor vinduet sitt), `reachcheck.py` for byen, `drives.py`, kapittel 3 til 6, lagringen og gjennomspillingen, og draw calls i byen på Low.

**Den billigste testen, før byen bygges:** regissøren med tre hendelser på steder som finnes (telefonautomaten ved dineren, sirissene på SARO, antennene mot az 026) og merkene på kartet. Tom spiller kapittel 3 til 5 og svarer på tre ting: Så du dem? Fikk du lyst til å lete etter flere? Føltes linja på kartet som noe du fant selv? Ja på det meste: bygg Caliche. Gikk de forbi uten at han så dem: tydeligere, lyden først. Føltes de som støy: færre, og knyttet tettere til historien.

Den største uprøvde antakelsen er at det er spennende i seg selv å legge merke til og skrive ned rare ting uten at noe jager deg. Det er det testen sjekker.

**Det Tom må avgjøre:**

1. Én natt (A, og så kvelden før i B) eller flere dager (C)? Jeg anbefaler én natt med kvelden før.
2. Romvesener: bare som kultur og rykter i byen og FLASH som i dag, eller i tillegg et tvetydig glimt som bare synes på film? Jeg anbefaler det siste.

Navnet på byen kan vente til den bygges.

Faktaene om Roswell-historien (boka 1980, basen 1967, museet tidlig på 1990-tallet), Masters 10. til 13. april 1986 og Halley nærmest jorda 11. april 1986 er fra hukommelsen og ikke slått opp her. De sjekkes før noe av det står i spillet. Soltidene er regnet.

**Tom svarte 16. oktober:** heller mot A, én natt og byen om natta. Romvesenene skal alltid være der i bakgrunnen, som noe skummelt som kan skje når som helst, og tilfeldige bortføringer skal med. Hendelsesregissøren, hendelsene, kartet med linjene, Roswell som kultur og rykter og stemningen fra Twin Peaks skal med. B står som en mulighet senere, ikke som mål. Utdypet og forbedret i `HENDELSER.md`, med fire nye spørsmål til Tom om bortføringene.

## SARO som knutepunkt, lastebilen når som helst, lommelykt og undersøkelse (15. oktober)

Tom 15. oktober: lastebilen skal kunne brukes når som helst i vanlig spill (avgjort). På sikt: SARO og observatoriet som et knutepunkt der flere rare ting skjer, motellet lenger unna så man kjører dit (eller går et stykke), gå ut og undersøke antennene, lommelykt, og høyreklikk for zoom og undersøkelse. Og: kan noe av det som står i rulleteksten til They Are Here brukes?

Dette flytter spillet litt bort fra beslutningen 4. oktober om korte, håndlagde etapper og ikke en åpen verden. Forslaget under holder det likevel lite: noen få steder på samme vei, ikke et åpent landskap.

### 1. Lastebilen når som helst (avgjort)

- Bilen står på SARO-plassen fra start. Man går til førerdøra og setter seg inn når man vil, og kjører på veiene som finnes: gården, riksveien sør til dineren og sporet til STATION 01. Man går ut på plassene: SARO, porten ved stasjonen, dineren, og senere motellet og antennefeltet.
- Stedene har en tilstand før kapitlet sitt. Porten ved STATION 01 er lukket før kapittel 3. Dineren er åpen, men servitrisen sier bare noe vanlig; utklippet, telefonen til Ward og kartet kommer først i kapittel 5. Rom 6 er låst til kapittel 4. Den gamle veien er sperret ved milestolpe 0 (`ROAD CLOSED`, sperrebukker) til kapittel 6, så slutten bare kan skje der den skal.
- Målet står i notatboka som før. Kjører man bort, sier en kort linje hvor man skal når man kommer tilbake. Klokka går som vanlig, og det lagres ikke under kjøring.
- Teknisk er mye på plass: fri kjøring finnes i utviklermenyen (`World.freeDrive`), og turene går allerede gjennom alle områdene uten kutt. Det som mangler, er at bilen alltid har en interaksjon (ikke bare når et kapittel overstyrer den), fri kjøring mellom flere områder i vanlig spill, og tilstandene over. Tester: `drives.py`, `devmenu.py`, kapittel 3 til 6 og lagringen.

### 2. SARO som knutepunkt der rare ting skjer

Hovedhistorien og den ene slutten står. Rundt den kommer små hendelser man kan oppleve når man er ute, og som legger et funn i notatboka (Findings). De er aldri nødvendige for å komme videre, skjer én gang hver, og følger regel R1: fenomenet følger referansene (1947-linjene, az 026, 47 sekunder, 1420 MHz). Ingen monstre, og ingen lys over motellet (Tom 4. oktober).

Ti eksempler å velge fra:

1. Alle antennene snur seg samlet mot az 026 i 47 sekunder, uten at noen sitter ved pulten. Driftsloggen viser ingen kommando.
2. De røde varsellysene på antennene blinker fire og sju ganger, én gang, og så er de som før.
3. Rimfrost på én antenne (S-11) en varm natt. Den smelter mens man ser på.
4. En ring av vissent gress rundt foten av S-19. Jorda er varm.
5. Fotspor i grusen ut mot S-03 som stopper midt på stien.
6. En bil står ved gjerdet med lysene på og motoren varm, ingen i den. Når man kommer tilbake, er den borte.
7. Bilradioen: AM-stasjonen blir stille i nøyaktig 47 sekunder, samme minutt hver time.
8. Telefonen på motellkontoret ringer når man står der. Ingen i den, bare suset fra hydrogenlinja.
9. Lys over mesaen i nordvest som bare synes i feltkameraet, ikke med øynene.
10. Lommelykta og bilen flimrer når man er nær et av stedene over, svakt, som et forvarsel om THE EVENT.

### 3. Motellet lenger unna

**Bygget 16. oktober kveld:** motellet står nå 650 m sør langs riksveien på vestsiden, i SAROs eget område (ikke veiens), så man kan gå hele veien langs veiskulderen uten bytte og kjøre dit og la bilen stå på plassen. Bilen går over i veiens område 60 m sør for motellet. Detaljer i `log.md`.

I dag ligger Sierra Motor Court rett over riksveien fra SARO, rundt 45 m unna. Forslag: flytt det rundt 700 m sør langs riksveien, på vestsiden, i veiens område slik dineren nå står der. Det blir rundt 40 sekunder i bil eller 6 minutter til fots (1,9 m/s), og neonskiltet blir et landemerke på turen sør til dineren. Rom 6 er fortsatt et eget område innenfor døra.

For å kunne gå dit må veiskulderen mellom SARO og motellet bli gangbar, noe veiens område ikke har i dag. Kapittel 4 må skrives om fra «gå over veien» til «kjør eller gå», og `chapter4.py`, `reachcheck.py`, `drives.py` og gjennomspillingen må følge med.

### 4. Ut til antennene

Antennefeltet ligger nord for nettinggjerdet ved z -20: 27 antenner fra 42 til 420 m ut, og i dag kan man bare gå til S-03. Forslag: en port i gjerdet, grusspor mellom antennene, og ved hver antenne et apparatskap med skilt (navnet, az og el), en stige og en kabelgrøft. Ting å undersøke: skiltene, en driftslogg i skapet, boltene, kabler som er kuttet eller skjøtt. Mellom natriumlampene er det mørkt, og det er der lommelykta trengs. Detaljene ved foten bygges bare nær spilleren, så mobilen holder.

### 5. Lommelykt

- Den ligger allerede på passasjersetet i lastebilen (tegnet i `Truck.ts`). Man tar den derfra, eller fra en skuff i kontrollrommet.
- F på PC, en knapp på mobilen.
- Lyset bygges som frontlysene: tre falske lyspunkter langs strålen i stedets eget lampesett, som er billig på mobil. På Ultra kommer ett ekte spotlys med skygge.
- En mørk metallykt fra 1980-tallet nederst i høyre hjørne. Senere kan en hånd holde den (forslaget om kroppen fra 5. oktober).
- Batteri: jeg anbefaler uten, men at den flimrer nær hendelsene (punkt 10 over).

### 6. Høyreklikk: zoom og undersøk

- Hold høyre musknapp for å se nærmere: synsfeltet går mykt fra 70 til rundt 30 grader. På mobilen en lupeknapp man holder. Ikke mens feltkameraet er løftet eller et panel er åpent.
- Undersøk: står siktet på noe som kan undersøkes, viser zoomen en kort tekst nederst (engelsk, i spillet), og funnet går i notatboka hvis det betyr noe. Først tekst og zoom. Senere, for noen få ting (skiltene, bolten, driftsloggen, lommelykta), et nærbilde man kan snu.
- Kapittel 1 har allerede «Examine contact print» i fotolaben. Den nye undersøkelsen gjør det samme ute i verden.

### 7. Lenkene i rulleteksten til They Are Here

Vi tar aldri noe fra They Are Here selv, bare fra originalkildene og under deres egne lisenser. Sjekket 15. oktober:

| Hva | Lisens | Kan vi bruke det? |
| --- | --- | --- |
| Horror Engine (Unreal Marketplace) | Bare for Unreal Engine | Nei, ikke i three.js |
| Flashlight (Sketchfab, Hepatitis_Bee) | CC BY, kan lastes ned | Ja, med kreditering. Kan bli lommelykta i hånda hvis den er lett nok |
| Bob Lazar Ufo (Sketchfab, Batuhan13) | CC BY, kan lastes ned | Kanskje, som en fjern silhuett i et glimt. Lazar-historien er fra 1989 og spillet fra 1986, så den må ikke kalles det |
| Phone, Desk_Table_Chair, Book, Old monitor (Sketchfab) | CC BY | Mulig, men vi har modeller i kode alt. Bare verdt det for PC-nivået, hvis de er lette og passer stilen |
| Computer Setup (Sketchfab, Brandon Westlake) | UNVERIFIED, siden svarte ikke | Sjekkes på nytt |
| Walkie Talkie, Textured (Sketchfab) | CC BY | Vi har en walkie; bare som referanse |
| Freesound-lydene (romlyder, fremmedspråk, slag, «Paranormal Activity Sound» og flere) | UNVERIFIED: freesound.org svarte 403 herfra. Rulleteksten sier selv at «Dark ambient scream» og «OhDearScare» er CC BY, og at Timbres to lyder er CC BY-NC | Hver lyd sjekkes på siden sin før bruk. CC BY-NC binder hvis spillet noen gang skal selges |
| (16. oktober) Freesound-lydene sjekket på nytt | Freesound svarte. Lisensene står i `HENDELSER.md`, punkt 11b: tre er CC0, elleve CC BY, og Jump-Scare_8 og Timbres to er CC BY-NC | Ja, de CC0 og CC BY, med kreditering. Ikke de CC BY-NC |
| Flickr-bildene | Ukjent per bilde. Flere er nyhetsklipp og filmbilder (Independence Day er beskyttet) | Nei, høyst som referanse. De passer heller ikke i 1986 |
| Video fra Pexels (Luis Quintero) | Pexels-lisensen: fri bruk | Innholdet er ukjent, og video passer dårlig i stilen |

Det som brukes, føres i `THIRD_PARTY_NOTICES.md` og i rulleteksten. 3D-modeller er noe nytt i spillet (`GLTFLoader`, større enkeltfil), så det bør være noen få utvalgte. Repoet har ingen lisensfil for selve spillet.

### Rekkefølge jeg foreslår

1. Lastebilen når som helst, med tilstandene før hvert kapittel. Avgjort, middels stort.
2. Lommelykt og høyreklikk med zoom og undersøk. Grunnmuren for å gå ut og se.
3. Antennefeltet gangbart med port, spor og skap ved antennene.
4. Motellet 700 m sør, med kapittel 4 skrevet om.
5. Hendelsene, tre eller fire først, så flere.

Codex kan ta en avgrenset del underveis, for eksempel skapet og skiltet ved antennene som egen modul med kontrakt (som Kessler-porten), eller å gjøre Flashlight-modellen klar (sjekke polygoner, lage en liten glTF, lisensnotat).

**Det Tom må avgjøre:** (1) Motellet sør mot dineren (min anbefaling) eller nord gjennom antennefeltet? (2) Lommelykt med batteri eller uten? (3) Den gamle veien før kapittel 6: sperret ved milestolpe 0 (min anbefaling) eller åpen uten at noe skjer? (4) Skal hendelsene bare være stemning og funn, eller kan de også gi hint i hovedsaken?

**Tom svarte 15. oktober:** (1) motellet sør, (2) lommelykt uten batteri, (3) den gamle veien sperret ved milestolpe 0 til kapittel 6, (4) hendelsene kan også gi hint i hovedsaken. Punkt 1 og 2 i rekkefølgen bygges først.

**Bygget 15. oktober kveld:** lastebilen når som helst (fra kapittel 1, med kjetting i gangåpningen ved STATION 01 til kapittel 3 og ROAD CLOSED ved milestolpe 0 til kapittel 5 sender bilen ut), lommelykta fra passasjersetet (F, Light på telefon) og zoom med undersøk (høyre museknapp, Zoom på telefon). Foreløpig har lastebilen, den lenkede gangåpningen og sperringen undersøk-tekst; flere ting får det etter hvert.

## Innlevelse, lyd, grafikk og Unreal (5. oktober)

Tom spurte hva spillet trenger for mer innlevelse, om prosjektbiblioteket har noe vi trenger, om vi bør dekompilere Voices of the Void, og om spillet kan flyttes til Unreal. Hele natten er nå bygd, så dette handler om å løfte det som finnes.

**Det som gir mest, i rekkefølge:**

1. **Stemmer.** Tomás, Nora, Ward, radioen og telefonen er i dag sus i et stemmebånd med tekst under. Ekte stemmer på de 15 til 20 replikkene som høres (ikke alt som står i panelene) gir mer enn noe annet. Enklest: Tom og to eller tre andre leser dem inn med telefonen, og jeg legger på telefon-, radio- og båndfilter i koden. Alternativt designede stemmer (ikke kloning av ekte personer) laget lokalt på Toms PC.
2. **Lydbilde per sted.** Fugler som begynner ved gryet, vind og grus på den gamle veien, lysrørsus og kjøleskap i dineren, aircondition og tv-sus på motellet, metallknepp i antennene når de snur. Lydene finnes som CC0 (Freesound, Kenney) eller kan lages i koden.
3. **Musikk, sparsomt.** Spillet har bare tittelmusikken. Lave flater og stikk i noen få øyeblikk (utskriften kl. 02:13, kabelbruddet, Nora), ikke under kjøringen ut på veien. Kan lages i koden med Web Audio, eller hentes fra Scott Buckley (samme lisens som tittelmusikken).
4. **Grafikk i three.js.** Gryet over mesaen med skyer og lysstråler, tettere vegetasjon og steiner nær veiene, bedre folk i dineren og på motellet (enkle animasjoner), og på PC fargegradering og dybdeskarphet i overgangene. På sikt WebGPU-rendereren i three.js for PC.
5. **Kroppen.** Hender som holder kameraet og lommelykta, pust når det er kaldt, at hodet ser ned på papirene.

**Prosjektbiblioteket:** relevant er `scottstts/Threejs-Awesome-Graphics-Agent-Skills` (MIT: lys, atmosfære, vær, materialer i three.js), `ektogamat/threejs-conference` (MIT: WebGPU og TSL, våt asfalt, regn, filmisk etterbehandling i førsteperson) og `debpalash/VoiceStudio` (AGPL, lokale designede stemmer; programmet må kjøres på Toms PC, og lisensen gjelder programmet, ikke det som lages). `YuE` (musikkgenerering) krever et kraftig skjermkort og er ikke nødvendig. Lydene vi allerede bruker fra Morbidium og Loincloth Legends står i `THIRD_PARTY_NOTICES.md`.

**Voices of the Void:** ingen dekompilering for SIGNAL / 47. Spillets kode og assets har ingen lisens for gjenbruk, og kopien ligger bare på Toms PC. Det nyttige er å spille det og skrive ned hva som gjør stemningen (lyden i basen, signalbehandlingen, radioprat, døgnrytmen), så lager jeg vår egen versjon fra beskrivelsen.

**Unreal:** mulig, men ikke nå. Det gir mye bedre lys og folk (Lumen, MetaHuman, volumetrisk tåke, Sequencer for slutten), men Unreal kan ikke lenger lage nettversjoner, så Pages og spilling på telefon i nettleseren forsvinner. Alt spillet gjør (kapitlene, gåtene, panelene, lagringen) må skrives på nytt i C++ og Blueprints, og Unreal-editoren kan ikke kjøres i skymiljøet jeg jobber i, så all bygging og testing må skje på Toms PC. Forslag: gjør nettversjonen ferdig og spill den igjennom. Vil Tom prøve Unreal, start med en liten bit (den gamle veien i gry og THE EVENT) som egen prøve på hans PC, og bestem etter den.

## Status 4. oktober

Tom ba meg gjennomføre forslagene. Gjort og i spillet: arbeidsordren (3.1), kapittel 2 (3.2, heter «The Amended Record» i spillet), saksmappen (3.3), fotografiene i IndexedDB (3.4), de nye innstillingene og fotsteg ute (4). Teksturene (4) kom fra ChatGPT/Codex i PR #30 og #31 og er koblet inn med tekst i kode oppå. Ikke gjort: skjuling av smådetaljer på avstand (4), fordi målingene ikke viser behov ennå, bytte av GitHub-handlinger og npm audit (ingen fiks finnes). Del 2 og 5 gjelder fortsatt. Neste store steg er kapittel 3, feltreisen til STATION 01 (se todo.md).

## 1. Beslutninger Tom må ta

Avgjort 4. oktober: Tom ba meg gjennomføre forslagene mine, så alle fire er avgjort slik jeg foreslo under. Se memory.md.

1. **Rom 6 eller rom 47.** Designbibelen (`Docs/DesignBible13/design-bible.md`, K4) legger Nora Vega i rom 6 på Sierra Motor Court, og sier at kontoret og rom 6 er de eneste interiørene. `ART_BRIEF.md` og `todo.md` snakker om rom 47. Mitt forslag er rom 6. Tallet 47 bærer allerede mye (47 sekunder, SIGNAL / 47), og et rom med samme nummer kan fort virke som en vits.
2. **To eller tre slutter.** Designbibelen har to: A "Bryt referansen" og B "Fullfør én registrering", og skriver rett ut at det ikke finnes noen tredje, hemmelig slutt. De tre sluttene Silence, Answer og Listen kommer fra ChatGPT-samtalen. memory.md påsto at designbibelen også hadde tre. Det stemte ikke, og er rettet.
3. **Dale, R. og Reyes.** Webversjonen lar Dale skrive skiftloggen. Unity signerte "R.". Designbibelen kaller spilleren Reyes og vaktsjefen Dr. Evelyn Ward. Forslag: behold Dale som kollegaen som gikk hjem, og la Reyes og Ward komme inn gjennom arbeidsordren (punkt 3.1).
4. **Bilen bak gjerdet og telefonnummeret til motellet.** Står i todo.md under kapittel 1, men finnes ikke i Unity-kapitlet. I designbibelen er det arkivet i K2 som viser at Nora driver Sierra Motor Court. Et telefonnummer på et bilde i kapittel 1 hopper over den oppdagelsen. Forslag: la det være. Vil Tom ha et hint, kan motellskiltet lyse svakt i horisonten på FRAME 01, uten nummer.

## 2. Det som bør testes på ekte maskiner

Alt under er sjekket headless, men ingenting er prøvd på en ekte telefon ennå.

- Spill prologen og hele kapittel 1 fra https://tombonator3000.github.io/SIGNAL-47/ på telefonen. Se særlig etter: om kameraknappen og utløseren er lette å treffe med tommelen, om lupe og X/Y-glidere i fotopanelene er brukbare, og om fotolaben er for mørk.
- Legg `?debug` bak adressen og skriv ned fps-linja tre steder: ved skrivebordet, ute på gangveien og inne i fotolaben. Målt antall draw calls i skyen er 34 til 202, med 202 som verste når man ser sørover mot kontrollrommet fra B-12. Budsjettet er 300.
- Lyd: vinden blir sterkere ute, S-03-skapet summer når man står ved det, og sikkerhetslampen i laben surrer svakt. Si fra om noe av det blir for mye.
- Prøv Continue etter å ha lukket fanen midt i kapittel 1. Saken og begge fotografiene skal komme tilbake.

## 3. Neste store steg

### 3.1 Arbeidsordren og Ward (lite, gir mye)

Designbibelen sier at K1 skal åpne med en kort arbeidsordre om morgenserien, med Dr. Evelyn Ward som vaktansvarlig på telefon. Nettprologen har ikke dette. Et ark på pulten ved skiftloggen holder: hvem Reyes er, at morgenserien starter 06:00, og at avvik skal dokumenteres før den kan kjøres. Det gir spilleren en grunn til å bry seg om rapporten i kapittel 1, og det knytter overgangen til K2. Telefonen bør ikke brukes til Ward før framtidssamtalen, den skal fortsatt være en overraskelse.

### 3.2 Kapittel 2: "Den strøkne protokollen"

Etter designbibelen: Ward gir tilgang til et lite arkiv på SARO fordi B-12-rapporten viser et referanseproblem. Tre samlinger kan leses i valgfri rekkefølge (gammel feltjournal, korrigerte protokoller, moderne vedlikeholdskart). To sammenkoblinger er nødvendige. Feil valg viser hvilket dokument som ikke støtter koblingen, uten at spilleren må gå en runde til. Til slutt en kort telefon til Nora og et reisepunkt som låses opp.

Det meste av verktøyet finnes allerede. Panelene for kontaktkopi og referansefil (`src/ui/Panels.ts`) kan brukes til å legge to dokumenter ved siden av hverandre og peke på det som er fjernet. Arkivrommet kan bygges som fotolaben: én liten blokk med egne lamper, inngang fra servicegården eller fra gangen sør i kontrollrommet.

Jeg anslår omtrent like mye arbeid som kapittel 1.

### 3.3 Saksmappe i notatboka

Spilleren har nå et motorlogg-ark, to fotografier, en observasjon, et funn og en rapport. De ligger som enkeltdokumenter. Før kapittel 2 bør notatboka få en fane "Case file" med små utgaver av fotografiene og dokumentene, så alt kan leses igjen samlet. Det er samme idé som evidence board i todo.md, bare enklere, og det trengs uansett når arkivet kommer.

### 3.4 Lagring som tåler flere kapitler

Saken lagres i `localStorage` (`s47.case`), med fotografiene som JPEG-tekst. Målt etter et helt kapittel: 414 kB, altså rundt 200 kB per bilde. Nettleserne gir omtrent 5 MB per opprinnelse, og alle Toms Pages-sider deler opprinnelsen `tombonator3000.github.io`, så plassen deles med de andre prosjektene. Designbibelen har seks eksponeringer. Flytt bildene til IndexedDB før kapittel 2, og la `localStorage` bare holde saken og hvor langt man har kommet.

## 4. Mindre forbedringer

- **Innstillinger:** i dag finnes lydstyrke, musfølsomhet og grafikknivå. Invertert Y-akse, synsfelt (FOV) og tekststørrelse er billige å legge til og gjør mye for noen spillere.
- **Fotsteg ute:** samme trinnlyd brukes inne og ute. Litt lavere avspillingshastighet og et svakt grusaktig lag på betongen ute vil gjøre overgangen tydeligere.
- **Teksturer fra ChatGPT:** kapittel 1 har fått mange flater som tegnes av koden og kan byttes mot ekte grafikk uten å endre spillet: S-03- og B-12-skapene, B-12-skiltet med stripen, feltkartet i laben, gulvmerkingen på gangveien, skiltene og prosesskortene i fotolaben. Jeg kan legge dem inn i `ART_BRIEF.md` med mål og format hvis Tom vil.
- **Ytelse hvis telefonen sliter:** kontrollrommet tegnes fullt når man ser det gjennom vinduene fra B-12. Det er det dyreste synsfeltet (202 draw calls). Smådetaljer inne i rommet kan skjules når spilleren står mer enn 15 meter unna.
- **GitHub-handlinger:** configure-pages v5, setup-node v4 og upload-pages-artifact v3 er laget for Node 20, og GitHub tvinger dem over på Node 24. Publiseringen virker. Bytt til versjoner laget for Node 24 når de er bekreftet å finnes.
- **npm audit:** 3 high i byggverktøyet (braces via vite-plugin-singlefile). Gjelder bare bygging, ikke spillet. Oppgrader pluginen når den får en fiks.

## 5. Det jeg ikke har kunnet sjekke

- fps, berøring og lydmiks på ekte telefon og PC.
- Safari på iPhone og iPad. Headless-testene kjører Chromium.
- Hvordan kapittel 1 føles i tid. Designbibelen budsjetterer 50 minutter for hele K1, prologen inkludert. Testene går gjennom på noen sekunder fordi de hopper mellom punkter, så det sier ingenting om spilletiden.
