# Integrasjon av kveldens manus i Unreal

Dette er arbeidskontrakten for runtime. Innkjøring, overlevering og tidslinje er koblet inn i `S47Evening`, `S47EveningPack` og `S47Timeline`; senere scener har datadrevne fase- og resultatporter. Se `../../EVENING_RUNTIME.md` for status og bevis. Scene-ID-ene er forfatter-ID-er; de er ikke navn på funksjoner som allerede finnes i motoren. Gjenbruk prosjektets kjøring, samtaler, dokumenter, TV-avspilling og lagring. Spilltekst kommer fra modulenes engelske strenger.

## Rekkefølge og fasegrenser

| Fase | Inngang | Avslutning og tilstand |
|---|---|---|
| `commute` | Ny sak begynner på eksisterende vei inn mot byen, før skiftet. | I11_SARO_ARRIVAL krever faktisk parkering ved SARO. Sett intro_arrived og start skiftklokka 18:00 én gang. |
| `handover` | Telefonen ringer når spilleren er ved stasjonen, etter ankomst. | D01_BRIEF fullfører telefonen, setter handover_done/intro_completed og gir overleveringsdokumentene. |
| `early_shift` | Vanlig drift og kameratrening. | N01_ROUTINE bruker det medbrakte materialet. T01 får egen avstikker og verdensbasert kontroll. |
| `signal` | Den nye tidslinjens faktiske residual 00:13:47 finnes. | D21 er valgfri, N01_COMPARE er aktiv etterprøving. Uavklart konklusjon er gyldig. |
| `archive` | Arkivoppdraget og rapportene er åpnet på vanlig vis. | D02 finner reservenøkkel, åpner kontoret og gir et feltspor. D31 er valgfri telefonoppfølging. |
| `field` | STATION 01 og trafostasjonens oppfølging er tilgjengelig. | T02 bruker faktiske kontroller. Den erstatter ikke Nora, P13 eller C-kryssets utledning. |
| `all_night` | Hovedspillets ch5 er virkelig aktiv, rundt 04:10 i den nye tidsplanen. | Én av to sene dinerhilsener. A02/P01 er aktive byspor. Eksisterende kartgåte og uttrykkelig veivalg leder videre. |

Ingen manusnode setter ch5=`road`, starter ch6 eller løser P13. En tidlig dinerankomst bruker egne tilbud og replikker. Bare den eksisterende avsluttende veihandlingen skal starte siste kjøretur.

Innkjøringen har ikke en skiftklokke som kan gå forbi 18 og rulles tilbake. Vis sen ettermiddag uten minutt-HUD, lagre innledningsfasen og gå tydelig over til skiftstart ved SARO. Ikke la lasting eller gjentatt ankomst gi flere overleveringer. Reisekjøretøyet parkeres separat fra SAROs servicepickup.

## Dialog, avbrudd og mat

Scener med `entry=trigger` tilbys av et navngitt faktisk triggerpunkt. `entry=choice` kan bare nås gjennom et gyldig valg; de skal ikke tilbys globalt fordi deres lokale flaggkrav er tomme. Før et valg brukes, kontrolleres både valgets og målscenens krav.

Lagre aktiv scene-ID, neste linje og eventuelt valgt svar. For en meny med valg er ingen generell on_complete-effekt ment å kjøres før spilleren har valgt og nådd en avslutningsnode. Tildel dokumenter etter ID, slik at samme dokument ikke dupliseres ved gjenopptak.

Matkjøpet settes i menyvalgets on_select. Denne endringen og den ventende svarscenen må lagres sammen, ellers kan avbrudd mellom betaling og servering skape mat uten respons eller dobbelt kjøp. Vis faktisk tallerken/kopp eller innpakket mat i den aktuelle svarscenen. I06/I07 avslutter spise-/ta-med-valget. Forlatelse uten kjøp er gyldig. Ingen hovedhistoriegjenstand krever matkjøp.

Radio avbrytes/dempes ved utstigning og viktige samtaler, og fortsetter fra lagret posisjon i riktig mottaker. Musikk skal få plass mellom vertens korte innslag. Ingen skremmelyd tvinges inn i første hyggelige dinerbesøk.

## Dale og stemmene

Dale er aldri fysisk i scenen. D01 foregår på telefon, D11/D21/D31/D51 på passende ledig landlinje, D12/D41 på walkie og D61 som fast fragment i radio. Han vet bare hva spilleren har meldt. Stemmens kanalfilter skapes av den faktiske telefon-/radioavspillingen; spill inn et rent grunnopptak, slik at samme take kan brukes riktig i ulike kanaler.

`pack.json` bestemmer stemmeidentitet og tillatte kanaler. Ny Dale-dialog følger karakterstemmeinnstillingen og egne klipp, ikke den generelle `kinskiReadAloud`-/`bKinskiCalls`-koblingen som i dag også berører dokumentopplesning. Vanlig lokalradiovert er ikke Earl eller Kinski. Ward og Nora skal ikke få Dale-stemme av en generell fallback.

D61_FIXED_FRAGMENT_001 gjenbruker nøyaktig D01_HANDOVER_001. `recording_aliases` er en cue-til-opptak-kobling; ikke lag en ny take eller nye ord til dette avtrykket. Fragmentet er enveis, har ingen svarvalg og gir ingen bekreftelse på at den levende Dale er på linjen.

Nye replikker må produseres med eksisterende godkjent Kinski-profil og de øvrige karakterprofilene. Planlagt leveranseformat er 48 kHz PCM16 mono, med ren start/slutt og uten innbakt kanalfilter. Varighet, faktisk fil og endelig underteksttiming fylles fra de ferdige opptakene; de skal ikke gjettes fra antall ord. `recording_manifest.json` er en arbeidsliste, ikke et ferdig radioVoice-indeksformat. Import/adapter må eksplisitt bygge prosjektets faktiske katalogoppslag.

## Kontoret og VHS

Bruk den eksisterende `officeDoor`-interaksjonen i S47Chapter2 som utgangspunkt. D02_SERVICE_DRAWER gir en merket reservenøkkel; D02_OPEN_OFFICE krever denne. Dales hovednøkkel kan fortsatt være med ham hjemme. Ingen fysisk Dale bak døren. Nøkkelen er adskilt fra Noras STATION 01-nøkkel.

D02_TAKE_FIELD_FOLDER er den korte hovedruten gjennom rommet: EV_DALE_INDEX og EV_DALE_NOTE gir et konkret sted å kontrollere. Utklipp, kontaktkort og hele VHS er fordypning. Hele mediet skal ikke kreves for neste feltoppgave. Overleveringsmappen ligger allerede på SARO; den hyggelige innkjøringen blir ikke et skjult innsamlingskrav.

VHS_DALE_FIELD_01 er 72 sekunders planlagt film. Produser den som en bevegelig scene med synlig referansestolpe, kamerabevegelse/lysforandring, lyd og et tvetydig avvik. Miljølyd og skriftlige observasjoner brukes; ingen fysisk Dale eller Kinski-forteller. Unarius-filmene kan være offentlig materiale i samlingen, men er ikke dette egne feltopptaket.

Koble TV/VHS til samme dokumenterte film-/lydklokke- og strømmønster som KELVORA. `office_tape_started` er ikke ferdig sett film. Først faktisk avspilling forbi den relevante bevegelsen og frem til komplett-merket kan utløse D02_FIELD_TAPE_COMPLETE. Ved strømbrudd eller pause fryses tidsposisjonen og lyden; ved lasting gjenopptas den. Kontaktark kan beholdes som eget bevis uten å erstatte filmen med et stillbilde.

## T01, T02 og eksterne resultatflagg

Eksterne `ctx_*`-flagg settes bare av adapteren ut fra faktiske kapittel-/verdenstilstander. De kan ikke skrives av et dialogvalg. Se beskrivelsene i pack.json.

- `ctx_trafo_service_ok`: den fullførte T01-kontrollen og den synlige lyskilden er virkelig enige. Først da kan T01_NORMAL_RESULT gi det normale inspeksjonskortet.
- `ctx_trafo_isolation_verified`: den avgrensede, fiktive servicekontrollen for T02 er faktisk utført.
- `ctx_trafo_backup_excluded`: den normale nødforsyningsforklaringen er faktisk kontrollert.
- `ctx_trafo_indicator_checked`: en separat sammenligning har kontrollert indikatoren.
- `ctx_trafo_lamp_persists`: lyskilden fortsetter sitt observerbare forløp etter kontrollene.

T02s bekreftede konklusjon tilbys bare når alle fire siste betingelser er sanne. «Unresolved» lukker panelet uten å blokkere STATION 01/Nora-historien. Ikke bind «reparer strøm» til å fjerne det kosmiske fenomenet. T01s klage gjelder en inspeksjonslampe; ikke utvid dette ukritisk til en påvist byomfattende strømstans.

Dato og tid på inspeksjonskort settes når den faktiske kontrollen er ferdig. EV_SERVICE_LOG er en datert kopi fra før natten, ikke en kontinuerlig måling av hele byen. N01 kan derfor vise at årsak ikke er etablert; den gamle kopien kan ikke bevise hvilken strømtilstand byen hadde 00:13.

## Lagring og kompatibilitet

Bevar aktiv innledningsfase, valgt bestilling/ventende svar, intro_arrived, handover_done, alle oppdagede bevis-ID-er, dør-/nøkkeltilstand, VHS-posisjon og utførte kontrollresultater. Gamle fotografier beholder sitt faktiske innhold og tidspunkt.

Den nye tidslinjen får en eksplisitt versjon. Eksisterende saker beholder sine gamle 02:13-referanser eller migreres uttrykkelig; ikke endre gamle utskrifter eller bildestempler i stillhet. 1947-opptaket beholder egen tid. Finalen er fortsatt 05:29:00 til 05:29:47. Gamle saker uten intro kan få datert referansemateriale uten å bli sendt tilbake til matstoppen.

## Nødvendig produksjon

| Leveranse | Konkret arbeid | Ferdigbevis |
|---|---|---|
| Lyd | 109 unike takes, kanalmiks, vanlig kveldsvert og lystig eksisterende/egen musikk. | Lytting i faktisk mottaker, riktige undertekster og pause/avbrudd. |
| Innledning | Kjørbar eksisterende vei, reisebil, dinerbestilling og parkering ved SARO. | Både direkte rute og matstopp med ekte input og lasting. |
| Kontor | Dørrom, serviceskuff/nøkkel, filer, TV/VHS og synlige dokumenter. | Kort hovedrute og valgfri fordypning uten softlock. |
| Film | VHS-manusets syv bevegelige deler og miljølyd. | To ulike tidspunkter viser faktisk bevegelse og korrekt avspillingsposisjon. |
| Trafostasjon | Avstikker, serviceområde, to oppgaver, faktiske testresultater og inspeksjonskort. | T01/T02 kan skilles og lagres. |
| Adapter | Rollebaserte samtaler, fase-/datoporter og eksisterende dokument-/lagringssystem. | Kontrollscenariene i SCENARIOS.md og berørt regresjon. |

Ingen ny fysisk Dale-modell inngår i leveransen eller skal bestilles på grunnlag av eldre planutkast.
