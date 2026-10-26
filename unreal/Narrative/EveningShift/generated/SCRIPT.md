# SIGNAL / 47: Evening Shift

Forfatterpakke. Ikke runtime-integrert. Stemmeopptak: 109 rene takes finnes (voice/). Ingen video er produsert.

Spilltekst: engelsk. Regi: norsk. Medietider er planlagte, ikke ferdige opptakstider.

## arrival

### I01_DRIVE (commute / radio / live)

**Utløser:** Åpning på innkjøringen til Caliche, før SARO-skiftet. Avspilling stopper ved utstigning og kan fortsette i bilen.

**Regi:** Sen ettermiddag 13. april. Kjørbar innledning, ingen nedtelling. Egen vanlig radiovert, ikke Earl eller Dale. Lystig original eller klarert instrumental går før og mellom replikkene. Åpne hus, noen folk og vanlig trafikk gjør byen lesbar.

**Krav:** {"all": [], "none": ["arrival_radio_heard", "intro_arrived"]}

**RADIO_HOST [I01_DRIVE_001]**: Good afternoon, Caliche. Hope your Sunday has been an easy one.

*Lett og munter lokalradio.*

**RADIO_HOST [I01_DRIVE_002]**: We've still got a little sunshine left. Good time to get the washing off the line.

*Småprat, uten ladet pause.*

**RADIO_HOST [I01_DRIVE_003]**: A reminder from the feed store: tomorrow's delivery comes in after ten. Let them have their coffee first.

*Smil i stemmen.*

**RADIO_HOST [I01_DRIVE_004]**: And if you're heading out to work, the diner has the griddle on.

*Som en vanlig lokal opplysning.*

**RADIO_HOST [I01_DRIVE_005]**: Take it easy through town. There's no prize for getting to the next red light first.

*Lunt.*

**RADIO_HOST [I01_DRIVE_006]**: Here's another record for the road.

*Kort, gi plass til musikken.*


**Ved fullføring:** `{"set_flags": {"arrival_radio_heard": true}, "give_documents": []}`

### I02_DINER_HELLO (commute / physical / live)

**Utløser:** Første aktive henvendelse ved dinerdisken før intro_arrived. Stoppet er valgfritt.

**Regi:** Servitrisen tørker disken og legger frem meny. Vanlig lys, kjøkkenlyd og uanstrengt stemning. Ingen skjult horrorreplikk. Spilleren er taus.

**Krav:** {"all": [], "none": ["arrival_diner_finished", "food_bought", "intro_arrived"]}

**WAITRESS [I02_DINER_HELLO_001]**: Hi there. Sit wherever you like.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I02_DINER_HELLO_002]**: Kitchen's still going. Coffee's fresh enough that I can say it with a straight face.

*Lett humor.*

**WAITRESS [I02_DINER_HELLO_003]**: You want to see the menu, or are you just stretching your legs?

*Hverdagslig og varm. Ingen antydning til fare.*

- **I02_MENU**: Let me see the menu. → `I03_MENU`
  Krav: `{"all": [], "none": ["food_bought"]}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **I02_WATER**: Just a glass of water, please. → `I08_WATER`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **I02_ROAD**: How is the road out to SARO? → `I14_ROAD`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **I02_LEAVE**: I should keep moving. → `I09_LEAVE`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### I03_MENU (commute / physical / live)

**Utløser:** Åpnes fra velkomst eller kort småprat. Menyvalg er kjøpsøyeblikket og utføres én gang.

**Regi:** En enkel papirmeny med to synlige matvalg. Integrasjon må fullføre valgt svarnode også hvis samtalen avbrytes etter kjøp; samme bestilling skal ikke belastes eller gis to ganger.

**Krav:** {"all": [], "none": ["food_bought", "intro_arrived"]}

**WAITRESS [I03_MENU_001]**: Burger comes with fries. Sandwich comes with chips.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I03_MENU_002]**: Coffee with either one. I can wrap the food if you're taking it to work.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I03_MENU_003]**: What sounds good?

*Hverdagslig og varm. Ingen antydning til fare.*

- **I03_BURGER**: Buy a burger, fries and coffee. → `I04_BURGER`
  Krav: `{"all": [], "none": ["food_bought"]}`; ved valg: `{"set_flags": {"food_bought": true, "arrival_order_burger": true}, "give_documents": []}`
- **I03_SANDWICH**: Buy a sandwich, chips and coffee. → `I05_SANDWICH`
  Krav: `{"all": [], "none": ["food_bought"]}`; ved valg: `{"set_flags": {"food_bought": true, "arrival_order_sandwich": true}, "give_documents": []}`
- **I03_COOK**: What would you have? → `I15_RECOMMEND`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **I03_DECLINE**: Nothing today, thanks. → `I09_LEAVE`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### I04_BURGER (commute / physical / live)

**Utløser:** Svar på burgerkjøpet. Kjøpsflagget er allerede satt.

**Regi:** Vis burger, pommes frites og kaffekopp på et brett foran spilleren. Gjenstanden og reaksjonen må være synlig; et skjult flagg alene oppfyller ikke handlingen.

**Krav:** {"all": ["food_bought", "arrival_order_burger"], "none": ["arrival_diner_finished"]}

**WAITRESS [I04_BURGER_001]**: One burger, fries and coffee. Coming right up.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I04_BURGER_002]**: There you go. Careful with the plate.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I04_BURGER_003]**: Eating here, or shall I wrap it?

*Hverdagslig og varm. Ingen antydning til fare.*

- **I04_EAT**: Eat here. → `I06_EAT`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **I04_TAKE**: Take it with me. → `I07_TAKE`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### I05_SANDWICH (commute / physical / live)

**Utløser:** Svar på sandwichkjøpet. Kjøpsflagget er allerede satt.

**Regi:** Vis sandwich, chips og kaffekopp på et brett foran spilleren. Ingen andre ordre skal opprettes ved gjenåpning.

**Krav:** {"all": ["food_bought", "arrival_order_sandwich"], "none": ["arrival_diner_finished"]}

**WAITRESS [I05_SANDWICH_001]**: Sandwich, chips and coffee. You got it.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I05_SANDWICH_002]**: Here we are. Napkins are right beside you.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I05_SANDWICH_003]**: Staying for a minute, or taking that along?

*Hverdagslig og varm. Ingen antydning til fare.*

- **I05_EAT**: Eat here. → `I06_EAT`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **I05_TAKE**: Take it with me. → `I07_TAKE`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### I06_EAT (commute / physical / live)

**Utløser:** Spilleren velger å spise. Kort rolig bordsekvens, deretter fri kontroll og retur til bilen.

**Regi:** Vis at mat faktisk spises: tallerken/brett endres fra fullt til spist, kaffe reduseres, servitrisen samler inn. Ingen sultsystem eller klokkestraff. Rester skal ikke samtidig gis som ny takeaway.

**Krav:** {"all": ["food_bought"], "none": ["arrival_diner_finished", "food_taken"]}

**WAITRESS [I06_EAT_001]**: Sure. Take your time.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I06_EAT_002]**: Mind if I top that coffee up?

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I06_EAT_003]**: There. That should keep you going.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I06_EAT_004]**: Thanks for stopping in. Have a good shift.

*Hverdagslig og varm. Ingen antydning til fare.*


**Ved fullføring:** `{"set_flags": {"food_eaten": true, "arrival_diner_finished": true}, "give_documents": []}`

### I07_TAKE (commute / physical / live)

**Utløser:** Spilleren velger å ta med. Etter utlevering går kontrollen tilbake til disken, så bilen.

**Regi:** Vis at samme kjøpte mat pakkes i papirpose og kaffe får lokk. Gi én synlig pose i inventar/transportplass og én reaksjonstekst. Ved ankomst kan posen settes på SAROs personalbord; et senere spisevalg må bruke samme porsjon.

**Krav:** {"all": ["food_bought"], "none": ["arrival_diner_finished", "food_eaten"]}

**WAITRESS [I07_TAKE_001]**: No trouble. I'll put it in a bag.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I07_TAKE_002]**: Fork and napkins are in there. Coffee's got a lid.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I07_TAKE_003]**: Keep the bag upright. The bottom's got more confidence than strength.

*Smilende, vanlig spøk.*

**WAITRESS [I07_TAKE_004]**: Have a good evening out there.

*Hverdagslig og varm. Ingen antydning til fare.*


**Ved fullføring:** `{"set_flags": {"food_taken": true, "arrival_diner_finished": true}, "give_documents": []}`

### I08_WATER (commute / physical / live)

**Utløser:** Gratis vann etter velkomst. Valget avslutter ikke automatisk besøket.

**Regi:** Et glass vann settes synlig på disken; ingen mat eller kjøpsflagg. Ingen skjult betaling.

**Krav:** {"all": [], "none": ["food_bought", "arrival_diner_finished", "intro_arrived"]}

**WAITRESS [I08_WATER_001]**: Of course. Here you are.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I08_WATER_002]**: You've caught us between the supper crowd and the coffee crowd.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I08_WATER_003]**: Nice having a minute to wipe a table before somebody needs it.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I08_WATER_004]**: Let me know if you change your mind about food.

*Hverdagslig og varm. Ingen antydning til fare.*

- **I08_MENU**: Actually, I will look at the menu. → `I03_MENU`
  Krav: `{"all": [], "none": ["food_bought"]}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **I08_ROAD**: Which turn takes me to SARO? → `I14_ROAD`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **I08_LEAVE**: Thanks. I should get going. → `I09_LEAVE`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### I09_LEAVE (commute / physical / live)

**Utløser:** Spilleren avslår mat eller avslutter småprat. Fri kontroll ved utgang, deretter kjøring.

**Regi:** Rydd meny/vann ved behov. Ingen kjøp, ingen sultstraff og ingen hindring for å nå SARO.

**Krav:** {"all": [], "none": ["food_bought", "arrival_diner_finished"]}

**WAITRESS [I09_LEAVE_001]**: All right. Thanks for coming in.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I09_LEAVE_002]**: We're here if you need something later.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I09_LEAVE_003]**: Drive safely.

*Hverdagslig og varm. Ingen antydning til fare.*


**Ved fullføring:** `{"set_flags": {"arrival_diner_finished": true}, "give_documents": []}`

### I10_BACK_ON_ROAD (commute / radio / live)

**Utløser:** Spilleren forlater Caliche mot SARO, enten direkte eller etter diner. Radiosekvensen krever ikke matkjøp.

**Regi:** Gatens lyder viker for dekkstøy og musikk. Ingen skurring, avbrutt nødanrop eller fremmed skikkelse i denne sekvensen.

**Krav:** {"all": [], "none": ["arrival_departure_radio_heard", "intro_arrived"]}

**RADIO_HOST [I10_BACK_ON_ROAD_001]**: You're listening to local radio here in Caliche.

*Avslappet.*

**RADIO_HOST [I10_BACK_ON_ROAD_002]**: Church supper volunteers, your pans are clean and waiting in the kitchen. Please take yours home before somebody makes another casserole in it.

*Hverdagslig opplesning.*

**RADIO_HOST [I10_BACK_ON_ROAD_003]**: We'll leave you with some music now.

*Lyst.*

**RADIO_HOST [I10_BACK_ON_ROAD_004]**: Good evening to everybody starting work, and everybody heading home.

*Vanlig avslutning.*


**Ved fullføring:** `{"set_flags": {"arrival_departure_radio_heard": true}, "give_documents": []}`

### I11_SARO_ARRIVAL (handover / ui / live)

**Utløser:** Når bilen er parkert ved SARO og spilleren går inn på skiftet. Kan nås selv om dineren aldri ble besøkt.

**Regi:** Vis datolinje og skiftstart 18:00. Innkjøringen har ingen nedtelling; klokken begynner på skiftets tidspunkt ved dette punktet. Behold kjøpt mat. Dales overlevering eies av egen modul og kommer bare over telefon eller radio. Ingen fysisk kollega venter.

**Krav:** {"all": [], "none": ["intro_arrived"]}


**Ved fullføring:** `{"set_flags": {"intro_arrived": true}, "give_documents": []}`

### I12_ALL_NIGHT_BOUGHT (all_night / physical / live)

**Utløser:** Kort velkomst ved første diskhenvendelse i eksisterende All Night, bare etter tidligere matkjøp.

**Regi:** Gi plass til kapittelets eksisterende dialog. Ingen ny bestilling, automatisk bevisutlevering eller løsning av kart/P13. On_complete gir kontroll tilbake til eksisterende dinerflyt.

**Krav:** {"all": ["ctx_all_night_active", "food_bought"], "none": ["arrival_late_greeted"]}

**WAITRESS [I12_ALL_NIGHT_BOUGHT_001]**: Hi again. Back from SARO?

*Gjenkjennende, litt trøtt.*

**WAITRESS [I12_ALL_NIGHT_BOUGHT_002]**: Coffee's on. You can sit down a minute.

*Rolig omsorg, ingen trussel.*

**WAITRESS [I12_ALL_NIGHT_BOUGHT_003]**: That driver came in a little while ago. He's over there if you're looking for somebody who's been on the road.

*Pek diskret mot eksisterende sjåfør.*

**WAITRESS [I12_ALL_NIGHT_BOUGHT_004]**: There's a paper on the counter, too. Nobody's claimed it.

*Nevner den eksisterende avisen uten å forklare dens funn.*


**Ved fullføring:** `{"set_flags": {"arrival_late_greeted": true}, "give_documents": []}`

### I13_ALL_NIGHT_NO_FOOD (all_night / physical / live)

**Utløser:** Alternativ velkomst ved første diskhenvendelse i All Night hvis ingen mat ble kjøpt. Fungerer også når hele første stopp ble hoppet over.

**Regi:** Ordlyden forutsetter ikke tidligere møte. Samme retur til eksisterende dinerflyt som I12. Ikke sett food_bought; senere originaldialog kan håndtere øvrig servering.

**Krav:** {"all": ["ctx_all_night_active"], "none": ["food_bought", "arrival_late_greeted"]}

**WAITRESS [I13_ALL_NIGHT_NO_FOOD_001]**: Evening. Coming in from work?

*Varmt, trøtt.*

**WAITRESS [I13_ALL_NIGHT_NO_FOOD_002]**: Take a seat if you need one. Coffee's on.

*Avslappet.*

**WAITRESS [I13_ALL_NIGHT_NO_FOOD_003]**: That fellow over there just came off the road. He's been sitting with his cup for a while.

*Vis eksisterende sjåfør.*

**WAITRESS [I13_ALL_NIGHT_NO_FOOD_004]**: Paper's on the counter if you want something to read.

*Ingen trykk på avisen eller den relevante artikkelen.*


**Ved fullføring:** `{"set_flags": {"arrival_late_greeted": true}, "give_documents": []}`

### I14_ROAD (commute / physical / live)

**Utløser:** Valgfritt vanlig spørsmål om veien, fra velkomst eller vannglass.

**Regi:** Samtalen orienterer mot SARO. Ingen profeti, advarsel om sær oppførsel eller referanse til senere hendelser.

**Krav:** {"all": [], "none": ["food_bought", "arrival_diner_finished", "intro_arrived"]}

**WAITRESS [I14_ROAD_001]**: Keep on through town, then take the marked turn out to the array.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I14_ROAD_002]**: Big sign. Hard to miss unless you're arguing with the radio.

*Lett spøk.*

**WAITRESS [I14_ROAD_003]**: You'll want to eat before you get settled in out there.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I14_ROAD_004]**: Want the menu before you go?

*Hverdagslig og varm. Ingen antydning til fare.*

- **I14_MENU**: Yes, please. → `I03_MENU`
  Krav: `{"all": [], "none": ["food_bought"]}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **I14_LEAVE**: I will get going, thanks. → `I09_LEAVE`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### I15_RECOMMEND (commute / physical / live)

**Utløser:** Kort valggren fra menyen. Spørsmålet har et endelig svar og ingen obligatorisk løkke.

**Regi:** Menyen blir liggende. Ingen ny vare eller kjøpsflag før det konkrete kjøpevalget.

**Krav:** {"all": [], "none": ["food_bought", "arrival_diner_finished", "intro_arrived"]}

**WAITRESS [I15_RECOMMEND_001]**: If you're hungry, the burger.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I15_RECOMMEND_002]**: If you're driving, the sandwich is less of a project.

*Tørt og vennlig.*

**WAITRESS [I15_RECOMMEND_003]**: Either way, I'm putting napkins in the bag.

*Hverdagslig og varm. Ingen antydning til fare.*

**WAITRESS [I15_RECOMMEND_004]**: Your call.

*Hverdagslig og varm. Ingen antydning til fare.*

- **I15_BURGER**: Buy a burger, fries and coffee. → `I04_BURGER`
  Krav: `{"all": [], "none": ["food_bought"]}`; ved valg: `{"set_flags": {"food_bought": true, "arrival_order_burger": true}, "give_documents": []}`
- **I15_SANDWICH**: Buy a sandwich, chips and coffee. → `I05_SANDWICH`
  Krav: `{"all": [], "none": ["food_bought"]}`; ved valg: `{"set_flags": {"food_bought": true, "arrival_order_sandwich": true}, "give_documents": []}`
- **I15_LEAVE**: Thanks. Maybe another time. → `I09_LEAVE`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

**ARRIVAL_SHIFT_CARD**: SARO
APRIL 13, 1986
18:00

*Skiftkort ved faktisk ankomst. Ikke bruk klokkeslett som nedtelling i innkjøringen.*

**ARRIVAL_FOOD_BURGER**: Burger, fries and coffee

*Matens synlige navn på brett og i pose når arrival_order_burger er satt.*

**ARRIVAL_FOOD_SANDWICH**: Sandwich, chips and coffee

*Matens synlige navn når arrival_order_sandwich er satt.*

**ARRIVAL_FOOD_PACKED**: Your food is packed. Coffee lid is on.

*Vis sammen med synlig pose ved I07, ikke som erstatning for gjenstanden.*

**ARRIVAL_FOOD_EATEN**: An empty plate and a last sip of coffee.

*Kort synlig respons på ferdig måltid I06. Brettet skal også endres.*

**ARRIVAL_FOOD_AT_SARO**: Set your dinner on the break-room table.

*Valgfritt fysisk prompt bare hvis food_taken og ikke food_eaten. Bruk samme matporsjon, ingen ny gratis bestilling.*

**ARRIVAL_RETURN_TO_CAR**: Return to the truck when you are ready.

*Valgfri, nøktern retning etter diner. Spillerens innkjøring er uten nedtelling.*

## dale

### D01_HANDOVER (handover / phone / live)

**Utløser:** Første telefon ved SARO etter parkering. Egen skiftstart 18:00; ringingen venter ved åpne paneler.

**Regi:** Dale er hjemme eller på en annen vanlig landlinje. Ingen fysisk Dale vises. Valget om mat må være støttet av den faktiske innledningen.

**Krav:** {"all": ["intro_arrived"], "none": ["handover_done"]}

**DALE [D01_HANDOVER_001]**: Reyes? Dale. The logbook should be in front of you.

*Vanlig telefonstemme, rolig og litt tørr. Ingen truende pause.*

**DALE [D01_HANDOVER_002]**: I headed out early. The service folder is on the desk, under the work order.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D01_HANDOVER_003]**: The first survey is routine. Check calibration before you let it run.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D01_HANDOVER_004]**: Did you get something to eat?

*Nøkternt og konkret; ikke en skrekkforteller.*

- **D01_CH_FOOD**: I picked something up at Mesa Diner. → `D01_FOOD`
  Krav: `{"all": ["food_bought"], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **D01_CH_NOFOOD**: No, I haven't eaten yet. → `D01_NOFOOD`
  Krav: `{"all": [], "none": ["food_bought"]}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### D01_FOOD (handover / phone / live)

**Utløser:** Svar når spilleren selv har fortalt om maten.

**Regi:** Vennlig svar om mat, gyldig både hvis maten er spist og hvis den er tatt med. Dale ser ikke maten og hører ingen innspilt spillerlatter.

**Krav:** {"all": [], "none": []}

**DALE [D01_FOOD_001]**: Good. Better than trying to live on the coffee here.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D01_FOOD_002]**: I tried that once. Wouldn't recommend it.

*Nøkternt og konkret; ikke en skrekkforteller.*

- **D01_CH_FOOD_BRIEF**: What do I need to check? → `D01_BRIEF`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### D01_NOFOOD (handover / phone / live)

**Utløser:** Svar når spilleren oppgir at de ikke har spist. Et tidligere dinerbesøk uten matkjøp er mulig.

**Regi:** Vanlig kollegialt råd om pausemat. Ingen antakelse om hvilken vei spilleren kjørte.

**Krav:** {"all": [], "none": []}

**DALE [D01_NOFOOD_001]**: There are crackers in the bottom drawer. Still in the wrapper.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D01_NOFOOD_002]**: No promises about the coffee. It was here when I came in.

*Nøkternt og konkret; ikke en skrekkforteller.*

- **D01_CH_NOFOOD_BRIEF**: What do I need to check? → `D01_BRIEF`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### D01_BRIEF (handover / phone / live)

**Utløser:** Avslutning på telefonoverleveringen. Dokumentene registreres fra den fysiske mappen på spillerens arbeidsplass; ingen synlig Dale gir dem fra seg.

**Regi:** Sett fullføringsflagg først når siste replikk eller undertekst er ferdig. En avbrutt samtale kan fortsette. Ikke hopp til et senere kapittel.

**Krav:** {"all": [], "none": []}

**DALE [D01_BRIEF_001]**: Start with the log. I've marked what's done and what's left for you.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D01_BRIEF_002]**: Bank Three has two amber indicators. Amber is normal. Write it down if that changes.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D01_BRIEF_003]**: Your folder lists the local service records and the radio schedule. Get the dated copies. We need something to check our clocks against.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D01_BRIEF_004]**: Operations office is locked. The papers for tonight are already on your desk.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D01_BRIEF_005]**: Ward is on call for the morning run. She wants observations she can sign, not our best guess.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D01_BRIEF_006]**: You can reach me on the service line. If something doesn't fit the log, call before you throw anything away.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D01_BRIEF_007]**: All right. Have a good shift.

*Vennlig avslutning med naturlig engelsk prosodi, uten uhyggelig undertone.*


**Ved fullføring:** `{"set_flags": {"handover_done": true, "intro_completed": true}, "give_documents": ["EV_HANDOVER", "EV_SERVICE_LOG", "EV_RADIO_SCHEDULE"]}`

### D11_POWER_PHONE (early_shift / phone / live)

**Utløser:** Valgfri praktisk hjelp etter at den vanlige strømfeilen er observert, før T01. Spilleren oppgir feilen ved oppringingen.

**Regi:** Hjelpsom kollega. Bruk spillets fiktive betjeningsfront; ingen detaljerte elektrikerinstrukser.

**Krav:** {"all": ["handover_done"], "none": ["dale_power_call_done", "trafo_normal_checked"]}

**DALE [D11_POWER_PHONE_001]**: Dale. You have the service sheet?

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D11_POWER_PHONE_002]**: Then start with the feeder marked on that sheet. A receiver warning and a supply fault are two different things.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D11_POWER_PHONE_003]**: The junction station is down the short gravel turnoff. Leave the truck outside the service gate.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D11_POWER_PHONE_004]**: Use the service check on the cabinet. Note what the affected light does, not just what the indicator says.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D11_POWER_PHONE_005]**: There should be one clear result. If the result is not clear, leave it unresolved and call me.

*Nøkternt og konkret; ikke en skrekkforteller.*


**Ved fullføring:** `{"set_flags": {"dale_power_call_done": true}, "give_documents": []}`

### D12_POWER_WALKIE (early_shift / walkie / live)

**Utløser:** Spilleren kaller frivillig fra T01 etter å ha lest servicekortet, og oppgir sted og skilttekst.

**Regi:** Kort radiobånd med lett filtrering. Dale er på avstand og svarer bare på det spilleren har rapportert.

**Krav:** {"all": ["field_service_read"], "none": ["dale_walkie_help_done", "trafo_normal_checked"]}

**DALE [D12_POWER_WALKIE_001]**: I hear you. Check the cabinet label against the service sheet.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D12_POWER_WALKIE_002]**: If they match, run the check and watch the light at the far end.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D12_POWER_WALKIE_003]**: Write down the result and the time. Keep that for the next check.

*Nøkternt og konkret; ikke en skrekkforteller.*


**Ved fullføring:** `{"set_flags": {"dale_walkie_help_done": true}, "give_documents": []}`

### D21_SIGNAL_PHONE (signal / phone / live)

**Utløser:** Tilgjengelig når spilleren velger å rapportere den faktiske residualen og utskriften etter 00:13:47.

**Regi:** Den tause spillerens rapportvalg oppgir minus trettini på papirutskriften. Dale kan ikke se skjermen hjemmefra.

**Krav:** {"all": ["ctx_signal_present", "handover_done"], "none": ["dale_signal_call_done"]}

**DALE [D21_SIGNAL_PHONE_001]**: Dale. What did it actually print?

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D21_SIGNAL_PHONE_002]**: Minus thirty-nine. On the paper?

*Kort naturlig tenkepause, uten dramatisk gisp.*

**DALE [D21_SIGNAL_PHONE_003]**: Keep the original. Put the clock reading beside it before you do anything else.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D21_SIGNAL_PHONE_004]**: Do not correct the number just because it looks wrong.

*Nøkternt og konkret; ikke en skrekkforteller.*

- **D21_CH_BEFORE**: Have you seen a negative range before? → `D21_BEFORE`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **D21_CH_POWER**: Could this be the supply fault? → `D21_POWER`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **D21_CH_KEEP**: I will keep the printout. → `D21_END`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### D21_BEFORE (signal / phone / live)

**Utløser:** Svar om tidligere feil, basert på spørsmålet spilleren nettopp valgte.

**Regi:** Litt mer nølende enn under overleveringen. Dale har sett uforklarlige resultater, men gir ingen fasit.

**Krav:** {"all": [], "none": []}

**DALE [D21_BEFORE_001]**: I've seen numbers that shouldn't have come out of that solver.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D21_BEFORE_002]**: I haven't worked out why. I kept the records.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D21_BEFORE_003]**: Keep yours too. We can argue about the explanation later.

*Nøkternt og konkret; ikke en skrekkforteller.*

- **D21_CH_BEFORE_END**: I will compare the records. → `D21_END`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### D21_POWER (signal / phone / live)

**Utløser:** Svar som viser til den lokale kontrollprøven uten å løse sammenligningen automatisk.

**Regi:** Pek mot en kontroll spilleren kan gjøre selv. Ikke forutsett at den vanlige strømfeilen allerede er rettet.

**Krav:** {"all": [], "none": []}

**DALE [D21_POWER_001]**: Check your inspection card. Put the test time beside the new printout.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D21_POWER_002]**: If you've cleared the supply fault, compare that time with this one.

*Nøkternt og konkret; ikke en skrekkforteller.*

- **D21_CH_POWER_END**: I will check the times. → `D21_END`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### D21_END (signal / phone / live)

**Utløser:** Avslutning med retur til den aktive SARO-oppgaven.

**Regi:** Kort praktisk avslutning. Gi kontroll tilbake til undersøkelsen uten ny hendelse eller automatisk løsning.

**Krav:** {"all": [], "none": []}

**DALE [D21_END_001]**: All right. Put the paper somewhere safe.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D21_END_002]**: Call me if the next observation disagrees with the first one.

*Nøkternt og konkret; ikke en skrekkforteller.*


**Ved fullføring:** `{"set_flags": {"dale_signal_call_done": true}, "give_documents": []}`

### D31_OFFICE_PHONE (archive / phone / live)

**Utløser:** Valgfri oppringing fra arkivtelefonen etter at kontoret er åpnet. Spillerens kontaktvalg forteller Dale at reservenøkkelen og feltmappen er funnet.

**Regi:** Ingen fysisk Dale i døråpning, speil eller monitor. Han kjenner kontorbesøket fordi spilleren oppgir det ved oppringingen.

**Krav:** {"all": ["office_open", "office_evidence_taken"], "none": ["dale_office_call_done"]}

**DALE [D31_OFFICE_PHONE_001]**: You found a spare.

*Tilbakeholden, men menneskelig. Han vet dette fordi spilleren nettopp har fortalt at kontoret er åpnet.*

**DALE [D31_OFFICE_PHONE_002]**: I should have told you there was more than a service folder.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D31_OFFICE_PHONE_003]**: What are you looking at?

*Nøkternt og konkret; ikke en skrekkforteller.*

- **D31_CH_WARD**: Why did you keep this from Ward? → `D31_WARD`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **D31_CH_TAPE**: The field tape. What should I look at? → `D31_TAPE`
  Krav: `{"all": ["vhs_seen"], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **D31_CH_RECORD**: Did you change the old report? → `D31_RECORD`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **D31_CH_LEAVE**: I will take the reference sheet. → `D31_END`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### D31_WARD (archive / phone / live)

**Utløser:** Svar om hemmeligholdelsen uten å utpeke Ward som skurk.

**Regi:** Dale er forsiktig og litt beskjemmet. Han forklarer hvorfor han holdt usikre spor for seg selv.

**Krav:** {"all": [], "none": []}

**DALE [D31_WARD_001]**: Ward wants a result somebody else can repeat. She's right to.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D31_WARD_002]**: I had clippings, bad frames and people who remembered different nights.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D31_WARD_003]**: I kept the lot until I could put two observations on the same page.

*Nøkternt og konkret; ikke en skrekkforteller.*

- **D31_CH_WARD_END**: What can I actually check? → `D31_END`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### D31_TAPE (archive / phone / live)

**Utløser:** Telefonisk hjelp til vurdering av feltopptaket, enten filmen er sett eller bare omtalt i feltoversikten. Ingen Dale-stemme legges til selve VHS-filmen.

**Regi:** Bruk stolpen som stabil referanse og feltoversikten som veiviser. Ikke forutsett fullført VHS-avspilling eller et annet dokument.

**Krav:** {"all": [], "none": []}

**DALE [D31_TAPE_001]**: Watch the pole, not the patch.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D31_TAPE_002]**: The patch could be a recording fault. The pole gives you a fixed reference.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D31_TAPE_003]**: Use the field sheet. I marked the camera position.

*Nøkternt og konkret; ikke en skrekkforteller.*

- **D31_CH_TAPE_END**: I will compare it on site. → `D31_END`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### D31_RECORD (archive / phone / live)

**Utløser:** Svar på mistanken om den gamle rapporten. Dale avgrenser egen kunnskap; Nora og arkivundersøkelsen beholder sine roller.

**Regi:** Dale svarer bare for egne notater. Han vet ikke hvem som endret den gamle rapporten eller hva Nora vil fortelle.

**Krav:** {"all": [], "none": []}

**DALE [D31_RECORD_001]**: I haven't worked through those survey files.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D31_RECORD_002]**: The folder in my office is my own. I haven't changed an old report.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D31_RECORD_003]**: Keep the originals together. Check whose name is on the signed copy.

*Nøkternt og konkret; ikke en skrekkforteller.*

- **D31_CH_RECORD_END**: I'll check the signatures. → `D31_END`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### D31_END (archive / phone / live)

**Utløser:** Retur til kontor, arkiv eller feltoppgave uten å starte neste kapittel direkte.

**Regi:** Gi ett gjennomførbart neste steg. Vis ingen ny markør som avslører fenomenets årsak.

**Krav:** {"all": [], "none": []}

**DALE [D31_END_001]**: Start with one thing you can check. Take a photograph from the same place.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D31_END_002]**: And don't take my word for something just because I wrote it down.

*Nøkternt og konkret; ikke en skrekkforteller.*


**Ved fullføring:** `{"set_flags": {"dale_office_call_done": true}, "give_documents": []}`

### D41_FIELD_WALKIE (field / walkie / live)

**Utløser:** Valgfri tilbakemelding etter den faktisk gjennomførte T02-kontrollen. Spillerens kontaktvalg rapporterer kontrollresultatet før Dale svarer.

**Regi:** Kinski-profilen brukes bare i walkien. Dale bekrefter spillerens rapport språklig; han ser ikke stedet direkte.

**Krav:** {"all": ["trafo_anomaly_checked"], "none": ["dale_field_call_done"]}

**DALE [D41_FIELD_WALKIE_001]**: Say that again. The service result and the light disagree?

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D41_FIELD_WALKIE_002]**: You checked the ordinary supply, the backup and the indicator?

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D41_FIELD_WALKIE_003]**: Good. Write down every check you made. Keep that separate from what you think caused it.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D41_FIELD_WALKIE_004]**: Bring the record back to SARO. Ward will need to read it.

*Nøkternt og konkret; ikke en skrekkforteller.*


**Ved fullføring:** `{"set_flags": {"dale_field_call_done": true}, "give_documents": []}`

### D51_LAST_PHONE (all_night / phone / live)

**Utløser:** Valgfri kort samtale før avreise. Bruk en ledig telefonhandling uten å overta Wards obligatoriske dinertelefon.

**Regi:** Samtalen er valgfri. Den kreves ikke for P13 og skal ikke sende spilleren tilbake gjennom hele byen.

**Krav:** {"all": ["ctx_all_night_active", "handover_done"], "none": ["dale_last_call_done"]}

**DALE [D51_LAST_PHONE_001]**: Reyes? I'm here.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D51_LAST_PHONE_002]**: If you've got witness statements, keep their exact words with the record.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D51_LAST_PHONE_003]**: Give Ward what you've actually checked. Leave the rest marked uncertain.

*Nøkternt og konkret; ikke en skrekkforteller.*

**DALE [D51_LAST_PHONE_004]**: Call me when you get back in.

*Helt vanlig avslutning, uten kunnskap om den kommende slutten.*


**Ved fullføring:** `{"set_flags": {"dale_last_call_done": true}, "give_documents": []}`

### D61_FIXED_FRAGMENT (field / radio / fixed)

**Utløser:** Valgfritt sent radioavtrykk i den lokale mottakeren etter T02, bare når det er ro og riktig rekkefølge er oppfylt.

**Regi:** Gjenbruk nøyaktig første overleveringsopptak gjennom radio. Ingen svarvalg, samtale eller bekreftelse på at den levende Dale er på linjen.

**Krav:** {"all": ["trafo_anomaly_checked", "handover_done", "ctx_room_quiet"], "none": ["dale_fragment_heard"]}

**DALE [D61_FIXED_FRAGMENT_001]**: Reyes? Dale. The logbook should be in front of you.

*Nøyaktig samme opptak som første overleveringsreplikk. Ingen nye ord eller improvisasjon.*


**Ved fullføring:** `{"set_flags": {"dale_fragment_heard": true}, "give_documents": []}`

**DALE_UI_PHONE**: Call Dale

*Valg ved egnet landlinje; ikke erstatning for Nora eller Ward.*

**DALE_UI_WALKIE**: Contact Dale

*Walkie, bare i tilgjengelig sendefase.*

**DALE_UI_NO_ANSWER**: No answer. The record is still yours to check.

*Ingen tilfeldig samtale blokkerer etterforskningen.*

## office

### D02_LOCKED_OFFICE (archive / physical / live)

**Utløser:** Ved undersøkelse av eksisterende låst Operations office under hovedarkivreisen. Får ikke utløses under hyggelig kveldsoverlevering.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["ctx_archive_open"], "none": ["office_open", "office_door_inspected"]}

**DESCRIPTION [D02_LOCKED_OFFICE_001]**: OPERATIONS. The handle gives a little, then stops. A paper label beside the lock reads: SPARE IN SERVICE DRAWER.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_LOCKED_OFFICE_002]**: A coffee ring has spread across the word SPARE.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_door_inspected": true}, "give_documents": []}`

### D02_SERVICE_DRAWER (archive / physical / live)

**Utløser:** Spilleren åpner serviceskuffen ved arbeidsbenken etter å ha fått tilgang til arkivreisen. Adapteren knytter denne handlingen til en bestemt skuff, ikke et generelt søk.

**Regi:** Plukk opp reservenøkkelen med faktisk input. Egen nøkkel-ID for kontoret; Noras nøkkel, motelldører og øvrige nøkkelregler røres ikke.

**Krav:** {"all": ["ctx_archive_open"], "none": ["office_key_found"]}

**DESCRIPTION [D02_SERVICE_DRAWER_001]**: Fuses, a bulb in a cardboard sleeve, two pencils worn down to the metal. A small brass key is taped beneath the drawer label.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_SERVICE_DRAWER_002]**: The tag reads OPERATIONS / SPARE.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_key_found": true}, "give_documents": []}`

### D02_OPEN_OFFICE (archive / physical / live)

**Utløser:** Bruk Operations-reservenøkkelen på den eksisterende låste kontordøren. Gi fysisk døråpning før fullføring.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["ctx_archive_open", "office_key_found"], "none": ["office_open"]}

**DESCRIPTION [D02_OPEN_OFFICE_001]**: The spare turns once. The latch catches on the frame until you lift the handle.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_OPEN_OFFICE_002]**: An ordinary office: a desk lamp, a coat over the chair, invoices clipped to a board. There is a clean patch on the desk where somebody usually puts a mug.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_open": true}, "give_documents": []}`

### D02_DESK_FIRST_LOOK (archive / physical / live)

**Utløser:** Første nære undersøkelse av skrivebordet etter at kontoret er åpnet.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_desk_inspected"]}

**DESCRIPTION [D02_DESK_FIRST_LOOK_001]**: A lunch receipt holds down a stack of maintenance forms. Someone has written MILK on the back.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_DESK_FIRST_LOOK_002]**: Under the invoices is a folder marked FIELD CHECKS. The label is newer than the folder. A pencilled list names a tape, a service-road photograph and two newspaper cuttings.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_desk_inspected": true}, "give_documents": ["EV_HANDOVER", "EV_SERVICE_LOG", "EV_RADIO_SCHEDULE", "EV_DALE_INDEX"]}`

### D02_READ_FIELD_INDEX (archive / physical / live)

**Utløser:** Spilleren åpner feltoversikten på skrivebordet. Dokumentet vises i eksisterende dokumentgrensesnitt.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_index_read"]}

**DESCRIPTION [D02_READ_FIELD_INDEX_001]**: The first page lists what each item can establish. Most entries end with a question or a correction.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_index_read": true}, "give_documents": ["EV_DALE_INDEX"]}`

### D02_READ_CLIPPINGS (archive / physical / live)

**Utløser:** Undersøk avisutklippene i FIELD CHECKS-mappen. Ikke lås videre hovedreise til at alle fordypningsdokumenter er lest.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_clippings_read"]}

**DESCRIPTION [D02_READ_CLIPPINGS_001]**: Two local cuttings, neither about a crash. A third rectangle on the page is empty.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_READ_CLIPPINGS_002]**: In the margin: GET THE ORIGINAL BEFORE USING THE DATE.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_clippings_read": true}, "give_documents": ["EV_DALE_CLIPPINGS"]}`

### D02_READ_DALE_NOTE (archive / physical / live)

**Utløser:** Spilleren undersøker den brettede arbeidsmerknaden under feltoversikten.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_note_read"]}

**DESCRIPTION [D02_READ_DALE_NOTE_001]**: The note starts as a list of things to check. Halfway down, the handwriting gets smaller.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_note_read": true}, "give_documents": ["EV_DALE_NOTE"]}`

### D02_SAVE_CONTACT (archive / physical / live)

**Utløser:** Spilleren undersøker kontaktkortet ved telefonen eller velger å beholde en kopi. Bare skriv av faktiske kontaktinstrukser; ingen automatisk oppringing.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_contact_read"]}

**DESCRIPTION [D02_SAVE_CONTACT_001]**: A contact card has been tucked under the telephone. There are no names on the back. Just the word FIELD, underlined twice.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_contact_read": true, "vhs_contact_saved": true}, "give_documents": ["EV_DALE_CONTACT"]}`

### D02_START_FIELD_TAPE (archive / physical / live)

**Utløser:** Spilleren trykker PLAY på VHS-spilleren i det åpne kontoret. Start VHS_DALE_FIELD_01 som faktisk bevegelig film med lyd. Gjentatt avspilling tilbys av spilleren selv, ikke av en ny engangsscene.

**Regi:** Start bare etter fysisk PLAY. Ingen Dale-stemme, Kinski-forteller, fysisk Dale eller tale fra et avtrykk. Avbrutt avspilling setter ikke vhs_seen.

**Krav:** {"all": ["office_open", "ctx_power_available"], "none": ["office_tape_started"]}

**DESCRIPTION [D02_START_FIELD_TAPE_001]**: A handwritten sleeve: SERVICE ROAD / FIELD 01. The tape counter has been copied onto the label.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_START_FIELD_TAPE_002]**: There is no narration listed. The note on the sleeve says: WATCH THE POST, THEN THE LINE.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_tape_started": true}, "give_documents": []}`

### D02_FIELD_TAPE_COMPLETE (archive / physical / live)

**Utløser:** Avspillingsadapteren utløser dette først når hele VHS_DALE_FIELD_01 faktisk er ferdig. Ikke bruk scenevisning eller PLAY som bevis på fullført film.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open", "office_tape_started"], "none": ["vhs_seen"]}

**DESCRIPTION [D02_FIELD_TAPE_COMPLETE_001]**: The tape runs on past the movement. Wind, the fence, the same stretch of wire. Then the counter stops.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_FIELD_TAPE_COMPLETE_002]**: The final card gives a place to stand: the low culvert outside the service gate, beside the post with two white bands.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"vhs_seen": true, "office_field_lead_filed": true}, "give_documents": ["EV_DALE_INDEX", "EV_DALE_NOTE"]}`

### D02_TAKE_FIELD_FOLDER (archive / physical / live)

**Utløser:** Spilleren tar med kopien av feltoversikten. Dette er den korte hovedruten gjennom kontoret: ett konkret feltspor, uten krav om alle avisutklipp, kontaktkort eller fullført VHS.

**Regi:** Gi konkret spor til T02 og gå tilbake til gjeldende arkiv-/feltoppdrag. Å ta mappen starter ikke STATION 01, Room 6, P13 eller finalen. Nora beholder sin nøkkel og historiefunksjon.

**Krav:** {"all": ["office_open"], "none": ["office_evidence_taken"]}

**DESCRIPTION [D02_TAKE_FIELD_FOLDER_001]**: The field sheet locates the camera outside the substation fence, beside a low culvert and a post painted with two white bands.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_TAKE_FIELD_FOLDER_002]**: It asks for a comparison from the same position, with the nearest pole and its two outer conductors in frame. The tape may help explain what to look for.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_evidence_taken": true, "office_field_lead_filed": true}, "give_documents": ["EV_DALE_INDEX", "EV_DALE_NOTE"]}`

### D02_FIELD_LOCKED_OFFICE (field / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["ctx_archive_open"], "none": ["office_open", "office_door_inspected"]}

**DESCRIPTION [D02_FIELD_LOCKED_OFFICE_001]**: OPERATIONS. The handle gives a little, then stops. A paper label beside the lock reads: SPARE IN SERVICE DRAWER.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_FIELD_LOCKED_OFFICE_002]**: A coffee ring has spread across the word SPARE.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_door_inspected": true}, "give_documents": []}`

### D02_FIELD_SERVICE_DRAWER (field / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Plukk opp reservenøkkelen med faktisk input. Egen nøkkel-ID for kontoret; Noras nøkkel, motelldører og øvrige nøkkelregler røres ikke.

**Krav:** {"all": ["ctx_archive_open"], "none": ["office_key_found"]}

**DESCRIPTION [D02_FIELD_SERVICE_DRAWER_001]**: Fuses, a bulb in a cardboard sleeve, two pencils worn down to the metal. A small brass key is taped beneath the drawer label.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_FIELD_SERVICE_DRAWER_002]**: The tag reads OPERATIONS / SPARE.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_key_found": true}, "give_documents": []}`

### D02_FIELD_OPEN_OFFICE (field / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["ctx_archive_open", "office_key_found"], "none": ["office_open"]}

**DESCRIPTION [D02_FIELD_OPEN_OFFICE_001]**: The spare turns once. The latch catches on the frame until you lift the handle.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_FIELD_OPEN_OFFICE_002]**: An ordinary office: a desk lamp, a coat over the chair, invoices clipped to a board. There is a clean patch on the desk where somebody usually puts a mug.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_open": true}, "give_documents": []}`

### D02_FIELD_DESK_FIRST_LOOK (field / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_desk_inspected"]}

**DESCRIPTION [D02_FIELD_DESK_FIRST_LOOK_001]**: A lunch receipt holds down a stack of maintenance forms. Someone has written MILK on the back.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_FIELD_DESK_FIRST_LOOK_002]**: Under the invoices is a folder marked FIELD CHECKS. The label is newer than the folder. A pencilled list names a tape, a service-road photograph and two newspaper cuttings.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_desk_inspected": true}, "give_documents": ["EV_HANDOVER", "EV_SERVICE_LOG", "EV_RADIO_SCHEDULE", "EV_DALE_INDEX"]}`

### D02_FIELD_READ_FIELD_INDEX (field / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_index_read"]}

**DESCRIPTION [D02_FIELD_READ_FIELD_INDEX_001]**: The first page lists what each item can establish. Most entries end with a question or a correction.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_index_read": true}, "give_documents": ["EV_DALE_INDEX"]}`

### D02_FIELD_READ_CLIPPINGS (field / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_clippings_read"]}

**DESCRIPTION [D02_FIELD_READ_CLIPPINGS_001]**: Two local cuttings, neither about a crash. A third rectangle on the page is empty.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_FIELD_READ_CLIPPINGS_002]**: In the margin: GET THE ORIGINAL BEFORE USING THE DATE.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_clippings_read": true}, "give_documents": ["EV_DALE_CLIPPINGS"]}`

### D02_FIELD_READ_DALE_NOTE (field / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_note_read"]}

**DESCRIPTION [D02_FIELD_READ_DALE_NOTE_001]**: The note starts as a list of things to check. Halfway down, the handwriting gets smaller.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_note_read": true}, "give_documents": ["EV_DALE_NOTE"]}`

### D02_FIELD_SAVE_CONTACT (field / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_contact_read"]}

**DESCRIPTION [D02_FIELD_SAVE_CONTACT_001]**: A contact card has been tucked under the telephone. There are no names on the back. Just the word FIELD, underlined twice.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_contact_read": true, "vhs_contact_saved": true}, "give_documents": ["EV_DALE_CONTACT"]}`

### D02_FIELD_TAKE_FIELD_FOLDER (field / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Gi konkret spor til T02 og gå tilbake til gjeldende arkiv-/feltoppdrag. Å ta mappen starter ikke STATION 01, Room 6, P13 eller finalen. Nora beholder sin nøkkel og historiefunksjon.

**Krav:** {"all": ["office_open"], "none": ["office_evidence_taken"]}

**DESCRIPTION [D02_FIELD_TAKE_FIELD_FOLDER_001]**: The field sheet locates the camera outside the substation fence, beside a low culvert and a post painted with two white bands.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_FIELD_TAKE_FIELD_FOLDER_002]**: It asks for a comparison from the same position, with the nearest pole and its two outer conductors in frame. The tape may help explain what to look for.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_evidence_taken": true, "office_field_lead_filed": true}, "give_documents": ["EV_DALE_INDEX", "EV_DALE_NOTE"]}`

### D02_NIGHT_LOCKED_OFFICE (all_night / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["ctx_archive_open"], "none": ["office_open", "office_door_inspected"]}

**DESCRIPTION [D02_NIGHT_LOCKED_OFFICE_001]**: OPERATIONS. The handle gives a little, then stops. A paper label beside the lock reads: SPARE IN SERVICE DRAWER.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_NIGHT_LOCKED_OFFICE_002]**: A coffee ring has spread across the word SPARE.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_door_inspected": true}, "give_documents": []}`

### D02_NIGHT_SERVICE_DRAWER (all_night / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Plukk opp reservenøkkelen med faktisk input. Egen nøkkel-ID for kontoret; Noras nøkkel, motelldører og øvrige nøkkelregler røres ikke.

**Krav:** {"all": ["ctx_archive_open"], "none": ["office_key_found"]}

**DESCRIPTION [D02_NIGHT_SERVICE_DRAWER_001]**: Fuses, a bulb in a cardboard sleeve, two pencils worn down to the metal. A small brass key is taped beneath the drawer label.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_NIGHT_SERVICE_DRAWER_002]**: The tag reads OPERATIONS / SPARE.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_key_found": true}, "give_documents": []}`

### D02_NIGHT_OPEN_OFFICE (all_night / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["ctx_archive_open", "office_key_found"], "none": ["office_open"]}

**DESCRIPTION [D02_NIGHT_OPEN_OFFICE_001]**: The spare turns once. The latch catches on the frame until you lift the handle.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_NIGHT_OPEN_OFFICE_002]**: An ordinary office: a desk lamp, a coat over the chair, invoices clipped to a board. There is a clean patch on the desk where somebody usually puts a mug.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_open": true}, "give_documents": []}`

### D02_NIGHT_DESK_FIRST_LOOK (all_night / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_desk_inspected"]}

**DESCRIPTION [D02_NIGHT_DESK_FIRST_LOOK_001]**: A lunch receipt holds down a stack of maintenance forms. Someone has written MILK on the back.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_NIGHT_DESK_FIRST_LOOK_002]**: Under the invoices is a folder marked FIELD CHECKS. The label is newer than the folder. A pencilled list names a tape, a service-road photograph and two newspaper cuttings.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_desk_inspected": true}, "give_documents": ["EV_HANDOVER", "EV_SERVICE_LOG", "EV_RADIO_SCHEDULE", "EV_DALE_INDEX"]}`

### D02_NIGHT_READ_FIELD_INDEX (all_night / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_index_read"]}

**DESCRIPTION [D02_NIGHT_READ_FIELD_INDEX_001]**: The first page lists what each item can establish. Most entries end with a question or a correction.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_index_read": true}, "give_documents": ["EV_DALE_INDEX"]}`

### D02_NIGHT_READ_CLIPPINGS (all_night / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_clippings_read"]}

**DESCRIPTION [D02_NIGHT_READ_CLIPPINGS_001]**: Two local cuttings, neither about a crash. A third rectangle on the page is empty.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_NIGHT_READ_CLIPPINGS_002]**: In the margin: GET THE ORIGINAL BEFORE USING THE DATE.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_clippings_read": true}, "give_documents": ["EV_DALE_CLIPPINGS"]}`

### D02_NIGHT_READ_DALE_NOTE (all_night / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_note_read"]}

**DESCRIPTION [D02_NIGHT_READ_DALE_NOTE_001]**: The note starts as a list of things to check. Halfway down, the handwriting gets smaller.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_note_read": true}, "give_documents": ["EV_DALE_NOTE"]}`

### D02_NIGHT_SAVE_CONTACT (all_night / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Observasjonstekst uten innspilt forteller. Avsluttes tilbake til samme fysiske oppdrag; intet kapittelhopp.

**Krav:** {"all": ["office_open"], "none": ["office_contact_read"]}

**DESCRIPTION [D02_NIGHT_SAVE_CONTACT_001]**: A contact card has been tucked under the telephone. There are no names on the back. Just the word FIELD, underlined twice.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_contact_read": true, "vhs_contact_saved": true}, "give_documents": ["EV_DALE_CONTACT"]}`

### D02_NIGHT_TAKE_FIELD_FOLDER (all_night / physical / live)

**Utløser:** Valgfri senere fysisk kontorhandling; samme faktisk utførte handling og papir, uten faserewind.

**Regi:** Gi konkret spor til T02 og gå tilbake til gjeldende arkiv-/feltoppdrag. Å ta mappen starter ikke STATION 01, Room 6, P13 eller finalen. Nora beholder sin nøkkel og historiefunksjon.

**Krav:** {"all": ["office_open"], "none": ["office_evidence_taken"]}

**DESCRIPTION [D02_NIGHT_TAKE_FIELD_FOLDER_001]**: The field sheet locates the camera outside the substation fence, beside a low culvert and a post painted with two white bands.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*

**DESCRIPTION [D02_NIGHT_TAKE_FIELD_FOLDER_002]**: It asks for a comparison from the same position, with the nearest pole and its two outer conductors in frame. The tape may help explain what to look for.

*Vises som kort observasjonstekst. Ingen stemme eller spillerreplikk.*


**Ved fullføring:** `{"set_flags": {"office_evidence_taken": true, "office_field_lead_filed": true}, "give_documents": ["EV_DALE_INDEX", "EV_DALE_NOTE"]}`

### EV_HANDOVER: Evening handover / Sunday, April 13

typed / SARO / OPERATIONS / 13 APR 1986

18:00 HANDOVER
Evening operator: Reyes. Duty officer: Ward. Field maintenance contact: Dale, by telephone or service radio.

Carry out the ordinary room checks before starting the survey. There are 27 antennas on the installation list. The azimuth reference card remains 026. Record what the instruments say; do not adjust a reading simply to make it agree with the card.

The local service log and radio transmission schedule are in this handover folder. The day operator collected the copies. Keep them in the shift file. If an entry needs checking later, the service desk and station window in Caliche can help; there is no need to go down just to collect the same papers.

If the feeder complaint repeats, take the service-road route to the substation and use the marked inspection position. Follow the local isolation card. Do not enter the switching yard or work on exposed conductors.

Operations spare: service drawer at the workbench. Please put it back when you have finished. The office latch needs lifting. Milk is in the small refrigerator. Leave the receipt with petty cash.

**Produksjon:** Ordinær overlevering ved skiftstart 18:00, lenge før ny residual. D01_BRIEF gir EV_HANDOVER, EV_SERVICE_LOG og EV_RADIO_SCHEDULE fra mappen på SARO. Byvinduene er valgfrie oppfølginger og alternative kilder, ikke pålagt henteoppdrag. Dale er bare tilgjengelig via telefon/radio/walkie; Ward er egen vakthavende. Utstyrs- og K3-regler følger eksisterende spillkontrakt.

### EV_SERVICE_LOG: Caliche service log / released copy

typed / LOCAL SERVICE DESK / 13 APR 1986

SERVICE COPY
Prepared Sunday, April 13, 1986. Collected for SARO by the day operator; enclosed with the evening handover. Entries below are civil time as entered by the attendant; they are not precision timing records.

April 12, 19:06. Complaint: exterior lamp on service-road inspection circuit dimming intermittently. Other premises reported normal supply. No town-wide interruption entered.

April 13, 16:40. Maintenance inspection requested. Complaint remains open until the local test is completed and the result entered on the inspection card.

Supply labels and the inspection-circuit identifier must be read at the marked service panel. A lamp lighting does not by itself prove which supply is carrying it. Compare the panel indication with the permitted local test and the actual lamp.

No entry in this book means no report was entered here. It does not establish that every circuit remained live. Retain this copy if a later occurrence needs to be compared with the service record.

RELEASED COPY. Keep the original at the service desk. Later entries must be dated separately; do not add them to this sheet.

**Produksjon:** Ny ordinær kveldsreferanse i Dales overleveringsmappe på SARO. Dagens dagoperatør har hentet kopien; spilleren hentet den ikke på matstoppet. Tidligere lokal feil kan prøves i T01 og sammenlignes senere, men verken den feilen eller manglende nye journaloppføringer beviser årsak til residualen eller forsyningstilstanden i hele byen 00:13. Bruk egen inspeksjonskontroll for T01/T02; ingen reelle høyspenningsinstrukser og ingen kobling fra SAROs K3 til bynettet.

### EV_RADIO_SCHEDULE: 1420 AM / transmission schedule

typed / PUBLIC COPY / SUNDAY 13 APRIL 1986

1420 AM / LOCAL TRANSMISSION SCHEDULE
Sunday, April 13 into Monday, April 14. Station clock. Programs may be interrupted for local bulletins.

18:00–20:00. Evening records, weather and local notices. Evening staff on duty. Public copies and messages at the station window.

20:00–00:00. Music service. Recorded segments and local continuity. The person at the window may be covering other duties; leave a message if it is unattended.

00:00–04:00 MONDAY. THE NIGHT LINE / EARL. Telephone calls and local reports. An open microphone or ON AIR lamp does not establish that any particular caller was connected.

04:00. Night Line ends. Transmitter service and the scheduled morning program follow. An empty studio after four is normal; consult the actual program log for interruptions or changes.

This is a schedule, not proof of what was transmitted. For a disputed item, keep the date and approximate time and request the corresponding log entry or preserved program segment.

The station is an AM broadcast service. This sheet is not a tuning instruction for observatory equipment.

**Produksjon:** Fysisk kveldskopi som dagoperatøren allerede har hentet, vedlagt Dales overleveringsmappe på SARO. Stasjonsvinduet er en valgfri senere kilde til supplerende materiale; ingen obligatorisk hentetur. Earl arbeider bare 00–04; kveldens innhold og bemanning er egne roller. Adapteren må synkronisere vindu, ON AIR og faktisk lyd. Sendeskjemaet dokumenterer planlagt normaldrift, ikke hva som faktisk ble sendt rundt residualen.

### EV_DALE_INDEX: Field checks / working index

hand / DALE / WORKING COPY / 13 APR

FIELD CHECKS
Not a report. Keep this with the maintenance forms. Some of these are ordinary faults. Some may be bad recordings.

1. SERVICE ROAD / FIELD 01. VHS cassette in the desk unit. Environment sound only. The middle section contains the movement I could not account for. The recording date on the camera was not set correctly. Do not use it to establish a date or an exact time.

2. CAMERA POSITION. Outside the substation fence. Low concrete culvert beside the service gate. Stand on the road side of the post with two white bands. Face the nearest pole so the two outer conductors and the top of the pole remain in frame. The fence corner should sit at the lower left.

3. COMPARISON. First inspect the ordinary supply indication and the permitted lamp test. Return to the same position. Compare the nearest pole, the two outer conductors and the moving patch separately. Do not use camera shake as movement in the scene.

4. LIMIT. The cassette cannot establish what supplied the lamp or whether the shape was in the air. It does establish where the camera stood. A fresh observation from that position would be more useful than another opinion about the tape.

5. BACKGROUND. Two newspaper cuttings and a service note. Dates uncertain on one clipping. These are leads, not a map to a destination. The survey-station papers belong to their own file.

Leave the originals here. Take this copy if you go out. Tell Ward where you are going.

**Produksjon:** Kort hovedspor som kan tas uten full VHS eller øvrig fordypning. T02 må bygge nøyaktig gjenkjennelig kulvert, to hvite bånd, stolpe, lederpar og gjerdehjørne som filmen viser. Gjenbruk ikke STATION 01-identitet eller C-kryss. Klippets dato er usikker med vilje og kan ikke brukes som fullført tidsbevis.

### EV_DALE_CLIPPINGS: Local cuttings / dates to check

hand / FIELD CHECKS / BACKGROUND

CUTTING A
PECOS VALLEY SENTINEL / OCTOBER 22, 1985
DRIVERS REPORT STALLED ENGINES ON COUNTY ROAD

Two motorists reported brief electrical failures on a county road east of the highway over the weekend. Both vehicles restarted. A county spokesman said no general supply interruption had been reported. No cause was entered in the notice.

Dale, pencil: Cars are not supplied by the town feeder. Same evening is not the same cause. Ask for a location before putting this beside the service complaint.

CUTTING B
HEADLINE SAVED; DATE MISSING
RANCHER FINDS FENCE LAMP BURNING AFTER LINE FAULT

The lamp had an auxiliary supply. The owner said the indicator had shown the main line was disconnected. The remainder of the item is missing.

Dale, pencil: Ordinary explanation. Keep it. A lit lamp is not evidence that a disconnected line is live. Need the whole page and the maintenance entry.

In the empty space below: I have two accounts of a movement near the service road. Neither witness agrees on the date. Do not make a pattern out of that yet.

**Produksjon:** Nye fiktive diegetiske utklipp skrevet for denne pakken, ikke ekte avisgjengivelse. Ingen koordinater eller åttemilsreferanse som løser P13. Dales merknader avgrenser kunnskap og tilbyr vanlig forklaring; ingen bredere 1947-fotografi eller sluttspoiler.

### EV_DALE_CONTACT: Field contact / keep with the radio

typed / OPERATIONS / CONTACT CARD

FIELD MAINTENANCE CONTACT: DALE
Use the SARO service telephone or the marked service-radio channel. Ask the operator for Dale / field maintenance. If the line is busy, leave the place you are calling from and the circuit label you copied.

Keep the installation name, your position and the observation separate. Say what changed, what stayed still and what control you used. An uncertain reading is useful if you say it is uncertain.

If there is an immediate service problem, notify Ward, the duty officer, first. This card does not authorize switching or entry to a locked electrical yard.

Handwritten addition: I may be on another line. Keep your note. Do not wait beside the gate just to get me on the radio. — D.

**Produksjon:** Kortet gir kontaktinstruks, ikke ny oppdiktet tallfrekvens eller et telefonnummer som ikke finnes i spillet. Integrasjon må gi egne konkrete oppringings-/walkiehandlinger i egnet modul. Ingen fysisk Dale. Kan beholdes uten å utløse samtale eller blokkere hovedreise.

### EV_DALE_NOTE: A note beneath the inspection sheets

hand / DALE / UNSENT WORKING NOTE

I started this folder because the service complaint kept coming back under different names. Weak lamp. Bad connection. Somebody seeing a shape out by the wire. It is tempting to put them all in the same box.

The first two have ordinary tests. The third may be a camera fault. I have watched that tape enough times to start anticipating the movement. That is not the same as understanding it.

I marked the camera position because it is the one thing I could check afterwards. The culvert, the two white bands and the nearest pole are still there. Stand there before making a copy of my conclusion. I have not made one.

On the recording the lamp is visible. You cannot tell from the picture which supply feeds it. Check that before treating the lamp as a contradiction. Keep the actual supply test and the movement on the tape as separate observations.

I have put copies of the local notices behind this page. There may be something worth comparing in the old survey papers, but I have not established a connection. Similar words in two accounts are a reason to check the originals, not permission to fill the gap.

If you take the sheet, leave me a note. If I am not on the telephone, try the service radio. Do not tell the others I have found something. I have found somewhere to look.

**Produksjon:** Gradvis fordypning etter et menneskelig, ordinært kontor. Dale har jobbet med spor og kjenner ståstedet, men vet ikke årsak, C, Nora eller finalen. Ingen talt opplesning med Kinski-profil; dokumenttekst er skriftlig.

### VHS_DALE_FIELD_01: Service Road / Field 01

Planlagt varighet: 72 s. Status: ikke produsert.

Produksjonsmanus for ny faktisk 72 sekunders bevegelig VHS-film med miljølyd. Ingen fil er generert. Ingen talt Dale, fysisk Dale, spillerstemme eller Kinski-forteller; Dale finnes bare som håndskrevne kort. Kameraets feil dato brennes ikke inn som pålitelig tid. Filmstedet modelleres sammen med T02, og samme ståsted må kunne finnes i gameplay. Avviket er bevegelig og uklart, men adskilles fra kamera-/trackingbevegelse med stabile referanser. Ikke gjenbruk film som et statisk bevisfoto. Uavklart årsak; ingen synlig full alien eller forklaring av C/finalen. Første avspilling kan pauseres/avbrytes og spilles igjen; bare hele avspilling gir vhs_seen. Tekstkort er engelske og lesbare også med undertekster.

**0–7 s (planlagt)**

Bilde: Bevegelig opptak starter nede ved veikanten; kameraet løftes fra grusen forbi lav kulvert og gjerdet. Et kort skriftlig pålegg i håndskrift er synlig over den levende filmen. Ingen person eller hånd må identifiseres som Dale.

Lyd: Båndstart, svak motor i det fjerne, vind mot mikrofonen og kontinuerlig elektrisk romling. Ingen tale.

Undertekst: SERVICE ROAD / FIELD 01
Camera date incorrect. Use position, not timestamp.

**7–17 s (planlagt)**

Bilde: Rolig panorering viser serviceporten og stolpen med nøyaktig to hvite bånd. Kameraet finner inspeksjonsståstedet og rammer inn nærmeste stolpe, to ledere og gjerdehjørnet nederst til venstre. En avgrenset inspeksjonslampe lyser.

Lyd: Vind, små metalliske klirr i gjerdet, jevn svak romling. Lydperspektivet følger panoramaet. Ingen spenningsdramaturgi eller musikk.

Undertekst: Road side of the two-band post.
Culvert beside the service gate.

**17–27 s (planlagt)**

Bilde: Kameraet holdes i samme utsnitt med liten naturlig håndholdt drift. Stolpen og begge lederne er hele i bildet. En liten støvstripe går langs bakken i én retning. Pålegget kommer fra et skriftlig notat, uten opplesning.

Lyd: Vind i samme retning og et lavt, uregelmessig gjerdetikk. Lampen gir ingen egen overdreven effektlyd.

Undertekst: Nearest pole and both outer wires in frame.
The lamp alone proves nothing about its supply.

**27–39 s (planlagt)**

Bilde: En smal grålig forskyvning oppstår ved den øverste lederen. Den glir sakte sideveis mot støvets retning, krysser det tomme mellomrommet under lederen og stanser et øyeblikk ved stolpen. Nærmeste stolpe, gjerde og ledere forblir stabile. Det kan ligne en mørk fold i luften eller en lokal feil i båndet, uten gjenkjennelig kropp. Ingen skjermomfattende glitch skjuler bevegelsen.

Lyd: Samme miljølyd fortsetter. En kort lav demping av rumlingen, hørbar men ikke et høyt støt. Ingen stemme eller pust fra et vesen.

Undertekst: (ingen)

**39–49 s (planlagt)**

Bilde: Kameraet tar et lite skritt sidelengs. Den grå forskyvningen glir bak stolpens silhuett og et lite stykke ned, mens gjerdehjørnet endrer plass ved normal parallakse. Den forsvinner gradvis før bevegelsen i kameraet stopper. Hold tydelig lesbar bevegelse og dekning; ingen teleportert modell eller statisk pålimt klatt.

Lyd: Ett vanlig grusskritt, klær mot mikrofonen, vind og gjerdet. Romlingen går tilbake til tidligere nivå uten dramatisk stinger.

Undertekst: (ingen)

**49–61 s (planlagt)**

Bilde: Kameraet går tilbake til merket ved kulverten og holder det opprinnelige utsnittet. Ledningene beveger seg svakt normalt i vinden. Lampen lyser fremdeles. Ingen ny form kommer; båndet fortsetter som en faktisk sammenhengende registrering.

Lyd: Grus, vind og vanlig elektrisk romling. En fugl langt unna. Ingen tale.

Undertekst: Did the patch move with the camera?
Compare the pole, not the tracking line.

**61–72 s (planlagt)**

Bilde: Sakte tilt ned mot kulverten og de to hvite båndene som gjør plassen gjenfinnbar. Bildet holder seg levende med støv og svak kameradrift under siste håndskrevne pålegg. Film slutter ved72, ingen ytterligere skjult hendelse.

Lyd: Vind og fjernt motorbrum; vanlig båndstopp ved slutten. Ingen Dale-opplesning.

Undertekst: Return to this position.
Check the ordinary supply first. Then make a fresh observation.
— D.

**OFFICE_KEY_ITEM**: Operations spare key

*Eget nøkkelnavn; aldri Noras nøkkel.*

**OFFICE_DOOR_LOCKED**: Operations office — locked

*Dørens fokusbeskrivelse før reservenøkkel.*

**OFFICE_DOOR_UNLOCK**: Use the Operations spare

*Handlingen på eksisterende låste kontordør.*

**OFFICE_SERVICE_DRAWER**: Service drawer

*Gjenfinnbar fysisk skuff ved arbeidsbenken.*

**OFFICE_FIELD_FOLDER**: Field checks

*Den ordinære mappen under arbeidsbilagene.*

**OFFICE_READ_INDEX**: Read the working index

*Åpner EV_DALE_INDEX.*

**OFFICE_READ_CLIPPINGS**: Examine the local cuttings

*Valgfri fordypning.*

**OFFICE_READ_NOTE**: Read the folded note

*Åpner EV_DALE_NOTE.*

**OFFICE_KEEP_CONTACT**: Copy the field contact

*Kopier kontaktkort uten automatisk samtale.*

**OFFICE_TAKE_COPY**: Take the field sheet

*Kort hovedrute med konkret spor; øvrig research valgfritt.*

**OFFICE_PLAY_TAPE**: Play Service Road / Field 01

*Faktisk film og miljølyd, ingen forteller.*

**OFFICE_REPLAY_TAPE**: Replay the field tape

*Kan gjentas uten å duplisere funn.*

**OFFICE_TAPE_NO_POWER**: The VHS deck has no power. The field sheet is beside it.

*Vis bare ved faktisk strømmangel; nødvendig feltspor ligger på papiret.*

**OFFICE_TAPE_UNAVAILABLE**: The tape will not play. The field sheet still gives the camera position.

*Feilvisning hvis filmfil mangler/feiler; ikke simuler film med stillbilde eller gi vhs_seen.*

**OFFICE_FIELD_LEAD**: Service road: compare the view from the culvert beside the two-band post.

*Konkret feltspor til T02, separat fra STATION 01 og P13.*

**OFFICE_INVESTIGATION_NOTE**: The recording gives a position to check. It does not identify what moved.

*Saksnotat ved avsluttet tape, uten automatisk forklaring.*

**OFFICE_CONTACT_ACTION**: Contact Dale on the service line

*Kontaktknapp overlates til egnet telefon-/walkiemodul; ingen fysisk Dale.*

**OFFICE_RETURN_TASK**: Keep the survey papers separate. Continue the archive check.

*Retur til gjeldende hovedarkivreise; ingen automatisk ny fase.*

## field

### N01_ROUTINE (early_shift / ui / live)

**Utløser:** Første rutinekontroll i mappen på SARO etter telefonoverlevering.

**Regi:** Vis som kort undersøkelsestekst uten fortellerstemme. Gå tilbake til samme fysiske oppgave etterpå.

**Krav:** {"all": ["handover_done"], "none": ["routine_checked"]}

**DESCRIPTION [N01_ROUTINE_001]**: The service copy records an open inspection complaint. The radio sheet is a programme schedule, not a recording of the broadcast.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [N01_ROUTINE_002]**: Both copies are dated. Neither tells you what will happen later tonight.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {"routine_checked": true}, "give_documents": ["EV_SERVICE_LOG", "EV_RADIO_SCHEDULE"]}`

### N01_COMPARE (signal / ui / live)

**Utløser:** Spilleren legger egne dokumenter ved siden av den faktiske utskriften. Ikke automatisk ved å nærme seg bordet.

**Regi:** Vis som kort undersøkelsestekst uten fortellerstemme. Gå tilbake til samme fysiske oppgave etterpå.

**Krav:** {"all": ["routine_checked", "ctx_signal_present"], "none": ["record_compared"]}

**DESCRIPTION [N01_COMPARE_001]**: The residual is on the new printout. The service copy predates it.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [N01_COMPARE_002]**: The old complaint and this printout belong on the same timeline. That does not make them the same event.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

- **N01_CH_COMPARE_EARLIER**: The earlier complaint does not establish the cause of this printout. → `N01_COMPARE_CAUTION`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **N01_CH_COMPARE_CAUSE**: The earlier complaint proves the supply caused the residual. → `N01_COMPARE_WRONG`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **N01_CH_COMPARE_UNCERTAIN**: There is not enough evidence to connect them. → `N01_COMPARE_CAUTION`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### N01_COMPARE_WRONG (signal / ui / live)

**Utløser:** Konkret tilbakemelding uten straff. Behold dokumenter og tillat ny vurdering.

**Regi:** Vis som kort undersøkelsestekst uten fortellerstemme. Gå tilbake til samme fysiske oppgave etterpå.

**Krav:** {"all": [], "none": []}

**DESCRIPTION [N01_COMPARE_WRONG_001]**: The copy records a complaint and an inspection request. It does not record the supply at the moment of the residual.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [N01_COMPARE_WRONG_002]**: Use the actual inspection result and the later observation. The dates alone cannot settle the cause.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

- **N01_CH_REVISE**: Record the comparison as unresolved. → `N01_COMPARE_CAUTION`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### N01_COMPARE_CAUTION (signal / ui / live)

**Utløser:** Gyldig avslutning av N01, også ved uavklart resultat.

**Regi:** Vis som kort undersøkelsestekst uten fortellerstemme. Gå tilbake til samme fysiske oppgave etterpå.

**Krav:** {"all": [], "none": []}

**DESCRIPTION [N01_COMPARE_CAUTION_001]**: Comparison filed: earlier service complaint; later residual. A shared cause is not established.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [N01_COMPARE_CAUTION_002]**: Keep the actual inspection result with the printout.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {"record_compared": true}, "give_documents": []}`

### N02_REFLECTION (early_shift / ui / live)

**Utløser:** Kun etter at spilleren faktisk har flyttet ståsted og tatt den vanlige kontrollen.

**Regi:** En forklarlig kontroll, ikke en skjult skikkelse. Tilby ståstedshjelp fremfor å kreve perfekt sikting.

**Krav:** {"all": ["ctx_room_quiet"], "none": ["field_reflection_checked"]}

**DESCRIPTION [N02_REFLECTION_001]**: From the first position, the bright edge resembles a person behind the glass.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [N02_REFLECTION_002]**: From the side, it follows the room lamp. The photograph shows the same reflection.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {"field_reflection_checked": true}, "give_documents": []}`

### T01_SERVICE_LOG (early_shift / ui / live)

**Utløser:** Les fysisk servicekort ved trafostasjonens betjeningsfront.

**Regi:** Vis som kort undersøkelsestekst uten fortellerstemme. Gå tilbake til samme fysiske oppgave etterpå.

**Krav:** {"all": ["handover_done"], "none": ["field_service_read"]}

**DESCRIPTION [T01_SERVICE_LOG_001]**: Inspection circuit. The open complaint describes a weak exterior lamp, not a failure of the observatory receiver.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [T01_SERVICE_LOG_002]**: The label on the service card matches the marked inspection front.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {"field_service_read": true}, "give_documents": ["EV_TRAFO_CARD"]}`

### T01_REQUEST_CHECK (early_shift / ui / live)

**Utløser:** Velg den fiktive servicekontrollen etter å ha lest kortet.

**Regi:** Start en avgrenset verdenstest via adapteren. Scenen setter ikke suksess. Den lokale modellen må faktisk gi forventet kontrollrespons. Ingen inngrep i eksponerte høyspentdeler.

**Krav:** {"all": ["field_service_read"], "none": ["trafo_normal_checked"]}

**DESCRIPTION [T01_REQUEST_CHECK_001]**: The service check compares the panel result with the actual inspection light.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [T01_REQUEST_CHECK_002]**: Watch the marked light after the test. Do not file a result from the indicator alone.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### T01_NORMAL_RESULT (early_shift / ui / live)

**Utløser:** Vis bare når den faktiske T01-testen er ferdig med forventet lys-/panelresultat.

**Regi:** Adapteren legger til faktisk dato/klokkeslett i inspeksjonskortet. Ikke bruk et manusfast tidspunkt som målte data.

**Krav:** {"all": ["field_service_read", "ctx_trafo_service_ok"], "none": ["trafo_normal_checked"]}

**DESCRIPTION [T01_NORMAL_RESULT_001]**: The completed service check and the inspection light now agree.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [T01_NORMAL_RESULT_002]**: The ordinary lamp complaint is resolved under this control. Keep the actual completion time on the inspection card.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {"trafo_normal_checked": true}, "give_documents": ["EV_TRAFO_CHECK"]}`

### T01_INCOMPLETE (early_shift / ui / live)

**Utløser:** Forsøk på å avslutte T01 før faktisk verifisert kontroll, uten hard straff.

**Regi:** Behold tidligere valg og la spilleren kontrollere på nytt. Ikke gi trafo_normal_checked.

**Krav:** {"all": ["field_service_read"], "none": ["ctx_trafo_service_ok"]}

**DESCRIPTION [T01_INCOMPLETE_001]**: The check is not complete. There is no result to file yet.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [T01_INCOMPLETE_002]**: The service card and the marked light are still available. Return when you can compare them.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### T02_BASELINE_RESULT (field / ui / live)

**Utløser:** Faktisk normalbaseline i feltfasen for en eldre sak, uten tilbakespoling til kveldsoverleveringen.

**Regi:** Kvitter bare den fysiske controllerens gyldige normalrecord. Bevar originalt tidspunkt og gamle kort. Ingen automatisk feltkonklusjon.

**Krav:** {"all": ["ctx_trafo_service_ok"], "none": ["trafo_normal_checked"]}

**DESCRIPTION [T02_BASELINE_RESULT_001]**: The completed service check and the inspection light now agree.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [T02_BASELINE_RESULT_002]**: The ordinary lamp complaint is resolved under this control. Keep the actual completion time on the inspection card.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {"trafo_normal_checked": true, "field_service_read": true}, "give_documents": ["EV_TRAFO_CHECK"]}`

### T02_REFERENCE (field / ui / live)

**Utløser:** Legg EV_DALE_NOTE eller kontaktarket ved kjent ståsted; hovedsporet krever ikke at hele VHS er sett.

**Regi:** Vis som kort undersøkelsestekst uten fortellerstemme. Gå tilbake til samme fysiske oppgave etterpå.

**Krav:** {"all": ["office_evidence_taken", "trafo_normal_checked"], "none": ["field_reference_matched"]}

**DESCRIPTION [T02_REFERENCE_001]**: The culvert, the two white bands and the nearest pole match the reference sheet.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [T02_REFERENCE_002]**: The old photograph gives you a place to stand. It does not tell you which supply is carrying the light now.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {"field_reference_matched": true}, "give_documents": []}`

### T02_CHECKS (field / ui / live)

**Utløser:** Etter at ståstedet er identifisert tilbyr panelet de separate fiktive kontrollene.

**Regi:** Vis ikke korrektvalget før faktiske eksterne verdenstilstander er oppfylt. Spilleren får ikke selverklære testene gjennomført ved å velge en tekst.

**Krav:** {"all": ["field_reference_matched"], "none": ["trafo_anomaly_checked"]}

**DESCRIPTION [T02_CHECKS_001]**: The service indication, the local control and the light must be compared as separate observations.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [T02_CHECKS_002]**: A backup supply or a faulty indicator would be an ordinary explanation. Check those before drawing a conclusion.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

- **T02_CH_VERIFIED**: File the observed disagreement after all checks. → `T02_VERIFIED`
  Krav: `{"all": ["ctx_trafo_isolation_verified", "ctx_trafo_backup_excluded", "ctx_trafo_indicator_checked", "ctx_trafo_lamp_persists", "ctx_trafo_photo_available"], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **T02_CH_UNRESOLVED**: Keep this unresolved. → `T02_UNRESOLVED`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### T02_VERIFIED (field / ui / live)

**Utløser:** Kort konklusjon etter virkelig gjennomførte kontroller, med faktisk lysforløp synlig.

**Regi:** Vis som kort undersøkelsestekst uten fortellerstemme. Gå tilbake til samme fysiske oppgave etterpå.

**Krav:** {"all": ["ctx_trafo_isolation_verified", "ctx_trafo_backup_excluded", "ctx_trafo_indicator_checked", "ctx_trafo_lamp_persists", "ctx_trafo_photo_available"], "none": []}

**DESCRIPTION [T02_VERIFIED_001]**: The local isolation result is recorded. The checked backup does not account for the light. The indicator has been compared with the independent service check.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [T02_VERIFIED_002]**: The marked light continues its visible sequence.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [T02_VERIFIED_003]**: Observation filed: under these controls, the light and the service result disagree. Cause unknown.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {"trafo_anomaly_checked": true}, "give_documents": ["EV_TRAFO_FOLLOWUP"]}`

### T02_UNRESOLVED (field / ui / live)

**Utløser:** Avslutt panelet uten å fabrikere et bekreftet avvik. Hovedhistoriens STATION 01-/Nora-rute er fortsatt tilgjengelig.

**Regi:** Vis som kort undersøkelsestekst uten fortellerstemme. Gå tilbake til samme fysiske oppgave etterpå.

**Krav:** {"all": [], "none": []}

**DESCRIPTION [T02_UNRESOLVED_001]**: Unresolved. The reference is useful, but the current supply and the visible light have not yet been accounted for together.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [T02_UNRESOLVED_002]**: Keep the reference and your checks. An unanswered observation is not a failed case.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {"field_followup_unresolved": true}, "give_documents": []}`

### A02_COMPARE (all_night / ui / live)

**Utløser:** Aktiv sammenligning av den virkelige nåvarianten med tidligere foto eller datert overleveringsreferanse.

**Regi:** Først når to faktiske bilder/referanser er tilgjengelige. Ingen automatisk konklusjon fra nærheten til figuren.

**Krav:** {"all": ["ctx_all_night_active", "handover_done"], "none": ["field_cutout_compared"]}

**DESCRIPTION [A02_COMPARE_001]**: The price sticker and the torn lower corner match the earlier reference.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

**DESCRIPTION [A02_COMPARE_002]**: It is the same cardboard figure. Its position has changed.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

- **A02_CH_MOVED**: Same object, different position. Who moved it is unconfirmed. → `A02_FILED`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`
- **A02_CH_LIVING**: This proves the figure is alive. → `A02_CAUTION`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### A02_CAUTION (all_night / ui / live)

**Utløser:** Spilleren får en presis korreksjon, uten tapte bevis.

**Regi:** Vis som kort undersøkelsestekst uten fortellerstemme. Gå tilbake til samme fysiske oppgave etterpå.

**Krav:** {"all": [], "none": []}

**DESCRIPTION [A02_CAUTION_001]**: The comparison establishes position and identity. It does not show what moved the figure.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*

- **A02_CH_REVISE**: File only what the comparison shows. → `A02_FILED`
  Krav: `{"all": [], "none": []}`; ved valg: `{"set_flags": {}, "give_documents": []}`

**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### A02_FILED (all_night / ui / live)

**Utløser:** Registrer spillerens sammenligning, ikke bare et automatisk nærhetsfunn.

**Regi:** Vis som kort undersøkelsestekst uten fortellerstemme. Gå tilbake til samme fysiske oppgave etterpå.

**Krav:** {"all": [], "none": []}

**DESCRIPTION [A02_FILED_001]**: Filed: same figure, changed position. The cause remains unconfirmed.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {"field_cutout_compared": true}, "give_documents": []}`

### P01_STUDIO_AFTER (all_night / ui / live)

**Utløser:** Kort lydetterspill etter lukket sammenligningspanel, bare når lydsystemet faktisk har spilt lyden.

**Regi:** Bruk romlig printerlyd og avvent opptatt-tilstander. Ingen Dale eller svarende spøkelse i studioet. Engelsk tekst er lydtekst, ikke fasit om lydens opprinnelse.

**Krav:** {"all": ["field_cutout_compared", "ctx_room_quiet"], "none": ["field_studio_after_heard"]}

**DESCRIPTION [P01_STUDIO_AFTER_001]**: From inside the closed studio: the short rhythm of a printer. Then the ordinary carrier again.

*Ingen innspilt stemme. Tekst på panelet eller i notatboken.*


**Ved fullføring:** `{"set_flags": {"field_studio_after_heard": true}, "give_documents": []}`

### EV_TRAFO_CARD: Inspection circuit / service card

typed / CALICHE JUNCTION / INSPECTION FRONT

Local inspection circuit. Compare the identifier on this card with the marked service front before requesting a check.

Open complaint: weak exterior inspection light. The observatory receiver is a separate system. This card is not an instruction to change receiver calibration.

The permitted service check has a visible response at the marked inspection light. Compare the completed panel result with that response. File an uncertain result as uncertain.

**Produksjon:** Fiktivt servicekort ved trygg betjeningsfront utenfor avsperrede høyspentdeler. Ikke tegn dette som en virkelig elektrisk koblingsinstruks.

### EV_TRAFO_CHECK: Inspection card / completed local check

hand / OPERATOR RECORD / CURRENT SHIFT

Completed local service check: panel result and inspection light agreed after the ordinary complaint was handled.

Completion date and time: recorded from the actual check. Observed response retained with the service result.

This establishes the result of that check at that time. It is not a continuous record of every circuit throughout the night.

**Produksjon:** Bare gis etter ctx_trafo_service_ok. Adapteren påfører faktisk lokal dato og klokkeslett; feltet er ikke en fast 20:xx-gjetning.

### EV_TRAFO_FOLLOWUP: Inspection card / later disagreement

hand / FIELD FOLLOW-UP / CURRENT SHIFT

Reference position matched against the retained field sheet. Local isolation checked; backup explanation excluded under the permitted control; panel indication compared against the independent service result.

The marked light continued its visible sequence. The result and the observation did not agree under these controls.

Cause unknown. Keep this with the normal inspection card and the reference sheet. Do not write a source or a distance that was not measured.

**Produksjon:** Bare etter alle fire faktiske T02-verdensbetingelser. Tilføy faktisk dato/klokkeslett og eget referansebilde ved lagring; aldri automatisk ved avspilling av VHS.

**FIELD_UI_SERVICE**: Read the service card

*Ved fysisk betjeningsfront.*

**FIELD_UI_TEST**: Run the service check

*Fiktiv kontroll; avvent virkelig resultat.*

**FIELD_UI_WATCH**: Compare the result with the inspection light

*Etter at testen er ferdig.*

**FIELD_UI_LOG**: Keep the inspection card

*Når en datert kontroll finnes.*

**FIELD_UI_REFERENCE**: Compare the reference position

*Krever kontaktark eller hovednotat.*

**FIELD_UI_BACKUP**: Check the local backup indication

*En selvstendig kontroll i fiksjonen.*

**FIELD_UI_INDICATOR**: Compare the independent service result

*Kan ikke gjøres riktig bare ved å trykke konklusjon.*

**FIELD_UI_LEAVE**: Leave this unresolved

*Gyldig avslutning uten hovedhistorielås.*

**FIELD_UI_WAIT**: The check is still running.

*Ingen automatisk suksess mens testen pågår.*

**FIELD_UI_N01**: Compare the local records with the printout

*Etter faktisk residual og med dokumenter tilgjengelige.*

**FIELD_UI_NO_REFERENCE**: Keep a reference before deciding what changed.

*Tilby datert referanse uten å kreve et gammelt valgfritt matstopp.*
