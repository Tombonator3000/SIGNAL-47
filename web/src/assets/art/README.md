# Grafikk til spillet

Legg bildefilene fra `web/ART_BRIEF.md` her, i undermapper med nøyaktig navn fra briefen, for eksempel:

```
room/tex_floor_hextile.jpg
sky/sky_milkyway_equirect.jpg
yard/vane_b12.png
lab/sign_blank.png
```

Spillet bruker et bilde automatisk når fila finnes, og tegner teksturen i kode når den mangler. På GitHub: gå inn i denne mappa, velg Add file, Upload files, og skriv undermappa foran filnavnet (for eksempel `room/`) hvis den ikke finnes ennå. Når endringen er på main, bygger Pages spillet på nytt med bildene i løpet av et par minutter.

Hvilke filer som er i bruk og hvor, står i `src/core/art.ts` og i tabellene i ART_BRIEF.md.
