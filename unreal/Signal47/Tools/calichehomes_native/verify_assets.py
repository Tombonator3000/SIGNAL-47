"""Fersk, lesende editorverifikasjon av lagrede Caliche Homes-assets.

Samme S47_HOMES_SOURCE/TRIMS/REPORT som import_assets. Ingen importer,
lagring, sletting, materialsettere eller grafmutasjon. Bare bevis-JSON skrives.
"""
import os
from pathlib import Path
import sys
import traceback

sys.path.insert(0, str(Path(__file__).resolve().parent))
import native as N


def main(root, trims, report):
    protected = N.protect_dependencies(report)
    own = N.PROJECT / 'Content/CalicheHomes'
    before = {str(p.relative_to(N.PROJECT)): N.sha(p) for p in sorted(own.rglob('*')) if p.is_file()}
    report.check('saved native directory exists', own.is_dir() and bool(before))
    N.verify_all(root, trims, report)
    N.check_preserved(protected, report)
    after = {str(p.relative_to(N.PROJECT)): N.sha(p) for p in sorted(own.rglob('*')) if p.is_file()}
    report.check('fresh verifier left all owned native bytes untouched', before == after)
    report.data.update(status='PASS', activation='NOT_STAGED: runtime assembly is a separate atomic gate')
    report.flush()
    N.unreal.log('HOMES_SAVED_NATIVE_PASS')


if __name__ == '__main__':
    root, trims = Path(os.environ['S47_HOMES_SOURCE']).resolve(), Path(os.environ['S47_HOMES_TRIMS']).resolve()
    output = Path(os.environ['S47_HOMES_REPORT']).resolve()
    assert not any(output.is_relative_to(p) for p in (root, trims, N.PROJECT / 'Content')), 'Bevisrapporten må ligge utenfor kilde-/assetmappene'
    report = N.Report(output, 'saved-native-read-only')
    try:
        main(root, trims, report)
    except Exception:
        report.data.update(status='FAIL', error=traceback.format_exc())
        report.flush()
        raise
