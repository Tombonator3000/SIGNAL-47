# NightSky12 — bevis fra faktisk testkandidat

Kildecommit `9481134`, Unity SHA `79cafef6f5a59d042d101652cb5fb55942de4b13af91e8949a080d4caf580512`. Dette er en kandidat. Tastatur-/musreisen, nye fotobevis, lagring/gjenstart, bevegelse og ytelse er **UNVERIFIED** fordi Kubuntu-skjermen er låst. Visual10 er fortsatt standard.

**Publisering etter bevisinnsamlingen:** [GitHub-kontrollen](repository-visibility.json) registrerte at repoet var offentlig og at publisering derfor ventet på autorisasjon. Brukeren godkjente senere publiseringen; kilde, dokumentasjon og dette bevissettet er nå tilgjengelig i [utkast til PR8](https://github.com/Tombonator3000/SIGNAL-47/pull/8). De opprinnelige JSON-registreringene er bevart som historikk fra innsamlingstidspunktet. Ingen merge eller nye interaktive tester er utført.

Se [resultater og avgrensning](verification.json), [pakkeinnhold](content-verification.json), [byggeutdrag](unity-build-excerpt.txt) og [skjermlåsens negative test](screen-lock-preflight.json). Byggets working-tree-markør gjelder dokumentasjon; Unity-kildene ble kontrollert identiske med kodecommiten. Fullstendige lokale originalbygge-/feillogger er identifisert med hash i resultatfilen; innsjekkede tekstutdrag oppgir filter og linjenumre.

## Visuell egenvurdering

[Visual10 før](Baseline/05-service-yard-array.png) og [NightSky12 etter](Final/05-service-yard-array.png) er uendrede bilder fra de respektive utpakkede spillpakkene med samme faste kamera. [Utsyn fra kontrollrommet](Final/04-window-array.png) viser himmelen mot lokal belysning.

De sju øvrige sluttbildene viser [0°](Final/sky-heading-000.png), [90°](Final/sky-heading-090.png), [180°](Final/sky-heading-180.png), [270°](Final/sky-heading-270.png), [senit](Final/sky-zenith.png), og begge sider av kubens +X/+Y-grense: [venstre](Final/sky-seam-left.png) / [høyre](Final/sky-seam-right.png). Alle ni sluttbilder er inspisert. Stjernene er finere, Melkeveien synlig og dempet, og antenner/arbeidslys forblir tydelige. Den tidligere polutstrekkingen og en synlig overgang ved den prøvde flategrensen er ikke observert i sluttbildene. Bevegelsesflimring og andre synsvinkler må fortsatt prøves.

`world-capture-manifest.json` og `sky-capture-manifest.json` oppgir kameraer, runtime-identitet og maskin. Fastkameraet setter en eksplisitt inspeksjonstilstand. Dette er ikke bevis for navigasjon, fotografier samlet av spilleren, lagring eller fps. Manifestet for verdensbilder omtaler også andre utsnitt som ligger lokalt; de er ikke del av denne ni-bilders egenvurderingen. Motellblokkeringen er heller ikke levert som spillbart kapittel.

## Avvik som ble rettet

[Den første feilaktige polen](Failed/initial-polar-distortion.png) og dens [manifest](Failed/initial-sky-capture-manifest.json) kommer fra en tidligere kandidat, ikke sluttpakken. Den equirektangulære runtime-filtreringen strakte stjerner til radiale striper. Seks kubeflater med filtrering i synsretningen fjernet den observerte feilen. Første importkode brukte feil API-type og fikk [kompilatorfeil](Failed/import-api-compile-excerpt.txt); sluttbygget bruker TextureImporterSettings og kompileringen består.

Originaler, kilde-/pakkeidentitet og uverifiserte forhold beholdes adskilt. `file-hashes.json` gjør det mulig å kontrollere at dette bevissettet ikke er endret. Det overordnede prosjektets grafikkmål er fortsatt ikke ferdig oppnådd.
