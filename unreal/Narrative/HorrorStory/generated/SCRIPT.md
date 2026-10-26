# SIGNAL / 47: Horror Story H1

Forfatterpakke. Ikke runtime-integrert. Ingen lyd eller video er produsert.

Spilltekst: engelsk. Regi: norsk. Medietider er planlagte, ikke ferdige opptakstider.

## ordinary_evening

### H01_BRIEF (handover / phone / live)

**Utløser:** H01: Etter eksisterende Dale-overlevering: hent kameraet, kontroller én rutinevisning og gå de faktisk tilgjengelige dørene på normalrunden. Ikke påstå at en fremtidig forbindelsesdør allerede er åpen. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h01_ready"], "none": ["hr_h01_recorded", "hr_h01_brief_presented"]}

**DALE [H01_BRIEF_001]**: Keep the survey running. Write down anything you have to change, even if it fixes itself.

*Fiktiv kollega. Tørr, erfaren, menneskelig. Kun fjernstemme; ingen fysisk Dale eller VHS-forteller.*

**DALE [H01_BRIEF_002]**: The camera is beside the east door. Take a reference before you touch the fault.

*Fiktiv kollega. Tørr, erfaren, menneskelig. Kun fjernstemme; ingen fysisk Dale eller VHS-forteller.*

**DALE [H01_BRIEF_003]**: And try your way out while the lights are good. A door on a plan is no use if you have never opened it.

*Fiktiv kollega. Tørr, erfaren, menneskelig. Kun fjernstemme; ingen fysisk Dale eller VHS-forteller.*


**Ved fullføring:** `{"set_flags": {"hr_h01_brief_presented": true}, "give_documents": []}`

### H01_CHECK (handover / ui / live)

**Utløser:** H01: Prøv SAROs østdør og den tilgjengelige korridorutgangen fra innsiden. Sammenlign en vanlig lampe sett direkte og via glass, og trekk for en faktisk opak skjerm når H3 har levert den. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h01_ready", "ctx_hr_h01_checked"], "none": ["hr_h01_recorded"]}

**DESCRIPTION [H01_CHECK_001]**: The lamp, its reflection and the dark glass agree when you change the light. This is an ordinary reference.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H01_CHECK_002]**: The notebook records the doors you actually opened. Untried routes stay unmarked.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h01_recorded": true}, "give_documents": []}`

### H01_INCOMPLETE (handover / ui / live)

**Utløser:** H01: Hvis en planlagt utgang eller skjerm ikke er levert, før den som uprøvd og gi normalrutinen en eksisterende vei. Ikke kvitter for to fluktruter før H3-topologien er målt. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h01_ready", "ctx_hr_h01_insufficient"], "none": []}

**DESCRIPTION [H01_INCOMPLETE_001]**: One route is still untested. Check the door from this side; the drawing alone does not show whether it opens.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H02_BRIEF (early_shift / walkie / live)

**Utløser:** H02: Kjør eksisterende avstikker til trafostasjonens tilgjengelige kontrollfront. Finn T01-klagen på servicekortet, observer inspeksjonslampen og utfør den fiktive, avgrensede servicekontrollen. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h02_ready"], "none": ["hr_h02_recorded", "hr_h02_brief_presented"]}

**DALE [H02_BRIEF_001]**: The complaint is about the inspection lamp. Just that lamp.

*Fiktiv kollega. Tørr, erfaren, menneskelig. Kun fjernstemme; ingen fysisk Dale eller VHS-forteller.*

**DALE [H02_BRIEF_002]**: Use the marked service control outside the fence. Leave the locked equipment alone.

*Fiktiv kollega. Tørr, erfaren, menneskelig. Kun fjernstemme; ingen fysisk Dale eller VHS-forteller.*

**DALE [H02_BRIEF_003]**: Watch the lamp and the indicator separately. Tell me whether they agree.

*Fiktiv kollega. Tørr, erfaren, menneskelig. Kun fjernstemme; ingen fysisk Dale eller VHS-forteller.*


**Ved fullføring:** `{"set_flags": {"hr_h02_brief_presented": true}, "give_documents": []}`

### H02_CHECK (early_shift / ui / live)

**Utløser:** H02: Sammenlign lampen og den separate indikatoren før/etter normal kontroll. Hold STATION 01, SAROs K3 og byens hovedforsyning atskilt. Spilleren åpner ikke høyspenningsområdet. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h02_ready", "ctx_hr_h02_checked"], "none": ["hr_h02_recorded"]}

**DESCRIPTION [H02_CHECK_001]**: The inspection lamp follows its local control. The separate indicator agrees with the service test.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H02_CHECK_002]**: The complaint can be explained here. This does not establish the power state of the whole town.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h02_recorded": true}, "give_documents": []}`

### H02_INCOMPLETE (early_shift / ui / live)

**Utløser:** H02: Avbrutt eller uenig prøve føres som uavklart og kan tas opp fra den merkede kontrollfronten. Kalibrering kan fortsette, men nye R2-kveldssaker krever fullført journal, kontroll, faktisk lampesammenligning og fysisk SARO-arkivering før overgangen til ubetjent survey. Gamle saker får ingen ny gate. T02 tilbyr ny normal kontroll der den historiske baselinen mangler. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h02_ready", "ctx_hr_h02_insufficient"], "none": []}

**DESCRIPTION [H02_INCOMPLETE_001]**: The readings do not agree yet. You can finish the calibration, but the unattended survey needs the completed inspection filed at SARO. Keep the card and repeat the comparison at the marked control.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H03_BRIEF (signal / ui / live)

**Utløser:** H03: Etter reell residual 00:13:47 og eksisterende signalhendelse: bruk S-03/B-12-ruten, to faktiske eksponeringer og vanlig fremkalling. Behold nåværende bevis-ID-er og porter. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h03_ready"], "none": ["hr_h03_recorded", "hr_h03_brief_presented"]}

**DESCRIPTION [H03_BRIEF_001]**: The residual has a time of its own: 00:13:47. The earlier service complaint does not explain it.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H03_BRIEF_002]**: Mark one detail on FRAME 01 that you can check at B-12. Change one local condition, then expose again.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h03_brief_presented": true}, "give_documents": []}`

### H03_CHECK (signal / ui / live)

**Utløser:** H03: Velg eksisterende aktiv kontroll, noter direkte observasjon og sammenlign samme detalj på de to fremkalte bildene. Ingen nytt panelvalg erstatter lukkeren eller bildepikslene. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h03_ready", "ctx_hr_h03_checked"], "none": ["hr_h03_recorded"]}

**DESCRIPTION [H03_CHECK_001]**: The local condition changed. The marked reference remains on the controlled print.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H03_CHECK_002]**: File what the two exposures support. The next question is where B-12 got that reference.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h03_recorded": true}, "give_documents": []}`

### H03_INCOMPLETE (signal / ui / live)

**Utløser:** H03: Feil fremkalling beholder negativet; lukk mørkeromsdøren og lag ny kopi. Et uleselig bilde kan ikke bære påstanden. P03 forblir åpent til eksisterende kontroll eller gyldig ny kopi finnes. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h03_ready", "ctx_hr_h03_insufficient"], "none": []}

**DESCRIPTION [H03_INCOMPLETE_001]**: The controlled detail is not readable on this copy. Keep the negative and print it again with the darkroom shut.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H04_BRIEF (archive / phone / live)

**Utløser:** H04: Følg begrunnet serviceskuffspor til reservenøkkel og eksisterende kontordør. Hent feltmappen; den korte hovedruten krever ikke hele VHS-filmen. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h04_ready"], "none": ["hr_h04_recorded", "hr_h04_brief_presented"]}

**DALE [H04_BRIEF_001]**: The service drawer has a spare. Brass tag, OFFICE. I should have put it in the handover.

*Fiktiv kollega. Tørr, erfaren, menneskelig. Kun fjernstemme; ingen fysisk Dale eller VHS-forteller.*

**DALE [H04_BRIEF_002]**: Take the field folder. I wrote down where I stood so somebody else could try it.

*Fiktiv kollega. Tørr, erfaren, menneskelig. Kun fjernstemme; ingen fysisk Dale eller VHS-forteller.*

**DALE [H04_BRIEF_003]**: I saw the same shape from two places. I do not know whether it was looking at me.

*Fiktiv kollega. Tørr, erfaren, menneskelig. Kun fjernstemme; ingen fysisk Dale eller VHS-forteller.*


**Ved fullføring:** `{"set_flags": {"hr_h04_brief_presented": true}, "give_documents": []}`

### H04_CHECK (archive / ui / live)

**Utløser:** H04: Legg Dales daterte ståstedsnotat ved faktisk vindusutsyn og referansestolpe. Se eventuell VHS som bevegelig opptak og test ett av stedene fysisk senere. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h04_ready", "ctx_hr_h04_checked"], "none": ["hr_h04_recorded"]}

**DESCRIPTION [H04_CHECK_001]**: The notes specify a post and a standing place. They can be checked in the world.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H04_CHECK_002]**: The archive gives STATION 01 a separate history. Dale has not identified the person in the old record.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h04_recorded": true}, "give_documents": []}`

### H04_INCOMPLETE (archive / ui / live)

**Utløser:** H04: Feltmappe gir hovedsporet selv uten strøm til VHS. Manglende sett film føres som usett; opptaket gjenopptas fra lagret sted. Nøkkelen har konkret spor i serviceskuff, ikke tilfeldig spawn. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h04_ready", "ctx_hr_h04_insufficient"], "none": []}

**DESCRIPTION [H04_INCOMPLETE_001]**: The field folder gives you the post and the standing place. The tape is still unviewed; there is enough to make the field check.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

## retained_reference

### H05_BRIEF (field / ui / live)

**Utløser:** H05: Kjør til STATION 01 via hovedvei/surveyspor. Utfør E09A/fastmerke A, lokal nulltest, kabelundersøkelse og receiver/E10. T02 følger samme feltreise via eksisterende trafoadkomst. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h05_ready"], "none": ["hr_h05_recorded", "hr_h05_brief_presented"]}

**DESCRIPTION [H05_BRIEF_001]**: E09A fixes A against the transit and B. C is a separate closing sight line.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H05_BRIEF_002]**: The empty footing and the marked post can be photographed together. Check them before deciding what moved.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H05_BRIEF_003]**: Use Dale's post as a second observation point. A position written in his notebook is a lead until you check it yourself.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h05_brief_presented": true}, "give_documents": []}`

### H05_CHECK (field / ui / live)

**Utløser:** H05: Hold A fra transitkortet adskilt fra C-linjen. Sammenlign to nåtidige, fysisk kontrollerte referansesteder fra Dales notat mot de faste surveymerkene. T02 krever egne faktiske indikator-, kilde- og backupkontroller. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h05_ready", "ctx_hr_h05_checked"], "none": ["hr_h05_recorded"]}

**DESCRIPTION [H05_CHECK_001]**: Both controlled observation points resolve against fixed survey references. The positions form an arrangement rather than an isolated sighting.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H05_CHECK_002]**: The cut faces and the covered lamp answer separate questions. Neither identifies who cut the cable.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H05_CHECK_003]**: The receiver repeats the 1947 correction at the same place in the fragment. Take the record and field prints to Nora Vega.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h05_recorded": true}, "give_documents": []}`

### H05_INCOMPLETE (field / ui / live)

**Utløser:** H05: Feil bilde kan fremkalles på nytt fra negativet. T02 kan være uavklart uten å sperre hovedhistorien; kartleggingssporet får da obligatorisk normal-/ekstern-lyskontroll ved SARO i H07, ikke en valgfri skyggemann som eneste bevis. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h05_ready", "ctx_hr_h05_insufficient"], "none": []}

**DESCRIPTION [H05_INCOMPLETE_001]**: The lamp follow-up remains unresolved. Keep that limit on the card. The cable record, timing record and trip to Nora remain available.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H05_FIXED_FRAGMENT (field / radio / fixed)

**Utløser:** Spill eksisterende Tomás-fragment fra receiveren i hytta. Gjenbruk den ferdige eksisterende kilden, ikke produser ny timing fra denne teksten.

**Regi:** Fast avtrykk. Ord og avspillingsrytme kommer fra eksisterende opptak. Ingen svarvalg, ingen ny replikk til et nytt spørsmål.

**Krav:** {"all": ["ctx_hr_h05_receiver_playing"], "none": []}

**DESCRIPTION [H05_FIXED_FRAGMENT_001]**: [Field receiver: "Reference west. No. East. Hold the last reading."]

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H06_BRIEF (field / physical / live)

**Utløser:** H06: Ta feltkopier og original/amendert rapport til Nora i Rom 6. Bruk eksisterende P10/P11/P12, brevet, feltkortet og returlinjen; gi samtalen ro. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h06_ready"], "none": ["hr_h06_recorded", "hr_h06_brief_presented"]}

**NORA [H06_BRIEF_001]**: Tomás was my brother. Nineteen. He checked the reading twice before he wrote it down.

*Eksisterende Nora-identitet. Rolig, konkret; kjenner 1947, ikke invasjonens forfatterfasit.*

**NORA [H06_BRIEF_002]**: Yes. I cut it. The lamp was off. The reference stayed.

*Eksisterende Nora-identitet. Rolig, konkret; kjenner 1947, ikke invasjonens forfatterfasit.*

**NORA [H06_BRIEF_003]**: What you heard is what he said then. I have asked it other things. It gives me the same words.

*Eksisterende Nora-identitet. Rolig, konkret; kjenner 1947, ikke invasjonens forfatterfasit.*


**Ved fullføring:** `{"set_flags": {"hr_h06_brief_presented": true}, "give_documents": []}`

### H06_CHECK (field / physical / live)

**Utløser:** H06: Sammenlign hennes opprinnelige plate med B-12-kontrollen. Still Noras konkrete nye spørsmål til returlinjen og hør at samme gamle korreksjon kommer igjen. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h06_ready", "ctx_hr_h06_checked"], "none": ["hr_h06_recorded"]}

**NORA [H06_CHECK_001]**: C ran east from STATION 01. The old road crosses it at a bolt in the shoulder.

*Eksisterende Nora-identitet. Rolig, konkret; kjenner 1947, ikke invasjonens forfatterfasit.*

**NORA [H06_CHECK_002]**: Keep the original where somebody can read it. I tried leaving out a line. It did not make the place go away.

*Eksisterende Nora-identitet. Rolig, konkret; kjenner 1947, ikke invasjonens forfatterfasit.*

**NORA [H06_CHECK_003]**: Do not finish his observation. And do not stop on the line.

*Eksisterende Nora-identitet. Rolig, konkret; kjenner 1947, ikke invasjonens forfatterfasit.*


**Ved fullføring:** `{"set_flags": {"hr_h06_recorded": true}, "give_documents": []}`

### H06_INCOMPLETE (field / ui / live)

**Utløser:** H06: P10/P11/P12 og papirene blir stående tilgjengelig. Manglende plate/tema fører tilbake til bordet eller originalkilden, ikke til et nytt angrep. Brevet er aldri en tidsprøve. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h06_ready", "ctx_hr_h06_insufficient"], "none": []}

**DESCRIPTION [H06_INCOMPLETE_001]**: The letter and the field card are still on the table. Ask about the missing part before leaving.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H07_BRIEF (field / phone / live)

**Utløser:** H07: Etter Rom 6 og tidligst 04:04: returner til SARO og sikre mottakeroriginal, tomt/påbegynt hendelsesark og ferdige fjernlogger før møtet. Utfør normal kontroll av kjent skjerm/utgang. Den senere HR_SARO_ORIGINAL-kompositten lages først etter faktisk lyskontroll, reaktivt møte og flukt. Første prototype krever fysisk to gyldige ruter fra H3. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h07_ready"], "none": ["hr_h07_recorded", "hr_h07_brief_presented"]}

**WARD [H07_BRIEF_001]**: Bring the original run off the rack. Keep the receiver copy with it.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [H07_BRIEF_002]**: Hondo and Site 11 are sending their local records. I want the records, not three accounts of the same broadcast.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**DESCRIPTION [H07_BRIEF_003]**: The ventilation stops. A white band moves across the outside wall. You can still see the doorway and the floor.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h07_brief_presented": true}, "give_documents": []}`

### H07_CHECK (field / ui / live)

**Utløser:** H07: Kontroller ekstern lysflate mot lokal lampe og opak skjerm. Inntrengeren undersøker en ny lyd fra spillerens radio, mens et separat gammelt fragment fortsetter uendret. Reaksjon bygger på faktisk sikt/lyd, ikke på valgtekst. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet. Presenter ferdig romobservasjon først etter faktisk lyskontroll, reaktivt møte og gjennomført flukt; ingen fremtidig hendelsesbeskrivelse i før-angrepsbunten. Skrekkrevisjon R2: aktører er kjent fra kvelden; denne kontrollen krever faktisk observert samordning av søk og avsøkning, ikke første bevis på reaksjon.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h07_ready", "ctx_hr_h07_checked", "ctx_hr_h07_escape_complete"], "none": ["hr_h07_recorded"]}

**DESCRIPTION [H07_CHECK_001]**: The local lamp is off. The band still crosses the glass, and the opaque screen cuts it off.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H07_CHECK_002]**: It searched one room earlier. Now the outside band is closing the exits as the visitor moves. The recorded fragment has not changed.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h07_recorded": true}, "give_documents": []}`

### H07_INCOMPLETE (field / ui / live)

**Utløser:** H07: Ingen fotoport under jakt. Journalfør en faktisk sanset reaksjon fra begge ruter; spilleren kan gjenta kontrollen før faren uten å måtte velge avledning som fluktmetode. Fangst tilbyr tydelig lokal gjenopptakelse fra trygt sjekkpunkt, ikke ny tapt-tid-gren. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h07_ready", "ctx_hr_h07_insufficient"], "none": []}

**DESCRIPTION [H07_INCOMPLETE_001]**: A photograph is not required. Check the original's recorded location. If it is not secured, reach it by a clear route; then use the service exit.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H07_FIRST_WARNING (field / ui / live)

**Utløser:** Første avsøkning, før inntrengerkontakt. Hold fysisk kontroll hos spilleren og vis kjent opak dekning og serviceutgang.

**Regi:** Én tydelig reaksjonsmulighet. Ingen blendende fullskjerm og ingen umiddelbar fangst.

**Krav:** {"all": ["ctx_hr_h07_scan_started"], "none": ["hr_h07_first_warning_presented"]}

**DESCRIPTION [H07_FIRST_WARNING_001]**: The band is moving toward the next window. The wall between the windows stays dark.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H07_FIRST_WARNING_002]**: The service route is behind the screen. The front yard is in the light.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h07_first_warning_presented": true}, "give_documents": []}`

### H07_CAPTURE (field / ui / live)

**Utløser:** Kun etter bekreftet fysisk fangst. Ikke spill ved vellykket flukt.

**Regi:** Kort særskilt overgang. Ikke tapt tid i fiksjonen, tilfeldig ny returposisjon eller ny obligatorisk fangst.

**Krav:** {"all": ["ctx_hr_h07_captured"], "none": []}

**DESCRIPTION [H07_CAPTURE_001]**: Caught inside SARO.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H07_CAPTURE_002]**: Resume from before the inspection. The findings already secured are kept.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H07_ESCAPE_COMPLETE (field / ui / live)

**Utløser:** Bare etter faktisk sikret original og fullført flukt til det målte sikre servicebilpunktet. Lys-/radiokontroll alene utløser ikke denne noden. Lagres én gang med fluktresultatet.

**Regi:** Rolig etterarbeid. Ingen ny tilfeldig bortføring eller ny obligatorisk fangst. Denne utgangen kan nås via begge målte ruter.

**Krav:** {"all": ["ctx_hr_h07_original_secured", "ctx_hr_h07_escape_complete"], "none": ["hr_h07_escape_complete_presented"]}

**DESCRIPTION [H07_ESCAPE_COMPLETE_001]**: The original is secured in the truck. The visitor has lost contact. Its search has ended.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h07_escape_complete_presented": true}, "give_documents": []}`

## arrival

### H08_BRIEF (field / phone / live)

**Utløser:** H08: På trygg plass: legg SARO-original og eget romnotat ved Hondo- og Site11-utskrifter. Hent bevart kopi ved sen ankomst. Sammenlign proveniens, uavhengige kanaler, klokker, faseorden og kommende spor. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h08_ready"], "none": ["hr_h08_recorded", "hr_h08_brief_presented"]}

**WARD [H08_BRIEF_001]**: Hondo kept its power when you lost yours. Their paper is a local record. Do not put your outage on their chart.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [H08_BRIEF_002]**: Site 11 took the second reading with the audio feed unplugged. Their transit still recorded it.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [H08_BRIEF_003]**: Compare the times and the order. Then tell me what is observation and what is your explanation.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*


**Ved fullføring:** `{"set_flags": {"hr_h08_brief_presented": true}, "give_documents": []}`

### H08_CHECK (field / ui / live)

**Utløser:** H08: Hondo/Site11 beholder sine ulike lokale måleverdier. Sammenlign med vanlig sendeplan og kontroller at lydmating var frakoblet ved Site11s kontroll. Like ord alene godkjennes ikke. Ring Ward og send presise begrensninger. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet. Varslingsgrunnlaget blir eksternt sikret som dikterte måletabeller/kilde-ID-er og datert readback, ikke automatisk kopi av fotoarkivet.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h08_ready", "ctx_hr_h08_checked", "ctx_hr_remote_sources_verified"], "none": ["hr_h08_recorded"]}

**DESCRIPTION [H08_CHECK_001]**: The local source sheets agree on phase order and measured interval. Their separate optical readings show continuing approach.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H08_CHECK_002]**: The warning can state coordinated preparation for an arrival. Identity and purpose remain unestablished. Call Ward and keep those limits on the report.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h08_recorded": true}, "give_documents": []}`

### H08_INCOMPLETE (field / ui / live)

**Utløser:** H08: Manglende sanntidsmottak erstattes av bevart lokalt stempel/originalkopi på telex. Uverifisert kilde føres som foreløpig og ringer Ward for kildekontroll; varsel om lokal fare kan sendes straks. Sterk nettverksslutning venter på de to uavhengige fjernkildene. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h08_ready", "ctx_hr_h08_insufficient"], "none": []}

**DESCRIPTION [H08_INCOMPLETE_001]**: One remote source is not verified. Send the local warning now, then ask Ward for the original source sheet before recording a network conclusion.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H08_COMPARE_RECORDS (field / ui / live)

**Utløser:** Spilleren har faktisk samlet og sammenlignet alle tre lokale kilder og klokke-/kontrollark. Den komplette SARO-kilden krever faktisk lys-/reaksjonskontroll og gjennomført flukt. Før-angrepsbunten inneholder ikke disse ferdige observasjonene.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_remote_sources_verified", "ctx_hr_h07_checked", "ctx_hr_h07_escape_complete"], "none": []}

**DESCRIPTION [H08_COMPARE_RECORDS_001]**: The source sheets contain different local readings. Their sequence and timing agree.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H08_COMPARE_RECORDS_002]**: Hondo and Site 11 each recorded approaching tracks after the reference sweep. SARO is one prepared place among several.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H08_COMPARE_RECORDS_003]**: The records support a warning about a coordinated arrival. They do not identify who is coming.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": ["HR_SARO_ORIGINAL", "HR_HONDO_LOCAL", "HR_SITE11_LOCAL", "HR_NETWORK_CHECK"]}`

### H08_WARNING_ACK (field / phone / live)

**Utløser:** Kvittering først når Ward faktisk er nådd, advarselen lest tilbake og 06:00-jobben suspendert. Ikke et tilfeldig tidsutløp. Kvitteringen krever faktisk diktering av valgte kildeark og Wards daterte logg/readback; foto og negative blir hos spilleren.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_warning_acknowledged", "ctx_hr_remote_sources_verified"], "none": ["hr_h08_warning_ack_presented"]}

**WARD [H08_WARNING_ACK_001]**: The network warning is entered. The six o'clock series remains suspended after your earlier report. The other sites have read it back: nobody closes a reference or completes a run.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [H08_WARNING_ACK_003]**: I have written the source IDs and the measurements into the call log. The warning is outside SARO. Keep the photographs and the originals with their source sheets.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [H08_WARNING_ACK_002]**: The field courier is on the old Roswell road, east of the mapped reference. Check the map at the diner before you agree a handover point.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*


**Ved fullføring:** `{"set_flags": {"hr_h08_warning_ack_presented": true}, "give_documents": ["HR_WARNING_RECEIPT"]}`

### H09_BRIEF (all_night / physical / live)

**Utløser:** H09: Rolig diner: les E15, det innrammede utklippet fra juli 1947 som allerede henger ved døren. Legg E09A-tracingen på E14, transit på STATION 01 og nord mot nord. Behold P13 som reell kartoppgave. Dagens avis i byen er en annen, valgfri kilde. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h09_ready", "ctx_hr_warning_acknowledged"], "none": ["hr_h09_recorded", "hr_h09_brief_presented"]}

**DRIVER [H09_BRIEF_001]**: Last October it all quit past the eight-mile post. Engine, lights, radio. Then it came back.

*Eksisterende lastebilsjåfør. Praktisk, lavmælt, bare egne veiopplevelser.*

**DRIVER [H09_BRIEF_002]**: I came through again before I stopped here this morning. Nothing quit. That is one trip. I cannot tell you what made the other one different.

*Eksisterende lastebilsjåfør. Praktisk, lavmælt, bare egne veiopplevelser.*

**DESCRIPTION [H09_BRIEF_003]**: On the table the tracing is still loose. Place its transit on STATION 01, north to north. The framed newspaper clipping has no survey scale.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h09_brief_presented": true}, "give_documents": []}`

### H09_CHECK (all_night / phone / live)

**Utløser:** H09: Finn C ved 8.15-bolten og avtal separat 10.4-bolt med Ward. Sjåførens oktoberstopp ved8.15 må tas med i risikovurderingen. Spør om hans nylige gjennomkjøring: det er et nåtidig vitnemål, ikke et generelt trygghetsbevis. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h09_ready", "ctx_hr_h09_checked", "ctx_hr_warning_acknowledged"], "none": ["hr_h09_recorded"]}

**WARD [H09_CHECK_001]**: C is the bolt at eight point one five. The handover is ten point four. I have both marked.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [H09_CHECK_002]**: The old road is the direct approach to the courier. It is a practical route, not a finding that the reference is safe.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [H09_CHECK_003]**: Take the originals through. Stay in the cab, keep moving and take no reading at the bolt. If the truck falters before the crossing, turn back while you can.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*


**Ved fullføring:** `{"set_flags": {"hr_h09_recorded": true}, "give_documents": []}`

### H09_INCOMPLETE (all_night / ui / live)

**Utløser:** H09: E15 er et gammelt, tilgjengelig dinerutklipp og skal ikke ha 04:45-port. Leveringsregelen gjelder dagens avis i byen. Feil kartplassering får eksisterende karttips; bildet løser ikke C alene. Hvis varsling mangler, gi konkret telefonhandling før avreise. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h09_ready", "ctx_hr_h09_insufficient"], "none": []}

**DESCRIPTION [H09_INCOMPLETE_001]**: C is not placed yet. Put the transit on STATION 01 first. Keep the old framed clipping beside the map; it establishes the photograph, not the crossing.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H10_BRIEF (all_night / ui / live)

**Utløser:** H10: Gå til servicebilen og velg tydelig overleveringsoppdrag. Kjør den eksisterende veien fra dinerens mile0 mot det avtalte10.4-punktet, forbi C ved8.15 uten planlagt stopp. Varsling er allerede levert. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h10_ready", "ctx_hr_warning_acknowledged"], "none": ["hr_h10_recorded", "hr_h10_brief_presented"]}

**DESCRIPTION [H10_BRIEF_001]**: The originals are in the cab. Ward has the warning and the dictated measurement tables.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H10_BRIEF_002]**: Handover: mile 10.4. C: mile 8.15. Pass the reference without stopping or taking a reading.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H10_BRIEF_003]**: Leave for the handover when you are ready.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h10_brief_presented": true}, "give_documents": []}`

### H10_CHECK (all_night / ui / live)

**Utløser:** H10: Før avreise bekreft at originalene er med, at Ward har kvittert og at C og overleveringspunkt er forskjellige. Ikke lag en ny parkerings-/avlesningsoppgave ved C. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h10_ready", "ctx_hr_h10_checked", "ctx_hr_h10_stalled"], "none": ["hr_h10_recorded"]}

**DESCRIPTION [H10_CHECK_001]**: At the crossing the engine stops. You did not turn the key off.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H10_CHECK_002]**: The warning has already gone through. The originals are beside you.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h10_recorded": true}, "give_documents": []}`

### H10_INCOMPLETE (all_night / ui / live)

**Utløser:** H10: Uferdig kart eller ukvittert varsel sender spilleren tilbake til den konkrete handlingen før avreise. Under finalen tilbys ingen falsk redningsgren eller skyldtekst. Vanlig Continue starter ved det tydelig oppgitte sikre sjekkpunktet. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h10_ready", "ctx_hr_h10_insufficient"], "none": []}

**DESCRIPTION [H10_INCOMPLETE_001]**: The warning has not been acknowledged. Use the diner payphone before leaving.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### HR_SARO_ORIGINAL: SARO / original run and incident note

typed / 14 APR 1986 / LOCAL RECORD

LOCAL SOURCE / SARO 07. Keep the original receiver record with the separate room-observation sheet. The room observation was written here; it was not received over the network.

The outside band crossed successive windows. The local lamp control did not remove it. An opaque screen interrupted it at the wall.

A visitor reacted to a new radio sound, as in the earlier encounter. Here the outside band moved across the exits in step with the visitor's search. The retained fragment continued unchanged. The new observation concerns coordination, not the first presence of a responsive visitor.

**Produksjon:** Ny forfatterkilde, ikke eksisterende runtimebevis. Dato/tid og måleverdier må bindes til faktisk hendelse i senere adapter. Dokumentet tildeles bare etter den fysiske handlingen; manglende fil godkjennes ikke som foto. Før angrepet finnes bare mottakeroriginal og tomt/påbegynt hendelsesark. Denne sammensatte kilden tildeles først etter faktisk lyskontroll, reaktivt møte og flukt.

### HR_HONDO_LOCAL: Hondo / local record

typed / 14 APR 1986 / LOCAL RECORD

LOCAL SOURCE / HONDO. Copied from Hondo's battery-backed event logger and the operator's separate optical sheet. This is not the SARO telex forwarded back.

Power remained available during the earlier SARO blackout. During the later inspection cycle our local supply and the outside band changed independently.

LOCAL SHEET / reference passes: 03:56:08 R1; 03:56:55 R2; 03:57:42 R3. R1, R2 and R3 are Hondo's surveyed bearings. They are not a copied SARO azimuth setting.

PAIRED OPTICAL HEADS / surveyed baseline 480 m. At 04:00:00, local triangulated slant ranges: track H-A 18.4 km, H-B 21.1 km. At 04:00:30: H-A 18.3 km, H-B 21.0 km. At 04:01:00: H-A 18.2 km, H-B 20.9 km. Range uncertainty per reading: 0.04 km. The closing trend exceeds the measurement uncertainty over the full interval.

Our optical recorder shows the band returning to three surveyed bearings in the same order. After the third pass, two separate tracks entered the recorded approach sector. Their angular separation changed; they are not a still photograph or a repeated radio trace.

Observation interval: 03:56:00 to 04:04:00 MST, 14 APR 1986. Clock check against the time service before and after: within one second. Optical-sheet readings retained with the original. Identity unknown.

**Produksjon:** Ny forfatterkilde, ikke eksisterende runtimebevis. Dato/tid og måleverdier må bindes til faktisk hendelse i senere adapter. Dokumentet tildeles bare etter den fysiske handlingen; manglende fil godkjennes ikke som foto. De numeriske fjernmålingene er ny fiktiv forfatterkilde. Ikke hent dem fra en faktisk test, gamle telex eller gjenta dem som målt spillbevis. Produser to separate kildeark med lokal skisse og råverdier; paired-head oppsett er nytt fjernstasjonsgrunnlag, ikke eksisterende SARO-utstyr.

### HR_SITE11_LOCAL: Site 11 / local record

typed / 14 APR 1986 / LOCAL RECORD

LOCAL SOURCE / SITE 11. Local receiver chart plus independently clocked transit observations. Receiver was disconnected from the intersite audio feed during the second observation.

Three reference bearings were occupied in sequence. Two incoming tracks were then observed in the measured approach sector. Successive local readings show continued approach. The two tracks held their order while the reference cycle advanced.

LOCAL SHEET / reference passes: 03:56:08 R1; 03:56:56 R2; 03:57:42 R3. These labels refer to Site 11's own survey diagram. The middle boundary differs by one second, within the checked clocks; do not erase that difference.

PAIRED OPTICAL HEADS / surveyed baseline 620 m. At 04:00:00, local triangulated slant ranges: track S-A 22.6 km, S-B 24.0 km. At 04:00:30: S-A 22.5 km, S-B 23.9 km. At 04:01:00: S-A 22.4 km, S-B 23.8 km. Range uncertainty per reading: 0.04 km. Directions differ from Hondo's local observations. Identity unknown; no predicted arrival minute entered.

Observation interval: 03:56:00 to 04:04:00 MST, 14 APR 1986. Clock check before and after: within one second. The receiver trace alone cannot establish an approach; the transit readings are a separate observation.

The sequence boundaries agree with Hondo to the resolution of the local clocks. The approach measurements differ by station, as separate locations should. The instruments were not running an agreed survey sequence.

**Produksjon:** Ny forfatterkilde, ikke eksisterende runtimebevis. Dato/tid og måleverdier må bindes til faktisk hendelse i senere adapter. Dokumentet tildeles bare etter den fysiske handlingen; manglende fil godkjennes ikke som foto. De numeriske fjernmålingene er ny fiktiv forfatterkilde. Ikke hent dem fra en faktisk test, gamle telex eller gjenta dem som målt spillbevis. Produser to separate kildeark med lokal skisse og råverdier; paired-head oppsett er nytt fjernstasjonsgrunnlag, ikke eksisterende SARO-utstyr.

### HR_NETWORK_CHECK: Source check / three local records

typed / 14 APR 1986 / LOCAL RECORD

SARO: room observation and original receiver run. Hondo: local event logger and optical record. Site 11: local receiver and transit sheet with the intersite audio feed disconnected.

The ordinary radio programme explains none of the optical changes. Matching words on a radio would establish only a shared recording; these records contain different local measurements.

The same ordered reference cycle occurs at separated sites during the same measured interval. Two stations independently record continuing incoming tracks after that cycle. A coordinated preparation for an arrival is the supported working warning. Identity and purpose remain unknown.

**Produksjon:** Ny forfatterkilde, ikke eksisterende runtimebevis. Dato/tid og måleverdier må bindes til faktisk hendelse i senere adapter. Dokumentet tildeles bare etter den fysiske handlingen; manglende fil godkjennes ikke som foto.

### HR_WARNING_RECEIPT: Ward / warning and suspension receipt

typed / 14 APR 1986 / LOCAL RECORD

RECEIVED / E. WARD. SARO original identified; local observations separated from receiver copy. Hondo and Site 11 originals requested and their source checks retained.

The 06:00 morning series remains suspended after the earlier local safety report. No fresh observer is to close a reference or complete the old run. Other sites have now acknowledged the same instruction. This does not claim that the external activity has stopped.

The warning, source IDs and dictated measurement tables are retained in Ward's dated call log outside SARO. Photographs, negatives and complete source sheets remain in the transfer folder. The courier has the field map. The handover point is the existing survey bolt at mile 10.4 on the old Roswell road; it is not C.

Time of acknowledgement is entered when this call is actually completed. Do not preprint a successful acknowledgement before the line has been answered.

**Produksjon:** Ny forfatterkilde, ikke eksisterende runtimebevis. Dato/tid og måleverdier må bindes til faktisk hendelse i senere adapter. Dokumentet tildeles bare etter den fysiske handlingen; manglende fil godkjennes ikke som foto.
