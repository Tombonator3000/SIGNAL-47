# Cloud tank med ni fluidstadier

**Status:** Referanse. **Gjelder:** Original SC13-sekvens fra eksisterende wavelet-bake. **Sist kontrollert:** 31. oktober 2026.

Ni 256³-volumer fra samme Mantaflow-kilde som [CloudTank-wavelet-v2](../CloudTank-wavelet-v2/README.md): ramme 48, 54, 60, 66, 72, 78, 84, 90 og 96. Den redigerbare [Blender-kilden](../CloudTank-wavelet-v2/CloudTank.blend) og den opprinnelige baken er bevart. Dette er et tettere utvalg fra samme cache, uten ny simulering eller endret tidsskala.

[atlas.json](atlas.json) og PNG-atlasene er uendrede kopier fra pakkeren. Alle frames bruker samme bounds, akserekkefølge og DecodeScale 2 som den opprinnelige trefeltkilden. PNG er lineær 16-bit, 4096×4096 med 16×16 Z-snitt. Projeksjonsbildene viser tetthetsdata, ikke Unreal-render. Rå VDB og NPY-data beholdes i lokal produksjonsoutput; manifestets råstier er proveniens.

Native sekvens ligger under `/Game/FinaleSky/CloudSequence`; alle ni faktiske TextureRHI-er er kontrollert som 256³ PF_R16F. Spillet holder frames residente, binder bare et nabopar per bank og bruker en eksplisitt blandefaktor fra samme Chapter6-fase. Det egne sekvensmaterialet har 19 innganger. Den tidligere trefeltkilden og originalmaterialet er beholdt.

Verktøy og importvalg står i [finalesky/README](../../../Signal47/Tools/finalesky/README.md). [STATUS](../../../../Docs/Active/STATUS.md) fører aktuell levering.
