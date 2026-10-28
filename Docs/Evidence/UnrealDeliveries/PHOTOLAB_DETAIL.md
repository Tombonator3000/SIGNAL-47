> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Fotolabens detaljpass

Status: levert 28. oktober 2026. Fotolab R1 er flettet via [PR181](https://github.com/Tombonator3000/SIGNAL-47/pull/181), merge `8c70ecb69f24bae15197e31bab95c924e1fd18e5`, og identisk testet pakke er installert i `/home/tom/Games/Signal47/Linux`. Ti originale detaljdeler, PBR, tre LOD og redigerbar Blender-kilde følger med. Hele matrisen består 110/110 etter én kjøring. Installert Low-fremkalling og ny prosess med Continue fra faktisk disk består; begge bilder er vurdert. Alle 50 personlige Saved-filer er uendret. Motel R4, SC01, MetaHuman og godkjent atmosfære er bevart.

Tom ønsket en mer detaljert fotolab med lyd, lamper og detaljer som hører fysisk til miljøet. Dette passet forbedrer eksisterende arbeidsstasjoner med skapfronter og metallbeslag, forstørrerhus/fokushjul/negativholder, analog timer, kjemilagring, arkivskuffer, vask med kran/avløp og ventilasjon med synlige fester. Det nye laget har 23 616 trekanter fordelt på ti deler. Eksisterende papirer, bilder, dør, våtbenk, arkiv og rapport blir stående på sine opprinnelige spillankre.

[Blender-kilde og produksjonsoppskrift](../../../unreal/ArtSource/PhotoLab/README.md). Ingen original mesh-node skjules; det nye laget bruker eksisterende kollisjonsarealer og lager ikke egne blokkeringer. Manglende eller feil native assets gir den opprinnelige laben. Eget sammenligningsflagg er `-S47PhotoLabDetailOff`.

Fotolaben har allerede fem fysiske lyskilder og posisjonert ballastlyd. De nye lampehusene følger eksisterende glass og lysåpninger, slik at normallys, rødt safelight, forstørrer, halv spenning, strømbrudd og tilbakekomst bruker samme drivere. Ingen ny alltid-på lys-/lydloop er lagt over historien. Ventilasjonen er fysisk innredning rundt rommets eksisterende lydmiljø.

## Kontrollomfang

Tre Blender-forhåndsvisninger er åpnet og vurdert. Dette er nøytrale studiobilder med forenklet eksisterende innredning, ikke spillbilder. Fersk gjenåpning av Blender-filen består 48 grupperte kontroller av ti modeller, PBR-filer, mål, eksport og åpninger for lys/vask.

Native import bruker samme målte FBX/PBR-løype som kontoret, med tre detaljnivåer (100/50/20 prosent), Nanite av, ray tracing på og dekorativ kollisjon av. Den første importen lagret alle delene, men kontrollen av lysåpninger avdekket en feil i UE5.8s GetTriangleVertices-wrapper: den reserverer tre arrayelementer og legger så til tre ekstra. Importkontrollen bruker derfor indekserte vertex-instances; ingen motorfil er endret. Fersk kontroll av lagrede assets består 56 grupperte kontroller. Trekanttallene er 23 616 / 11 808 / 4 725 for de tre detaljnivåene. Lys-/vaskåpninger er kontrollert i faktisk native LOD0.

Målrettet spillkontroll bruker den eksisterende `S47LightingReview` for normal/safe/print og strømbrudd, pluss eksisterende kapittel-1-forløp. Fullmatrisen kjøres først for den samlede spillleveransen før fletting. Ingen test av menneskelig input, lytting eller helspill-FPS hevdes.

Lokale råbevis og nattstatus: `/home/tom/scratch-bm/Signal47-photolab-night-20261027`. Motel R4, SC01, MetaHuman, atmosfære, himmel/terreng og personlige lagringer skal bevares. Claude eier de separate TB2-kildene i PR179; de fem bygningsmodellene endres ikke her.

## Pakket kandidat R1

Fem faktiske spillbilder er åpnet og vurdert: normal, safelight, printing, fremkalt foto på snoren og rapporten. Skap, vask/rør, forstørrer, arkivinnredning og lampehus følger eksisterende arbeidsstasjoner. Safelight-modusen er fortsatt mørk/rød; vi endret ikke den godkjente lysstyringen for å pynte på bildene. Eksisterende halv spenning, blackout og restore-kontroll består. Ingen nye blokkeringer eller synlige varseloverlegg ble observert i utsnittene.

Rapportprøven hadde først en lokal parserfeil: den ventet teksten `document report`, som denne API-ruten ikke logger. Spillet avsluttet normalt, og bildet viser riktig LOCAL REPORT med knappen for arkivering. FAIL-kvitteringen er bevart sammen med separat bildeaksept. Ingen ny spillkjøring ble nødvendig. Prøven setter opp rapportstadiet gjennom API og beviser ikke fysisk gange/raycast, menneskelig input eller at hele saken er fullført.

Kildefrys og pakket payload ble hashet før de målrettede prøvene og er uendret før fullmatrisen. Bare cook-listen, den nye fotolabkroken og nye fotolabfiler endrer runtime fra installert motel R4. Fullmatrisen kjøres én gang med alle originale scenarioer og kontrolluttrykk; adapteren tilfører bare offscreen-flagg og absolutt guard-sti.

## Sluttkontroll og avgrensning

Fullmatrisen avsluttet normalt på 53 min 49 sek, 110/110, uten samtidige motorjobber eller registrerte motorfeil. Den ble ikke gjentatt. Installert Low brukte en egen /tmp-profil. Første korte kjøring lagret `interpret-first` og `dev1=3`; en ny prosess åpnet samme case med Continue og viste det fremkalte bildet og riktig oppgave. Begge bilder er faktisk vurdert. Dette er API-styrt forløp, ikke menneskelig gjennomspilling, fysisk labvandring, lyttetest eller helspill-FPS.

Kanonisk opprydding pensjonerte bare payload i motellets gamle R3/R2. Gjeldende fotolabkandidat og to eldre komplette kopier (installasjonsbackup av R4 og opprinnelig R4) beholdes. SC01-R4 og PR174 er særskilt beskyttede kandidater for andre oppgaver; TB2/nedlasting er også beskyttet. Kilder, bilder, logger, Saved og Blender-filer er bevart. Ingen ny offentlig spillrelease.

Claudes fem TB2-bygninger i PR179 er ikke importert her. «Trado» er ikke avklart; godkjent terreng/himmel er derfor bevart. Nattens fotolabmål er levert. ArtSource/PhotoLab/README.md beskriver produksjonscheckpointet før integrasjon; denne rapporten er gjeldende leveringsstatus.
