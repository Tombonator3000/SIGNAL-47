# Bærbar SARO-radio og alienvarsler

**Status:** Gjeldende leveringsbevis. **Gjelder:** PR308, runtime `af6cc33c`, UE5.8.4/Linux/Vulkan. **Sist kontrollert:** 1. november 2026.

Installert lokalt kl.09:41 CET etter normal merge til main `a9a65538`. Ingen offentlig spillrelease. [Installasjonsresultat](FieldRadio-2026-11-01/installation.json).

## Det spilleren får

En egen SARO-feltradio ligger på kontrollrommets supervisorpult. E tar den med, kroppen og dens query-kollisjon fjernes, og eierskapet følger saken i feltet `fieldRadio`. Gamle saker uten feltet kan fortsatt hente den. Fremtidig ukjent radioversjon bevares uendret og holdes inaktiv. Tomás' Rom6-radio, geometri, batterihistorie og 4+7-sekvens er urørt; tre kopier av spillets egne eksisterende mesh-deler gir SARO-apparatet uten ny modellimport.

Skurringen følger brede, glatte avstandsforandringer til aktive små Roswells, SC02-kroppen, SC04, SC11 og F10-pale. Aktive individer bak spilleren og utenfor bildet kan varsle; fremtidige/deaktiverte fiender gjør det ikke. Lyden følger radioen hos spilleren og oppgir ingen nøyaktig avstand eller retning. Egne korte, lave originalcues skiller nærvær, faktisk synlig oppdagelse og jakt. Delt prioritering, fades, cooldown og lokal radiodemping hindrer lydstabling; vanlig Foley beholdes. Pause, modal/cinematic, radio-/hovednivå og stille sekvenser følger eksisterende lydløype. Ingen ny Dale-samtale eller snapzoom.

## Avgrenset kontroll

Editorbygg og 1924 selvtester består. Én målrettet native prøve består13 kontroller: normal sporbar pickup, separat Rom6-original, normal Flow-snapshot/gjenoppretting, eldre manglende felt, faktisk aktiv Roswell bak kameraet og stillhet når den fjernes. Det er en uttrykkelig testfigur, ikke måling av naturlig møtefrekvens. Pickup-gjenoppretting bruker samme adapter som Continue; det er ikke en full tittel-/diskrunde etter pickup. [Kontroll og prøvegrenser](FieldRadio-2026-11-01/native-check.json).

Normal Linux-pakke består1924 selvtester og kort High/Vulkan-røykprøve. Installert standardstarter består25 sekunders vanlig Fortsett fra en isolert kopi av den gamle SARO-auto2-saken. Alle149 Saved-elementer, inkludert118 personlige filer, er hash-/modusbevart. Ingen personlig sak er skrevet av prøven.

[Før](FieldRadio-2026-11-01/before.png) og [etter, faktisk pakket spill](FieldRadio-2026-11-01/after.png) bruker samme kamerafikstur ved pulten. [Seks sekunders original lydprøve](FieldRadio-2026-11-01/encounter-preview.wav) er rendret offline fra den virkelige lydgrafen; den er ikke et opptak av Toms spilløkt. Menneskelig lytting, skrekk-/komfortaksept og naturlig helnattstempo står fortsatt åpne.

## Linux-start og vedlikehold

Nedlastingsstarteren forklarer manglende internstarter/binær/ufullstendig arkiv med konkret sti og exitkode. Rapporten tåler tom feillogg og avgrenset Vulkan-diagnostikk. Normal pakking validerer internstarter/binær og PAK/UTOC/UCAS, og inkluderer `Check-Linux.sh` og `LINUX-README.md`. [Krav og bruk](../../../unreal/Signal47/Build/Linux/launch/README.md). Dette endrer ingen driver eller grafikkstandard. Den gamle offentlige pakken er ikke republisert, og kameratens Arch/niri/RTX5090-start er fremdeles UNVERIFIED.

To gamle genererte testpayload er pensjonert; gjeldende og to eldre komplette testpakker er beholdt. Ferdige `Saved/Cooked` og `Saved/StagedBuilds` er ryddet etter avsluttet levering. Nettoeffekt er omtrent12,7GB frigjort og24,3GB ledig;606 øvrige kilde-Saved-elementer og alle personlige lagringer er bevart. Neste pakking må recooke. [Vedlikehold](FieldRadio-2026-11-01/maintenance.json).

Den gamle SARO-renderstansen PLAY-31-16, mottakerens grafiske oppstart PLAY-31-38 og videre alien-animasjonsarbeid er fortsatt åpne. Denne leveransen endrer ikke statusen deres.
