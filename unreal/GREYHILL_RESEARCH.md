# Greyhill Incident som grunnlag for jaktmekanikk i Signal / 47

[Detaljert oppfølging med kode](GREYHILL_DOOR_CODE_ANALYSIS.md) viser dørpassering, lyd, kollisjon og lagring, med egen C++-referansemodell og presis teststatus.

Statisk analyse, 28. oktober 2026. Se også [videre analyser](GREYHILL_NEXT_ANALYSES.md) og [offentlig kontrollsammendrag](Research/Greyhill/analysis-summary.json).

## Resultat

Den lokale Steam-utgaven er undersøkt direkte. 110 Blueprint-filer og hovedkartet Game_Level er hentet ut og dekodet. Til sammen inneholder de 963 funksjonsoppføringer. Antallet inkluderer hendelser, delegatsignaturer og små funksjoner som bare går videre til hovedgrafen. Det er ikke 963 selvstendige gameplay-systemer.

Alle 111 filene består UAssetAPI sin byteidentiske rekonstruksjon i minnet. Beregnet bytekodestørrelse samsvarer med den deklarerte størrelsen i alle 963 funksjoner. Dette gir godt bevis for at filene og bytekoden er lest riktig. Det beviser ikke hvordan hele spillet oppleves eller at hver kodebane brukes under spilling.

Dette er dekoding og disassemblering av cooked Unreal Blueprint-logikk, ikke en full gjenoppbyggbar C++-dekompilering av Windows-programmet. Den native Shipping.exe er ikke dekompilert. Ingen prosentandel av hele spillets kode er fastslått.

Anbefalingen for Signal 47 er å bruke jaktens struktur: oppdage, forfølge, miste kontakt, søke og trekke seg tilbake. Signal 47 har allerede en lokal designrevisjon med fire slike møter. Denne leveransen gir bevis og en konkret tilpasning til dem; den endrer ikke spillkoden.

## 1. KILDE OG METODE

Spill: Greyhill Incident, Steam AppID 1905020, installert build 12751524.
Pakkefil: Greyhill_Incident-WindowsNoEditor.pak, med 20 163 oppføringer.
Motor: EngineAssociation 4.27 i den utpakkede .uproject-filen.

Biblioteket ble funnet som Tombonator3000/prosjektbibliotek. START_HER.md, SOK.md, prosjektbibliotek-skillen, decompile-skillen og profilen for decomp.wtf ble lest. Originalen og forken har identisk decompile-skill ved den festede revisjonen.

decomp.wtf-skillen er laget rundt matching med opprinnelig kompilator. Den festede README-en beskriver PC fra scratch som kommende støtte, mens eksisterende offentlige dekompileringsprosjekter har en annen inngang. Originalprosjektet er arkivert. Det er derfor ikke dokumentert som en ferdig importvei for denne moderne PC-utgaven. Programmet og WSL ble ikke installert.

I stedet er skillens krav om konkrete funksjoner og bytekontroll tilpasset Unreal: installert UnrealPak 5.8 for liste/utpakking, UAssetAPI 1.1.0 for lesing av UE 4.27-pakker og Kismet-bytecode, og en liten egen leser for indekser, instruksjonsadresser og tekstvisning. UAssetAPI ble hentet fra NuGet. Verktøyet kjører med den allerede installerte .NET-runtime-en fra Unreal. Ingen spillfiler ble skrevet tilbake.

Funn-ID-ene viser til detaljbevis i det private, lokale uttrekket. Repoets analysis-summary.json inneholder bare eget kontrollsammendrag og kontrollsummer. Bytekode, full dekodet JSON, binærfiler og private maskinstier publiseres ikke. Den lokale tekstvisningen er et instruksjonsutdrag med goto og flow-stack, ikke kompilérbar C++ eller en bevist rekonstruksjon av editorens opprinnelige noder.

## 2. SLIK VIRKER ALIEN-JAKTEN

Syn og hørsel [AI-01, AI-02]
Alien_AI_BP bruker PawnSensing til syn og AIPerception med AISenseConfig_Hearing til lyd. Synskomponenten har SightRadius 1650, SensingInterval 0,1 sekund og PeripheralVisionAngle 60. Dette er klassens serialiserte standardverdier. Radius tilsvarer nominelt 16,5 meter ved vanlig Unreal-skala; faktisk rekkevidde er ikke målt i kjøring.

En viktig detalj: vellykket hørselsstimulus fra BP_Player går inn i en felles bane som setter CAN_SEE_PLAYER til true og kan starte forfølgelse. Dette er ikke bevis for et avansert system som bare søker ved lydens sted. Vi bør gjøre dette mer presist i Signal 47: lyd skal gi et undersøkelsespunkt, mens syn gir bekreftet spillerkontakt.

Forfølgelse og minne [AI-03, AI-04]
Når spilleren sees, lagrer AI-en LAST_SEEN_PLAYER_POSITION. En retriggerbar forsinkelse på 0,7 sekund nullstiller synsflagg når synshendelsene opphører. Tapt-kontakt-banen har deretter en forsinkelse på 1 sekund før bevegelse mot den lagrede posisjonen. Forfølgelse bruker MoveTo med TARGET_ACTOR. Ved ankomst til søkepunktet kan aktøren spille en se-seg-rundt- eller skanneanimasjon.

Dette er en lesbar grunnstruktur: spilleren bryter sikt, fienden går til siste observerte sted og undersøker. Signal 47 bør bevare denne årsakskjeden og la spilleren se hva fienden leter etter.

Skjulesteder [AI-05]
Enter_Object_BP samler flere typer skjulesteder og kjøretøy i samme klasse. AI-en kan bruke objektet som TARGET_ACTOR og gå til en egen AlienPosition. Ved fangst finnes en bane som plasserer både spilleren og alien ved objektets markører før fangstsekvensen.

Dette viser at et skjulested ikke nødvendigvis avslutter jakten. Men Signal 47 bør bare la fienden undersøke et bestemt skjulested når den faktisk har sett inngangen eller fått et lokalt spor. Ikke kopier automatisk kjennskap til spillerens gjemmested eller synlig teleportering til fangstmarkører.

Lys [AI-06]
Det finnes funksjoner kalt LookForLight og Flashlight_Caught. Den undersøkte LookForLight-banen gjør en BoxTrace 700 enheter fra head, med volum 70 x 70 x 50, og kan starte jakt hvis sporingen treffer spilleren. I denne banen er det ikke funnet en eksplisitt måling av lommelyktens styrke eller et krav om at lommelykten er på. Navnet alene beviser derfor ikke et realistisk lysdeteksjonssystem.

Signal 47 kan bruke lommelykten som en tydelig risiko, men dette må implementeres og testes som egen synsregel. Ikke presenter det som en direkte bevist kopi av Greyhills lysmodell.

Vandring og skriptede oppgaver [AI-07]
AI-en kan velge et tilfeldig tilgjengelig navigasjonspunkt innen WALK_AROUND_RADIUS. Den har også HAS_SCRIPTED_TASK og et eget SCRIPTED_TASK_TARGET. Det gjør det mulig å kombinere forfattet plassering og inngang med fri lokal jakt.

For Signal 47: regissøren velger når og hvor et møte starter. Selve fienden løser syn, lyd og navigasjon etter de samme reglene i alle fire møter.

## 3. TALL OG ANDRE GAMEPLAY-LOOPER

Bevegelse [LOOP-03]
Spiller, standardverdier: gange 200, sprint 440, huking 150.
Alien, navngitte standardverdier: normal gange 70, fast walk 270, run 300.
Roaming-radius: 3000.
Tallene er Unreal-enheter. Forholdet mellom fartene er mer nyttig enn å overføre tallene direkte. Klassen setter faktisk MaxWalkSpeed fra FAST_WALK_SPEED i jaktbanen. RUN_WALK_SPEED er funnet som verdi, men skal ikke uten videre omtales som brukt i alle jakter.

Lyd og dekning [LOOP-01, LOOP-02]
Spillerskritt sender ReportNoiseEvent med Loudness 0,001 ved huking og 0,6 ellers, og MaxRange-parameter 1330. Dette er inngangsverdier til lydsansingen, ikke en måling av effektiv høreavstand. Lydvolumet spilleren hører er også skilt fra AI-stimulus.

CORN_ZONE setter IS_IN_CORNFIELD på spilleren og alien-aktørene. Begge får ekstra kornlyd ved fottrinn. Den undersøkte soneklassen alene beviser ikke at kornet blokkerer syn. Lærdommen er at underlaget kan gjøre motstanderens bevegelse hørbar. I Signal 47 kan grus, metalltrinn og porter gi slik informasjon uten å innføre huking eller et ekstra terrengsystem.

Utholdenhet og forsvar [LOOP-03, LOOP-05]
Spillerklassen har STAMINA/MAX_STAMINA 105, en utladningsverdi 100 og lading 50 som standard. Sprint trekker fra delta-tid ganger utladning. Andre baner endrer lading og utholdenhet, så en enkel beregnet sprintvarighet er ikke en verifisert spillmåling. Revolveren spawner prosjektil, rapporterer en lydhendelse, trekker patroner og har cooldown. Baseballbat og grep bruker også egne begrensninger.

Dette gir knapphetsvalg i Greyhill, men Signal 47s nåværende retning trenger ikke pistol, ammunisjonsjakt eller ny utholdenhetsøkonomi. Vi kan skape kostnad gjennom tid ute av dekning, rutevalg og det å måtte stoppe og arbeide under press.

Fangst og frigjøring [LOOP-04]
Et frigjøringssystem øker BREAK_FREE_RATE ved input og reduserer den over tid med BreakFree_AlienForce. Player_Breaked_Free øker PlayerCatchedCount; en bane gir PlayerDead ved teller >=4. Det finnes også en egen reset-hendelse. Ikke tolk dette som fire liv gjennom hele spillet uten å følge alle tilbakestillinger.

Signal 47 bør beholde allerede planlagte, korte fangstoverganger med bevarte bevis og uten tidstap i nye saker. Gjentatt tastemosing vil trekke oppmerksomheten fra undersøkelse og flukt.

Regi og oppgaver [LOOP-06, WORLD-01]
Scripted_JumpScares_BP bruker overlap, en Triggered?-sperre, lyd, forsinkelse og animasjon. Hovedkartet har mange trigger- og dispatcher-styrte hendelser for samtaler, bortføringer, gjenstander og kapittelprogresjon. SaveGame/LoadGame bevarer blant annet alien-synlighet, død, helse og oppgave-/dørflagg.

Det overførbare er en konkret rytme: en vanlig praktisk oppgave, et tydelig varsel, et aktivt møte, en unnslippelse og et fysisk etterspor. Ikke anta at hvert skriptet Greyhill-møte er fri AI. Skill regissørens hendelser fra jaktens simulerte regler.

## 4. KONKRET TILPASNING TIL SIGNAL / 47

Grunnlaget er den lokale HISTORIE_SC02_SC13_REVISJON.md fra 28. oktober 2026, sammenholdt med OPPGAVER.md, memory.md og todo.md. Den sier at fire jaktsekvenser beholdes: SC02 ved trafoen, SC04 i annexet, SC08 i arrayet og SC11 ved SARO. Huking og den ekstra biljakten er kuttet. Eldre kommentarer om fem møter og egen huking er derfor ikke lagt til grunn her. Dette er en kontroll av den lokale prosjektkopien, ikke en erklæring om andre aktive chatters eller fjern-greners leveringsstatus.

Felles jaktmodell, foreslått egen implementering:

Vandring -> Undersøk lyd -> Søk -> Tilbaketrekning
                    |          |
                    +-> Bekreftet syn -> Jakt
                                           |
                                 mistet sikt -> Søk ved siste spor
                                           |
                                      nær kontakt -> Fangst

Nytt syn eller en ny hørbar hendelse kan avbryte søk. Regissøren begrenser møtet til det aktuelle området. Tilbaketrekning skjer på en troverdig rute eller utenfor spillerens sikt, ikke som synlig teleportering.

Behold egne variabler for siste synssted, siste lydsted, tidspunkt og kildetype. Fienden får ikke lese spillerens nåværende posisjon når kontakten er brutt. Lyden kan ha et avvik i posisjon; et apparat kan lokke den bort uten at spilleren selv blir sporet. Opake vegger og lukkede dører må bryte syn. Et vindu må følge den avtalte synsregelen, uavhengig av at det stopper bevegelse.

SC02, trafoen: lær reglene på ett sted
Et skritt i grusen gir en observerbar hodevending. Syn kan starte jakt. Vestlig og nordlig/østlig rute må begge kunne føre til bilen. Et opakt skap eller en vegg bryter syn; fienden går til siste spor. Fangst gir kort omstart ved skuret med T01 bevart og uendret klokke. Dette er den beste første produksjonsbiten fordi alle grunnreglene kan prøves i ett avgrenset møte.

SC04, annexet: bruk lyd med hensikt
En kjent dør kjøper tid. En eksisterende lydkilde kan slås på fra dekning; fienden går mot lydens sted. Spilleren velger lang rute bak vegger eller en kort rute når søket er lest. Mottakerlogg, T01 og sikrede originaler overlever fangst og lasting. Den gamle biljakten gjeninnføres ikke.

SC08, arrayet: bevar kontrollen før jakt
Valgfri rekkefølge A/B og kort retur skal fortsatt fungere. Antennene følger Reyes uavhengig av fiendens syn og navigasjon; de to årsakskjedene skal ikke blandes i én skjult trigger. Etter prøven kommer aktøren mellom fundamentene. Spilleren velger skjermet omvei eller kort kryssing. Søkeminne gjør fundamentene til funksjonell dekning.

SC11, SARO: samme regler under større press
Fienden avsøker bygningen med samme syns- og lydregler. Forskjellen er innganger, ruter og oppgaven spilleren må fullføre. Eksisterende opplyste rom, elektrisk utstyr og dører gir lesbare valg. Ingen ny nøkkel- eller sikringsjakt trengs under angrepet. Regissøren hindrer at tilfeldige bortføringer og andre låsende sekvenser overlapper møtet.

## 5. IMPLEMENTERINGSKONTRAKT

Første prioritet er et gjennomgående SC02-forløp med egen alienressurs, navigasjon, syn, lyd, kort jakt, mistet kontakt, søk, fangst og korrekt gjenopptak. Det er et forslag til neste implementering, ikke kode som er levert her.

Bruk eksisterende prosjektkoordinator for å eie hvilket møte som er aktivt, og et separat jaktobjekt for persepsjon, bevegelse og minne. Eksisterende interaksjonslyder bør sende en enkel hendelse med sted, styrke og årsak. Ingen nye motorer, innlimte Greyhill-Blueprints eller avhengighet av Greyhill ved runtime er nødvendig.

Persepsjon bør ha en avgrenset oppdateringsfrekvens, for eksempel 10 Hz som første prøveverdi. Dette er vårt forslag, ikke en verifisert ytelsesgrense. Unngå Greyhill-lignende mange GetAllActorsOfClass-oppslag og ekstremt korte Delay-løkker i jaktens varme bane; behold direkte referanser til relevante aktører og ruter.

Lagringskontrakten må dekke bevis, oppgaveflagg, dørtilstander og aktivt møte. Ved lasting kan fienden gjenopprettes på en kontrollert, nåbar posisjon uten å gi en umiddelbar, uunngåelig fangst. Ingen personlig lagring skal brukes som testprofil.

Bygget må prøves med ekte input i minst disse tilfellene:
## 1. Syn får kontakt på åpen rute, men ikke gjennom opak vegg.
## 2. En lyd bak veggen gir søk ved lydstedet, uten løpende sporing av skjult spiller.
## 3. Mistet kontakt stanser oppdatering av spillerens posisjon i AI-minnet.
## 4. Spilleren kan komme forbi søket etter å ha skiftet rute.
## 5. Begge planlagte fluktruter er nåbare, også når døren er lukket.
## 6. Stengt dør stopper eller forsinker fienden etter scenens regel; path-fail gir ingen teleportering.
## 7. Fangst bevarer T01/originaler og gir ikke tidstap i nye saker.
## 8. Pause og lasting midt i søk gir ingen doble triggere eller umiddelbar fangst.
## 9. Tilfeldig bortføring starter ikke inne i en låsende jaktsekvens.
10. SC08s A/B-prøve fungerer i begge rekkefølger før jakten.
11. Oppgaven kan fullføres uten en ny kamp-, ammunisjons- eller hukemekanikk.
12. En faktisk spiller kan forstå når fienden ser, hører og har mistet kontakten, med spillbildet og lyden alene.

Ingen av disse Signal 47-runtime-prøvene er utført i denne analyseleveransen.

## 6. LEVERANSE OG GRENSER

PASS: pakkeliste, selektiv utpakking, parsing av 111 filer, 963 bytekodestørrelser og 111 byteidentiske rekonstruksjoner i minnet.
PASS: statiske funn med navngitte egenskaper, funksjoner og bytekodeadresser i evidensfilen.
UNVERIFIED: faktisk Greyhill-spillopplevelse, alle grenene i kjøring, effektiv høreavstand, lysmodell og balansering.
IKKE UTFØRT: full native C++-dekompilering, gjenbygging av hele spillet, import av Greyhill-ressurser til Signal 47 og implementering av jakt i Signal 47.

Rapporten kan brukes som implementeringsgrunnlag. Det komplette uttrekket er bevart separat og lokalt utenfor repoet. Det offentlige repoet inneholder egen analyse og et kontrollsammendrag, ikke spillets kode, tekstur-/modellkilder eller full dekodet JSON. En egen implementering av reglene kan lages i prosjektets eksisterende Unreal-kode.

## Kilder
[Prosjektbibliotek-skill](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/skills/prosjektbibliotek/SKILL.md)
[Biblioteksøk](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/SOK.md)
[Decompile-skill](https://github.com/Tombonator3000/decomp.wtf/blob/575b4a30ed481ff5497dfb9ce4f599368a95a04a/plugins/decomp-wtf/skills/decompile/SKILL.md)
[Bibliotekets verktøyprofil](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/repoer/decomp-wtf-2026-10-16.md)
[UAssetAPI](https://github.com/atenfyr/UAssetAPI)
[UAssetAPI-bruk](https://atenfyr.github.io/UAssetAPI/guide/basic.html)
[UAssetAPI-rekonstruksjonskontroll](https://github.com/atenfyr/UAssetAPI/blob/7353081/UAssetAPI/UAsset.cs)
Signal 47-grunnlag: [historierevisjonen](HISTORIE_SC02_SC13_REVISJON.md).
Primærbevis: brukerens installerte pakke, dekodede egenskaper og bytekode, bevart privat. [Offentlig kontrollsammendrag](Research/Greyhill/analysis-summary.json) oppgir SHA-256 og deknings-/valideringsstatus.
