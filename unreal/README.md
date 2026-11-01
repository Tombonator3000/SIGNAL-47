# SIGNAL / 47 i Unreal

**Status: Gjeldende inngang** · UE 5.8.4, Linux/Vulkan.

- [Start her: dokumentoversikt](../Docs/README.md).
- [Faktisk leveringsstatus](../Docs/Active/STATUS.md), [åpne oppgaver](../Docs/Active/TODO.md) og [beslutninger](../Docs/Active/DECISIONS.md).
- [Historie og design](../Docs/Design/README.md), [tekniske guider](../Docs/Active/Guides/README.md), [undersøkelser](../Docs/Research/README.md) og [leveringsbevis](../Docs/Evidence/README.md).
- [Arbeidsregler](AGENTS.md), [koordinering](OPPGAVER.md) og [append-only logg](log.md).

## Bygg og kjør

Prosjektet er `Signal47/Signal47.uproject`. Bruk de eksisterende skriptene under `Signal47/Scripts/` og én tung jobb om gangen. [Porteringsguiden](../Docs/Active/Guides/PORTERING.md) forklarer modulene, spillrommet og koordinatkonverteringen. Generert innhold, lokale lisensierte ressurser, Binaries, Intermediate og Saved committes ikke. Se AGENTS før bygging eller import.

Den tidligere lange [README-en er bevart](../Docs/Archive/Superseded/Unreal/README.md). Den er historikk, ikke en konkurrerende statuskilde.


### Små Roswell-aliens i F10-menyen

Under **Roswell aliens** velges 1, 2, 3 eller 6 individer. Ståprøven varer åtte aktive sekunder. **Curious: stalk and hide** gir lytting, lusking, vindus-/hjørnetitting og flukt ved oppdagelse. **Hunt** er en eksplisitt utviklerprøve uten historieskade. Gruppen kan bytte atferd på stedet eller fjernes samlet; egne valg prøver fysisk takvandring. Plasseringen avviser manglende gulv og opptatte steder og viser faktisk antall opprettet. Individene deler modellressurs, men har hver sin riggpose, navigasjon, oppfatningsminne og fotlyd. Den store Pale-jakten er separat.

Den avgrensede utviklerprøven kjøres med `-S47Case=0 -S47Evening=parking -UserDir=/tmp/<egen-profil> -S47RoswellAIContract=/absolutt/bildemappe`. Den bruker de vanlige menyhandlingene og kontrollerer flere kropper, ståtid, pause, lusking/fotkontakt, eksplisitt jakt, tak og nullstilling. Lydprøven lagres i bildemappen. Dette er en isolert test, ikke en hel natt eller menneskelig spillaksept.


### Freeform

**NEW FREEFORM** oppretter en egen sak med lokal recovery eller valgfri død/lasting. Meny og lagringskort viser hvilken modus saken tilhører. Eksisterende saker uten modusfelt åpnes som Story.

Et døgn varer 75 aktive minutter: dag kl.06:00–19:00 tar 30 minutter, natt kl.19:00–06:00 tar 45. Pause og menyer holder tiden.

Arbeidsordren og oppdragene finnes ved hjørne-PC/telex. Høyre PC retter antennene, midt-PC tar opp, venstre PC viser tilstand, og lydutstyret sammenligner original og kontroll. Rutinearbeid registreres digitalt; undersøkelser får merket pakke ved skriveren og originaler i arkivet. Servicebenken står ved den tilgjengelige fasaden til bygget ved SARO-skiltet. Caliches forsyningsluke selger deler og film som hentes, lastes og leveres fysisk. Film fylles på ved benken uten tap av gamle fotografier.

Utviklerprøven bruker eksplisitt `-S47FreeformTest -UserDir=/tmp/<egen-profil>`. Den starter en isolert utviklersak og skriver `Saved/FreeformTest/result.json` og runtime-bilder. Prøven bruker produksjonscallbacker og navngitte plasserings-/hendelsesfixtures; den er ikke menneskelig input/lytting eller naturlig tre døgn med spillfølelse. Vanlig Freeform startes fra menyen. Se STATUS for faktisk bygget og installert versjon.

OPS har en valgfri **Service work order** med valg mellom SARO-reservedel og en ny forsyningstur. Neste servicehandling følger deg i HUD og notatbok; den kan nedprioriteres uten å miste arbeidet. Den valgte forsyningsruten må ha en ny relé-/donorpakke lastet i bilen og levert ved benken. S-03 må gjennomføre faktisk bevegelsestest før OPS kan signere resultatet.

Med en pakke i hendene: se på ledig gulv og bruk vanlig interaksjon for håndtering, kontrollert nedsetting eller slipp. Se på cargo-racket for å laste; pakken må være sikret før kjøring. En gratis gjenbrukbar servicekasse kan hentes og returneres ved benken. Pose og fysisk tilstand følger lagringen.
