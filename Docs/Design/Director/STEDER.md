# Stedskatalogen: hvor nattens hendelser skjer

**Status:** Under arbeid. **Gjelder:** Freeform, TODO ANYWHERE-20261108 og ARRAY-ORBS-20261108 (milepæl M1). **Sist kontrollert:** 8. november 2026 mot koden i `S47EventPlaces.h/.cpp`.

Tom 8. november: «poenget med freeform er at hendelser ved gården kan komme hvor som helst i verdenen». Hendelsene henter nå stedet sitt fra én felles liste med steder. Gården er ett av stedene på lista, på lik linje med kirka eller et tørrløp ute på sletta.

## Lista

Alle posisjoner er i road-local meter, landets felles ramme (`S47OpenLand`). Området (`saro`, `road`, `station01`) regnes ut fra posisjonen når det trengs, og lagres ikke.

| Gruppe | Steder |
|---|---|
| Gården | `farm` (beitet der merkene alltid har ligget), `oldhouse` (dører og vinduer), `barn` |
| SARO | `saro` (bygget), `saro_yard` (den østre gården), `array` |
| Motellet | `motel` |
| Caliche | `gas`, `sheriff`, `laundry`, `cinema`, `feed`, `church`, `souvenir`, `radio` (1420 AM), `h01` til `h04`, `diner` |
| Stasjonene | `station01`, `feeder` (når feltet har den) |
| Ute på landet | stedene fra `FS47Places` (vindmølla, tanken, gruva, ballongen, kua, hytta, varden, stikka, trauet) |
| Åpent land | `land:wash_west`, `land:wash_mine`, `land:wash_east`, `land:flat_north`, `land:playa`, `land:far_south`, `land:far_west`, `land:far_east` |
| Ved veien | `road:pullout_north`, `road:turnoff`, `road:cattle_guard`, `road:pullout_south` |

Egenskaper: innendørs, dør, vindu, telefon, ved vei, åpent land, langt ute, folk, gård, by, SARO, tørrløp, parkering, array. Dørene i Caliche og døra i gamle huset er de dørene et dørbesøk kan komme til.

## Trekningen

`Pick(behov, salt)` velger et sted ut fra hendelsens krav, spillerens posisjon, natta og spillets frø. Samme frø, natt, behov og historikk gir samme svar, så en innlasting endrer ingenting.

- Steder som hadde noe samme natt, vektes kraftig ned. De neste tre nettene vektes de gradvis mindre ned.
- Steder nær spilleren vektes opp, og når spilleren kjører, også steder i kjøreretningen.
- En andel trekninger går langt unna (over 900 m), til noe som høres på radioen.
- `Used(sted, art)` merker at noe faktisk skjedde der. Det lagres under `eventPlaces`.

## Hva som bruker lista

- **Lys over et sted.** `farm_lights` er nå «sett herfra»: et sted 250 til 1600 m unna på samme grunn. `town_lights` er «hørt om»: et sted på en annen grunn, en kjøretur unna. Hvem som melder det, følger stedet (noen i byen, en trucker, nattportieren, en rancher).
- **Merker og besøk** (`site_visit`) legges der lysene var. Ringens størrelse og hvem som kommer, avhenger av stedets egenskaper og ikke av om stedet er gården.
- **Merket grunn og fragment** (`landing_trace`): åpen grunn innen 700 m på grunnen spilleren står på. Er stedet langt unna, kommer et radiotips.
- **Dørbesøk, titting og opplyst rom** skjer der spilleren er. Lista sier hvilket sted det er, gir tittelen og merker stedet som brukt.
- **Feltbesøket** får navn etter nærmeste sted. **Samtalen på rolige netter** handler om et sted fra lista.
- **Dronelysene (ARRAY-ORBS)** kommer første gang over S-02 og S-03, der de alltid har kommet. Deretter kommer de over et annet antennepar, trukket med spillets frø og besøkets.

Ikke flyttet: skikkelsen ved arrayen, `service_presence`, feederen og de små tingene på SARO. De hører til stasjonen.

## F10

Gruppen «Anywhere: events at a place from the list» har en stedsvelger. Den står på «random», som er spillets egen trekning, eller på hvert sted i tur. Hver hendelse kan startes ved valgt eller tilfeldig sted. «Where the draw puts things now» viser hvor trekningen ville lagt hendelsene fra der spilleren står.

## Regler som står fast

Én fare per natt, ro etter fare og fredet første natt gjelder som før (`S47Rhythm`). Prøver som starter en hendelse med `Force` uten valgt sted, beholder sitt gamle sted: gården fra SARO, den østre gården og lys de kan se fra gata.
