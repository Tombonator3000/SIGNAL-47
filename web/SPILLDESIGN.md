# SPILLDESIGN.md: designråd for SIGNAL / 47

Status 4. oktober 2026: en rådgivende gjennomgang, laget etter «advisor»-prinsippet fra Claude Code-dokumentasjonen (https://code.claude.com/docs/en/advisor): en sterkere modell leser hele sammenhengen ved et viktig veiskille og gir råd som utvikleren tar stilling til. Gjennomgangen ble gjort av en egen agent på den sterkeste modellen, med historien, designbibelen, koden og Toms ønsker som grunnlag. Rådene er ikke kanon før de står i `memory.md` som beslutning.

**Oppdatert 4. oktober kveld:** Tom har lagt om historien og slutten (se `HISTORIE.md`). Det endrer tre ting her. Kapittel 5 og 6 finnes ikke lenger, så det som står om K5 under, gjelder ikke. Rådet om bilstoppet (tapt tid, ingen skikkelse) er erstattet av THE EVENT, der Tom har valgt et ansikt som kan minne om en Grey i omtrent tre bilder. Og walkie-talkien og lysene over motellet bør etter den nye slutten droppes helt, fordi de bruker opp finalen før den kommer.

Tre råd går imot det Tom har bedt om, og venter på Tom:

- Walkie-talkien: rådet er at den bare tar inn fragmentet igjen, uten blendende lys og blackout i rom 6. Tom ba om lys, fremmede lyder og at spilleren våkner senere (som i «Signs»).
- Lysene over motellet: rådet er ett rolig lys over mesaen i sør, ikke tre punkter over motellet.
- Albumet: rådet er at frie bilder framkalles på våtbenken som vanlige kopier, ikke polaroider, og at kameraet ikke får REC og dato (det er et filmkamera).

Resten bygges i rekkefølgen under, med bevisbordet v2 først.

Skrevet 4. oktober 2026 etter AGENTS, HISTORIE, memory, todo, designbibelen, WorldCase22, Station26 og koden for prologen, kapittel 1 til 4, `Panels.ts` og `Board.ts`. Beslutninger, ikke en meny.

## Kort diagnose

Det som virker: prologen lærer reglene med hendene (bank 3, kalibrering, utskriften, telefonen, 47 sekunder). Fotokjeden er ekte, bildet er et bevis som ikke kan jukses med (R4). Natten går som én klokke uten sluttskjermer. Kapittel 4 bruker spillerens valg fra kapittel 1 (passiv eller aktiv) i Noras første spørsmål, og det er spillets beste enkeltidé. Bordet i kapittel 2 har en regeltabell som er lett å utvide.

De fem største risikoene:

1. **Fire måter å si «dette tror jeg» på.** Kapittel 1: klikk i bildet pluss knapper (`referenceFile`, `twoExposures`). Kapittel 2: bordet. Kapittel 3: fanepanelet `fieldRecord` med tre knapper per side, altså nøyaktig det Tom kalte knotete. Kapittel 4: chips og tre setninger. Spilleren lærer grensesnittet på nytt hvert kapittel, og bordet risikerer å bli en femte variant som bare finnes i arkivet.
2. **Tre setninger med én riktig lekker svaret.** Lokkesvarene («author-guilt», «47 minutes later», «T. Vega, answering her now») er alltid den som tar for hardt i, og feil koster bare en teller. En utålmodig spiller prøver alle tre. På bordet er det 17 linjer og 11 regler, så også der går det an å prøve par til noe holder.
3. **Bordets grammatikk har tre ledd.** Tråd linje til linje gir en lapp, lappen må dras til spørsmålet, og P05 krever i tillegg at en stedslinje nåles fast. Meldingen «Connect two lines first, then pin the note» forteller at spilleren allerede er forvirret. Jeg er uenig i dette oppsettet, se under.
4. **Telefon.** Bordet er 1180 × 760 skalert ned til minst 0,7. I 844 × 390 blir det 826 × 532 i en 390 høy flate: tekst på 9 til 10 px, og en tråd fra øverst til nederst krever rulling midt i et drag med pekerfangst. `.bline` har 5 px polstring, for lite for en finger.
5. **Spillet konkluderer for spilleren.** Notatene (`g.note`) gjentar det spilleren nettopp gjorde og legger til tolkningen («That is a changed report, not a motive»). Bibelens del 02 sier at notatboken skal huske, ikke konkludere. Og verden mellom funnene er stille: toasts er fortelleren, og uroen fra prologen bæres ikke videre av lyd og lys i kapittel 2 til 4.

## Kjerneloopen

**Se noe på et instrument, fest det på film eller papir, legg det ved siden av et annet spor, si hva de to viser sammen, og gjør den handlingen i verden som følger av det.** Observer, dokumenter, sammenlign, konkluder, handle. Hvert kapittel gjør alle fem, og «si hva de viser sammen» skjer alltid på samme bord.

- **Prolog:** observer (mottakeren), dokumenter (utskriften), sammenlign (telefonen mot smellet). Konklusjonen holdes bevisst tilbake. Riktig.
- **Kapittel 1:** avviker. Sammenligningen er delt på to paneler med knapper. Merkingen i bildet er en ekte observasjon og blir, men «hvilken referanse» og «hva viser de to bildene» flyttes til bordet: kortene FRAME 01, FRAME 02, S-03-loggen og installasjonsarket.
- **Kapittel 2:** på bordet. Observasjonen er svak (bare å plukke papir), men telefonen til Nora er en god handling.
- **Kapittel 3:** avviker mest. P06, P08 og P09 flyttes til bordet (kort: E09A, FRAME 03, FRAME 04, E10, mottakeren i hytta). P07 forblir fysisk: dekk lampa, observer, og resultatet dukker opp som en lapp på bordet av seg selv.
- **Kapittel 4:** samtalen er handlingen og forblir samtale. Men svar gis med bevis, ikke setninger: når Nora spør hvorfor, legger spilleren rettelsen eller feltkortet på bordet. Mild og kritisk tone beholdes, det er tone, ikke funn. Lokkesetningene i P11 og P12 fjernes.
- **Kapittel 5:** P14, «legg A, B og C sammen», er bordets høydepunkt: tre kort trådet i en trekant over sakskartet.

## Bevisbordet

**Et kort** viser stempel (E07), tittel, et bilde hvis det finnes, høyst fire linjer, og TEXT for hele dokumentet. Kort spilleren ikke har, står grå med «hvor det ligger» (behold).

**En tråd** betyr én ting: «disse to linjene sier noe om hverandre». Det er en påstand med ett av tre svar: holder (rød, blir stående, gir en lapp), motsier (grå, henger i tre sekunder med Unity-svaret, telles som feil), eller tom («sier ingenting om hverandre», tråden faller, teller ikke). Kontekst-tråder uten lapp (E07-hodet til E08) tegnes tynnere, så bakgrunn og funn ser forskjellige ut.

**Funn registreres slik:** en lapp som holder, hopper selv inn i spørsmålet den hører til. Spilleren drar ikke lapper. Når alle plassene er fylt, lyser RECORD på spørsmålskortet. Ett trykk fører funnet i journalen med klikkelyden. Det er den bevisste konklusjonen, uten en tredje dragtype. P05s stedslinje blir en tredje plass som fylles av en tråd mellom «SITE REGISTER» og «Field destination».

**Hint i tre nivåer** per spørsmål, som i bibelen: spørsmålet, retningen, neste grep. Dagens fjerde nivå («Thread the three references in E07 to...») er fasiten. Lås det til etter nivå tre pluss to nye feil. Etter tre tomme tråder på rad pulserer Hint én gang. Hinttekst føres aldri i journalen.

**Skalering:** samme bord hele natten. Nytt kapittel legger til kort og spørsmål. Forrige kapittels kort faller sammen til en bunke FILED i venstre kant som kan trekkes ut. Besvarte spørsmål står som RECORDED. I kapittel 4 tegnes trådene for P10 til P12 automatisk når samtalen registrerer dem, så bordet er hele bildet før kapittel 5. Posisjoner lagres i saken (`moved` forsvinner ved lasting).

**Journalen** får faner: Oppgaver, Notater, Bord, Papirer, Bilder. Notater kuttes til én linje i spillerens egne ord, uten tolkning. Bordet åpnes fra journalen og fra ethvert arbeidsbord.

**Telefon (under 700 px høyde):** ikke fri plassering. Kortene i én kolonne, spørsmålet fast øverst, kobling med trykk-så-trykk (finnes, men må sies på skjermen), tråder som korte merker. Linjer minst 44 px høye.

**Feller:** pikseljakt (hele linjeraden er mål), prøv-alle-par (få linjer, meningsfulle lokkesvar, ikke straff), uleselig på telefon, lapper som dekker kort, og hint som staver paret.

## Stemning og etterbehandling

R1 betyr at verden er ærlig: ingenting overnaturlig utenfor A, B og C. Uroen må komme fra stillhet og fra at verden lytter, ikke svarer. Ørkennatt i april gir tørt lyn langt borte, coyoter mellom 03 og 04, en hund ved motellet, kompressoren på dineren. Ingen av dem reagerer på spilleren.

Bibelen sier «ingen flimrende fullskjermfilter som standard». Jeg justerer: mild gradering og korn som standard, jitter og sporingslinjer av. Innstilling av, mild, full; av i Low. Ett fullskjermspass i three.js, kornet flyttes fra CSS inn i passet.

Mild nivå:

- Oppløsning 0,85 i High, adaptivt som i dag.
- Gradering: svartnivå 0,03, skygger mot grønnblått, varme høylys, lett S-kurve. Full: dobbelt.
- Glød: terskel 0,8, styrke 0,25, liten radius. Bare natriumlys, CRT og neon.
- Kromatisk forskyvning 1,5 px i ytterste 20 prosent.
- Korn 0,06. Full: 0,12 med 1 px jitter ved 2 Hz, av ved «reduced motion».
- Camcorder: bare med søkeren oppe. Men feltkameraet er et filmkamera med våtbenk, så REC og dato hører ikke hjemme der. Gi søkeren et 1986 SLR-blikk: delt prisme, lysmålernål, bildeteller. VHS-følelsen ligger på hele natten («et bånd noen fant»), aldri på selve fotografiet (R4).
- Fotsteg etter sone med dagens tre filer, filtrert: betong rått, stålrist med etterklang, grus med støy over, teppe lavpass, asfalt dempet. Torden finnes (`audio.thunder`): to til tre ganger per kapittel, styrke 0,3 til 0,5, aldri nærmere. Coyote som ett nytt opptak under 0,2.
- Stillhet: tre sekunder uten toast etter hvert funn. Kutt toasts i kapittel 2 til 4 med en tredel.
- Lys: færre og varmere. Lyset over mesaen er ett rolig punkt uten stråler, ikke større enn en lys stjerne.

## Toms idéer vurdert

- **Walkie-talkien (Signs): bygg med endring.** Radioen i skoesken, ja. Slått på tar den inn bærebølgen og det samme fragmentet (R5). Men trykk-for-å-snakke som gir blendende farger og en spiller som våkner senere, er en tilfeldig overnaturlig hendelse i rom 6, et sted uten referanse. Det bryter R1 og R5 og bruker opp tapt tid før veien. Endring: trykket sender et klikk ut, og det samme fragmentet kommer tilbake. Nora: «He pressed it too. Twice.» Valgfritt etter P12.
- **TV med sus: bygg.** Billig, riktig for 1986, og det blå flimmeret er det beste lyset rommet kan få. Den reagerer på ingenting, det er poenget.
- **Tre lys over motellet: bygg med endring.** Tre punkter over motellet er et fenomen uten referanse, og det stjeler veiens «første gang noen andre ser noe». A, B og C er fastmerker, ikke himmelobjekter. Bygg ett rolig lys lavt over mesaen i sør, synlig fra plassen for den som ser den veien, fotograferbart, bildetekst «Light over the mesa, 04:50». Samme lys som står der når bilen dør.
- **Fritt kamera og album: bygg.** Fra kameraet er hentet. Ikke polaroidramme: laben er en våtbenk. Frie bilder ligger på rullen og framkalles neste gang spilleren står ved benken, høyst tolv per rull. Bildetekst fra det spillet vet: sted, klokke, objektnavn hvis `inFrame` traff noe. Saksbildene beholder nummer, de frie er unummererte.
- **Journal med faner: bygg med endring.** Oppgaver, Notater, Bord, Papirer, Bilder nå. Personer, Steder og Signaler: vent. Signaler blir en side med fire oppføringer i K5.
- **Bilen stopper: bygg med endring, bare på C.** Motoren dør, lysene i 4/7, radioen tar bærebølgen, lyset over mesaen, dashbordklokka. Hvitt i to til tre sekunder. Så: klokka viser 05:19 i stedet for 05:08, bilen står vendt mot SARO, motoren går, og kameraets teller har gått ett frem. Framkalt viser bildet veien og lyset fra et punkt spilleren aldri sto på (instrumentene beholder spor, som i 1947, og R4 holder). Ingen skikkelse, heller ikke i motlys: det leses som romvesen og lukker spørsmålet bibelen vil holde åpent. Tapt tid brukes én gang i spillet, her. Autolagring før bolten.
- **Hånd-ikon: bygg.** Hånd på det som kan brukes, prikk ellers. På telefon vises hånden på Use-knappen.
- **Voices of the Void:** signalbibliotek: vent, som Signaler-siden i K5. Vedlikeholdsturer: ikke, natten er seks timer og R1 forbyr fyllinnhold. Basen som hjem finnes allerede (kruset, Dales logg, automaten).
- **Contact:** allerede i lyden. Ett grep til i K5: med hodetelefonene på faller all annen lyd bort mens avbruddet velges.

## Rekkefølge

1. **Bordet v2:** lapper hopper selv inn i spørsmålet, RECORD-knapp, tynne kontekst-tråder, kolonne på telefon. Ferdig når en førstegangsspiller på telefon registrerer P04 og P05 uten nivå fire og uten å spørre hva som skal dras hvor.
2. **Journal med fem faner og korte notater.** Ferdig når en spiller finner FRAME 02 og E07 på under ti sekunder når Nora ber om dem.
3. **Kapittel 3 på bordet** (P06, P08, P09), P07 fysisk, fanepanelet slettes. Ferdig når `chapter3.py` går og en tester forklarer hvorfor A er flyttet uten hint.
4. **Kapittel 4 svarer med bevis**, TV-sus, hånd-ikon. Ferdig når en tester gjengir hva Nora gjorde og hvorfor med egne ord, og aldri fikk et feilsvar hun ikke forsto.
5. **Etterbehandling og lydlag.** Ferdig når Low holder fps på Toms telefon, og en tester nevner stemningen uoppfordret uten å spørre om bildet er ødelagt.
6. **Fritt kamera og album.** Ferdig når en tester fotograferer noe ingen ba om, finner det i albumet med fornuftig tekst, og saksbildene fortsatt har nummer.
7. **Roswell-veien og stoppet på C**, etter Toms ja til tapt tid. Ferdig når en tester etterpå sier «noe skjedde der den gamle linja krysser» uten å ha hørt ordet abduksjon, og ingen så et romvesen.
8. **Dineren og Wards telefon.** Ferdig når testeren vil tilbake til SARO før seks uten å bli bedt om det.

## Spilltest

Etter hvert kapittel, uten å hjelpe:

1. Hva tror du skjedde klokka 02:13, og hva betyr -39 LY? (Fanger «tidskode»-lesingen.)
2. Hvor skal du nå, og hvorfor dit?
3. Hva viste de to bildene, med dine egne ord?
4. Hva ble endret i 1947-rapporten, og vet du hvem og hvorfor? (Riktig etter kapittel 2: «hvem, ikke hvorfor».)
5. Hvilke tråder trakk du som ikke holdt, og skjønte du hvorfor?
6. Hva var C?
7. Hvor sto du lengst uten å vite hva du skulle gjøre, og hva trykket du på der?
8. Hva var det mest ubehagelige i natt, og skjedde det på skjermen eller i hodet ditt?
