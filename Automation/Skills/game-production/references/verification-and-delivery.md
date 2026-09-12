# Verification, persistence and delivery

Read the relevant sections for the changed slice. Do not turn a minor prop adjustment into a full storage audit. Reuse valid evidence unless the change invalidates it.

## Evidence is specific to a claim

| Evidence | What it supports | What it does not establish |
| --- | --- | --- |
| Concept or UI sketch | Intended visual/design direction | Implemented behavior |
| Authoring render | Mesh/material/pose under recorded conditions | Engine import or in-game appearance |
| Runtime capture | Visible state in a specific build | How a real user reached that state |
| API-driven runtime scenario | State contracts, controlled errors, scene restoration | Mouse/keyboard/touch usability |
| Real input journey | Actual navigation and central actions exercised | Performance on untested hardware |
| Release measurement | Recorded conditions and build | Every device, preset or future revision |

Keep isolated tests separate from user saves. Opt-in fault injection needs an explicitly disposable profile and must not run in normal play. When copying real fixtures, preserve originals, record any relocation of paths or intentional payload changes, recompute required checksums and compare source hashes afterward. If photos/audio are part of the saved state, validate actual media load/decoding, not only file existence or JSON fields.

## Persistence changes

Define durable state, immutable originals, temporary writes, backup policy and the point an action becomes committed. Check the affected cases: no save, valid save, incompatible/corrupt primary, valid backup, unavailable media, write failure, in-flight save, canceled action, restart and scene reload.

For destructive replacement or a new playthrough, preserve the required old files before clearing active references. Cancellation should not mutate durable state. An error must not silently advance the world or open an empty case. Keep failure feedback actionable and accurate about where preserved files are available.

If a user reviews a selected saved item before confirming, restore the exact validated selection. Recheck identity/content when it could have changed; avoid separate reads that pair one payload with another preview. Validate paths and identifiers appropriate to the import boundary.

Backups must belong to the active playthrough. Avoid silently falling back to an unrelated case after switching. Describe atomicity precisely: an atomic primary-file replacement does not prove an atomic transaction across backup, media and archive directories. Exercise failures at meaningful boundaries and verify the preserved bytes, not just an error return.

## Real-world input and performance

Exercise the central action, its response, progression and restart/continue through actual controls when possible. For menus cover focus, enabled states, confirm/cancel, Escape/back and the requested input devices. Test the intended display size and additional sizes when the change affects layout. A synthetic state setup can supplement this, not replace it.

If a screen is locked or required hardware is absent, do not bypass it or count captures as native input evidence. Continue permissible independent checks and keep the affected gates UNVERIFIED. Distinguish this situation from a failing product.

Measure the relevant release build on the stated hardware, renderer, resolution and preset. Record warmup, capture duration, frame intervals and tail latency/stutters where relevant; an average FPS alone can hide stalls. Separate instrumented functional runs from performance runs. Do not reuse old performance figures after an affected visual/system change.

## Match source, build and package

Record a source revision plus relevant source-content identity when building with uncommitted changes. Hash the actual payload; a native launcher may be identical across different games/builds. Preserve a manifest covering the files and licenses that must ship. Keep known working packages immutable where the project already uses that convention.

Validate the archive's paths, file types, contents and executable permissions. Extraction tools may normalize permissions: compare stored archive modes with the manifest, actual file bytes/paths with the package and required runtime access separately. Explain expected normalization instead of treating unequal modes as either corrupt content or an unconditional pass.

Launch from the freshly extracted package at a different path, with the intended launcher and an isolated profile. A previous source-build run is not delivery proof. Make dependent shell steps fail closed: if extraction or launcher resolution fails, stop that attempt rather than falling back to the default development executable. Verify the actual launched artifact before recording PASS.

## Release and handoff

Keep functional, visual, input, performance and delivery gates separate. FAIL needs the observed defect; UNVERIFIED needs the missing evidence; N/A needs a reason. Passing API checks or merging a PR does not promote an unverified candidate automatically.

Before pushing/publishing, confirm actual destination visibility and the authorization already present. Before an authorized merge, verify the intended head and relevant checks; after it, verify the resulting state. Do not invent a required approval, bypass a required check or claim CI ran when only local checks ran.

A compact record can use:

- Scope and accepted criteria; source/build/package identity.
- Environment and exact procedure or reproducible script.
- Evidence paths and per-gate results with their limits.
- Candidate/default distinction, opening instructions and next dependency.

Update the project's existing handoff. Preserve historical design documents and make the current record identify what supersedes them. Do not assume ignored local artifacts will exist in a fresh clone.
