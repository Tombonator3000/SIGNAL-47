# Arbeidsmal for bygninger i Blender og Unreal

**Status:** Gjeldende arbeidsmal. **Gjelder:** SIGNAL / 47, Unreal-utgaven. **Sist kontrollert:** 4. november 2026.

Bruk malen ved nye bygninger og ombygging av eksisterende bygg. Fyll inn oppdragsfeltene og gjennomfør arbeidet innenfor bestillingen. Prosjektets [Unreal-regler](../../../unreal/AGENTS.md) og nyere brukerbeskjeder gjelder foran malen. Malen gir ikke i seg selv et nytt modelloppdrag eller tillatelse til installasjon, kjøp eller publisering.

## Kort bestilling

Kopier denne teksten, fyll inn det som er kjent, og legg ved referansene:

```text
Bruk Docs/Active/Guides/BLENDER_BUILDING_WORKFLOW.md.

Bygg og sted: [navn, område, eksisterende asset eller nytt bygg]
Oppgave: [hva som skal bygges, endres eller bevares]
Omfang: [eksteriør / eksteriør og interiør / bestemte deler]
Mål: [grunnflate, høyder, etasjer, takform; kjente mål først]
Rom og ferdsel: [rom, forbindelser, trapper, innganger, kjøreadkomst]
Uttrykk: [materialer, alder, slitasje og særpreg for New Mexico i 1986]
Referanser: [bilder, modellfiler, godkjent bygg eller designnotat]
Bevar: [verdensplassering, dører, spillankre, kollisjon, lys, skilt]
Gjenbruk: [kjente prosjektassets, Tripo-modeller eller andre eide kilder]
Leveranse: [Blender-kilde / eksport / Unreal-integrasjon]
Ytelsesramme: [eksisterende mål eller avtalte grenser for dette bygget]

Gjennomfør oppdraget til de bestilte filene og kontrollene er levert.
Bruk rimelige antakelser for mindre, reversible detaljer og noter dem.
Avklar bare manglende opplysninger som faktisk hindrer korrekt arbeid.
```

## Arbeidsinstruks til agenten

### 1. Undersøk kildene og sikre arbeidsområdet

Les prosjektreglene, oppgaveloggen og berørte designføringer. Kontroller Git-status og eierskap før endringer. Gjenbruk en egnet arbeidskopi og følg gjeldende grenser for kopier og diskplass.

Søk først i prosjektbiblioteket, eksisterende prosjektassets, Toms Tripo-modeller og eide originalkilder. Velg gjenbruk, tilpasning eller ny geometri etter passform, kvalitet og samlet arbeid. Kontroller lisens og opprinnelse; kjøpte eller lisensierte assets skal fortsatt være utenfor offentlig Git. Bevar originaler, rigger og fungerende generatorer.

Undersøk den faktiske Blender-scenen før redigering. Bruk en navngitt collection for oppdraget uten å slette uvedkommende innhold. Bruk tilkoblet Blender MCP når sceneadgangen er bekreftet, eller prosjektets fungerende Blender-/bpy-skript. Kontroller faktisk verktøyversjon og eksportløype. Manglende MCP er ikke alene grunn til å stoppe eller installere nye verktøy.

### 2. Fastsett mål og det som må bevares

Bruk meter i modellgrunnlaget og dokumenter akse-/skalakonverteringen til prosjektets Unreal-import. Registrer eksisterende verdensplassering, orientering, terrengkontakt, gulvhøyder, pivot og mål før utskifting. Bruk prosjektets koordinatkonvensjoner i [porteringsguiden](PORTERING.md).

Registrer dørblad, hengsler, interaksjonspunkter, kollisjon, lys, skilting, navigasjon og historie-/lagringsankre som berøres. De skal fortsatt fungere etter modellbyttet. Endre ikke plassering eller pivot som en utilsiktet følge av eksport.

Match referansenes silhuett, proporsjoner, taklinje, åpninger og særpreg. Skill kjente mål fra antakelser. Utform usette fasader sammenhengende med resten av bygget. Ikke presenter detaljer utledet fra et bilde som målte fakta.

### 3. Bygg hovedform og tilgjengelige rom

Blokker først fundament, gulv, yttervegger, etasjer og tak. Kontroller proporsjoner og terrengkontakt før smådetaljer. Gi synlige konstruksjoner passende tykkelse, og lag faktiske åpninger der spilleren skal se eller gå gjennom.

Bygget skal fungere fra relevante sider og spilleravstander. Fasadebilder, billboards og kameraprojeksjon erstatter ikke et tilgjengelig bygg. For utilgjengelig bakgrunnsarkitektur tilpasses detaljnivået til avtalt siktavstand; ikke lag unødvendig interiør.

Når interiør inngår, bygg en sammenhengende romplan med tilstrekkelig klaring, gulv, himling og tilgjengelige forbindelser. Kontroller at vinduer og dører stemmer mellom innside og utside. Bevar eller bygg fungerende trapper, repos og terskler. Hold bevegelige dører og andre interaktive deler adskilt fra statisk bygningsgeometri.

### 4. Detaljer, materialer og spillklar geometri

Legg til karmer, recesser, sålbenker, beslag, takkanter, renner, nedløp, ventilasjon og rekkverk der byggets type og referanser tilsier det. Koble delene troverdig sammen. Bruk gjenbrukbare moduler og instanser for gjentakelser; gi objekter og materialer beskrivende navn.

Bruk korrekt teksturskala, UV-er, normaler og materialslotter. Skill fargekart fra lineære datakart, og kontroller normalretning ved import. Materialene skal ha passende ruhet og respons i spillets lys. Blender-prosedyrenoder må bakes eller erstattes av native Unreal-materialer når eksporten ikke overfører dem.

Tilpass geometri, materialantall, kollisjonsproxy og LOD til byggets funksjon og eksisterende ytelsesramme. Alle LOD-er skal dele koordinatramme og bevare viktige åpninger og silhuett. Ikke anta at lik pivot eller like trekanttall beviser korrekt plassering. Behold redigerbare kilder selv om eksporten trenger anvendte modifikatorer eller transformasjoner.

### 5. Se resultatet og rett konkrete feil

Kjør en rask forhåndsvisning før kostbar rendering. Se bygget forfra, bakfra, fra siden og skrått ovenfra, samt fra relevante spillerhøyder. Ved interiør: se innganger, passasjer og berørte rom. Rett svevende deler, feil proporsjoner, utilsiktede gjennomskjæringer, manglende flater, feilvendte normaler og lyslekkasjer.

Når Unreal-integrasjon inngår, bruk prosjektets importløype og kontroller resultatet i motoren. Sammenlign før og etter ved samme relevante utsnitt og lys. Kontroller den berørte bygningsgruppen samlet, terrengkontakt og faktisk dør-/gangklarering. Se endrede materialer og lys i dag/natt når begge forhold påvirkes av oppdraget. En Blender-render er ikke en Unreal-kontroll.

Følg «Iterer først, test tungt mot slutten»: bygg pluss én rask relevant sjekk, og før-/etterbilder for visuelt arbeid. Ikke utvid et vanlig bygningspass til full regresjon. Kjør tunge jobber sekvensielt gjennom `require-idle` og `production_lease`, aldri samtidig med Toms spilløkt.

### 6. Lagre og lever

Lagre den redigerbare `.blend`-filen og nødvendige teksturkilder i prosjektets etablerte assetstruktur. Bruk vedlikeholdbart byggeskript når arbeidet utføres med bpy. Eksporter formatet den faktiske importløypa bruker, for eksempel FBX eller GLB; en ekstra GLB er bare nødvendig når bestillingen eller mottakeren trenger den. Kontroller mål, akser, pivot, materialer og fullstendighet i eksporten.

Fortsett gjennom kjøring, inspeksjon, retting og lagring. Et ukjørt skript eller et renderbilde alene er ikke en ferdig modellleveranse. Hvis én kontroll er utilgjengelig, fullfør uavhengig arbeid og oppgi nøyaktig hva som gjenstår.

Rapporter kort hva som er endret, faktiske filstier, gjenbrukte kilder, antatte detaljer og hva som er kontrollert i Blender og Unreal. Skill kilde, editorleveranse og installert spill. Oppdater berørt hoveddokument og oppgavelogg etter prosjektreglene. Bruk eksisterende autorisasjon for PR/levering; ikke innfør en ny godkjenningsrunde bare fordi et delsteg er ferdig.

## Grunnlag for malen

Tilpasset Toms referansebilde «OPUS 5.5 / BLENDER MCP», mottatt 4. november 2026, og prosjektets gjeldende Unreal-regler. Referansens krav om alt fra bunnen er erstattet med gjenbruk først; MCP og GLB er muligheter, ikke absolutte krav.

Prosjektbibliotekets [bpy-løsning uten GPU](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/losninger/bygg/blender-bpy-i-skyokt-uten-gpu.md) har PASS for sitt opprinnelige modell-/renderforsøk, ikke for vilkårlig Unreal-import. Bibliotekprofilen for [arjun988/blender-skills](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/repoer/prosjekter/arjun988--blender-skills.md) er UNVERIFIED mot prosjektets Blender-/UE-løype. Ingen ny addon eller programvare er installert som del av denne malen.

## Kritikerrunder mot referanse

Lagt til 6. november 2026 etter Toms tips om Mirages arbeidsflyt for 3D-scener. Flyten er lest fra skjermbilder av tråden, ikke prøvd med deres verktøy.

1. Ett referansebilde per modell før skripting. Finn kamera og øyehøyde fra forsvinningspunktene, så renderen kan legges rett ved siden av referansen.
2. Første runde i grå leire: form, proporsjoner og åpninger godkjennes før materialer.
3. Etter hver render sammenligner to uavhengige kritikeragenter renderen med referansen og leverer en rangert retteliste. De får bildene og referansen, ikke byggerens konklusjon.
4. Byggeren retter listen. Stopp når kritikerne bare finner småting, eller etter et fast antall runder.
5. Spillet stiller flere krav enn en filmscene: ekte kollisjon, dører og vinduer, LOD og spillbar skala. 2D-kort og forhåndsrendret bakgrunn teller ikke som ferdig bygg, bortsett fra ved vinduer eller horisont.
