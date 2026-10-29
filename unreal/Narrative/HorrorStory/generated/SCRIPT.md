# SIGNAL / 47: Horror Story H1

Forfatterpakke. Ikke runtime-integrert. Stemmeopptak: 5 rene takes finnes (voice/). Ingen video er produsert.

Spilltekst: engelsk. Regi: norsk. Medietider er planlagte, ikke ferdige opptakstider.

## ordinary_evening

### H01_BRIEF (handover / phone / live)

**Utløser:** H01: Etter eksisterende Dale-overlevering: hent kameraet, kontroller én rutinevisning og gå de faktisk tilgjengelige dørene på normalrunden. Ikke påstå at en fremtidig forbindelsesdør allerede er åpen. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h01_ready"], "none": ["hr_h01_recorded", "hr_h01_brief_presented"]}

**DALE [H01_BRIEF_001]**: Check the survey reference before the unattended run. Write down anything you have to change, even if it fixes itself.

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

**DESCRIPTION [H02_INCOMPLETE_001]**: The readings do not agree yet. Calibration can continue. Complete and file the lamp inspection, then follow the agreed safety work; do not restart an isolated automatic schedule.

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

**DESCRIPTION [H04_CHECK_001]**: Dale's field index identifies the culvert camera place. The separate array service card describes a different controlled comparison. Neither record is the result of a test you have not performed.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H04_CHECK_002]**: The archive gives STATION 01 a separate history. Dale has not identified the person in the old record.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h04_recorded": true}, "give_documents": []}`

### H04_INCOMPLETE (archive / ui / live)

**Utløser:** H04: Feltmappen gir hovedsporet. Vindus/stolpekontrollen er ufullstendig til notatet er holdt mot faktisk utsyn. VHS-filmen er valgfritt tillegg når den finnes og holder aldri igjen SC07. Nøkkelen har konkret spor i serviceskuff, ikke tilfeldig spawn. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h04_ready", "ctx_hr_h04_insufficient"], "none": []}

**DESCRIPTION [H04_INCOMPLETE_001]**: The field folder gives you the post and the standing place. Hold them against the actual view before you record a result. The tape is optional.

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

**DESCRIPTION [H05_BRIEF_003]**: The culvert camera place belongs to T02. The separate array service comparison concerns its own operator places; do not treat the two records as the same geometry.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h05_brief_presented": true}, "give_documents": []}`

### H05_CHECK (field / ui / live)

**Utløser:** H05: Hold A fra transitkortet adskilt fra C-linjen. Sammenlign to nåtidige, fysisk kontrollerte referansesteder fra Dales notat mot de faste surveymerkene. T02 krever egne faktiske indikator-, kilde- og backupkontroller. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h05_ready", "ctx_hr_h05_checked"], "none": ["hr_h05_recorded"]}

**DESCRIPTION [H05_CHECK_001]**: The empty footing and moved A-post resolve against the fixed transit references. C remains a separate sight line. The earlier array comparison is a distinct controlled observation.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H05_CHECK_002]**: The cut faces and the covered lamp answer separate questions. Neither identifies who cut the cable.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H05_CHECK_003]**: The receiver repeats the 1947 correction at the same place in the fragment. Take the record and field prints to Nora Vega.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h05_recorded": true}, "give_documents": []}`

### H05_INCOMPLETE (field / ui / live)

**Utløser:** H05: Feil bilde kan fremkalles på nytt fra negativet. T02 kan være uavklart uten å sperre hovedhistorien; kartleggingssporet får da obligatorisk normal-/ekstern-lyskontroll ved SARO i H07, uten noen valgfri figur som eneste bevis. Vis bare ved et faktisk utilstrekkelig resultat.

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

**NORA [H06_CHECK_003]**: Don't finish his observation. Get the originals to the courier, and don't stop at C.

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

**Krav:** {"all": ["ctx_hr_h07_ready", "ctx_hr_h07_checked", "ctx_hr_h07_escape_complete", "ctx_hr_h07_bundle_secured"], "none": ["hr_h07_recorded"]}

**DESCRIPTION [H07_CHECK_001]**: The local lamp is off. The band still crosses the glass, and the opaque screen cuts it off.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H07_CHECK_002]**: It searched one room earlier. Now the outside band is closing the exits as the visitor moves. The recorded fragment has not changed.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h07_recorded": true}, "give_documents": []}`

### H07_INCOMPLETE (field / ui / live)

**Utløser:** H07: Ingen fotoport under jakt. Journalfør en faktisk sanset reaksjon fra begge ruter; spilleren kan gjenta kontrollen før faren uten å måtte velge avledning som fluktmetode. Fangst tilbyr tydelig lokal gjenopptakelse fra trygt sjekkpunkt, ikke ny tapt-tid-gren. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h07_ready", "ctx_hr_h07_insufficient"], "none": []}

**DESCRIPTION [H07_INCOMPLETE_001]**: A photograph is not required. If the original was not secured, return to the covered checkpoint before the inspection and collect it before the encounter starts. Do not go back through an active search for another sheet.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H07_FIRST_WARNING (field / ui / live)

**Utløser:** Første avsøkning, før inntrengerkontakt. Hold fysisk kontroll hos spilleren og vis kjent opak dekning og serviceutgang.

**Regi:** Én tydelig reaksjonsmulighet. Ingen blendende fullskjerm og ingen umiddelbar fangst.

**Krav:** {"all": ["ctx_hr_h07_scan_started", "ctx_hr_h07_bundle_secured"], "none": ["hr_h07_first_warning_presented"]}

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

**Krav:** {"all": ["ctx_hr_h07_original_secured", "ctx_hr_h07_escape_complete", "ctx_hr_h07_bundle_secured"], "none": ["hr_h07_escape_complete_presented"]}

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

**Utløser:** H08: Hondo/Site11 beholder sine ulike lokale måleverdier. Sammenlign med vanlig sendeplan og kontroller at lydmating var frakoblet ved Site11s kontroll. Like ord alene godkjennes ikke. Ring Ward og send presise begrensninger. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet. Varslingsgrunnlaget blir eksternt sikret som tre kilde-ID-er og første grensetid, lest tilbake av Ward i datert logg. Måletabellene blir i dokumentene; ingen automatisk kopi av fotoarkivet.

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

**Krav:** {"all": ["ctx_hr_remote_sources_verified", "ctx_hr_h07_checked", "ctx_hr_h07_escape_complete", "ctx_hr_h07_original_secured", "ctx_hr_h07_bundle_secured"], "none": []}

**DESCRIPTION [H08_COMPARE_RECORDS_001]**: The source sheets contain different local readings. Their sequence and timing agree.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H08_COMPARE_RECORDS_002]**: Hondo and Site 11 each recorded approaching tracks after the reference sweep. SARO is one prepared place among several.

*Stille undersøkelsestekst, ingen fortellerstemme.*

**DESCRIPTION [H08_COMPARE_RECORDS_003]**: The records support a warning about a coordinated arrival. They do not identify who is coming.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": ["HR_SARO_ORIGINAL", "HR_HONDO_LOCAL", "HR_SITE11_LOCAL", "HR_NETWORK_CHECK"]}`

### H08_WARNING_ACK (field / phone / live)

**Utløser:** Kvittering først når Ward faktisk er nådd, Reyes har lest tre kilde-ID-er og første grensetid, og Ward har lest dem tilbake og ført dem i datert logg. 06:00-serien er allerede stanset. Ikke et tilfeldig tidsutløp; foto og negative blir hos spilleren.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_warning_acknowledged", "ctx_hr_remote_sources_verified"], "none": ["hr_h08_warning_ack_presented"]}

**WARD [H08_WARNING_ACK_001]**: The network warning is entered. The six o'clock series remains suspended after your earlier report. The other sites have read it back: nobody closes a reference or completes a run.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [H08_WARNING_ACK_003]**: Reading back: SARO zero-seven original run, Hondo local sheet, Site Eleven local sheet. First boundary, oh-three-fifty-six and eight seconds. That is in my dated log. The warning is outside SARO; keep the photographs and originals with their sheets.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [H08_WARNING_ACK_002]**: The field courier is on the old Roswell road, east of the mapped reference. Check the map at the diner before you agree a handover point.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*


**Ved fullføring:** `{"set_flags": {"hr_h08_warning_ack_presented": true}, "give_documents": ["HR_WARNING_RECEIPT"]}`

### H09_BRIEF (all_night / physical / live)

**Utløser:** H08 kartdel (tidligere H09): rolig diner etter kvittert varsel. Les E15, det innrammede utklippet fra juli 1947 som allerede henger ved døren. Legg E09A-tracingen på E14, transit på STATION 01 og nord mot nord. Behold P13 som reell kartoppgave. Dagens avis i byen er en annen, valgfri kilde. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h09_ready", "ctx_hr_warning_acknowledged"], "none": ["hr_h09_recorded", "hr_h09_brief_presented"]}

**DESCRIPTION [H09_BRIEF_001]**: The framed clipping by the door is the old one, July 1947. It is a photograph of a place, not a measurement.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [H09_BRIEF_002]**: Ward has the warning. What is left is the map: where C meets the old road, and where the courier can wait beyond it.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [H09_BRIEF_003]**: On the table the tracing is still loose. Place its transit on STATION 01, north to north. The framed newspaper clipping has no survey scale.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {"hr_h09_brief_presented": true}, "give_documents": []}`

### H09_CHECK (all_night / phone / live)

**Utløser:** H08 kartdel (tidligere H09): finn C ved 8.15-bolten og avtal separat 10.4-bolt med Ward. Sheriffbetjentens patruljeark er én kjøretur, ikke et generelt trygghetsbevis. Kravflagget må komme fra faktisk kontroll, aldri fra å åpne panelet.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h09_ready", "ctx_hr_h09_checked", "ctx_hr_warning_acknowledged", "ctx_hr_sc12_courier_104_confirmed"], "none": ["hr_h09_recorded"]}

**WARD [H09_CHECK_001]**: C is the bolt at eight point one five. The handover is ten point four. I have both marked.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [H09_CHECK_002]**: The old road is the direct approach to the courier. It is a practical route, not a finding that the reference is safe.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [H09_CHECK_003]**: Take the originals through. Stay in the cab, keep moving and take no reading at the bolt. If the truck falters before the crossing, turn back while you can.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*


**Ved fullføring:** `{"set_flags": {"hr_h09_recorded": true}, "give_documents": []}`

### H09_INCOMPLETE (all_night / ui / live)

**Utløser:** H08 kartdel (tidligere H09): E15 er et gammelt, tilgjengelig dinerutklipp og skal ikke ha 04:45-port. Leveringsregelen gjelder dagens avis i byen. Feil kartplassering får eksisterende karttips; bildet løser ikke C alene. Hvis varsling mangler, gi konkret telefonhandling før avreise. Vis bare ved et faktisk utilstrekkelig resultat.

**Regi:** Ingen feil-lyd eller skremmestikk. Bevar eksisterende funn og vis neste gjennomførbare handling. Settet markerer ikke oppdraget fullført.

**Krav:** {"all": ["ctx_hr_h09_ready", "ctx_hr_h09_insufficient"], "none": []}

**DESCRIPTION [H09_INCOMPLETE_001]**: C is not placed yet. Put the transit on STATION 01 first. Keep the old framed clipping beside the map; it establishes the photograph, not the crossing.

*Stille undersøkelsestekst, ingen fortellerstemme.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### H10_BRIEF (all_night / ui / live)

**Utløser:** H10: Gå til servicebilen og velg tydelig overleveringsoppdrag. Spillet lagrer et sikkert punkt ved avreise. Kjør den eksisterende veien fra dinerens mile 0 mot det avtalte 10.4-punktet, forbi C ved 8.15 uten planlagt stopp. 10.4 vises bare på kart og skilt; finalen ved C avbryter reisen. Varsling er allerede levert. Startteksten vises ved navngitt handling på stedet, én gang per relevant inngang.

**Regi:** Korte linjer med naturlige pauser. Ingen automatisk slutning utover det spilleren faktisk har kontrollert.

**Krav:** {"all": ["ctx_hr_h10_ready", "ctx_hr_warning_acknowledged", "ctx_hr_sc12_patrol_sheet_read", "ctx_hr_sc12_courier_104_confirmed"], "none": ["hr_h10_recorded", "hr_h10_brief_presented"]}

**DESCRIPTION [H10_BRIEF_001]**: The originals are in the cab. Ward has the warning, the three source IDs and the first boundary time.

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

The warning, three source IDs and the first boundary time (03:56:08) are read back and retained in Ward's dated call log outside SARO. The measurement tables, photographs, negatives and complete source sheets remain in the transfer folder. The courier has the field map. A handover point is to be agreed after the diner map and current route checks; this receipt does not confirm one.

Time of acknowledgement is entered when this call is actually completed. Do not preprint a successful acknowledgement before the line has been answered.

**Produksjon:** Ny forfatterkilde, ikke eksisterende runtimebevis. Dato/tid og måleverdier må bindes til faktisk hendelse i senere adapter. Dokumentet tildeles bare etter den fysiske handlingen; manglende fil godkjennes ikke som foto.

## unified_scenarios

### SC01_WINDOW_CHECK (early_shift / ui / live)

**Utløser:** Etter faktisk vindusobservasjon og kontroll fra trygg side. Foto er frivillig.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc01_window_seen", "ctx_hr_sc01_marks_checked"], "none": ["hr_sc01_window_check_presented"]}

**DESCRIPTION [SC01_WINDOW_CHECK_001]**: Something struck the outside of the glass. The marks are on that side. Switching the room lamp does not remove them.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [SC01_WINDOW_CHECK_002]**: Secure the known door. You can report what you saw without going outside after it.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_sc01_window_check_presented": true}, "give_documents": []}`

### SC01_FIRST_REPORT (early_shift / phone / live)

**Utløser:** Første melding etter faktisk hørt klask mot ruta og ekte kontakt/readback. Merker kan undersøkes separat senere; denne meldingen krever ingen etterforskningsrunde.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc01_window_seen", "ctx_hr_sc01_report_readback"], "none": ["hr_sc01_first_report_presented"]}

**DISPATCH [SC01_FIRST_REPORT_001]**: SARO control-room window. Something struck the outside of the glass and left marks. I have your time and location.

*Utsendt stedlig meldingsmottaker. Registrerer sted, tidspunkt, observerte forhold og egne faktiske utkallinger; kjenner ingen fremmed fasit.*

**DISPATCH [SC01_FIRST_REPORT_002]**: Stay behind a secured door if you can. Call us if the person enters the building. This report is open; I cannot say who was out there.

*Utsendt stedlig meldingsmottaker. Registrerer sted, tidspunkt, observerte forhold og egne faktiske utkallinger; kjenner ingen fremmed fasit.*


**Ved fullføring:** `{"set_flags": {"hr_sc01_first_report_presented": true}, "give_documents": ["HR_INITIAL_REPORT"]}`

### SC02_RETURN_WARNING (early_shift / ui / live)

**Utløser:** Bare mens ekte kropp/forvarsel og fysisk rute er etablert. Ingen fotoport.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc02_t01_complete", "ctx_hr_sc02_threat_visible"], "none": ["hr_sc02_return_warning_presented"]}

**DESCRIPTION [SC02_RETURN_WARNING_001]**: A pale body stands between the service front and the service truck. It turns towards a new sound. The opaque shed wall breaks the view.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [SC02_RETURN_WARNING_002]**: The route outside the fence goes around the shed. Watch the open crossing before you move. The high-voltage yard is not an exit.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_sc02_return_warning_presented": true}, "give_documents": []}`

### SC02_ESCAPE (early_shift / ui / live)

**Utløser:** Faktisk brutt kontakt/retrett; ingen videre pliktfangst.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc02_t01_complete", "ctx_hr_sc02_escape_complete"], "none": ["hr_sc02_escape_presented"]}

**DESCRIPTION [SC02_ESCAPE_001]**: The search has moved away from the route you used. The truck starts. Your completed lamp test is still on the service card.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [SC02_ESCAPE_002]**: Caliche has a working phone and people. Report the encounter and ask for someone to meet you.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_sc02_escape_presented": true}, "give_documents": ["HR_TRAFO_ENCOUNTER"]}`

### SC02_RETRY (early_shift / ui / live)

**Utløser:** Kun etter lokal fangst; ingen17-47min/gammel teller/etterspor.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc02_local_retry"], "none": []}

**DESCRIPTION [SC02_RETRY_001]**: Return to cover before the crossing. The lamp inspection is preserved. Read the search before trying the truck again.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### SC03_DISPATCH_RECEIPT (early_shift / phone / live)

**Utløser:** Faktisk sted-/tidsmelding og utkalling kvittert, ikke automatisk fra T01 eller å se skapning.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc02_escape_complete", "ctx_hr_sc03_dispatch_readback", "ctx_hr_sc03_ortiz_assigned"], "none": ["hr_sc03_dispatch_receipt_presented"]}

**DISPATCH [SC03_DISPATCH_RECEIPT_001]**: Caliche Junction service front. A tall pale person moved while you were watching and turned towards new sound. You reached your truck.

*Utsendt stedlig meldingsmottaker. Registrerer sted, tidspunkt, observerte forhold og egne faktiske utkallinger; kjenner ingen fremmed fasit.*

**DISPATCH [SC03_DISPATCH_RECEIPT_002]**: Dr. Ward has a relief technician coming to the Caliche fuel stop. Wait in view of the counter until the contact is identified.

*Utsendt stedlig meldingsmottaker. Registrerer sted, tidspunkt, observerte forhold og egne faktiske utkallinger; kjenner ingen fremmed fasit.*


**Ved fullføring:** `{"set_flags": {"hr_sc03_dispatch_receipt_presented": true}, "give_documents": ["HR_DISPATCH_RECEIPT"]}`

### SC03_RELIEF_CONTACT (early_shift / phone / live)

**Utløser:** Utkalling har faktisk egen person/vogn og avtalt møte. Ny identitet i forfatterkilden.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc03_dispatch_readback", "ctx_hr_sc03_ortiz_assigned"], "none": ["hr_sc03_relief_contact_presented"]}

**WARD [SC03_RELIEF_CONTACT_001]**: Mara Ortiz, district maintenance. White utility wagon, number twenty-three. She will meet you at the fuel station, in view of the counter.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [SC03_RELIEF_CONTACT_002]**: Tell her what you saw before you discuss equipment. She is there to bring help, not to send you back into a yard alone.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*


**Ved fullføring:** `{"set_flags": {"hr_sc03_relief_contact_presented": true}, "give_documents": []}`

### SC03_ORTIZ_IDENTIFY (early_shift / physical / live)

**Utløser:** Vanlig synlig ankomst før fare. Ortiz er vanlig og litt irritert over kjøreturen, viser ID og spør hva Reyes faktisk så. Ny hjelper, ikke Earl/sheriff/servitrise som fjernes.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc03_ortiz_assigned", "ctx_hr_sc03_ortiz_identified"], "none": ["hr_sc03_ortiz_identify_presented"]}

**ORTIZ [SC03_ORTIZ_IDENTIFY_001]**: Mara Ortiz, district maintenance. Wagon twenty-three. That's my card. Dr. Ward said you'd be by the pumps.

*Ny regional kveldshjelper. Servicetekniker i hvit vogn23; vanlig identifikasjon før synlig bortføring. Ingen eksisterende nødvendig NPC fjernes.*

**ORTIZ [SC03_ORTIZ_IDENTIFY_002]**: Forty minutes of washboard road to get out here. Fine. Tell me what you actually saw, not what you think it was.

*Ny regional kveldshjelper. Servicetekniker i hvit vogn23; vanlig identifikasjon før synlig bortføring. Ingen eksisterende nødvendig NPC fjernes.*

**ORTIZ [SC03_ORTIZ_IDENTIFY_003]**: All right. We write that down from here. Nobody needs you any closer to that yard tonight.

*Ny regional kveldshjelper. Servicetekniker i hvit vogn23; vanlig identifikasjon før synlig bortføring. Ingen eksisterende nødvendig NPC fjernes.*


**Ved fullføring:** `{"set_flags": {"hr_sc03_ortiz_identify_presented": true}, "give_documents": []}`

### SC03_HELPER_TAKEN (early_shift / ui / live)

**Utløser:** Observasjonstekst først etter virkelig synlig løft/forsvinning, de stille bleke skikkelsene i lyskanten og trygg retrett i stasjonen eller bilen. Lydtekst alene er ingen levert bortføring. Ingen redningsoppgave som later som.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc03_ortiz_identified", "ctx_hr_sc03_lift_observed", "ctx_hr_sc03_safe_retreat"], "none": ["hr_sc03_helper_taken_presented"]}

**DESCRIPTION [SC03_HELPER_TAKEN_001]**: Light struck wagon twenty-three. Ortiz rose clear of the ground with nothing under her, hung there a moment, and was gone. Her thermos rolled out across the asphalt.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [SC03_HELPER_TAKEN_002]**: At the edge of the light, several pale figures stood completely still. One of them turned its head towards you.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [SC03_HELPER_TAKEN_003]**: You are behind cover now. There was nothing you could have reached. Call it in from the working phone.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_sc03_helper_taken_presented": true}, "give_documents": []}`

### SC03_HELPER_TRACES (early_shift / ui / live)

**Utløser:** Undersøk vogna, termosen og merkene i grusen etter ro; valgfritt og ikke automatisk ved startet lys.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc03_safe_retreat", "ctx_hr_sc03_traces_checked"], "none": ["hr_sc03_helper_traces_presented"]}

**DESCRIPTION [SC03_HELPER_TRACES_001]**: Wagon twenty-three is empty, the driver's door open. Ortiz's thermos lies against the kerb with her name on a strip of tape.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [SC03_HELPER_TRACES_002]**: Narrow marks are pressed into the gravel by the wagon and lead nowhere. The wagon, the thermos and the marks show what remained. None of them explains where she went.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_sc03_helper_traces_presented": true}, "give_documents": ["HR_HELPER_TRACE_NOTE"]}`

### SC03_MISSING_RECEIPT (early_shift / phone / live)

**Utløser:** Reell separat savnetsak/readback. Ikke hevde full ettersporsundersøkelse eller lokal plansperre fra denne alene.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc03_lift_observed", "ctx_hr_sc03_safe_retreat", "ctx_hr_sc03_missing_readback", "ctx_hr_sc03_ortiz_identified", "ctx_hr_sc03_dispatch_readback"], "none": ["hr_sc03_missing_receipt_presented"]}

**DISPATCH [SC03_MISSING_RECEIPT_001]**: Mara Ortiz, district maintenance. Last seen at the Caliche fuel stop beside wagon twenty-three. You saw her lifted in a light and then could no longer see her.

*Utsendt stedlig meldingsmottaker. Registrerer sted, tidspunkt, observerte forhold og egne faktiske utkallinger; kjenner ingen fremmed fasit.*

**DISPATCH [SC03_MISSING_RECEIPT_002]**: That is now a missing-person report, linked to your earlier call. The deputy is on his way to you. Stay where you are and leave her things where they lie.

*Utsendt stedlig meldingsmottaker. Registrerer sted, tidspunkt, observerte forhold og egne faktiske utkallinger; kjenner ingen fremmed fasit.*


**Ved fullføring:** `{"set_flags": {"hr_sc03_missing_receipt_presented": true}, "give_documents": ["HR_MISSING_RECEIPT"]}`

### SC03_LOCAL_ORDER_HOLD (early_shift / phone / live)

**Utløser:** Faktisk Ward-ordre, uavhengig av fysisk betjening senere. Ingen nettverksvid ordre ennå.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc03_missing_readback", "ctx_hr_sc03_local_order_suspended"], "none": ["hr_sc03_local_order_hold_presented"]}

**WARD [SC03_LOCAL_ORDER_HOLD_001]**: The SARO six o'clock observer series is suspended. I have recorded that as a local order. Dispatch has Ortiz's case.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [SC03_LOCAL_ORDER_HOLD_002]**: Nobody is asking you to carry on a normal shift. Stay with the deputy and give him what the case needs.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*


**Ved fullføring:** `{"set_flags": {"hr_sc03_local_order_hold_presented": true}, "give_documents": ["HR_LOCAL_HOLD_RECEIPT"]}`

### SC03_RECEIVER_QUESTION (early_shift / physical / live)

**Utløser:** Sheriffbetjenten er faktisk kommet til bensinstasjonen etter savnetmeldingen. Han spør om SARO-mottakeren stod på klokka 21:00; den gjorde det. Dette er grunnen til returen i SC04, ikke en ordre om normal drift.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc03_missing_readback", "ctx_hr_sc03_deputy_at_stop"], "none": ["hr_sc03_receiver_question_presented"]}

**DEPUTY [SC03_RECEIVER_QUESTION_001]**: Your outfit up the road runs a receiver all night. Was it on at nine o'clock?

*Eksisterende sheriffbetjent ved Caliche-avdelingen (THE DEPUTY i FS47Caliche), revisjonens «sheriffen». Kort og praktisk; kjenner bare egne patruljer, patruljearket og savnetsaken. Tekst uten ny stemme, som hans eksisterende samtaler.*

**DESCRIPTION [SC03_RECEIVER_QUESTION_002]**: It was. The SARO receiver has been on passive monitoring since the shift began.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DEPUTY [SC03_RECEIVER_QUESTION_003]**: Then I want its log and the strip chart from nine for this report. You have the keys. I'll follow you up to the gate.

*Eksisterende sheriffbetjent ved Caliche-avdelingen (THE DEPUTY i FS47Caliche), revisjonens «sheriffen». Kort og praktisk; kjenner bare egne patruljer, patruljearket og savnetsaken. Tekst uten ny stemme, som hans eksisterende samtaler.*


**Ved fullføring:** `{"set_flags": {"hr_sc03_receiver_question_presented": true}, "give_documents": []}`

### SC04_LIMITED_RETURN (early_shift / physical / live)

**Utløser:** Ved SARO-porten etter at sheriffbetjenten har fulgt Reyes fra Caliche. Radioen kaller ham bort til Ortiz' vogn, og Reyes går inn alene for å hente bevis i en savnetsak. Ingen global avreisesperre.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc03_local_order_suspended", "ctx_hr_sc04_return_agreed"], "none": ["hr_sc04_limited_return_presented"]}

**DEPUTY [SC04_LIMITED_RETURN_001]**: That's my radio. They want me back at the wagon.

*Eksisterende sheriffbetjent ved Caliche-avdelingen (THE DEPUTY i FS47Caliche), revisjonens «sheriffen». Kort og praktisk; kjenner bare egne patruljer, patruljearket og savnetsaken. Tekst uten ny stemme, som hans eksisterende samtaler.*

**DEPUTY [SC04_LIMITED_RETURN_002]**: The receiver log and the strip chart from nine o'clock. Bring whatever else that case will need, and lock the door behind you. I'll be back up this road.

*Eksisterende sheriffbetjent ved Caliche-avdelingen (THE DEPUTY i FS47Caliche), revisjonens «sheriffen». Kort og praktisk; kjenner bare egne patruljer, patruljearket og savnetsaken. Tekst uten ny stemme, som hans eksisterende samtaler.*

**DESCRIPTION [SC04_LIMITED_RETURN_003]**: Inside: the receiver log and strip chart, the shift journal and the T01 card. Lock the automatic schedule so nothing runs while nobody is here.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_sc04_limited_return_presented": true}, "give_documents": []}`

### SC04_LOCAL_HOLD (early_shift / ui / live)

**Utløser:** Bare etter faktisk fysisk betjening/kontroll og sikrede originaler (mottakerlogg og stripskriver fra 21:00, skiftjournal og T01-journal). T01 arkiveres her én gang. Eget local hold-resultat.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc04_schedule_isolated", "ctx_hr_sc04_shift_originals_secured"], "none": ["hr_sc04_local_hold_presented"]}

**DESCRIPTION [SC04_LOCAL_HOLD_001]**: The local automatic schedule is isolated. The receiver still monitors passively, and a separately authorised service check remains possible.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [SC04_LOCAL_HOLD_002]**: The receiver log and strip chart from 21:00, the shift journal and the T01 card are secured for the missing-person report. T01 is filed here, once. Turning off a local schedule has not turned off the sky.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_sc04_local_hold_presented": true}, "give_documents": ["HR_LOCAL_ISOLATION_NOTE"]}`

### SC04_ANNEX_IMPRINT (early_shift / radio / fixed)

**Utløser:** Fast identisk fragment fra tidligere H01_BRIEF_002 med samme pause. Feil annexretning, ingen ny Dale-instruks eller fysisk Dale.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc04_schedule_isolated", "ctx_hr_sc04_annex_imprint_playing", "ctx_hr_sc01_dale_reference_heard"], "none": ["hr_sc04_annex_imprint_presented"]}

**DALE [SC04_ANNEX_IMPRINT_001]**: The camera is beside the east door. Take a reference before you touch the fault.

*Fiktiv kollega. Tørr, erfaren, menneskelig. Kun fjernstemme; ingen fysisk Dale eller VHS-forteller.*


**Ved fullføring:** `{"set_flags": {"hr_sc04_annex_imprint_presented": true}, "give_documents": []}`

### SC04_ANNEX_LONG_ESCAPE (early_shift / ui / live)

**Utløser:** Bare faktisk lang fluktlinje; ingen krav om avledning eller nytt pliktfoto.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc04_annex_long_escape"], "none": ["hr_sc04_annex_long_escape_presented"]}

**DESCRIPTION [SC04_ANNEX_LONG_ESCAPE_001]**: The solid wall broke the view. Behind the closed door, the search stayed at your last position while you used the longer passage.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [SC04_ANNEX_LONG_ESCAPE_002]**: Contact is broken and the search has withdrawn. The secured records are still with you.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_sc04_annex_long_escape_presented": true}, "give_documents": []}`

### SC04_ANNEX_SOUND_ESCAPE (early_shift / ui / live)

**Utløser:** Kort rute krever faktisk sanset lydreaksjon og gjennomført passasje. Ingen global usynlighet.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc04_annex_sound_escape"], "none": ["hr_sc04_annex_sound_escape_presented"]}

**DESCRIPTION [SC04_ANNEX_SOUND_ESCAPE_001]**: The ordinary radio drew the figure towards the new sound. From cover you saw it leave the shorter crossing.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [SC04_ANNEX_SOUND_ESCAPE_002]**: You crossed while the route was clear. Contact is broken and the search has withdrawn.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_sc04_annex_sound_escape_presented": true}, "give_documents": []}`

### SC04_ANNEX_RETRY (early_shift / ui / live)

**Utløser:** Lokal retry bevarer tidligere servicearbeid og papirer.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc04_annex_local_retry"], "none": []}

**DESCRIPTION [SC04_ANNEX_RETRY_001]**: The schedule and records remain secured. Return to the last covered point before the intrusion and choose a clear route.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### SC04_SAFE_ROOM_AGREEMENT (early_shift / phone / live)

**Utløser:** Avgrenset loggsikring i faktisk trygg situasjon etter annexflukt og ekte kontakt før kort fremoverovergang til 00:13:47; ingen tvangsordre om normal drift.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc04_control_room_safe", "ctx_hr_sc04_ward_on_line", "ctx_hr_sc04_schedule_isolated", "ctx_hr_sc04_shift_originals_secured"], "none": ["hr_sc04_safe_room_agreement_presented"]}

**WARD [SC04_SAFE_ROOM_AGREEMENT_001]**: I have you on the control-room line. Keep the door locked. The deputy is still at Ortiz's wagon; I am staying on this call.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [SC04_SAFE_ROOM_AGREEMENT_002]**: Read me the receiver log's heading and the nine o'clock entry. That is what the missing-person report needs. Leave the automatic schedule locked.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [SC04_SAFE_ROOM_AGREEMENT_003]**: Stay in cover until he can come back for you. Leave the receiver on passive. Nothing runs on its own tonight.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*


**Ved fullføring:** `{"set_flags": {"hr_sc04_safe_room_agreement_presented": true}, "give_documents": []}`

### SC05_PASSIVE_RESIDUAL (signal / ui / live)

**Utløser:** Faktisk passiv residual under fortsatt plansperre. Eksisterende mottaker/telefon47-sek-romkontroll må fortsatt gjøres.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc04_schedule_isolated", "ctx_hr_sc05_residual_passive"], "none": ["hr_sc05_passive_residual_presented"]}

**DESCRIPTION [SC05_PASSIVE_RESIDUAL_001]**: 00:13:47. A new residual appears in passive reception. The isolated automatic schedule has not resumed.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [SC05_PASSIVE_RESIDUAL_002]**: Use the existing receiver controls and preserve the print. The body at the junction and what took Ortiz are separate observations; this test asks what the signal retains.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_sc05_passive_residual_presented": true}, "give_documents": []}`

### BY02_REQUEST (archive / physical / live)

**Utløser:** Earl fysisk før04:00; datert arkivgrunn først. Ingen automatisk studioadgang.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_by02_dated_lead", "ctx_hr_by02_earl_present"], "none": ["hr_by02_request_presented"]}

**EARL [BY02_REQUEST_001]**: That date is on my off-air reel. Nineteen eighty-one. I kept the station log with it.

*Eksisterende1420AM-vert. Uttaler bare eget1981-materiales opphav og faktiske valgt bånd/live-kontroll. Fysisk replikk krever tilstedeværelse før04:00.*

**EARL [BY02_REQUEST_002]**: This is fourteen-twenty AM, the town station. Your fourteen-twenty-point-four-zero-five receiver is different equipment. Let us keep the sources straight.

*Eksisterende1420AM-vert. Uttaler bare eget1981-materiales opphav og faktiske valgt bånd/live-kontroll. Fysisk replikk krever tilstedeværelse før04:00.*


**Ved fullføring:** `{"set_flags": {"hr_by02_request_presented": true}, "give_documents": []}`

### BY02_REEL_IDENTIFY (archive / physical / live)

**Utløser:** Merking/kilde-/ruteopplegg faktisk tilgjengelig i offentlig lobby eller utlevering.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_by02_earl_present", "ctx_hr_by02_reel_identity"], "none": ["hr_by02_reel_identify_presented"]}

**EARL [BY02_REEL_IDENTIFY_001]**: The reel box, the off-air entry and my initials belong together. The transmitter was shut down; that does not by itself tell us what made the sound.

*Eksisterende1420AM-vert. Uttaler bare eget1981-materiales opphav og faktiske valgt bånd/live-kontroll. Fysisk replikk krever tilstedeværelse før04:00.*

**EARL [BY02_REEL_IDENTIFY_002]**: The marked source switch has two positions. Tape is this reel with the monitor feed disconnected. Live is tonight's monitor with the deck disconnected.

*Eksisterende1420AM-vert. Uttaler bare eget1981-materiales opphav og faktiske valgt bånd/live-kontroll. Fysisk replikk krever tilstedeværelse før04:00.*


**Ved fullføring:** `{"set_flags": {"hr_by02_reel_identify_presented": true}, "give_documents": []}`

### BY02_TAPE_CHECK (archive / ui / live)

**Utløser:** Fysisk isolert avspilling, ikke dialogvalg som måler signalveien.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_by02_reel_identity", "ctx_hr_by02_tape_isolated"], "none": ["hr_by02_tape_check_presented"]}

**DESCRIPTION [BY02_TAPE_CHECK_001]**: TAPE is selected. The live monitor is disconnected from the playback path. The dated reel supplies the old fragment.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [BY02_TAPE_CHECK_002]**: This establishes the source of this playback. It does not identify the source of the original sound or measure a bearing.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_by02_tape_check_presented": true}, "give_documents": []}`

### BY02_LIVE_CHECK (archive / ui / live)

**Utløser:** Ekte skilt live-monitor med bånd frakoblet; ingen kildekonklusjon fra valg alene.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_by02_reel_identity", "ctx_hr_by02_live_selected"], "none": ["hr_by02_live_check_presented"]}

**DESCRIPTION [BY02_LIVE_CHECK_001]**: LIVE MONITOR is selected. The reel deck is disconnected from this path. What you hear now belongs to tonight's monitor.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [BY02_LIVE_CHECK_002]**: A match in sound is not a shared source record. Keep tonight's monitor and the labelled reel separate.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_by02_live_check_presented": true}, "give_documents": []}`

### BY02_SOURCE_COMPARE (archive / ui / live)

**Utløser:** To lagrede fullførte kildeprøver og nytt spørsmål er sammenlignet. Aktuell TAPE-/LIVE-velger er gjensidig utelukkende; resultatet krever ikke at begge er valgt samtidig.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_by02_reel_identity", "ctx_hr_by02_question_compared", "ctx_hr_by02_tape_sample_verified", "ctx_hr_by02_live_sample_verified"], "none": ["hr_by02_source_compare_presented"]}

**DESCRIPTION [BY02_SOURCE_COMPARE_001]**: The isolated reel repeats the same old fragment after a new question. The recorded words have not answered it.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [BY02_SOURCE_COMPARE_002]**: The dated local reel is a separately preserved source. Record the source and timing. Direction still needs fixed landmarks and field observations.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_by02_source_compare_presented": true}, "give_documents": ["HR_EARL_SOURCE_CHECK"]}`

### BY02_LATE_COPY (archive / ui / live)

**Utløser:** Sen legitim datert kildekopi er identifisert mot konvolutt/logg/frakoblingsark, faktisk avspilt isolert på lobbydekket og nytt spørsmål sammenlignet med uendret fragment. Ingen Earl-replikk etter04:00. Bare metadata gir ingen lydkonklusjon.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_by02_dated_lead", "ctx_hr_by02_source_copy_verified", "ctx_hr_by02_question_compared"], "none": ["hr_by02_late_copy_presented"]}

**DESCRIPTION [BY02_LATE_COPY_001]**: Earl has left. In the lobby, a labelled envelope holds his dated source copy, the off-air log copy and the marked playback-disconnection sheet.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [BY02_LATE_COPY_002]**: You checked the copy on the isolated lobby deck. Its old fragment stays unchanged after your question. There is no new live reply from Earl.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [BY02_LATE_COPY_003]**: This is a checked copy of the identified source, not a claim to have handled the original reel. Keep that limit on the record.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_by02_late_copy_presented": true}, "give_documents": ["HR_EARL_SOURCE_COPY"]}`

### BY02_INCOMPLETE (archive / ui / live)

**Utløser:** Ingen konklusjon eller kildekvittering ved uavklart opphav.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_by02_source_insufficient"], "none": []}

**DESCRIPTION [BY02_INCOMPLETE_001]**: The source or signal path is not verified. Ask for the labelled off-air log and use the isolated playback position. A familiar sound alone is not the check.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### ARRAY_BRIEF (field / ui / live)

**Utløser:** Eget ordinært servicevedlegg fra EveningShift, ikke omskriving av kulvertindeks eller tre-bokser-gåte.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_array_service_card_acquired"], "none": ["hr_array_brief_presented"]}

**DESCRIPTION [ARRAY_BRIEF_001]**: The separate array service card marks A / ROW 1 WEST, B / ROW 1 EAST and REF / R1. It describes an ordinary comparison, not what the array will follow.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [ARRAY_BRIEF_002]**: Identify the nearby dishes and check their command history. Leave the reference lamp fixed. Choose either operator place first, then the other, then return briefly.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_array_brief_presented": true}, "give_documents": []}`

### ARRAY_COMMAND_CHECK (field / ui / live)

**Utløser:** Separat faktisk kommando-/drivkontroll; åpnet papir er utilstrekkelig.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_array_command_checked"], "none": ["hr_array_command_check_presented"]}

**DESCRIPTION [ARRAY_COMMAND_CHECK_001]**: The nearby dish identifiers match the local command and drive record you checked. The automatic schedule remains isolated; this is a separate bounded service check.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_array_command_check_presented": true}, "give_documents": []}`

### ARRAY_FIXED_REFERENCE (field / ui / live)

**Utløser:** Faktisk lampeplassering/innstilling i verdensrommet; ingen logger som måler peiling.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_array_reference_fixed"], "none": ["hr_array_fixed_reference_presented"]}

**DESCRIPTION [ARRAY_FIXED_REFERENCE_001]**: The reference lamp stays at REF / R1 at the same setting. Move yourself, not the lamp. Compare the dishes against the fixed Row One landmarks.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_array_fixed_reference_presented": true}, "give_documents": []}`

### ARRAY_POINT_A (field / ui / live)

**Utløser:** Faktisk A-posisjon/sikt/respons. Kan være første eller andre ståsted; ingen klokkeanimert fasit.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_array_command_checked", "ctx_hr_array_reference_fixed", "ctx_hr_array_a_observed"], "none": ["hr_array_point_a_presented"]}

**DESCRIPTION [ARRAY_POINT_A_001]**: At A / ROW 1 WEST, the observed dishes turn towards this operator position. REF / R1 and the fixed landmarks have not moved. Record what you actually see from here.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_array_point_a_presented": true}, "give_documents": []}`

### ARRAY_POINT_B (field / ui / live)

**Utløser:** Faktisk B-posisjon/sikt/respons, fritt A→B/B→A.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_array_command_checked", "ctx_hr_array_reference_fixed", "ctx_hr_array_b_observed"], "none": ["hr_array_point_b_presented"]}

**DESCRIPTION [ARRAY_POINT_B_001]**: At B / ROW 1 EAST, the observed dishes turn towards this operator position. The reference lamp is still at REF / R1. Compare their angles against the same landmarks.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_array_point_b_presented": true}, "give_documents": []}`

### ARRAY_RETURN (field / ui / live)

**Utløser:** Faktisk retur til det først valgte ståstedet; kontrollert forskjellig respons ved samme klokke.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_array_a_observed", "ctx_hr_array_b_observed", "ctx_hr_array_return_observed"], "none": ["hr_array_return_presented"]}

**DESCRIPTION [ARRAY_RETURN_001]**: On your short return to the first operator place, the dishes turn back towards it. The response follows your chosen route, not a fixed order of stations.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_array_return_presented": true}, "give_documents": []}`

### ARRAY_RESULT (field / ui / live)

**Utløser:** Styring/geometri adskilt fra lyd. Ingen løsning på C/Nora eller pliktfoto under senere fare.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_array_command_checked", "ctx_hr_array_reference_fixed", "ctx_hr_array_a_observed", "ctx_hr_array_b_observed", "ctx_hr_array_return_observed", "ctx_hr_array_geometry_compared", "ctx_hr_array_command_history_compared"], "none": ["hr_array_result_presented"]}

**DESCRIPTION [ARRAY_RESULT_001]**: The physical response followed the operator between two places and back while the reference lamp stayed fixed. The separate command record does not account for those changes.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*

**DESCRIPTION [ARRAY_RESULT_002]**: That is a result for this controlled path. It does not establish anyone's identity, a county-wide safe route or what happened to Tomás. STATION 01 records an older arrangement to examine next.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_array_result_presented": true}, "give_documents": ["HR_ARRAY_CONTROL_NOTE"]}`

### ARRAY_RETREAT (field / ui / live)

**Utløser:** Arrayets trusselmøte fysisk vellykket retrett. Ingen gammel Warning/Force eller ny stor STATION01-jakt.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_array_geometry_compared", "ctx_hr_array_escape_complete"], "none": ["hr_array_retreat_presented"]}

**DESCRIPTION [ARRAY_RETREAT_001]**: You used the clear passage and broke contact behind opaque cover. The searching figure has withdrawn. The completed comparison is preserved.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_array_retreat_presented": true}, "give_documents": []}`

### ARRAY_EMPTY_HOLD (field / ui / live)

**Utløser:** Bare faktisk observert tomt ståsted/retur026 etter riktig retrett; ikke gratis fra avslutning.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_array_escape_complete", "ctx_hr_array_empty_hold_observed"], "none": ["hr_array_empty_hold_presented"]}

**DESCRIPTION [ARRAY_EMPTY_HOLD_001]**: The dishes hold briefly on the empty place you left. Then they return to 026. Nobody is standing there now.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_array_empty_hold_presented": true}, "give_documents": []}`

### ARRAY_RETRY (field / ui / live)

**Utløser:** Lokal retrettretry, ingen gammel teller/tidstap/gjenganger.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_array_local_retry"], "none": []}

**DESCRIPTION [ARRAY_RETRY_001]**: The command, source and geometry checks are preserved. Resume from the covered point before the passage was searched; choose a clear retreat.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### ARRAY_INCOMPLETE (field / ui / live)

**Utløser:** Bevar faktiske delmålinger uten å fabrikkere samlet kontroll.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_array_insufficient"], "none": []}

**DESCRIPTION [ARRAY_INCOMPLETE_001]**: The comparison is incomplete. Keep the fixed lamp in place, check the other marked position and return to the first. Do not record a tracking result without the geometry and command checks.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {}, "give_documents": []}`

### SC12_PATROL_SHEET (all_night / physical / live)

**Utløser:** Etter 04:30 er sheriffbetjenten faktisk i dineren og har lagt sitt daterte patruljeark på disken; spilleren har lest det. Én kjøretur er begrenset veiopplysning, ingen trygghetsgaranti. Erstatter sjåførrollen.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_sc12_patrol_sheet_read"], "none": ["hr_sc12_patrol_sheet_presented"]}

**DEPUTY [SC12_PATROL_SHEET_001]**: Drove the loop at half past three. Nothing on the road. It's on the sheet.

*Eksisterende sheriffbetjent ved Caliche-avdelingen (THE DEPUTY i FS47Caliche), revisjonens «sheriffen». Kort og praktisk; kjenner bare egne patruljer, patruljearket og savnetsaken. Tekst uten ny stemme, som hans eksisterende samtaler.*

**DESCRIPTION [SC12_PATROL_SHEET_002]**: The patrol sheet is dated and timed in his hand: 03:30, north road and old Roswell road, no obstruction. One drive, an hour ago. It promises nothing about the road now.

*Engelsk undersøkelsestekst. Ingen diegetisk forteller eller Kinski-opplesning i denne pakken.*


**Ved fullføring:** `{"set_flags": {"hr_sc12_patrol_sheet_presented": true}, "give_documents": []}`

### SC12_ROUTE_READBACK (all_night / phone / live)

**Utløser:** Sheriffbetjentens daterte patruljeark og reell budkvittering trengs. Ingen 1985-kvittering eller gammel sjåførtur som fersk veiopplysning.

**Regi:** Kort, konkret og begrenset til faktisk kunnskap. Verden/hendelsesadapteren eier kravflagget. Ved pause, panel eller avbrudd gis ingen uopplevd konklusjon.

**Krav:** {"all": ["ctx_hr_warning_acknowledged", "ctx_hr_h09_checked", "ctx_hr_sc12_patrol_sheet_read", "ctx_hr_sc12_courier_104_confirmed"], "none": ["hr_sc12_route_readback_presented"]}

**WARD [SC12_ROUTE_READBACK_001]**: The deputy's sheet puts one clear drive on that road at half past three. I have its time in the log. It is one report, not a map of everywhere it is safe.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [SC12_ROUTE_READBACK_002]**: The courier is confirmed at bolt ten-point-four, beyond the mapped C crossing at eight-point-one-five. Carry the originals. No stopping or new reading at C.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*

**WARD [SC12_ROUTE_READBACK_003]**: The warning and the three source IDs are already outside SARO. You can stay at the diner; the handover keeps the originals together. It is not a new experiment.

*Eksisterende vakthavende. Må ikke slås sammen med Dale.*


**Ved fullføring:** `{"set_flags": {"hr_sc12_route_readback_presented": true}, "give_documents": ["HR_ROUTE_READBACK"]}`

### HR_INITIAL_REPORT: Initial window report

typed / SARO / NIGHT SHIFT / 13-14 APR 1986

CALICHE DISPATCH / INITIAL REPORT. SARO control-room window, outside face of glass. Reyes reports a moving hand or forearm. Identity not established. Exterior marks are a separate physical check and are not certified by this first call.

The incident time and caller's location are copied from the actual call log and read back. This report does not establish a relief dispatch or a missing person.

**Produksjon:** Adapter må sette faktisk in-game dato/tid/sted fra første meldingslogg. Ingen oppdiktet permanent klokkeslett.

### HR_TRAFO_ENCOUNTER: Caliche Junction encounter note

typed / SARO / NIGHT SHIFT / 13-14 APR 1986

The completed local lamp check remains recorded on its service card. Physical return filing at SARO is a separate action. At the service front a pale body moved under direct observation and turned towards new sound.

Reyes used the available route outside the fence, reached the service truck and broke contact. This note does not identify the body or explain the lamp fault.

**Produksjon:** Bare faktisk vellykket flukt. Lokal retry gir ikke denne før ny vellykket retrett.

### HR_DISPATCH_RECEIPT: District relief dispatch receipt

typed / SARO / NIGHT SHIFT / 13-14 APR 1986

CALICHE DISPATCH / RELIEF REQUEST. Reports at SARO and Caliche Junction linked under the current incident reference.

Regional technician Mara Ortiz, district wagon 23, assigned to meet Reyes at the public Caliche fuel stop. Actual dispatch time and contact readback attached.

This is an assignment record. It is not proof of arrival, abduction or a missing-person filing.

**Produksjon:** Reell utkalling har egen dato/tid/ID fra adapter. Ny person/vogn må produseres.

### HR_HELPER_TRACE_NOTE: Wagon 23: observed traces

typed / SARO / NIGHT SHIFT / 13-14 APR 1986

After reaching cover, Reyes checked the empty district wagon, the thermos on the asphalt and the marks in the gravel beside it. Tape on the thermos names Mara Ortiz.

The trace note records what remained. The separately witnessed lift and the subsequent missing-person report have their own records.

**Produksjon:** Faktiske navngitte rekvisitter (vogn, termos, merker i grusen) må finnes; ikke løft-credit fra bare objekter.

### HR_MISSING_RECEIPT: Mara Ortiz: missing-person receipt

typed / SARO / NIGHT SHIFT / 13-14 APR 1986

CALICHE DISPATCH / MISSING PERSON. Mara Ortiz, district maintenance, last seen at the public Caliche fuel stop beside wagon 23.

Reyes reports seeing Ortiz rise clear of support in an external light and then disappear. Dispatch has read back identity, place and actual report time, and linked the relief assignment.

Search assigned to the sheriff's office; the deputy is sent to the fuel stop. This report does not explain the disappearance.

**Produksjon:** Dynamisk hendelses-ID/dato/tid fra faktisk savnetmelding, adskilt fra utkallingskvittering.

### HR_LOCAL_HOLD_RECEIPT: SARO local morning-series hold

typed / SARO / NIGHT SHIFT / 13-14 APR 1986

DR. WARD / LOCAL WORK ORDER. SARO 06:00 observer series suspended after the Caliche missing-person report.

Dispatch owns the search for Mara Ortiz. Reyes is to remain at an established contact point. This order is local to SARO; network action requires later independent evidence.

Physical automatic-schedule isolation has its own check and must not be inferred from this order.

**Produksjon:** Bare faktisk Ward-ordre/readback. Ingen nettverkskvittering eller fysisk bryterstatus fra tekst alene.

### HR_LOCAL_ISOLATION_NOTE: Local automatic schedule: isolated

typed / SARO / NIGHT SHIFT / 13-14 APR 1986

The physical auto/survey schedule has been placed in its marked isolated state and checked locally. Passive reception remains available.

Secured for the missing-person report: the receiver log and strip chart covering 21:00, the dated shift journal and the T01 service card. T01 is filed with them, once. A separately authorised bounded array check remains possible; unattended operation stays suspended.

**Produksjon:** Ekte betjening/kontroll og papirhenting. T01-arkiveringen skjer her én gang (EveningShifts eksisterende filing). Ingen avbryting av all strøm eller K3 som global bryter.

### HR_EARL_SOURCE_CHECK: 1420 AM: identified original-source check

typed / SARO / NIGHT SHIFT / 13-14 APR 1986

Identified 1981 off-air reel and separately preserved local log examined at 1420 AM. Source switch checked in isolated TAPE and LIVE MONITOR paths; the unused input was disconnected in each test.

The reel repeated the same fragment after a new question. The result establishes this playback's dated source and fixed content. It does not measure a bearing or identify the original sound.

1420 AM is distinct from the SARO receiver at 1420.405 MHz. Field geometry needs operator positions and fixed landmarks.

**Produksjon:** Originalkontroll før Earl går04:00. Målinger stammer fra fysisk isolert opplegg; ingen Earl-forklaring etter avgang.

### HR_EARL_SOURCE_COPY: 1420 AM: checked dated source copy

typed / SARO / NIGHT SHIFT / 13-14 APR 1986

Earl's labelled collection envelope contains a copy identified to the 1981 off-air reel, its dated local log and a playback/disconnection sheet.

After Earl left, Reyes used the isolated lobby deck and checked the copy's fixed fragment against a new question. This is a verified copy-source path, not an examination of the original reel.

No bearing or present location follows from the audio. Earl did not answer after his departure.

**Produksjon:** Sen legitim kopi krever utleveringsrekvisitt/merking/deck/kildekontroll i runtime. Ingen gratis positivt faktum ved sen klokke.

### HR_ARRAY_CONTROL_NOTE: Array service comparison

typed / SARO / NIGHT SHIFT / 13-14 APR 1986

Separate command and drive record checked for the identified Row One near group. Fixed reference lamp left at REF / R1 at unchanged setting. The ordinary service card is EV_DALE_ARRAY_SERVICE; T02's culvert index remains separate.

Operator chose A / ROW 1 WEST or B / ROW 1 EAST first, moved to the other and briefly returned. Observed dish angles changed with those physical positions against fixed landmarks; the lamp stayed at REF / R1.

The selected route, local timings and actual observations belong in the field record. Audio supports sequence and timing, not bearing. This check does not establish identity, C or Nora's motive.

**Produksjon:** Servicekort/fysiske ståsteder eies av EveningShift/arrayadapter. Dynamiske navn/rute-/vinkeldata må fylles fra faktisk feltresultat.

### HR_ROUTE_READBACK: Old Roswell road: route and courier readback

typed / SARO / NIGHT SHIFT / 13-14 APR 1986

Caliche deputy's dated patrol sheet: north road and old Roswell road driven at 03:30, no obstruction recorded. Ward entered its time in her log. One drive is not proof of future safe passage.

After mapping C at bolt 8.15, Reyes agreed a courier handover at bolt 10.4 beyond it. Plan: carry the originals, pass C without stopping or taking another reading.

The warning, three source IDs and the first boundary time are already read back outside SARO. The diner remains a refuge; the handover keeps the source chain and is not another observer experiment.

**Produksjon:** Adapter må binde faktisk dato/tid fra patruljearket, kilde-ID-er og bud til avskrift; ingen gammel bergingskvittering eller sjåførtur som fersk opplysning.
