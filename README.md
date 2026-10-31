# SIGNAL / 47

Førstepersons kosmisk etterforskning ved SARO i New Mexico, 1986. Gjeldende spillutgave utvikles i Unreal Engine.

## Finn frem

- **[Start her](Docs/README.md)**: samlet dokumentoversikt.
- **[Status](Docs/Active/STATUS.md)**: hva som finnes på main, i testet kandidat, lokalt installert og offentlig nedlastbart.
- **[Todo](Docs/Active/TODO.md)**: åpne oppgaver.
- **[Beslutninger](Docs/Active/DECISIONS.md)**: gjeldende retning og planhierarki.
- [Unreal-prosjektet](unreal/README.md): utvikling, arbeidsregler og koordinering.

## Spill på Linux

[Komplett Linux-testutgave 29. oktober 2026](https://github.com/Tombonator3000/SIGNAL-47/releases/tag/playtest-linux-2026-10-29), x86-64 med Vulkan. Modeller, teksturer og lyd følger med. Du trenger verken Git, Unreal-editor eller separate Artifacts. Denne utgaven har samme runtime som installert Farm R1; se statuslenken ovenfor.

Last ned bare [Spill-SIGNAL47.sh](https://github.com/Tombonator3000/SIGNAL-47/releases/download/playtest-linux-2026-10-29/Spill-SIGNAL47.sh) til en skrivbar mappe og kjør:

```sh
bash Spill-SIGNAL47.sh
```

Den henter og kontrollerer alle tre spillarkivene automatisk, om lag 4 GB. Beregn minst 12 GB ledig plass. Prøv først vanlig oppstart; `--wayland` tvinger Wayland, mens `--x11` krever XWayland/DISPLAY i en Wayland-økt. Behold gammel `Linux/Signal47/Saved` før oppdatering. [Detaljer og kontrollgrunnlag](Docs/Evidence/UnrealDeliveries/DOWNLOAD_LINUX_2026-10-29.md).

[Linux-krav, riktig startfil og feilsøking på Arch/niri](Docs/Active/Guides/LINUX_RUNTIME.md). `Signal47.sh` ligger inne i spillmappen; vedlegget som lastes ned heter `Spill-SIGNAL47.sh`.

## Kilder, design og historikk

[Design](Docs/Design/README.md), [undersøkelser](Docs/Research/README.md), [bevis og leveringer](Docs/Evidence/README.md) og [arkiv](Docs/Archive/README.md) er atskilt. Web og Unity bevares som historiske referanser; nye spillendringer gjelder Unreal. Lisensierte assets holdes utenfor offentlig Git. [Kreditering og ressurser](Docs/THIRD_PARTY_NOTICES.md).

[Tidligere blandet rotintroduksjon](Docs/Archive/Superseded/README-before-docs-organization.md) er bevart, med eldre nett-/Unity-startoppskrifter og leveringsbeskrivelser.
