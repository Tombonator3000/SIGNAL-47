"""Explicit New night navigation for tests whose subject starts in the world.

This is not an optional bypass: a New night must show the real opening card and
its visible Begin night button. The dedicated intro.py exercises narration and
cancellation without this helper. Choose/confirm a full case before calling it.
"""


async def begin_opening(page, *, touch=False):
    button = page.locator('.opening-card button[data-a=begin-night]')
    await button.wait_for(state='visible')
    await button.scroll_into_view_if_needed()
    if touch:
        await button.tap()
    else:
        await button.click()
