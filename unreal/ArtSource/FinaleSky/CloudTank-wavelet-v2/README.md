# Egen cloud tank-kilde med wavelet-noise

Aktiv kilde fra `cloud-wavelet128x2-v01`. [CloudTank.blend](CloudTank.blend) beholder redigerbart domene og asymmetriske emittere. [source.json](source.json) beskriver base 128, wavelet ×2, 96 rammer og valgte rammer 48/72/96. Tetthetsoppdrift er -0.14, vorticity 0.18 og rotorer av. Wavelet bruker styrke 0.35, posisjonsskala 2 og tidsfaktor 0.1.

[atlas.json](atlas.json) beskriver de tre 256³-volumene: `density_noise` fra noise-cachen, lineær 16-bit, 4096×4096 PNG og 16×16 Z-snitt. Rekkefølgen er `density[z,y,x]`, X raskest. Normaliserte data dekodes med felles skala 2. De små `BLENDER-density-projections` viser simdata fra front/side/topp. Full VDB-cache og NPY-flyttallsdata beholdes separat i den lokale bakeoutputen.

Kilden er importert som `V_S47_CloudDensityA/B/C` under `/Game/FinaleSky/Clouds`, med `TC_HALF_FLOAT`, sRGB av, clamp og ingen mipmaps. Etter tegnet bilde er faktisk `TextureRHI` bekreftet som 256³ `PF_R16F`. Registryformat og beregnet målplattformstørrelse er konfigurasjonsinformasjon.

Volummetadataen `S47DensityScale=2` driver materialparameteren `DecodeScale=2`. Skymaterialets Custom-node har 18 innganger, inkludert tre tetthetsvolumer, `Density` og `DecodeScale`.

Verktøy og kommandoer står i [finalesky/README](../../../Signal47/Tools/finalesky/README.md). Denne kilden er separat fra de bevarte originalene og `CloudTank-v1`; råkildene endres ikke ved nativeimport.
