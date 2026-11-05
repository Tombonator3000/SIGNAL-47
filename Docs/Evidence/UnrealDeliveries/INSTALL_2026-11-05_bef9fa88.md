# Lokal installasjon 5. november 2026

**Status:** Referanse. **Gjelder:** lokal Linux/Vulkan-installasjon. **Sist kontrollert:** 5. november 2026.

`bef9fa8894045911b356bbf07298729b06d0dd4f`, etter PR415 og PR416, er installert via uendret standardstarter `/home/tom/Games/Signal47/Linux/Signal47.sh`. Ingen offentlig spillutgave er publisert.

Pakke og standardstarter består 2464 selvtester og røyktest. Den pakkede reisen består 213 kontroller over sju separate prosesser: 32, 20, 21, 51, 22, 45 og 22. Alle 126 eksisterende Saved-filer er uendret. Tittelbildet er visuelt kontrollert.

Testreisen bruker syntetisk input og erklærte fixtures. Reisen dekker ikke SC04-returen; den er prøvd i editor med `sc04doors` (22 kontroller), ikke i pakket bygg. Naturlig oppdagelse, OS-mus og menneskelig spilleaksept er ikke påstått.

Privat leveringsgrunnlag ligger i `/home/tom/scratch-bm/Signal47-delivery-20261105/delivery`, med før-/etterhasher for Saved. Tilbakerulling er forrige kandidat i `/home/tom/scratch-bm/Signal47-nightshift-20261104/delivery/candidate/Linux`.
