"""API-driven UI check for portraits in both build forms. No real input/FPS claim.

Run from web after both builds. S47_PORTRAIT_URL selects a Pages-style URL.
Uses existing development hooks to construct the People fixture and open Nora's
actual dialogue panel. It does not alter any game file.
"""
import asyncio
import json
import os
from pathlib import Path
from playwright.async_api import async_playwright

HERE=Path(__file__).resolve().parent
WEB=HERE.parents[4]
OUT=HERE.parent/'round13_15_qa/runtime'


async def main():
    OUT.mkdir(parents=True,exist_ok=True)
    urls=[('pages',os.environ.get('S47_PORTRAIT_URL','http://127.0.0.1:8873/SIGNAL-47/')),
          ('single',(WEB/'dist-single/index.html').as_uri())]
    checks=[];contexts=[]
    async with async_playwright() as pw:
        browser=await pw.chromium.launch(args=['--use-angle=swiftshader','--enable-unsafe-swiftshader','--ignore-gpu-blocklist','--autoplay-policy=no-user-gesture-required'])
        for build,url in urls:
            ctx=await browser.new_context(viewport={'width':1280,'height':800})
            page=await ctx.new_page();page.set_default_timeout(180000)
            errors=[]
            page.on('pageerror',lambda e:errors.append(str(e)))
            page.on('console',lambda m:errors.append(m.text) if m.type=='error' else None)
            await page.add_init_script('HTMLElement.prototype.requestPointerLock=function(){return Promise.resolve()};')
            await page.goto(url)
            await page.wait_for_selector('button[data-a=start]')
            await page.click('button[data-a=start]')
            await page.evaluate("S47.hold=true; S47.jump('chapter4'); S47.game.d.ui.close(); S47.tick(0.1)")
            count=await page.evaluate('S47.portraitCount()')
            checks.append([build+' includes all 11 portraits',count==11])
            # A fixture of everyone met, using the same state and card builders as play.
            await page.evaluate("""(()=>{
                S47.ch4.s.topics.tomas=true; S47.ch5.s.arrived=true;
                for(const id of ['attendant','deputy','host','radio_woman','souvenir_man']) S47.townSeen.add('met:'+id);
                const ui=S47.game.d.ui;
                ui.notebook([],[],[],()=>{},'Tonight',undefined,{people:S47.peopleItems()});
                document.querySelector('.notebook [data-t=people]').click();
            })()""")
            await page.wait_for_function("document.querySelectorAll('.nb-people img').length===11 && [...document.querySelectorAll('.nb-people img')].every(i=>i.complete && i.naturalWidth===512 && i.naturalHeight===640)")
            cards=await page.evaluate("[...document.querySelectorAll('.nb-people li')].map(e=>({name:e.querySelector('b')?.textContent,face:!!e.querySelector('img'),empty:!!e.querySelector('.noface')}))")
            checks.append([build+' all 11 actual People cards decode, no empty face',len(cards)==11 and all(x['face'] and not x['empty'] for x in cards)])
            await page.screenshot(path=str(OUT/(build+'-people-1280.png')))
            await page.set_viewport_size({'width':844,'height':390})
            await page.screenshot(path=str(OUT/(build+'-people-844.png')))
            await page.evaluate("S47.game.d.ui.close(); S47.ch4.openTalk(); S47.tick(0.1)")
            await page.wait_for_function("(()=>{const i=document.querySelector('.talk-portrait');return i && i.complete && i.naturalWidth===512 && i.naturalHeight===640 && i.alt==='N. VEGA'})()")
            checks.append([build+' Nora dialogue resolves N. VEGA to the portrait',True])
            await page.screenshot(path=str(OUT/(build+'-nora-844.png')))
            await page.set_viewport_size({'width':1280,'height':800})
            await page.screenshot(path=str(OUT/(build+'-nora-1280.png')))
            checks.append([build+' no browser errors',not errors])
            contexts.append({'build':build,'url':url,'cards':cards,'errors':errors})
            await ctx.close()
        await browser.close()
    status='PASS' if all(ok for _,ok in checks) else 'FAIL'
    (OUT/'portraits.json').write_text(json.dumps({'status':status,'checks':checks,'contexts':contexts,'method':'Existing game hooks and actual DOM image decoding; Chromium SwiftShader; no real input or hardware FPS'},ensure_ascii=False,indent=2)+'\n')
    for label,ok in checks:print(('PASS ' if ok else 'FAIL ')+label,flush=True)
    if status=='FAIL':raise SystemExit(1)


if __name__=='__main__':asyncio.run(main())
