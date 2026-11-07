"""Beslutningene i saro_hold_soak uten motor. Krasjutdraget er fra pid 4100828, 31. oktober."""
from datetime import datetime, timezone
import json
from pathlib import Path
import tempfile
import unittest

import saro_hold_soak as soak

CRASH = """[2026.10.31-10.20.08:901][  0]LogInit: Using SDL video driver 'wayland'
[2026.10.31-10.37.22:002][ 11]LogVulkanRHI: Creating new VK swapchain with VK_PRESENT_MODE_IMMEDIATE_KHR
[2026.10.31-10.40.58:672][ 66]LogS47HUD: Display: S47: used opsTerminal
[2026.10.31-10.42.01:277][117]LogCore: Error: appError called: Fatal error: [File:./Runtime/RenderCore/Private/RenderingThread.cpp] [Line: 1106]
GameThread timed out waiting for RenderThread after 60.00 secs
"""


def beats(count, step=10, start=1793500000.0, after_from=2):
    lines = []
    for index in range(count):
        at = datetime.fromtimestamp(start + index * step, timezone.utc)
        stamp = at.strftime('%Y.%m.%d-%H.%M.%S') + ':000'
        after = max(0, (index - after_from) * step)
        lines.append(f'[{stamp}][  1]LogS47SaroResumeTest: Display: S47: SARO resume heartbeat {index * step}.0s '
                     f'afterterminal {after}.0s frame{index * 300} stage30 pauses4 resumes4 uses3')
    return '\n'.join(lines) + '\n'


GOOD = "[2026.10.31-10.20.08:901][  0]LogInit: Using SDL video driver 'wayland'\n" + beats(15)
RESULT = dict(status='PASS', reason='ok', afterTerminalWallSeconds=120.5, terminalUses=3, pauseOpens=4,
              nativeFramesAfterTerminal=3600, RHI='Vulkan', lumenHardwareRayTracing=1)
META = dict(run=dict(returncode=0, stop_reason=None), video_driver='wayland', performance_status='UNCONTENDED')
GPU = dict(samples=180, free_min_mib=2000)
KERNEL = soak.scan_kernel('')


class Verdict(unittest.TestCase):
    def decide(self, text=GOOD, result=RESULT, meta=META, gpu=GPU, frames=None, kernel=KERNEL, crashes=()):
        return soak.verdict(meta, soak.scan_log(text), result, gpu, frames, kernel, list(crashes))

    def test_clean_run_passes(self):
        self.assertEqual(self.decide()[0], 'PASS')

    def test_real_crash_excerpt_is_a_render_failure(self):
        status, reasons, _ = self.decide(CRASH, result=None, meta=dict(META, run=dict(returncode=None, stop_reason=None)))
        self.assertEqual(status, 'FAIL')
        self.assertIn('renderstans', reasons[0])

    def test_hang_start_is_sixty_seconds_before_the_fatal(self):
        start = soak.stall_start(soak.scan_log(CRASH))
        self.assertEqual(datetime.fromtimestamp(start, timezone.utc).strftime('%H:%M:%S'), '10:41:01')

    def test_silence_without_fatal_fails(self):
        text = GOOD + "[2026.10.31-10.20.08:901][  0]LogTemp: nothing\n"
        late = beats(3) + beats(3, start=1793500000.0 + 70, after_from=0)
        self.assertEqual(self.decide(late)[0], 'FAIL')
        self.assertEqual(self.decide(text)[0], 'PASS')

    def test_device_lost_and_crash_dir_and_xid_fail(self):
        lost = GOOD + 'LogVulkanRHI: Error: failed, VkResult=-4 with error VK_ERROR_DEVICE_LOST\n'
        self.assertEqual(self.decide(lost)[0], 'FAIL')
        self.assertEqual(self.decide(crashes=[dict(dir='x', error='boom')])[0], 'FAIL')
        line = 'kernel: NVRM: Xid (PCI:0000:01:00): 109, pid=77, name=Signal47, channel 0x1, errorString CTX SWITCH TIMEOUT'
        self.assertEqual(self.decide(kernel=soak.scan_kernel(line, 77))[0], 'FAIL')

    def test_xid_elsewhere_and_gsp_noise_only_warn(self):
        line = ('kernel: NVRM: Xid (PCI:0000:01:00): 109, pid=5, name=UnrealEditor, channel 0x1, errorString x\n'
                'kernel: NVRM: GPU0 rpcRmApiAlloc_GSP: GspRmAlloc failed: status=0x00000019\n')
        status, _, warnings = self.decide(kernel=soak.scan_kernel(line, 77))
        self.assertEqual(status, 'PASS')
        self.assertEqual(len(warnings), 3)  # Xid, GSP, manglende CSV.

    def test_unclear_runs_are_never_pass(self):
        self.assertEqual(self.decide(result=None)[0], 'INVALID')
        self.assertEqual(self.decide(result=dict(RESULT, status='FAIL', reason='fixture'), meta=dict(META, run=dict(returncode=1, stop_reason=None)))[0], 'INVALID')
        self.assertEqual(self.decide(result=dict(RESULT, afterTerminalWallSeconds=60))[0], 'INVALID')
        self.assertEqual(self.decide(gpu=dict(samples=3))[0], 'INVALID')
        self.assertEqual(self.decide(GOOD.replace("'wayland'", "'x11'"))[0], 'INVALID')
        self.assertEqual(self.decide('nothing ran\n', result=None)[0], 'INVALID')
        stopped = dict(META, run=dict(returncode=-15, stop_reason='contention: another Unreal process started; evidence invalid'))
        self.assertEqual(self.decide(meta=stopped)[0], 'INVALID')

    def test_focus_run_needs_new_swapchains_and_no_pause_failure_is_expected(self):
        # -S47SmokeTest slår av US47Flow::Activation: et fokustap åpner ingen pausemeny, og en sluttsjekk som feiler er ikke ventet.
        meta = dict(META, focus_cycles=2, focus_events=[{}, {}], hold_began=1793500010.0, run=dict(returncode=0, stop_reason=None))
        self.assertEqual(self.decide(meta=meta)[0], 'INVALID')
        at = datetime.fromtimestamp(1793500040.0, timezone.utc).strftime('%Y.%m.%d-%H.%M.%S') + ':000'
        fresh = GOOD + f'[{at}][  1]LogVulkanRHI: Creating new VK swapchain with X\n'
        self.assertEqual(self.decide(fresh, meta=meta)[0], 'PASS')
        failed = dict(RESULT, status='FAIL', reason='same ordinary case remains interactive after the bounded observation')
        self.assertEqual(self.decide(fresh, result=failed, meta=dict(meta, run=dict(returncode=1, stop_reason=None)))[0], 'INVALID')

    def test_a_failed_run_does_not_trigger_the_queue_retry(self):
        # q.py starter på nytt hvis et av disse ordene står i utskriften fra en kjøring som feilet.
        with tempfile.TemporaryDirectory() as folder:
            session = Path(folder)
            (session / 'profile/Saved/Logs').mkdir(parents=True)
            (session / 'profile/Saved/Logs/Signal47.log').write_text(CRASH)
            (session / 'session.json').write_text(json.dumps(dict(META, run=dict(returncode=None, stop_reason=None))))
            (session / 'summary.txt').write_text('')
            code = soak.report(session)
            text = (session / 'summary.txt').read_text()
        self.assertEqual(code, 1)
        for word in ('BlockingIOError', 'ReservationBusy', 'Heavy Unreal job already running', 'production lease', 'contention'):
            self.assertNotIn(word, text)


class Measures(unittest.TestCase):
    def test_gpu_rows_and_stall_kind(self):
        rows = soak.parse_gpu('\n'.join(f'2026/11/07 03:53:{second:02d}.632, 7666, 10240, 2, 4, P0, 1935, 9501, 141.40, 56' for second in range(20)))
        self.assertEqual(len(rows), 20)
        self.assertEqual(soak.summarize_gpu(rows)['free_min_mib'], 10240 - 7666)
        self.assertIn('GPU_VENTER', soak.classify_stall(rows, rows[0]['t'])['kind'])
        busy = [dict(row, gpu=99) for row in rows]
        self.assertIn('GPU_REGNER', soak.classify_stall(busy, rows[0]['t'])['kind'])
        self.assertEqual(soak.classify_stall(rows, rows[-1]['t'])['kind'], 'UKJENT')

    def test_pmon_peak_for_our_pid(self):
        text = ('#Time gpu pid type sm mem enc dec jpg ofa fb ccpm command\n'
                ' 03:53:15 0 1941 G - - - - - - 90 0 kwin_w\n'
                ' 03:53:15 0 77 G 90 40 - - - - 6100 0 Signal47\n'
                ' 03:53:16 0 77 G 90 40 - - - - 6400 0 Signal47\n')
        self.assertEqual(soak.parse_pmon(text, 77), 6400)

    def test_frame_csv(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'p.csv'
            path.write_text('EVENTS,FrameTime,GPUTime\n,10,8\n,20,18\n,500,400\n[HasHeaderRowAtEnd],1\n')
            frames = soak.parse_frames(path)
        self.assertEqual((frames['frames'], frames['max_ms'], frames['over_100ms']), (3, 500, 1))

    def test_refuses_toms_installed_game_and_non_tmp(self):
        class Args:
            build = soak.INSTALLED / 'Linux'
            out = None
            allow_installed = False
            windowed = None
            video_driver = 'wayland'
            fixture = soak.FIXTURE
            focus_cycles = 0
            timeout = 1
        if (Args.build / 'Signal47/Binaries/Linux/Signal47').is_file():
            with self.assertRaisesRegex(ValueError, 'installerte'):
                soak.plan(Args)

    def test_command_reaches_continue(self):
        with tempfile.TemporaryDirectory() as folder:
            class Args:
                build = Path(folder)
                out = None
                allow_installed = False
                windowed = None
                video_driver = 'wayland'
                fixture = soak.FIXTURE
                focus_cycles = 0
                timeout = 480
            binary = Args.build / 'Signal47/Binaries/Linux/Signal47'
            binary.parent.mkdir(parents=True)
            binary.touch()
            command = soak.plan(Args)['command']
        # AS47HUD::StartTests leser -S47Open= bare under -S47SmokeTest; uten det står spillet på tittelen til testen gir opp.
        for flag in ('-S47Title', '-S47SmokeTest', '-S47Open=continue', '-S47SaroResumeTest'):
            self.assertIn(flag, command)
        seconds = next(int(item.split('=')[1]) for item in command if item.startswith('-S47Seconds='))
        self.assertGreater(seconds, Args.timeout)

    def test_fixture_shape(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'save.json'
            state = dict(checkpoint='nightService', area='saro', pose=dict(z=682.0), spine=dict(sc04d=dict(schedule=False)))
            path.write_text(json.dumps(dict(caseId=1, state=state)))
            self.assertEqual(len(soak.validate_fixture(path)), 64)
            state['spine']['sc04d']['schedule'] = True
            path.write_text(json.dumps(dict(caseId=1, state=state)))
            with self.assertRaises(ValueError):
                soak.validate_fixture(path)


if __name__ == '__main__':
    unittest.main()
