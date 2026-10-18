"""Sjekk det statiske konseptgalleriet mot en lokalt servert web-mappe."""
import argparse
import asyncio
from datetime import datetime, timezone
import json
from pathlib import Path
from urllib.parse import urljoin

from playwright.async_api import async_playwright

PROD = Path(__file__).resolve().parent


async def main(base):
    url = urljoin(base.rstrip('/') + '/', 'src/assets/art/concept/night_20261018/index.html')
    (PROD / 'qa').mkdir(exist_ok=True)
    checks = []
    errors = []
    failed_requests = []
    views = []
    async with async_playwright() as pw:
        browser = await pw.chromium.launch()
        for label, width, height in [('desktop', 1360, 900), ('mobile', 390, 844)]:
            page = await browser.new_page(viewport={'width': width, 'height': height}, device_scale_factor=1)
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.on('requestfailed', lambda request: failed_requests.append(request.url))
            page.on('console', lambda msg: errors.append(msg.text) if msg.type == 'error' else None)
            response = await page.goto(url, wait_until='networkidle')
            await page.locator('img').evaluate_all('imgs => Promise.all(imgs.map(img => img.decode()))')
            images = await page.locator('img').evaluate_all('imgs => imgs.map(i => ({file:i.getAttribute("src"),width:i.naturalWidth,height:i.naturalHeight,complete:i.complete}))')
            checks.append({'check': label + ': åtte dekodede 1920x1080-bilder', 'pass': len(images) == 8 and all(i['complete'] and i['width'] == 1920 and i['height'] == 1080 for i in images)})
            checks.append({'check': label + ': HTTP 200', 'pass': response.status == 200})
            overflow = await page.evaluate('document.documentElement.scrollWidth > innerWidth')
            checks.append({'check': label + ': ingen vannrett overflow', 'pass': not overflow})
            await page.screenshot(path=str(PROD / 'qa' / ('gallery-' + label + '.png')), full_page=True)
            await page.locator('details').evaluate_all('nodes => nodes.forEach(n => n.open = true)')
            overflow_open = await page.evaluate('document.documentElement.scrollWidth > innerWidth')
            checks.append({'check': label + ': kildepanelene kan åpnes uten overflow', 'pass': not overflow_open})
            checks.append({'check': label + ': alle åtte figurer har beskrivende alternativtekst', 'pass': await page.locator('img[alt]').count() == 8})
            hrefs = await page.locator('a').evaluate_all('nodes => [...new Set(nodes.map(n => n.href))]')
            link_status = []
            for href in hrefs:
                linked = await page.request.get(href)
                link_status.append({'url': href, 'status': linked.status})
            checks.append({'check': label + ': alle lenkene svarer HTTP 200', 'pass': all(x['status'] == 200 for x in link_status)})
            views.append({'label': label, 'viewport': [width, height], 'images': images, 'link_status': link_status})
            await page.close()
        await browser.close()
    checks.append({'check': 'Ingen nettleserfeil eller mislykkede forespørsler', 'pass': not errors and not failed_requests})
    result = {'result': 'PASS' if all(c['pass'] for c in checks) else 'FAIL',
              'utc': datetime.now(timezone.utc).isoformat(), 'url': url, 'checks': checks,
              'errors': errors, 'failed_requests': failed_requests, 'viewports': views}
    (PROD / 'gallery-verification.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({'result': result['result'], 'checks': len(checks), 'errors': errors, 'failed_requests': failed_requests}, ensure_ascii=False))
    if result['result'] != 'PASS':
        raise SystemExit(1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('web_url', help='URL til den serverte web-mappa')
    args = parser.parse_args()
    asyncio.run(main(args.web_url))
