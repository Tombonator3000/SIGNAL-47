# Kveldslys for EveningShift

23. oktober 2026. S47Sky/S47Lighting utvider den levende nattehimmelen til innkjøring og kveld. Eksisterende levende himmelmaterialer, stjerner, horisontring og native sol/skylight gjenbrukes. Ingen import av materialer eller nye binærassets er nødvendig.

`FS47Sky::Dawn` og `US47WorldLife::Dawn()` beholder den opprinnelige morgenskurven for historieporter. `Evening` beskriver kveldslyset; `Daylight()` gir den samlede kurven som brukes av himmel, stjerner, meteorer, horisont, tåke, månefyll og native lys. `SetSkyLighting` tar samlet dagslys og en valgfri kveldsverdi. Eksisterende fireargumentkall er fortsatt gyldige.

| Lokal klokke | Samlet dagslyskurve | Solhøyde | Solbidrag |
|---|---:|---:|---:|
| 17:20, innkjøring | 1,000000 | 11,73° | 1,000000 |
| 18:00, SARO/telefon | 0,685236 | 4,53° | 1,000000 |
| 18:27, solnedgang | 0,314764 | −0,33° | 0,146667 |
| 18:53, skumringsslutt | 0,046878 | −5,01° | 0,000000 |
| 19:07, full natt | 0,000000 | −7,53° | 0,000000 |

Dette er kunstnerisk kalibrerte kurver for spillets eksisterende faste eksponering. Referansetidene 18:27 og 18:53 kommer fra ANOMALY_PLAN.md. Vestsolen står ved azimut 281°. Eksisterende morgenlys ved azimut 79°, områdenes dawn lift og finalens eksplisitte solhøyde beholdes. Native sol går fra opptil 14 ved ettermiddag tilbake til natt; morgenens gamle faktor 8 er bevart. Tallene er spillverdier, ikke en fysisk dagslysmåling. Eksisterende blåtime-/soloppgangspalett brukes nå også mot vest.

Den vennlige innkjøringen får ingen automatiske lyn eller torden mens kveldslyset er aktivt. Tilfeldige meteorer begynner først når himmelen er mørk nok. Den gamle nattsyklusen fortsetter fra full natt.

## Kontrollgrunnlag

- PASS: whitespace-kontroll og lett matematisk referansekontroll av 108 ettminuttspunkter. Kurven går jevnt ned, aldri opp, og største fall per minutt er under 0,015.
- Åtte nye kontrolltilfeller ligger i `US47WorldLife::SelfTest` for `-S47SelfTest`: vestsol ved innkjøring, jevn kveldsnedgang, 18:00-lys, ingen tvungen torden/dagmeteor, blåtime, midnatt, opprinnelig dawn lift og finalens solhøyde.
- UNVERIFIED i dette delarbeidet: C++-bygg, faktiske selvtester, bilder, lysopplevelse, skygger og ytelse. Integratoren kjører Unreal sekvensielt sammen med resten av kveldens runtime.

Eksisterende `-S47Clock=17:20`, `-S47Clock=18:00`, `-S47Clock=18:27`, `-S47Clock=18:53`, `-S47Clock=19:07` og `-S47LifeLog` kan brukes for målrettede bildeprøver. `-S47LifeLog` viser nå både dawn og evening. Innkjøringen bruker sin egen lagrede klokketilstand; et rent lysflagg er ikke bevis på reise eller klokkeovergang.

Prosjektbiblioteket ble søkt først. Scenario Labs sin UE 5.8 lighting/rendering-kilde ved `f0adb5b5` ble lest som prinsippgrunnlag for fast eksponering og skille mellom praktiske lyskilder, emisjon og dokumenterte kunstneriske verdier. Hjelpeskriptet der er uttrykkelig uprøvd i Unreal og er ikke gjenbrukt. Ingen nye renderinnstillinger, eksponeringsmodus eller ekstra lysrigg er innført.
