# The browser tests on the graphics card instead of SwiftShader (Tom, 18 October: "ja bruk
# skjermkort"). Python imports this module at start-up when tools/gpu is on PYTHONPATH; with
# S47_GPU=1 it rewrites every Chromium launch's SwiftShader flags to the real GPU through ANGLE
# on OpenGL. The tests themselves are unchanged and still run on SwiftShader without it:
#   S47_GPU=1 PYTHONPATH=tools/gpu ~/.venvs/signal47/bin/python tools/walkthrough.py OUT 1280x800
# S47_GPU=vulkan uses ANGLE on Vulkan instead.
import os

if os.environ.get('S47_GPU'):
    try:
        from playwright.async_api._generated import BrowserType as _Async
    except Exception:   # an older Playwright: leave the tests as they are
        _Async = None
    if _Async is not None:
        _angle = 'vulkan' if os.environ.get('S47_GPU') == 'vulkan' else 'gl'
        _gpu = [f'--use-angle={_angle}', '--ignore-gpu-blocklist', '--enable-gpu'] + (['--enable-features=Vulkan'] if _angle == 'vulkan' else [])
        _launch = _Async.launch

        async def _gpu_launch(self, *a, **kw):
            args = [x for x in (kw.get('args') or []) if 'swiftshader' not in x and not x.startswith('--use-angle') and x != '--ignore-gpu-blocklist']
            kw['args'] = _gpu + args
            return await _launch(self, *a, **kw)

        _Async.launch = _gpu_launch
