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
