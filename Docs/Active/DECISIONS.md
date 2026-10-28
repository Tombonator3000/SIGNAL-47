# Gjeldende beslutninger

**Status:** Gjeldende. **Gjelder:** Retning og arbeidsregler for SIGNAL / 47. **Sist kontrollert:** 28. oktober 2026.

Dette dokumentet samler beslutninger som ellers ligger spredt i historiske notater. Det dokumenterer føringer, ikke at funksjonene er ferdig implementert. Se [STATUS](STATUS.md) for levering og [TODO](TODO.md) for arbeidet som gjenstår.

## Prosjekt og kanon

| ID | Beslutning | Grunnlag |
| --- | --- | --- |
| D-01 | Videre spillarbeid gjelder Unreal-utgaven. Web og Unity bevares som historikk og referanse. | Toms beslutning 22. oktober, [rotens AGENTS](../../AGENTS.md) og [Unreal-reglene](../../unreal/AGENTS.md). |
| D-02 | Spillet er kosmisk etterforskning ved SARO i New Mexico, 1986. Bevar 1420.405 MHz, 47 sekunder, 27 antenner og azimut 026. Spilltekst er engelsk, prosjektdokumentasjon norsk. | Gjeldende AGENTS og historisk kanonreferanse. |
| D-03 | Faktisk kode og siste relevante test-/leveringskvittering avgjør hva som er implementert. Designbilder, manus og en main-merge er separate fra pakket, installert og publisert spill. | Gjeldende arbeidsregler og [STATUS](STATUS.md). |
| D-04 | Lisensierte MetaHuman-/Fab-kilder og avledede private assets holdes utenfor offentlig Git. Bevar egne redigerbare kilder, proveniens og kreditering. | [Unreal-reglene](../../unreal/AGENTS.md) og de enkelte assetrapportene. |

## Historie: hvilke planer styrer?

Nyere eksplisitte beslutninger går foran eldre planer for den delen de endrer. De erstatter ikke automatisk alle andre krav.

1. **Toms siste direkte instrukser** gjelder først.
2. **[Ny kveldsrekkefølge, 28. oktober](../Design/Story/KVELD_PLAN_2026-10-28.md)** styrer den endrede forbindelsen mellom SC01, signal/strøm, trafotur og SC02/SC03. Den senere mørke trafoturen og enklere SC01 overstyrer eldre rekkefølge der de er uenige.
3. **[SC02 til SC13-revisjonen](../Design/Story/HISTORIE_SC02_SC13_REVISJON.md)** styrer revidert dramaturgi og fire aktleveranser der den avviker fra den eldre planen. Ny talende sjåfør i SC12 og diktering av måletabeller er kuttet; H08/H09 er slått sammen. Dette er ikke en beskjed om å fjerne den eksisterende bakgrunnssjåføren i diner.
4. **[Implementeringsplanen](../Design/Story/HISTORIE_IMPLEMENTERINGSPLAN.md)** fører scenario-/verdenskontrakter og produksjonsgrunnlag under revisjonene ovenfor. Den gamle regelen om full separat levering per scenario er erstattet av fire sammenhengende akter og det reduserte testregimet.
5. **[MISSION_FLOW](../Design/Story/MISSION_FLOW_1800_ROSWELL.md)** og eldre detaljvedlegg brukes for kontrollverdier, bevis og delscener der nyere beslutninger ikke sier noe annet.

De engelske forfatterkildene i [Narrative/HorrorStory](../../unreal/Narrative/HorrorStory/README.md) er produksjonsgrunnlag. En skrevet scene er ikke en ferdig verdenshandling. Gamle lagrede saker skal ikke få retroaktive krav fra nye innholdsrevisjoner.

## Nye føringer fra spillprøven

| ID | Beslutning | Konsekvens |
| --- | --- | --- |
| D-05 | Signalet rundt 00:13 skal utløse strømsvikten; trafoturen skjer senere i mørket, og første egentlige SC02-møte flyttes mot kl. 02. | Kvelds-/strøm-/reisekoordinatoren og relevante tekster må samordnes. Dette er pågående arbeid, ikke ferdig levering. |
| D-06 | SC01 skal bruke klask mot ruta og avtrykk, med enklere lampekontroll. | Ikke viderefør den tydelige hånden eller unødvendige turer inn/ut som fremtidig fasit. |
| D-07 | Rolig kveldsarbeid og uro skal fylle tiden før midnatt; nye hovedhendelser må passe den endrede rekkefølgen. | Kveldens oppgaver og SKY01 skal gi sammenheng uten tidlige alienmøter eller ventetid som innhold. |
| D-08 | Godkjent terreng/himmel, eksisterende interaksjoner og personlige lagringer bevares under visuelle forbedringer. Nye konkrete spillerfunn er åpne selv om en eldre leveranse bestod tester. | Følg [feilrapporten](Reports/PLAYTEST_TODO_2026-10-28.md); ikke lukk nye feil med gamle matriser. |
| D-09 | Første personpass er MetaHumans som stille stående/sittende mannekenger på de eksisterende stedene. | Radio-kvinne, røykende mann og diner-sjåfør får poserte statiske erstatninger. Bevar dagens servitrise og alle samhandlings-/tilstedeværelsesregler. [Produksjonskontrakt](../../unreal/Signal47/Tools/metahuman/mannequins-plan.json). |
| D-10 | Nanite utvides etter innhold og faktisk nytte, særlig tette solide miljømodeller. | Ingen global konvertering av vegetasjon, glass eller effekter; bevar RT-fallback og collision. [Kildegjennomgang](../Research/Unreal/UNREAL_SKILLS_2026-10-28.md). |
| D-11 | Den ønskede visuelle retningen er et troverdig retro New Mexico-motell og sammenhengende 1986-farger gjennom dag/natt. | Bevar keramikkmønster, mørkt trepanel, KELVORA-TV og de tre detaljerte bilene. Bare den dårlige blokkbilmodellen skulle bort. Fargeforsøk må passe dagens `M_WebTone`. |
| D-12 | Finalen skal vurderes med en stor, langsom UFO-avsløring bak to skybanker som velter fremover fra hver sin side. | SC13 er anbefalt hovedplassering. Konsepter er regi, ikke implementert effekt. [Finalenotat](../Design/Proposals/FINALE_CLOUD_TANK_UFO.md). |

## Arbeid, testing og levering

- **Én aktiv status og todo:** Oppdater [STATUS](STATUS.md) og [TODO](TODO.md) ved relevante endringer. Nye leveringsnotater skal beskrive én leveranse og lenke hit for dagens status, fremfor å opprette konkurrerende «current»-dokumenter.
- **Arbeidsdeling:** [OPPGAVER](../../unreal/OPPGAVER.md) fører hvem som eier filene og den pågående jobben. `log.md` er append-only historikk, ikke en ekstra statusfasit. Bruk avgrensede subagenter aktivt med tydelige filansvar.
- **Test etter risiko:** Raske relevante kontroller og én målrettet prøve av det som er endret. Dokumentasjon trenger ikke motorbygg. Ikke fullmatrise per liten modell, menyfane eller PR. En nødvendig samlet kontroll før installasjon/offentlig release kjøres én gang; gjenta bare ved nye relevante endringer, feil eller konkret uavklart risiko.
- **Én tung jobb:** Blender, Unreal og pakking kjøres sekvensielt gjennom prosjektets vern. Ingen tester samtidig med Toms spilløkt. Ingen fremmed prosess stoppes for å skaffe testvindu.
- **Vanlig Git-levering:** Spill-PR-er går mot `main`; kontroller riktig head og relevante sjekker før merge og bekreft faktisk resultat. Gitt autorisasjon til vanlig PR/main-merge gir ikke automatisk autorisasjon til en ny offentlig spillrelease.
- **Lokal installasjon:** Bevar personlige Saved, testet payload og kontrollert tilbakekopi. Git-merge, lokal installasjon og offentlig nedlasting føres hver for seg.
- **Lagringsplass:** Behold gjeldende verifiserte testbygg og maksimalt to eldre komplette testbygg. Installerte standardfiler og aktive nødvendige kandidater vernes; kanonisk prune bevarer kilder, assets, Saved, logger og bevis.

Uavklarte eldre notater skal merkes og henvises til gjeldende beslutning. De slettes ikke og blir ikke automatisk gjort til nye oppgaver bare fordi et gammelt avkrysningsfelt står åpent.
