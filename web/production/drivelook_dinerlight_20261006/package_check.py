#!/usr/bin/env python3
"""Reproducible package contracts; test images are synthetic, never render proof.

    python3 package_check.py --result /tmp/package-checks.json

Optional read-only controls from the 2026-10-06 capture:
    python3 package_check.py --recorded-root ../../shots --pages-tar /tmp/artifact.tar \
      --result /tmp/package-checks.json
The recorded root must contain dinerlight_low_844x390_20261006_v4 (positive)
and dinerlight_low_844x390_20261006_v2 (stale setup negative). Missing controls
are SKIPPED. Earlier package_checks.json records the historical 17-test contract.
Paths are resolved from the current directory. No game/browser is started.
"""
import argparse
import asyncio, copy, hashlib, importlib.util, io, json
from pathlib import Path
import sys, tarfile, tempfile, unittest, math
from datetime import datetime,timezone
from unittest.mock import patch
from types import SimpleNamespace
from PIL import Image

PROD=Path(__file__).resolve().parent
RECORDED_ROOT=PAGES_TAR=None
RECORDED_RESULTS=[]
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec)
    sys.modules[name]=m;spec.loader.exec_module(m);return m
pkg=load('package_contract_subject',PROD/'package_evidence.py')
fakes=load('dinerlight_package_fakes',PROD/'tool_check.py')
URL='http://test/'

class Page(fakes.DinerPage):
    cab_override=False
    async def goto(self,url,**kwargs):
        await super().goto(url,**kwargs)
        for name,body in [('Diner-mock.js',b'diner'),('OldRoad-mock.js',b'oldroad')]:
            self.events['response'](fakes.hash_checks.Response(URL+'assets/'+name,body=body))
    def model_state(self):
        st=super().model_state();st['post'].update(on=self.quality!='low',ultra=self.quality=='ultra')
        if self.cab_override:st['truck_view_state'].update(cab_visible=True,shell_visible=False)
        return st
    def frozen(self):
        st=self.model_state();saved=super().frozen();view=st['truck_view_state']
        saved.update(truck_driving=view['driving'],cab_visible=view['cab_visible'],shell_visible=view['shell_visible'],headlights=[])
        return saved
    async def evaluate(self,code,arg=None):
        if code==fakes.capture.DINERLIGHT_RESTORE:self.cab_override=False
        saved=await super().evaluate(code,arg)
        if code==fakes.capture.DINERLIGHT_LOOK and arg['cab']:
            self.cab_override=True;saved['after']=self.frozen()
        return saved
    async def screenshot(self,*,path,**kwargs):
        self.screenshots+=1;Image.new('RGB',(844,390),(23,47,11)).save(path,format='PNG')

class Browser(fakes.DinerBrowser):
    async def new_context(self,**kwargs):
        c=fakes.DinerContext(Page('',self.quality));self.contexts.append(c);return c

async def fixture(folder,quality='high'):
    report={'schema_version':1,'status':'CAPTURED','visual_review':'UNVERIFIED','failures':[],
      'requested_trips':list(pkg.PHASES),'url':URL,'interval_m':40,'fps':30,
      'quality_requested':quality,'source':{'git_head':'synthetic-mock-source'},'trips':[]}
    with patch('sys.stdout',new=io.StringIO()):
        for phase in pkg.PHASES:
            await fakes.capture.run_dinerlight_trip(Browser(quality=quality),
              SimpleNamespace(size=(844,390),quality=quality),report,phase,URL,folder)
    pkg.dump(folder/'manifest.json',report)
    return report

class PackageContracts(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
        self.owner=self.root/'production';self.owner.mkdir()
        self.source=self.root/'source';self.source.mkdir()
        self.manifest=asyncio.run(fixture(self.source))
        self.tar=self.root/'artifact.tar'
        with tarfile.open(self.tar,'w') as t:
            for name,body in [('index.html',b'html'),('assets/code.js',b'code'),
              ('assets/Diner-mock.js',b'diner'),('assets/OldRoad-mock.js',b'oldroad')]:
                member=tarfile.TarInfo('./'+name);member.size=len(body);t.addfile(member,io.BytesIO(body))
        self.before=pkg.snapshot(self.source)
        self.owner_patch=patch.object(pkg,'PRODUCTION_ROOT',self.owner);self.owner_patch.start()
    def tearDown(self):self.owner_patch.stop();self.temp.cleanup()
    def run_pack(self,*,sources=None,target=None,require_all=False):
        target=target or self.owner/'evidence'
        args=['--captures',*[str(p) for p in (sources or [self.source])],'--target',str(target),
          '--pages-tar',str(self.tar),'--expected-tar-sha256',pkg.sha(self.tar),'--served-from',URL]
        if require_all:args+=['--require-all']
        with patch('sys.stdout',new=io.StringIO()):code=pkg.main(args)
        report=json.loads((target/'evidence_manifest.json').read_text()) if (target/'evidence_manifest.json').is_file() else None
        return code,report,target
    def change(self,edit):
        m=copy.deepcopy(self.manifest);edit(m);pkg.dump(self.source/'manifest.json',m);self.before=pkg.snapshot(self.source)
    def assert_raw(self,report,target):
        self.assertEqual(pkg.snapshot(self.source),self.before)
        self.assertEqual(pkg.snapshot(target/'captures'/report['entries'][0]['package_name']),self.before)
        self.assertEqual(report['visual_status'],'UNVERIFIED')
    def test_valid_subset_preserves_raw_and_has_labelled_contact_sheets(self):
        code,r,target=self.run_pack();self.assertEqual(code,0);self.assertEqual(r['status'],'PARTIAL')
        self.assertEqual(r['completed_configs'],1);self.assert_raw(r,target)
        e=r['entries'][0];self.assertEqual(e['integrity_status'],'PASS')
        self.assertTrue(all(c['status']=='PASS' for c in e['checks']))
        self.assertTrue(all(row['matches'] for row in e['loaded_code']))
        for a in e['derived_assets']:
            p=target/'derived'/e['package_name']/a['file'];self.assertEqual(pkg.sha(p),a['sha256'])
            with Image.open(p) as image:self.assertEqual(image.size,(a['width'],a['height']))
        self.assertIn('UNVERIFIED',(target/e['gallery']).read_text())
    def test_existing_target_is_refused_without_touching_any_bytes(self):
        _,_,target=self.run_pack();before=pkg.snapshot(target)
        code,r,_=self.run_pack(target=target);self.assertEqual(code,2);self.assertEqual(pkg.snapshot(target),before)
        self.assertEqual(pkg.snapshot(self.source),self.before)
    def test_missing_inventory_or_unclosed_pending_response_cannot_pass(self):
        cases=[lambda m:m['trips'][0].pop('observed_code_responses'),
          lambda m:m['trips'][0].update(code_observation_closed=False),
          lambda m:m['trips'][0]['observed_code_responses'][0].update(hash_state='pending')]
        for i,edit in enumerate(cases):
            self.change(edit);code,r,target=self.run_pack(target=self.owner/f'fail{i}')
            self.assertEqual(code,1);self.assertEqual(r['status'],'FAIL');self.assert_raw(r,target)
            self.assertNotIn('gallery',r['entries'][0])
    def test_loaded_code_mismatch_png_hash_wrong_dpr_or_picture_preserves_fail_evidence(self):
        cases=[lambda m:m['trips'][0]['observed_code_responses'][0].update(sha256='0'*64),
          lambda m:m['trips'][0]['shots'][0].update(image_sha256='0'*64),
          lambda m:m['trips'][0]['shots'][0]['render_state'].update(pixel_ratio=2),
          lambda m:m['trips'][0]['shots'][0].update(picture='off')]
        for i,edit in enumerate(cases):
            self.change(edit);code,r,target=self.run_pack(target=self.owner/f'mismatch{i}')
            self.assertEqual(code,1);self.assert_raw(r,target)
    def test_late_console_and_nonfavicon_http_are_strict(self):
        cases=[lambda m:m['trips'][0]['console_messages'].append({'type':'error','text':'late error','location':{}}),
          lambda m:m['trips'][1]['failed_http'].append({'url':URL+'late.png','status':404,'resource_type':'image'})]
        for i,edit in enumerate(cases):
            self.change(edit);code,r,target=self.run_pack(target=self.owner/f'late{i}')
            self.assertEqual(code,1);self.assert_raw(r,target)
    def test_only_url_confirmed_favicon_is_nonblocking(self):
        text='Failed to load resource: the server responded with a status of 404 (Not Found)'
        self.change(lambda m:m['trips'][0]['console_messages'].append({'type':'error','text':text,'location':{'url':URL+'favicon.ico'}}))
        code,r,_=self.run_pack();self.assertEqual(code,0);self.assertEqual(r['entries'][0]['integrity_status'],'PASS')
    def test_require_all_blocks_subset_and_duplicate_configs_are_not_filtered(self):
        code,r,target=self.run_pack(require_all=True);self.assertEqual(code,1);self.assertEqual(len(r['missing_configs']),5);self.assert_raw(r,target)
        code,r,target=self.run_pack(sources=[self.source,self.source],target=self.owner/'duplicate')
        self.assertEqual(code,1);self.assertEqual(len(r['entries']),2)
        self.assertEqual(len(list((target/'captures').iterdir())),2);self.assertEqual(r['entries'][1]['integrity_status'],'FAIL')
    def test_malformed_manifest_preserves_raw_input_and_partial_failure_report(self):
        (self.source/'manifest.json').write_text('{broken json');self.before=pkg.snapshot(self.source)
        code,r,target=self.run_pack();self.assertEqual(code,1);self.assert_raw(r,target)
        self.assertIn('JSONDecodeError',r['entries'][0]['failure'])
    def test_wrong_manifest_containers_are_explicit_preserved_failures(self):
        for i,value in enumerate(([],None,{'trips':[None]})):
            pkg.dump(self.source/'manifest.json',value);self.before=pkg.snapshot(self.source)
            code,r,target=self.run_pack(target=self.owner/f'container{i}')
            self.assertEqual(code,1);self.assert_raw(r,target);self.assertIn('ValueError',r['entries'][0]['failure'])
    def test_sideproof_without_required_image_is_not_complete_evidence(self):
        for i,name in enumerate(('parked_sign_lights_off','approach_start_300m','prepark_forward_lights_on')):
            def edit(m):
                t=m['trips'][0];shot=next(s for s in t['shots'] if s['name']==name)
                (self.source/shot['file']).unlink(missing_ok=True);t['shots'].remove(shot)
            self.change(edit);code,r,target=self.run_pack(target=self.owner/f'missing-shot{i}')
            self.assertEqual(code,1);self.assert_raw(r,target)
            # Restore the removed synthetic PNG for the next counterexample.
            shot=next(s for s in self.manifest['trips'][0]['shots'] if s['name']==name)
            Image.new('RGB',(844,390),(23,47,11)).save(self.source/shot['file'])
    def test_ultra_trace_requires_raw_ledger_pose_preset_and_candidates(self):
        extra=self.root/'ultra';extra.mkdir();original=asyncio.run(fixture(extra,'ultra'))
        for i,key in enumerate(('body_light_ledger','camera_xyz','quality','light_findings')):
            m=copy.deepcopy(original);m['trips'][0]['onfoot_trace'][5]['state'].pop(key)
            pkg.dump(extra/'manifest.json',m)
            code,r,target=self.run_pack(sources=[extra],target=self.owner/f'trace{i}')
            self.assertEqual(code,1);self.assertEqual(r['entries'][0]['integrity_status'],'FAIL')
            self.assertEqual(pkg.snapshot(extra),pkg.snapshot(target/'captures'/r['entries'][0]['package_name']))
    def test_natural_park_camera_sync_is_observed_not_invented_for_old_or_stale_rows(self):
        def stale_park(m):
            s=next(s for s in m['trips'][0]['shots'] if s['name']=='arrival_end_parked')
            s['camera_xyz'][0]+=1
            s['camera_player_horizontal_m']=1
        cases=[lambda m:m['trips'][0].pop('park_camera_sync'),
          lambda m:m['trips'][0].pop('park_callback_state'),
          lambda m:m['trips'][0]['park_camera_sync'].update(normal_tick_frames=100),
          lambda m:m['trips'][0]['park_camera_sync'].update(camera_assignment_calls=1),stale_park]
        for i,edit in enumerate(cases):
            self.change(edit);code,r,target=self.run_pack(target=self.owner/f'camera{i}')
            self.assertEqual(code,1);self.assert_raw(r,target)
    def test_ultra_final_camera_must_sync_but_immediate_diagnostic_can_be_stale(self):
        extra=self.root/'ultra-camera';extra.mkdir();original=asyncio.run(fixture(extra,'ultra'))
        st=original['trips'][1]['onfoot_trace'][0]['state']
        st['camera_xyz'][0]+=50;st['camera_player_horizontal_m']=math.hypot(st['camera_xyz'][0]-st['player_xyz'][0],st['camera_xyz'][2]-st['player_xyz'][2])
        pkg.dump(extra/'manifest.json',original)
        code,r,_=self.run_pack(sources=[extra]);self.assertEqual(code,0);self.assertEqual(r['entries'][0]['integrity_status'],'PASS')
        for i,where in enumerate(('trace','image')):
            m=copy.deepcopy(original);t=m['trips'][1]
            st=t['onfoot_trace'][-1]['state'] if where=='trace' else next(s for s in t['shots'] if s['name']=='ultra_onfoot_after_0_5s')
            st['camera_xyz'][0]+=1;st['camera_player_horizontal_m']=1
            pkg.dump(extra/'manifest.json',m)
            code,r,_=self.run_pack(sources=[extra],target=self.owner/f'ultra-final{i}')
            self.assertEqual(code,1);self.assertEqual(r['entries'][0]['integrity_status'],'FAIL')
    def test_setup_sync_requires_one_ordinary_rest_frame_with_no_assignments_or_movement(self):
        cases=[lambda m:m['trips'][1].pop('setup_sync'),
          lambda m:m['trips'][1]['setup_sync'].update(normal_frames=0),
          lambda m:m['trips'][1]['setup_sync'].update(normal_frames=2),
          lambda m:m['trips'][1]['setup_sync'].update(simulation_advance_s=.25),
          lambda m:m['trips'][1]['setup_sync'].update(driver_pose_unchanged=False)]
        for key in ('clock_assignment_calls','ui_assignment_calls','sky_assignment_calls','placement_calls'):
            cases.append(lambda m,k=key:m['trips'][1]['setup_sync'].update({k:1}))
        for key,value in (('autopilot_mps',20),('test_input_active',True),('old_control_active',True),
                         ('speed_mps',1),('busy',True),('driver_xyz',[5,0,0]),('heading_rad',1),
                         ('chapter_phase','ch5'),('area','saro'),('leg','saro'),('clock_seconds',19220)):
            cases.append(lambda m,k=key,v=value:m['trips'][1]['setup_sync']['after'].update({k:v}))
        for i,edit in enumerate(cases):
            self.change(edit);code,r,target=self.run_pack(target=self.owner/f'setup{i}')
            self.assertEqual(code,1);self.assert_raw(r,target)
            self.assertTrue(any('setup' in c['check'] and c['status']=='FAIL' for c in r['entries'][0]['checks']))
        # A moved initial PNG cannot be legitimised by copying its pose into route_start.
        def moved_start(m):
            t=m['trips'][1]
            for st in (t['route_start'],t['shots'][0]):st['camera_xyz'][0]+=1
        self.change(moved_start);code,r,target=self.run_pack(target=self.owner/'first-png-after-motion')
        self.assertEqual(code,1);self.assert_raw(r,target)
        self.assertTrue(any('sync end agrees' in c['check'] and c['status']=='FAIL' for c in r['entries'][0]['checks']))
    def test_every_png_rejects_stale_hud_or_sky_and_missing_observed_clock_inputs(self):
        def image(m):return m['trips'][1]['shots'][0]
        cases=[lambda m:image(m).update(hud_clock='05:00'),
          lambda m:image(m).pop('hud_clock'),
          lambda m:image(m)['sky_uniforms'].update(uDawn=0),
          lambda m:image(m)['sky_uniforms'].update(uSun=1),
          lambda m:image(m)['sky_uniforms'].update(uSunDir=[0,0,0]),
          lambda m:image(m).pop('sky_clock_inputs'),
          lambda m:image(m)['sky_clock_inputs'].pop('sun_elev_deg'),
          lambda m:image(m).update(sky_clock_inputs=None),
          lambda m:image(m).update(sky_uniforms=None),
          lambda m:m['trips'][0]['shots'][-1].update(hud_clock='05:20')]
        for i,edit in enumerate(cases):
            self.change(edit);code,r,target=self.run_pack(target=self.owner/f'clock{i}')
            self.assertEqual(code,1);self.assert_raw(r,target)
            self.assertTrue(any('HUD and Sky clock agree for PNG' in c['check'] and c['status']=='FAIL' for c in r['entries'][0]['checks']))
    def test_clock_roundoff_is_allowed_but_sky_changes_above_tolerance_are_blocking(self):
        # A held diagnostic is not constrained to identical route-start uniform bytes.
        def perturb(m,delta):m['trips'][0]['shots'][-1]['sky_uniforms']['uDawn']+=delta
        self.change(lambda m:perturb(m,5e-7));code,r,target=self.run_pack()
        self.assertEqual(code,0);self.assertEqual(r['entries'][0]['integrity_status'],'PASS');self.assert_raw(r,target)
        self.change(lambda m:perturb(m,2e-6));code,r,target=self.run_pack(target=self.owner/'above-clock-tolerance')
        self.assertEqual(code,1);self.assert_raw(r,target)
    def test_unsafe_image_name_and_extra_png_cannot_be_silently_dropped(self):
        self.change(lambda m:m['trips'][0]['shots'][0].update(file='../outside.png'))
        code,r,target=self.run_pack();self.assertEqual(code,1);self.assert_raw(r,target)
        self.change(lambda m:None);Image.new('RGB',(1,1)).save(self.source/'extra.png');self.before=pkg.snapshot(self.source)
        code,r,target=self.run_pack(target=self.owner/'extra');self.assertEqual(code,1);self.assert_raw(r,target)
    def test_target_confinement_and_unsafe_tar_are_blocking(self):
        code,r,target=self.run_pack(target=self.root/'outside');self.assertEqual(code,2);self.assertFalse(target.exists())
        with tarfile.open(self.tar,'w') as t:
            p=tarfile.TarInfo('../escape.js');p.size=3;t.addfile(p,io.BytesIO(b'bad'))
        code,r,target=self.run_pack();self.assertEqual(code,1);self.assert_raw(r,target)
        self.assertEqual(json.loads((target/'pages_artifact_index.json').read_text())['status'],'FAIL')
    def test_actual_v4_positive_and_v2_stale_setup_negative_are_read_only_controls(self):
        if RECORDED_ROOT is None or PAGES_TAR is None:
            self.skipTest('Optional recorded controls were not requested; capture revalidation UNVERIFIED')
        names=('dinerlight_low_844x390_20261006_v4','dinerlight_low_844x390_20261006_v2')
        missing=[str(p) for p in (PAGES_TAR,*[RECORDED_ROOT/n/'manifest.json' for n in names]) if not p.is_file()]
        if missing:self.skipTest('Recorded controls unavailable: '+', '.join(missing))
        if json.loads((RECORDED_ROOT/names[0]/'manifest.json').read_text()).get('status')=='CAPTURING':
            self.skipTest('Optional v4 positive control is still CAPTURING; capture revalidation UNVERIFIED')
        hashes=pkg.artifact_index(PAGES_TAR);helper=pkg.load_capture()
        for name,status in zip(names,('PASS','FAIL')):
            folder=RECORDED_ROOT/name;before=pkg.snapshot(folder)
            report=pkg.validate(folder,json.loads((folder/'manifest.json').read_text()),hashes,
              'https://tombonator3000.github.io/SIGNAL-47/',helper)
            self.assertEqual(report['integrity_status'],status);self.assertEqual(len(report['images']),24)
            self.assertEqual(pkg.snapshot(folder),before)
            if status=='FAIL':
                self.assertTrue(any(('setup render sync' in c['check'] or 'HUD and Sky clock' in c['check']) and c['status']=='FAIL' for c in report['checks']))
            RECORDED_RESULTS.append({'input':str(folder),'expected_integrity_status':status,
              'observed_integrity_status':report['integrity_status'],'checks':len(report['checks']),
              'checks_passed':sum(c['status']=='PASS' for c in report['checks']),
              'images':len(report['images']),'raw_files_unchanged':True})
    def test_exact_six_configurations_package_and_preserve_every_synthetic_raw_file(self):
        sources=[]
        for quality in pkg.QUALITIES:
            for w,h in pkg.SIZES:
                folder=self.root/f'{quality}_{w}x{h}';folder.mkdir();m=asyncio.run(fixture(folder,quality))
                if (w,h)!=(844,390):
                    for t in m['trips']:
                        for s in t['shots']:
                            s['viewport'].update(width=w,height=h)
                            s['render_state'].update(buffer_width=w,buffer_height=h)
                            Image.new('RGB',(w,h),(23,47,11)).save(folder/s['file'])
                            s['image_sha256']=pkg.sha(folder/s['file'])
                    pkg.dump(folder/'manifest.json',m)
                sources.append(folder)
        before={p:pkg.snapshot(p) for p in sources}
        code,r,target=self.run_pack(sources=sources,require_all=True)
        self.assertEqual(code,0);self.assertEqual(r['status'],'PACKAGED');self.assertEqual(r['completed_configs'],6)
        self.assertEqual(r['missing_configs'],[])
        self.assertEqual({e['config'] for e in r['entries']},pkg.EXPECTED_CONFIGS)
        self.assertEqual(sum(len(e['images']) for e in r['entries']),156)
        for source,e in zip(sources,r['entries']):
            self.assertEqual(pkg.snapshot(source),before[source])
            self.assertEqual(pkg.snapshot(target/'captures'/e['package_name']),before[source])

def main(argv=None):
    global RECORDED_ROOT,PAGES_TAR
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--result',type=Path)
    parser.add_argument('--recorded-root',type=Path)
    parser.add_argument('--pages-tar',type=Path)
    args=parser.parse_args(argv)
    if bool(args.recorded_root)!=bool(args.pages_tar):
        parser.error('--recorded-root and --pages-tar must be supplied together')
    RECORDED_ROOT=args.recorded_root.resolve() if args.recorded_root else None
    PAGES_TAR=args.pages_tar.resolve() if args.pages_tar else None
    RECORDED_RESULTS.clear()
    stream=io.StringIO();suite=unittest.defaultTestLoader.loadTestsFromTestCase(PackageContracts)
    result=unittest.TextTestRunner(stream=stream,verbosity=2).run(suite)
    output={'status':'PASS' if result.wasSuccessful() else 'FAIL','tests_run':result.testsRun,
      'failures':len(result.failures),'errors':len(result.errors),'browser_launched':False,
      'evidence_kind':'synthetic_package_contracts_with_optional_read_only_capture_controls','rendered_lighting':'UNVERIFIED',
      'checked_utc':datetime.now(timezone.utc).isoformat(),
      'packager_sha256':pkg.sha(PROD/'package_evidence.py'),'test_sha256':pkg.sha(Path(__file__)),
      'capture_helper_sha256':pkg.sha(pkg.WEB/'tools/drivelook.py'),
      'fixture_helper_sha256':pkg.sha(PROD/'tool_check.py'),
      'skipped':[{'test':str(t),'reason':why} for t,why in result.skipped],
      'recorded_controls_status':'CHECKED' if len(RECORDED_RESULTS)==2 else 'SKIPPED',
      'recorded_controls':RECORDED_RESULTS,'output':stream.getvalue()}
    if args.result:args.result.write_text(json.dumps(output,indent=2)+'\n')
    print(stream.getvalue());print(f'{output["status"]}: {result.testsRun} tests, {len(result.skipped)} skipped; rendered lighting UNVERIFIED')
    return 0 if result.wasSuccessful() else 1

if __name__=='__main__':raise SystemExit(main())
