# Focused regression for the motel television's actual frame, observation and persistence.
# Run sequentially with the other browser tests: python3 tools/moteltv.py OUTDIR [WxH]
import asyncio, base64, json, os, sys
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = map(int, (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []

def check(ok, text):
    checks.append((bool(ok), text))
    print(('PASS ' if ok else 'FAIL ') + text, flush=True)

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        browser = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await browser.new_context(viewport={'width': W, 'height': H})).new_page()
        pg.set_default_timeout(240000)
        errors = []
        pg.on('pageerror', lambda e: errors.append(str(e)))
        await pg.add_init_script('HTMLElement.prototype.requestPointerLock = function() { return Promise.resolve(); };')
        if os.environ.get('S47_NO_IDB'):
            await pg.add_init_script("Object.defineProperty(window, 'indexedDB', { value: undefined });")
        await pg.goto(URL)
        await pg.evaluate('localStorage.clear()')
        await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]')
        ev = pg.evaluate
        if os.environ.get('S47_NO_IDB'):
            check(await ev('S47.saves.ready.then(()=>S47.saves.fallback)'), 'IndexedDB is unavailable and the existing localStorage backend is active')
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter3'); S47.tick(1); S47.setAbductions(false); S47.setPicture('off'); S47.world.placeTruck('motel'); S47.world.enter('saro'); S47.tick(0.1)")
        await ev("""window.tvState = { ok: true, saro: true, here: true, truckOnLot: true, cam: S47.camera, clock: 4 * 3600 };
          window.tvStep = (seconds, extra = {}) => { for (let t = 0; t < seconds - 1e-8; t += .05) S47.motelTv.update(Math.min(.05, seconds - t), {...tvState, ...extra}); };
          window.aimTv = () => { const p = S47.player, tv = S47.world.motel.tv.group.localToWorld(new S47.three.Vector3(-.045,.18,.1815));
            p.yaw = Math.atan2(-(tv.x-p.pos.x), -(tv.z-p.pos.z)); p.pitch = Math.atan2(tv.y-p.floorY-p.eye, Math.hypot(tv.x-p.pos.x,tv.z-p.pos.z)); p.update(0,0,0,false); S47.camera.updateMatrixWorld(true); };
          window.clearTv = () => { S47.motelTv.reset(); S47.townSeen.delete('ev:tv'); for (const k of [...S47.townSeen]) if (k.startsWith('mark:tv@')) S47.townSeen.delete(k);
            S47.game.docs = S47.game.docs.filter(d => d.id !== 'motel-tv-frame'); };
          clearTv();""")
        walk = await ev("""(() => { const m = S47.world.motel, p = S47.player;
          p.place(m.at.x-39.6,m.at.z+24.1,Math.PI/2);
          const a = m.anchors.officeInside; let n = 0;
          while (Math.hypot(p.pos.x-a.x,p.pos.z-a.z)>.15) { p.yaw=Math.atan2(-(a.x-p.pos.x),-(a.z-p.pos.z)); p.update(1/30,0,-1,false); if (++n>300) return false; }
          return true; })()""")
        await ev('S47.tick(.1); aimTv()')
        check(walk, 'the ordinary player movement reaches the TV viewing place through the office door')
        check(await ev("S47.game.d.inter.update(S47.camera)?.id === 'court:officeTv'"), 'the screen can be targeted by the ordinary interaction ray')
        await ev('const m=S47.world.motel; S47.player.place(m.at.x-44,m.at.z+21.5,0); aimTv()')
        check(not await ev('S47.motelTv.inView(S47.camera)'), 'a nearby camera aimed at the TV through the north wall cannot witness it')
        await ev('const a=S47.world.motel.anchors.officeInside; S47.player.place(a.x,a.z,a.yaw); S47.tick(.1); aimTv()')

        # A panel cannot consume the only frame; leaving the area cannot award it.
        await ev('S47.motelTv.now(); tvStep(20); tvStep(60,{ok:false})')
        check(await ev("S47.motelTv.mode === 'carrier' && Math.abs(S47.motelTv.t-20)<.01 && !S47.townSeen.has('ev:tv')"), 'carrier freezes during a panel or cinematic')
        await ev('tvStep(.1,{saro:false,here:false})')
        check(await ev("S47.motelTv.mode === 'card' && !S47.townSeen.has('ev:tv')"), 'leaving the area cancels without spending the event')

        # Looking away through the frame gives no false observation. A subsequent attempt works.
        await ev('clearTv(); S47.player.yaw += Math.PI; S47.player.update(0,0,0,false); S47.motelTv.now(); tvStep(47.1)')
        check(await ev("S47.motelTv.mode === 'frame' && !S47.townSeen.has('ev:tv') && !S47.game.docs.some(d=>d.id==='motel-tv-frame')"), 'standing with your back to the set does not record a finding')
        await ev('tvStep(3)')
        check(await ev("S47.motelTv.mode === 'card' && !S47.townSeen.has('ev:tv')"), 'a wholly missed frame remains available later')
        await ev('clearTv(); S47.motelTv.now(); tvStep(47.1); aimTv(); tvStep(.1)')
        check(await ev("S47.townSeen.has('ev:tv') && S47.game.docs.some(d=>d.id==='motel-tv-frame' && d.image.startsWith('data:image/jpeg'))"), 'turning toward the still visible frame records the actual image')
        await ev('tvStep(60,{ok:false})')
        check(await ev("S47.motelTv.mode === 'frame' && S47.motelTv.t < 1"), 'the held frame also freezes while a panel or cinematic is open')
        await ev('tvStep(3); S47.game.d.inter.get("court:officeTv").use()')
        check(await pg.locator('.docview').count() > 0, 'using the TV reopens its recorded frame')
        await pg.screenshot(path=f'{OUT}/tv_document.png')
        await ev("document.querySelector('.docview [data-a=back]')?.click()")

        # A truck parked by the southern rooms must be inside the actual aerial view, still at
        # its actual heading. Hiding only that mesh must alter pixels around its projected box.
        await ev("""clearTv(); const m=S47.world.motel,g=S47.world.saroTruck.group;
          g.position.set(m.at.x-33,m.floorY,m.at.z+60); g.rotation.y=.63; g.updateMatrixWorld(true);
          S47.motelTv.now(); tvStep(47.1);""")
        check(await ev("S47.townSeen.has('ev:tv') && !!S47.motelTv.frame"), 'a truck parked near the southern rooms produces a witnessed runtime frame')
        diff = await ev("""(() => { const T=S47.motelTv,g=S47.world.saroTruck.group,r=S47.renderer;
          const pos=g.position.clone(),rot=g.rotation.clone(),target=r.getRenderTarget(),vp=r.getViewport(new S47.three.Vector4()).toArray(),sc=r.getScissorTest();
          const a=T.shoot().getContext('2d').getImageData(0,0,320,240).data.slice(),box=new S47.three.Box3().setFromObject(g),cam=T.cam;
          const lo=[Infinity,Infinity],hi=[-Infinity,-Infinity],ch=Math.min(r.domElement.height,r.domElement.width*3/4),cw=ch*4/3;
          for(const x of [box.min.x,box.max.x]) for(const y of [box.min.y,box.max.y]) for(const z of [box.min.z,box.max.z]) { const q=new S47.three.Vector3(x,y,z).project(cam),px=160+q.x*r.domElement.width/cw*160,py=120-q.y*r.domElement.height/ch*120;lo[0]=Math.min(lo[0],px);lo[1]=Math.min(lo[1],py);hi[0]=Math.max(hi[0],px);hi[1]=Math.max(hi[1],py); }
          g.visible=false;const b=T.shoot().getContext('2d').getImageData(0,0,320,240).data;g.visible=true;
          let inside=0,outside=0,ni=0,no=0;
          for(let y=0;y<240;y++) for(let x=0;x<320;x++) {const i=(y*320+x)*4,d=Math.abs(a[i]-b[i])+Math.abs(a[i+1]-b[i+1])+Math.abs(a[i+2]-b[i+2]);if(x>lo[0]-3&&x<hi[0]+3&&y>lo[1]-3&&y<hi[1]+3){inside+=d;ni++;}else{outside+=d;no++;}}
          return {inside:inside/Math.max(1,ni),outside:outside/no,lo,hi,pose:g.position.equals(pos)&&g.rotation.equals(rot),render:r.getRenderTarget()===target&&JSON.stringify(vp)===JSON.stringify(r.getViewport(new S47.three.Vector4()).toArray())&&sc===r.getScissorTest()}; })()""")
        check(diff['lo'][0] >= 0 and diff['lo'][1] >= 0 and diff['hi'][0] < 320 and diff['hi'][1] < 240, f'the whole transformed truck is inside the aerial frame ({diff["lo"]}, {diff["hi"]})')
        check(diff['inside'] > 4 and diff['inside'] > 5 * diff['outside'], f'the actual truck changes only its own part of the image ({diff["inside"]:.1f} vs {diff["outside"]:.2f})')
        check(diff['pose'] and diff['render'], 'capture preserves the truck pose and renderer target, viewport and scissor')
        frame = await ev('S47.motelTv.shoot().toDataURL("image/png")')
        with open(f'{OUT}/tv_actual_south_lot.png','wb') as f: f.write(base64.b64decode(frame.split(',',1)[1]))

        # Capture failure must not grant a fabricated frame or spend the event.
        await ev('tvStep(3); window.keptTvDoc=S47.game.docs.find(d=>d.id==="motel-tv-frame"); clearTv(); const shot=S47.motelTv.shoot; S47.motelTv.shoot=()=>null; S47.motelTv.now(); tvStep(47.1); S47.motelTv.shoot=shot; 0;')
        check(await ev("!S47.townSeen.has('ev:tv') && !S47.game.docs.some(d=>d.id==='motel-tv-frame')"), 'a failed render never substitutes artwork or claims the truck was seen')

        # Save the real observation, reload and verify the exact image is refiled without replay.
        await ev("S47.game.docs.push(keptTvDoc); S47.townSeen.add('ev:tv'); S47.townSeen.add('mark:tv@14400'); S47.motelTv.reset(); S47.saveNow('manual',0)")
        original = await ev('keptTvDoc.image')
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]')
        await pg.wait_for_function('S47.started()',polling=200)
        await ev('S47.hold=true; S47.tick(.2)')
        restored = await ev("S47.game.docs.find(d=>d.id==='motel-tv-frame')?.image")
        check(restored == original, 'Continue restores the exact saved television image')
        check(await ev("S47.townSeen.has('ev:tv') && S47.motelTv.mode==='card'"), 'Continue keeps the observation spent and the set on its test card')
        await ev("S47.game.d.inter.get('court:officeTv').use()")
        check(await pg.locator('.docview').count() > 0, 'the saved image can be opened from the TV after Continue')
        await ev("document.querySelector('.docview [data-a=back]')?.click(); S47.game.docs=S47.game.docs.filter(d=>d.id!=='motel-tv-frame'); S47.game.d.inter.get('court:officeTv').use()")
        check('unavailable' in await pg.locator('.toasts').inner_text(), 'legacy observations with no image say that the earlier picture is unavailable')
        check(not errors, 'no browser runtime errors: ' + '; '.join(errors[:3]))
        with open(f'{OUT}/moteltv.json','w') as f: json.dump({'checks':checks,'truck':diff},f,indent=2)
        await browser.close()
        print(f'{sum(ok for ok,_ in checks)}/{len(checks)} PASS',flush=True)
        sys.exit(0 if all(ok for ok,_ in checks) else 1)

asyncio.run(main())
