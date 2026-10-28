> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

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

Etter det første runtimebildet fra faktisk parkering ble kurven justert: full dagslyspalett beholdes til 18:00, deretter faller `smoothstep(remaining/67 minutter)^1.4` til null 19:07. Den tidligere 107-minutterskurven ga 0,685 ved 18:00 og viste for tidlig nattpreg. Stjernene får en separat sterkere kveldsdemping (`min(1, Evening/0.05)`), slik at de er helt borte gjennom tidlig skumring og først kommer frem omkring skumringsslutt 18:53. Dempingen er 1 ved 18:35, 0,941757 ved 18:53 og 0 ved 19:07. Dette trengs fordi det eksisterende stjernematerialets HDR-farger ellers gir mange store, tydelige stjerner selv med delvis demping. Materialenes opprinnelige morgenfade er bevart. Retaket fra faktisk parkering er nå gjennomgått: det tidligere nattpreget ved 18:00 er rettet i dette utsnittet. Det første skumringsbildet fra 18:35 er også tatt opp og ga en separat stjernefeil, dokumentert nedenfor.

Den vennlige innkjøringen får ingen automatiske lyn eller torden mens kveldslyset er aktivt. Tilfeldige meteorer begynner først når himmelen er mørk nok. Den gamle nattsyklusen fortsetter fra full natt.

## Kontrollgrunnlag

- PASS: whitespace-kontroll og lett matematisk referansekontroll av 108 ettminuttspunkter. Kurven går jevnt ned, aldri opp, og største fall per minutt er under 0,03 (beregnet maksimum 0,025296).
- Elleve nye kontrolltilfeller ligger i `US47WorldLife::SelfTest` for `-S47SelfTest`: vestsol ved innkjøring, jevn kveldsnedgang, 18:00-lys uten stjerner, solnedgang/18:35 uten stjerner, senere stjernefade etter skumringsslutt, ingen tvungen torden/dagmeteor, blåtime, midnatt, opprinnelig dawn lift og finalens solhøyde.
- Før kurverettingen: integratoren har bygget koden og kjørt worldlife-selvtestene PASS. Innkjøringsbildet er visuelt gjennomgått og viser sammenhengende ettermiddagslys. Bildet ved faktisk SARO-parkering viste for mørk himmel og for tydelige stjerner ved 18:00; dette er FAIL for første kurve, og årsaken til rettingen ovenfor.
- PASS etter kurverettingen: integratoren har bygget editorvarianten. To faktiske runtimebilder fra den automatiserte kjøreruten er åpnet og visuelt gjennomgått: `route/direct/direct-commute.png` og `route/direct/direct-parking.png` under `/home/tom/scratch-bm/Signal47-evening-runtime/`. Innkjøringen har sammenhengende varm ettermiddagshimmel, lange skygger og ingen synlige nattlige frontlyspøler. Klokka er skjult bak «LATE AFTERNOON». Parkeringsbildet viser SARO 18:00 med blå himmel, ingen synlige stjerner, varme solbelyste antenner og bilflater samt lesbare skyggesider. Retningslysvarslet fra første opptak er ikke synlig. Ingen ny konkret lys-/himmelfeil er funnet i disse to stillbildene.
- Rutegrunnlag: `direct-v4.log` bekrefter fysisk kjøring 2648,9 m og faktisk ankomst innen første sekund av 18:00. Ruten feilet deretter i stage 11 ved fysisk gangsti til servicedøra; bildene dokumenterer innkjøring/parkering og er ikke bevis på bestått helreise. Kjøringen bruker kontrollere/API, ikke menneskelig input. SHA256 for parkering: `7e7cbea00d7282fd3a3307e392ea9a9374ab5ffde7aca631ed4b23f6913d1b51`; innkjøring: `687a0b399169dadbc53aec936a7b397fddeee5690f6d6d2f6571e62e5b998250`.
- FAIL i første 18:35-opptak: `route/burger/burger-dusk.png` er åpnet og visuelt gjennomgått. Bygninger og lokal belysning er lesbare, men himmelen viser altfor mange store stjerner bare åtte minutter etter solnedgang. Den tidligere `Evening*2`-dempingen er derfor erstattet med terskelen 0,05 beskrevet ovenfor. Dette endrer kun kveldsstjernene, ikke morgenskurven, skypaletten eller native lys.
- PASS etter siste retting: editorbygg 11, Linux-pakke fra `d3000904` og pakket selvtest med 1254 sjekker, inklusive de elleve sky-kontrollene. De nye pakkede bildene `packed/direct/direct-commute.png`, `direct-parking.png` og `direct-dusk.png` er åpnet og visuelt gjennomgått. 18:35 har nå blå skumringshimmel uten det for tidlige stjernefeltet, med varme lampepøler og lesbar silhuett. Duskbildets SHA256 er `3b96f3e099798c6da42844787e3d47fbd5899f376bba64e704e4b1da0bd53dda`. Den pakkede direkte reisen består hele ruten og normal avslutning; 2647,4 kjørte meter. Dusk bruker et uttrykkelig fremoveroppsett etter overleveringen, ikke 35 minutters målt spilling.
- UNVERIFIED: egne stillbilder ved nøyaktig 18:27/18:53/19:07, nattfog etter prioritetsbyttet, menneskelig vurdert bevegelseskvalitet og ytelse. Ingen menneskelig gjennomspilling eller ny ytelsesmåling inngår i denne bildegjennomgangen. Integratoren kjører Unreal sekvensielt sammen med resten av kveldens runtime.

Eksisterende `-S47Clock=17:20`, `-S47Clock=18:00`, `-S47Clock=18:27`, `-S47Clock=18:53`, `-S47Clock=19:07` og `-S47LifeLog` kan brukes for målrettede bildeprøver. `-S47LifeLog` viser nå både dawn og evening. Innkjøringen bruker sin egen lagrede klokketilstand; et rent lysflagg er ikke bevis på reise eller klokkeovergang.

Prosjektbiblioteket ble søkt først. Scenario Labs sin UE 5.8 lighting/rendering-kilde ved `f0adb5b5` ble lest som prinsippgrunnlag for fast eksponering og skille mellom praktiske lyskilder, emisjon og dokumenterte kunstneriske verdier. Hjelpeskriptet der er uttrykkelig uprøvd i Unreal og er ikke gjenbrukt. Ingen nye renderinnstillinger, eksponeringsmodus eller ekstra lysrigg er innført.

SKY.md (24.10, `sky-pass`) legger volumetriske skyer, cirrus, skumringsbånd, tåke og sanntids skylight oppå denne kurven. Kurvene og historieverdiene her er uendret.
