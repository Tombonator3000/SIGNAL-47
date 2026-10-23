# Kveldslys for EveningShift

23. oktober 2026. S47Sky/S47Lighting utvider den levende nattehimmelen til innkjøring og kveld. Eksisterende levende himmelmaterialer, stjerner, horisontring og native sol/skylight gjenbrukes. Ingen import av materialer eller nye binærassets er nødvendig.

`FS47Sky::Dawn` og `US47WorldLife::Dawn()` beholder den opprinnelige morgenskurven for historieporter. `Evening` beskriver kveldslyset; `Daylight()` gir den samlede kurven som brukes av himmel, stjerner, meteorer, horisont, tåke, månefyll og native lys. `SetSkyLighting` tar samlet dagslys og en valgfri kveldsverdi. Eksisterende fireargumentkall er fortsatt gyldige.

| Lokal klokke | Samlet dagslyskurve | Solhøyde | Solbidrag |
|---|---:|---:|---:|
| 17:20, innkjøring | 1,000000 | 11,73° | 1,000000 |
| 18:00, SARO/telefon | 1,000000 | 4,53° | 1,000000 |
| 18:27, solnedgang | 0,539701 | −0,33° | 0,146667 |
| 18:53, skumringsslutt | 0,047088 | −5,01° | 0,000000 |
| 19:07, full natt | 0,000000 | −7,53° | 0,000000 |

Dette er kunstnerisk kalibrerte kurver for spillets eksisterende faste eksponering. Referansetidene 18:27 og 18:53 kommer fra ANOMALY_PLAN.md. Vestsolen står ved azimut 281°. Eksisterende morgenlys ved azimut 79°, områdenes dawn lift og finalens eksplisitte solhøyde beholdes. Native sol går fra opptil 14 ved ettermiddag tilbake til natt; morgenens gamle faktor 8 er bevart. Tallene er spillverdier, ikke en fysisk dagslysmåling. Eksisterende blåtime-/soloppgangspalett brukes nå også mot vest.

Etter det første runtimebildet fra faktisk parkering ble kurven justert: full dagslyspalett beholdes til 18:00, deretter faller `smoothstep(remaining/67 minutter)^1.4` til null 19:07. Den tidligere 107-minutterskurven ga 0,685 ved 18:00 og viste for tidlig nattpreg. Stjernene får en separat sterkere kveldsdemping (`min(1, Evening*2)`), slik at de fortsatt er helt borte ved solnedgang og først kommer frem etter denne. Materialenes opprinnelige morgenfade er bevart. Retak må vise om dette løser det observerte 18:00-bildet.

Den vennlige innkjøringen får ingen automatiske lyn eller torden mens kveldslyset er aktivt. Tilfeldige meteorer begynner først når himmelen er mørk nok. Den gamle nattsyklusen fortsetter fra full natt.

## Kontrollgrunnlag

- PASS: whitespace-kontroll og lett matematisk referansekontroll av 108 ettminuttspunkter. Kurven går jevnt ned, aldri opp, og største fall per minutt er under 0,03 (beregnet maksimum 0,025296).
- Ni nye kontrolltilfeller ligger i `US47WorldLife::SelfTest` for `-S47SelfTest`: vestsol ved innkjøring, jevn kveldsnedgang, 18:00-lys uten stjerner, solnedgang uten stjerner, ingen tvungen torden/dagmeteor, blåtime, midnatt, opprinnelig dawn lift og finalens solhøyde.
- Før kurverettingen: integratoren har bygget koden og kjørt worldlife-selvtestene PASS. Innkjøringsbildet er visuelt gjennomgått og viser sammenhengende ettermiddagslys. Bildet ved faktisk SARO-parkering viste for mørk himmel og for tydelige stjerner ved 18:00; dette er FAIL for første kurve, og årsaken til rettingen ovenfor.
- UNVERIFIED etter kurverettingen: nytt C++-bygg, de ni faktiske selvtestene, 18:00-/skumringsretak og ytelse. Integratoren kjører Unreal sekvensielt sammen med resten av kveldens runtime.

Eksisterende `-S47Clock=17:20`, `-S47Clock=18:00`, `-S47Clock=18:27`, `-S47Clock=18:53`, `-S47Clock=19:07` og `-S47LifeLog` kan brukes for målrettede bildeprøver. `-S47LifeLog` viser nå både dawn og evening. Innkjøringen bruker sin egen lagrede klokketilstand; et rent lysflagg er ikke bevis på reise eller klokkeovergang.

Prosjektbiblioteket ble søkt først. Scenario Labs sin UE 5.8 lighting/rendering-kilde ved `f0adb5b5` ble lest som prinsippgrunnlag for fast eksponering og skille mellom praktiske lyskilder, emisjon og dokumenterte kunstneriske verdier. Hjelpeskriptet der er uttrykkelig uprøvd i Unreal og er ikke gjenbrukt. Ingen nye renderinnstillinger, eksponeringsmodus eller ekstra lysrigg er innført.
