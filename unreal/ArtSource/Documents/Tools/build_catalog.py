#!/usr/bin/env python3
"""Extract the document asset copy from the game's existing authoring sources.

Uses only Python's standard library. --check validates the authored catalog
fields while permitting later image-generation metadata such as prompt/hashes.
Running without --check refreshes those authored fields and preserves extras.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys


HERE = Path(__file__).resolve().parent
ASSET_ROOT = HERE.parent
REPO = HERE.parents[3]
OUT = ASSET_ROOT / "catalog.json"
OFFICE_JSON = "unreal/Narrative/EveningShift/modules/office.json"
LORE_MD = "unreal/DALE_OFFICE_DOCUMENTS.md"
CPP_DIR = "unreal/Signal47/Source/Signal47/Private/"
DOCS_CPP = CPP_DIR + "S47Docs.cpp"
CH2_CPP = CPP_DIR + "S47Chapter2.cpp"
CH3_CPP = CPP_DIR + "S47Chapter3.cpp"
CH5_CPP = CPP_DIR + "S47Chapter5.cpp"
OFFICE_CPP = CPP_DIR + "S47Office.cpp"
DINER_TS = "web/src/world/Diner.ts"
STRING = r'((?:[^"\\]|\\.)*)'
DALE_FOLDER_COPY = (
    "SARO\nFIELD MAINTENANCE\nWORKING PAPERS\nSERVICE ROAD / FIELD 01\nDALE\n"
    "KEEP WITH INSPECTION SHEETS\nPRIVATE WORKING COPY\nNot a report.\n"
    "Leave the originals here. - D.\nWORKING COPY"
)
EXPECTED_IDS = [
    "DALE_FOLDER", "EV_HANDOVER", "EV_SERVICE_LOG", "EV_RADIO_SCHEDULE", "EV_DALE_INDEX", "EV_DALE_CLIPPINGS", "EV_DALE_CONTACT", "EV_DALE_NOTE",
    "EV_DALE_SKYLOG", "EV_DALE_WITNESS", "EV_DALE_EXPLANATIONS", "EV_DALE_FRAME18", "EV_DALE_TAPELOAN", "EV_DALE_UNSENT",
    "E15", "E07", "E06", "E08", "B12_LINEAGE", "E09A", "E10", "BINDER_NOTE", "BINDER_1981", "BINDER_1983", "BINDER_1985", "BINDER_1947", "K3_SERVICE",
    "PROP_COUNTY_LIGHTS_1978", "PROP_SENTINEL_MISSING_HOURS_1974", "PROP_CATTLE_EXAM_1982", "PROP_CONTACT_FLYER_1984",
]


def read(path: str) -> str:
    return (REPO / path).read_text(encoding="utf-8")


def decode(value: str) -> str:
    return json.loads('"' + value + '"')


def cpp_constant(path: str, name: str) -> str:
    pattern = rf'\b{re.escape(name)}\s*=\s*((?:\s*TEXT\("(?:[^"\\]|\\.)*"\)\s*)+);'
    match = re.search(pattern, read(path))
    if not match:
        raise ValueError(f"Cannot find literal C++ constant {path}:{name}")
    return "".join(decode(part) for part in re.findall(r'TEXT\("' + STRING + r'"\)', match[1]))


def cpp_doc_meta(path: str, symbol: str, name: str) -> str:
    match = re.search(rf'\b{re.escape(symbol)}\([^)]*\)[^{{]*\{{(.*?)\n\}}', read(path), re.S)
    if not match:
        raise ValueError(f"Cannot find document function {path}:{symbol}")
    value = re.search(rf'\bD\.{re.escape(name)}\s*=\s*TEXT\("' + STRING + r'"\)', match[1])
    if not value:
        raise ValueError(f"Cannot find {name} in {path}:{symbol}")
    return decode(value[1])


def source(path: str, text: str, *, anchor: str | None = None, symbol: str | None = None) -> dict:
    result = {"path": path}
    if anchor is not None:
        result["anchor"] = anchor
    if symbol is not None:
        result["symbol"] = symbol
    result["text_sha256"] = hashlib.sha256(text.encode("utf-8")).hexdigest()
    return result


def build() -> dict:
    assets: list[dict] = []

    def add(id_: str, title: str, slug: str, group: str, status: str, src: dict,
            text: str, checks: list[str], use: str, **extra) -> None:
        assets.append({
            "id": id_, "title": title,
            "filename": f"jpg/{len(assets) + 1:02d}-{slug}.jpg",
            "group": group, "status": status, "source": src, "text": text,
            "key_checks": checks, "intended_use_nb": use, **extra,
        })

    add("DALE_FOLDER", "SARO / Field maintenance / Working papers", "dale-folder", "dale_office", "optional_prop",
        source("unreal/ArtSource/Documents/Tools/build_catalog.py", DALE_FOLDER_COPY, symbol="DALE_FOLDER_COPY"), DALE_FOLDER_COPY,
        ["Cover text matches the approved SARO / FIELD MAINTENANCE / WORKING PAPERS production artwork.",
         "Retain SERVICE ROAD / FIELD 01, DALE, KEEP WITH INSPECTION SHEETS and PRIVATE WORKING COPY.",
         "Retain Not a report., Leave the originals here. - D., and WORKING COPY.",
         "No FIELD CHECKS or TAKE A COPY label is present on this artwork.",
         "A worn work folder; no new case facts, 1947 diagrams or destination coordinates.",
         "Dale collects uncertain leads; he is not an omniscient intelligence officer."],
        "Nytt originalt produksjonsomslag til Dales feltmappe, uten nye kanonbevis.",
        source_state_nb="Omslagsteksten følger det visuelt godkjente produksjonsbildet. Den er skrevet i denne generatoren og endrer ikke runtimeetiketten i S47Office.cpp.")

    office = json.loads(read(OFFICE_JSON))["documents"]
    expected_office = ["EV_HANDOVER", "EV_SERVICE_LOG", "EV_RADIO_SCHEDULE", "EV_DALE_INDEX", "EV_DALE_CLIPPINGS", "EV_DALE_CONTACT", "EV_DALE_NOTE"]
    if [doc["id"] for doc in office] != expected_office:
        raise ValueError("EveningShift document inventory changed; review the source before regenerating")
    office_checks = {
        "EV_HANDOVER": ["18:00 on Sunday, April 13, 1986; operator Reyes, duty officer Ward.", "27 antennas and azimuth reference 026.", "Operations spare belongs in the service drawer at the workbench, not in a mug."],
        "EV_SERVICE_LOG": ["April 12 at 19:06 and April 13 at 16:40 are entered civil times, not precision measurements.", "A lit lamp does not identify its supply; no town-wide interruption was entered.", "Do not add later test results or conflate the town circuit with SARO K3."],
        "EV_RADIO_SCHEDULE": ["AM broadcasting, separate from the observatory's 1420.405 MHz signal.", "Earl's Night Line runs 00:00 to 04:00 on Monday.", "A schedule is not proof of actual transmission; do not invent a call sign or tuning instruction."],
        "EV_DALE_INDEX": ["The T02 position is a low concrete culvert, road side of a post with two white bands, outside the substation fence.", "View keeps nearest pole and its two outer conductors in frame; fence corner is lower left.", "Camera date is wrong; no precise video time, STATION 01 identity or C crossing is established.", "Do not add the separate future ARRAY-01 service positions to this index."],
        "EV_DALE_CLIPPINGS": ["Cutting A: PECOS VALLEY SENTINEL / OCTOBER 22, 1985.", "Cutting B has no known date and describes an auxiliary lamp supply.", "Dale's pencil notes retain ordinary explanations and uncertainty.", "No coordinates, eight-mile clue or E15/1947 photo in these clippings."],
        "EV_DALE_CONTACT": ["Telephone or marked service-radio channel; do not invent a frequency or phone number.", "Ward is the duty officer and is notified first for immediate service problems.", "A contact instruction card, not a photographic contact sheet."],
        "EV_DALE_NOTE": ["Handwritten note beneath the inspection sheets; Dale has found somewhere to look, not a conclusion.", "Keep lamp supply tests and uncertain tape movement separate.", "No revelation of Nora, Tomas, 1947 identities, C, or the finale."],
    }
    office_use = {
        "EV_HANDOVER": "Ordinært overleveringsark fra mappen på SARO ved kveldsskiftets start.",
        "EV_SERVICE_LOG": "Datert frigitt journalkopi i overleveringsmappen; bakgrunn for T01/T02.",
        "EV_RADIO_SCHEDULE": "Ordinær offentlig sendeskjemakopi vedlagt overleveringen.",
        "EV_DALE_INDEX": "Den korte hovedruten fra Dales feltmappe til det faktiske kameraståstedet.",
        "EV_DALE_CLIPPINGS": "To fiktive lokale avisutklipp med Dales kritiske blyantmerknader i privatmappen.",
        "EV_DALE_CONTACT": "Fysisk kontaktkort under kontortelefonen, mulig å ta med.",
        "EV_DALE_NOTE": "Privat forklaring under inspeksjonsarkene, frivillig fordypning.",
    }
    for index, doc in enumerate(office):
        copy = "\n\n".join(doc["body"])
        add(doc["id"], doc["title"], doc["id"].lower().replace("_", "-"), "evening_documents", "authoring_existing",
            source(OFFICE_JSON, copy, anchor=f"/documents/{index}"), copy, office_checks[doc["id"]],
            office_use[doc["id"]], kind=doc["kind"], stamp=doc["stamp"],
            production_notes_nb=doc["production_nb"])

    lore = read(LORE_MD)
    optional = ["EV_DALE_SKYLOG", "EV_DALE_WITNESS", "EV_DALE_EXPLANATIONS", "EV_DALE_FRAME18", "EV_DALE_TAPELOAN", "EV_DALE_UNSENT"]
    lore_checks = {
        "EV_DALE_SKYLOG": ["6 April; kitchen clock about 20:40, not a precision measurement.", "One white light above a ridge; aircraft remain possible.", "The horizon sketch is a witness memory, not measured flight geometry."],
        "EV_DALE_WITNESS": ["29 March; a copied statement with Dale's initials, not an official police finding.", "Mrs. Bell requested the word person, not creature; Dale did not see the shape.", "No confirmed species or identity."],
        "EV_DALE_EXPLANATIONS": ["Three aircraft reports and one star/window-glass explanation remain in the file.", "The service-road witness directions disagree; keep that discrepancy.", "Do not convert a crossed-out explanation into positive proof."],
        "EV_DALE_FRAME18": ["Preserve the LEFT edge and visible porch post as the only useful height reference.", "A pale mark may be kitchen-window glare; the photo is inconclusive.", "A photograph/negative, if illustrated, must be an actual visible prop rather than invented completed evidence."],
        "EV_DALE_TAPELOAN": ["A borrowed public lecture, not Dale's local Service Road / Field 01 VHS.", "Return Tuesday; family birthdays were recorded underneath.", "Use fictional sleeve artwork; no actual third-party lecture footage or attribution is supplied."],
        "EV_DALE_UNSENT": ["An unsent, unaddressed personal letter beneath ordinary working sheets.", "Keep the everyday shopping, forgotten milk and sleep details.", "No all-knowing explanation or new plot revelation."],
    }
    for id_ in optional:
        match = re.search(rf'^### {id_}: ([^\n]+)\n(.*?)(?=^### |^## |\Z)', lore, re.M | re.S)
        if not match:
            raise ValueError(f"Missing lore draft {id_}")
        paragraphs = [line[2:] for line in match[2].splitlines() if line.startswith("> ")]
        if len(paragraphs) != 1:
            raise ValueError(f"Expected one exact blockquote for {id_}")
        copy = paragraphs[0]
        placement = next(line for line in match[2].splitlines() if line.startswith("Plassering:"))
        add(id_, match[1], id_.lower().replace("_", "-"), "dale_private_drafts", "optional_prop",
            source(LORE_MD, copy, anchor=f"### {id_}: {match[1]}"), copy,
            lore_checks[id_] + ["Optional authoring draft; this asset does not register a new runtime document or unlock."],
            placement.removeprefix("Plassering: "), source_state_nb="Eksisterende frivillig manusutkast, ikke registrert i Narrative-JSON eller runtime.")

    diner = read(DINER_TS)
    clip = re.search(r'const clipping=this\.tex\(1024,1536,g=>\{(.*?)\n    \}\);', diner, re.S)
    if not clip:
        raise ValueError("Cannot find original newspaper layout")
    clip_source = clip[1]
    masthead = re.search(r"g\.fillText\('(PECOS VALLEY SENTINEL)'", clip_source)[1]
    headline = "\n".join(re.findall(r"g\.fillText\('(LIGHT HELD OVER MESA|FOR AN HOUR)'", clip_source))
    subtitle_match = re.search(r"\['([^']+)','([^']+)'\]\.forEach", clip_source)
    subtitle = "\n".join(subtitle_match.groups())
    caption = re.search(r"g\.fillText\('(Seen from the Kessler ranch, 3 a\.m\.)'", clip_source)[1]
    newspaper = "\n\n".join([masthead, headline, subtitle, caption])
    add("E15", cpp_doc_meta(CH5_CPP, "US47Chapter5::DocClipping", "Title"), "e15-sentinel-1947", "archive_newspaper", "runtime_reference",
        source(DINER_TS, newspaper, symbol="clipping canvas / masthead, headline, subtitle, photo caption"), newspaper,
        ["Metadata date is July 1947; NO printed date appears on the clipping.",
         "The separate frame pencil note July '47 is not printed newspaper content.",
         "Photo: flat mesa, a steady light high above it, TWO RANCHERS at a fence.",
         "Photo: low to the RIGHT of the mesa, a smudge with a SHORT TAIL; right edge trimmed close.",
         "Composition follows the crop B relationships of photo_1947_master: flat mesa, steady high light, short-tailed smudge low to its right, two foreground ranchers at fence; generated reinterpretation, not a pixel-identical crop.",
         "Source has stylized body-text rules, not a supplied verbatim article; do not invent a historical article transcription."],
        "Gammelt fiktivt avisutklipp fra dinerens vegg, bevart som senere hovedrutebevis.",
        date_metadata="July 1947", kind="newspaper", stamp="E15",
        transcript=cpp_doc_meta(CH5_CPP, "US47Chapter5::DocClipping", "Transcript"),
        transcript_source={"path": CH5_CPP, "symbol": "US47Chapter5::DocClipping"},
        historical_layout_reference_nb="Nettkilden er historisk layoutreferanse; dagens Unreal DocClipping henter samme eksporterte avisflate.")

    archive_specs = [
        ("E07", CH2_CPP, "ORIGINAL", "US47Chapter2::DocOriginal", "e07-original-survey-1947", ["1947, STATION 01, signed N. VEGA / FIELD TECHNICIAN.", "Original drawing MUST retain A, B and C.", "The retained mark survives local lamp isolation; the final observation is interrupted."], "Original feltprotokoll med komplett oppstilling, til sammenligning i arkivet."),
        ("E06", CH2_CPP, "AMENDED", "US47Chapter2::DocAmended", "e06-amended-survey-1947", ["1947, STATION 01, same N. Vega signature as E07.", "Amended arrangement omits C; otherwise align the two diagrams for comparison.", "Do not redact the omission/development explanation or the No repeat observation line."], "Revidert servicekopi som viser den konkrete forskjellen fra originalen."),
        ("E08", CH2_CPP, "INDEX", "US47Chapter2::DocIndex", "e08-archive-sleeve", ["STATION 01 / 1947 / B-12 / A - B - C.", "Fixed-point symbol is an outlined TRIANGLE with a SHORT HORIZONTAL BAR underneath.", "A triangle without the bar is explicitly not a matching reference.", "This is an archive sleeve index, not a player photograph."], "Gulnet arkivhylster med registerkort og korrekt fastpunktsmerke."),
        ("B12_LINEAGE", CH2_CPP, "LINEAGE", "US47Chapter2::DocLineage", "b12-reference-lineage", ["B-12 refers to STATION 01 / OLD SURVEY STATION.", "Preserve the outlined triangle with short horizontal bar beneath it.", "Do not turn this card into the unrelated T02 service-road index."], "Kartotekkort som kobler dagens B-12 til det gamle målefeltet."),
        ("E09A", CH3_CPP, "TRANSIT", "US47Chapter3::DocTransit", "e09a-field-transit", ["A is in the concrete footing on the transit sight line; B is the fixed marker WEST.", "C is a closing sight LINE from the original arrangement, not a separate movable marker.", "Keep the north orientation and physical relations clear if including a diagram."], "Felttransitark fra stålrammen ved målepilaren på STATION 01."),
        ("E10", CH3_CPP, "TIMING_1947", "US47Chapter3::DocTiming", "e10-timing-record-1947", ["Carrier ceased at 02:17:00; relay impact/reference motion at 02:17:47, exactly 47 seconds later.", "Retain the correction Reference west. No. East. Hold the last reading.", "Final observation stays Not entered.", "Only the historical base page is printed; the conditional 1986 received-fragment addition is not part of this 1947 prop."], "Blyantført gammel tidsprotokoll på hyttebenken, uten senere dynamisk spillerobservasjon."),
    ]
    for id_, path, constant, function, slug, checks, use in archive_specs:
        copy = cpp_constant(path, constant)
        add(id_, cpp_doc_meta(path, function, "Title"), slug, "survey_archive", "runtime_reference",
            source(path, copy, symbol=constant), copy, checks, use,
            kind="typed" if id_ != "E10" else "hand", stamp=cpp_doc_meta(path, function, "Stamp"),
            source_document_symbol=function)

    binder_specs = [
        ("BINDER_NOTE", "BINDER_NOTE", "Direction solve / exceptions / Systems note", "binder-systems-note", ["Systems note dated 03/84; the unsigned-range explanation is retained.", "Pencil question: Then why does it keep doing it?"], "Maskinskrevet systemnotat med kritisk blyantmerknad, først i avvikspermen."),
        ("BINDER_1981", "BINDER_PRINT_1", "Direction solve / September 22, 1981", "binder-1981", ["09/22/81 01:52:07; source distance -34 LY.", "1420.405 MHz, PULSE GROUP 4 / 7, INPUT ERROR; pencil clock?"], "Første eldre maskinutskrift, stemplet som inndatafeil."),
        ("BINDER_1983", "BINDER_PRINT_2", "Direction solve / March 2, 1983", "binder-1983", ["03/02/83 03:10:44; source distance -36 LY.", "1420.405 MHz, PULSE GROUP 4 / 7, INPUT ERROR; pencil baseline file reloaded."], "Andre eldre maskinutskrift med Dales noterte kontrollforsøk."),
        ("BINDER_1985", "BINDER_PRINT_3", "Direction solve / October 19, 1985", "binder-1985", ["10/19/85 23:58:31; source distance -38 LY.", "1420.405 MHz, PULSE GROUP 4 / 7, INPUT ERROR; pencil third time. Ask Systems."], "Tredje eldre maskinutskrift, med samme oppsett som de andre."),
        ("BINDER_1947", "BINDER_1947", "Direction solve / July 1947 / undated day", "binder-1947", ["DATE 07/--/47 02:17:00; the day is intentionally unknown.", "1420.405 MHz, PULSE GROUP 4 / 7, source distance -00 LY.", "Old yellowed paper, NO stamp and NO pencil.", "The solver did not exist in 1947; preserve the contradiction without explaining it."], "Permens siste, eldste ark med den bevisste tidsmessige motsigelsen."),
        ("K3_SERVICE", "SERVICE_PAGE", "RX bank 3 / Service record", "k3-service-record", ["09/23/81, 03/03/83, 10/21/85 maintenance entries.", "K3 trips BEFORE the power loss: approximately 1 minute, then before the dip, then 40 seconds.", "Keep initials M.O. and the conclusion IT TRIPS BEFORE, NOT AFTER.", "SARO's internal RX Bank 3 / K3 only; do not merge with town feeder or STATION 01 circuits."], "Vedlikeholdsklipsark fra SAROs rack, til senere mønstersammenligning."),
    ]
    for id_, constant, title, slug, checks, use in binder_specs:
        copy = cpp_constant(DOCS_CPP, constant)
        add(id_, title, slug, "saro_technical_archive", "runtime_reference",
            source(DOCS_CPP, copy, symbol=constant), copy, checks, use, kind="typed")

    if len(assets) != 27:
        raise ValueError(f"Expected 27 source-bound base assets, got {len(assets)}")
    supplemental_path = "unreal/ArtSource/Documents/supplemental.json"
    supplemental = json.loads(read(supplemental_path))
    expected_supplemental = EXPECTED_IDS[27:]
    if [item["id"] for item in supplemental["assets"]] != expected_supplemental:
        raise ValueError("Expected four ordered optional background props")
    for index, item in enumerate(supplemental["assets"]):
        if item["status"] != "optional_prop" or len(item["text"].split()) > 150:
            raise ValueError(f"Supplemental prop {item['id']} must be optional and at most 150 words")
        if not item["filename"].startswith(f"jpg/{index + 28:02d}-") or not item["filename"].endswith(".jpg"):
            raise ValueError(f"Wrong supplemental filename for {item['id']}")
        assets.append({**item, "source": source(supplemental_path, item["text"], anchor=f"/assets/{index}")})
    if [asset["id"] for asset in assets] != EXPECTED_IDS or len({asset["filename"] for asset in assets}) != len(EXPECTED_IDS):
        raise ValueError("Catalog order, IDs or filenames differ from the 31 expected assets")
    return {
        "schema_version": 1,
        "language": "en",
        "notes_nb": [
            "31 rekvisittbilder til SIGNAL / 47. Spilltekst er engelsk; norske metadata forklarer bruk og begrensninger.",
            "27 grunnrekvisitter bruker eksisterende forfatter-/runtimekilder. Fire ekstra bakgrunnsrekvisitter er nyskrevet fiksjon i supplemental.json, uten runtimekobling eller nye kanoniske bevis.",
            "source.text_sha256 gjelder tekstfeltet, ikke bildefilen. De fire nye tekstene er under 150 ord hver; datoene ligger før spillnatten 13. april 1986.",
            "authoring_existing betyr eksisterende EveningShift-forfatterkilde. runtime_reference betyr allerede definerte hovedrute-/arkivdokumenter. optional_prop betyr mappeomslag eller frivillige private manusutkast uten ny runtime-registrering.",
            "De seks private utkastene registreres ikke som nye Narrative-JSON- eller runtime-dokumenter av denne assetleveransen.",
            "Katalogens samlede arkivmateriale er ikke et påstått innhold i Dales private mappe; survey- og tidsprotokoller har egne steder og hovedruteporter.",
            "Ingen kritiske kanonfelt, måleverdier eller sammenligningsforskjeller skal skjules av dekorativ sladding. Manusets ukjente datoer forblir ukjente.",
            "E15s dato July 1947 er metadata. Avisutklippet har ingen trykt dato; separat blyantnotat på rammen er historisk datogrunnlag.",
            "I den nye county-rapporten kan kun [REDACTED]-feltene for navn/adresse sladdes. Ingen observasjoner, konklusjonsbegrensninger eller ledetråder skjules.",
            "Innebygde bildeprompter, bildehasher og størrelser kan legges til senere uten å endre de kildeuttrukne tekstfeltene.",
        ],
        "assets": assets,
    }


def check_fields(actual: dict, expected: dict, path: str = "catalog") -> list[str]:
    problems = []
    for key, value in expected.items():
        current = actual.get(key)
        if key == "assets" and isinstance(current, list):
            if len(current) != len(value):
                problems.append(f"{path}.assets: expected {len(value)}, got {len(current)}")
            for index, asset in enumerate(value):
                if index < len(current):
                    problems.extend(check_fields(current[index], asset, f"{path}.assets[{index}]"))
        elif current != value:
            problems.append(f"{path}.{key}: differs from current source extraction")
    return problems


def preserve_extras(existing: dict, expected: dict) -> dict:
    merged = {**existing, **expected}
    old_assets = {item.get("id"): item for item in existing.get("assets", [])}
    merged["assets"] = [{**old_assets.get(item["id"], {}), **item} for item in expected["assets"]]
    return merged


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate authored catalog fields without writing")
    args = parser.parse_args()
    expected = build()
    existing = json.loads(OUT.read_text(encoding="utf-8")) if OUT.exists() else {}
    if args.check:
        problems = check_fields(existing, expected)
        if problems:
            print("\n".join(problems), file=sys.stderr)
            return 1
        print(f"PASS: {len(expected['assets'])} assets; unique IDs/filenames; exact source text, titles, dates, checks and metadata.")
        return 0
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(preserve_extras(existing, expected), ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT.relative_to(REPO)}: {len(expected['assets'])} source-bound document assets.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
