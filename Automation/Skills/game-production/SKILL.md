---
name: game-production
description: Develop games and interactive visual experiences from a brief or existing prototype into coherent, testable production slices. Use for game production planning, design bibles, art pipelines, asset review, or coordinated gameplay and visual polish. Adapt to the existing engine and platform; do not apply the full workflow to unrelated small code fixes.
---

# Game Production

Make the project feel deliberately authored and carry the requested outcome from intent through tested increments to delivery. This skill adds production decisions learned from SIGNAL / 47 and the PaperRoute development account. It is portable guidance, not a tool connection, model choice, or promise about development time.

Respond in the user's language. Use their existing scope and authorization. Do not require a new approval merely because this skill was invoked. Confirm only decisions or consequential actions that still lack necessary authorization. Do not import authorization from another project.

## Choose the useful depth

- **Assessment:** evaluate a reference, workflow or feasibility question. Explain what transfers and what does not; do not silently implement the proposed game feature.
- **Design:** produce the requested brief, story, maps, visual direction or roadmap. Make it actionable, while distinguishing proposals from implemented content.
- **Production:** continue an existing project or build the authorized slice. Work through a playable result and its delivery, not just a plan.

If brainstorming or gauntlet-loop is available and applicable, coordinate with it: brainstorming resolves intent; this skill guides content and art production; Gauntlet supplies the execution and evidence loop. Reuse their decisions and records. Do not create a second design bible, duplicate test dossier or recurring approval ceremony. If they are unavailable, the workflow below remains usable.

## Recover project truth

Read applicable instructions, the current handoff, canonical source, relevant code and evidence. Check actual Git/PR status before assuming a referenced branch was merged. Identify read-only references, editable sources, generated files, current playable build and default launcher separately. Old PDFs and mirrored project files may be historical snapshots.

Preserve the project's working stack, controls, authored assets, provenance and domain constants. Do not move an established Unity game to a browser because an inspiration project uses Three.js. Match new tools to a missing production capability, not to a list of accounts the user owns.

State the intended player action, response and outcome. Select a few observable acceptance criteria and a bounded next step. Reuse approved direction; do not restart art exploration on every turn. Label new assumptions as provisional rather than user-approved.

## Plan content before multiplying it

For story scope, design bibles or roadmaps, read [design-and-scope.md](references/design-and-scope.md). Set the requested content ceiling and dependencies. Estimate playtime separately from measured playtime. Turn important revelations into player actions and observable evidence. Freeze decisions sufficiently for the next production step, while keeping unresolved larger decisions explicit.

Make the central interaction playable before investing in a large art library. In an established game, retain the working interaction as the baseline. A representative room or short sequence should demonstrate interaction, visual direction and progression before expanding into many environments.

## Develop behavior and appearance as distinct work

Maintain separate acceptance criteria for gameplay and art, even within one conversation. A visual change should not casually rewrite physics, persistence or puzzle logic. A behavior change should not silently replace the approved visual language. Use focused branches/worktrees for uncertain experiments when useful; one source and integration owner remain clear. Delegate only when independently authorized and available.

For substantial visual/3D work, read [art-and-assets.md](references/art-and-assets.md). Prove one asset's complete route into the real product before making a family. Establish a small visual test scene using actual materials, lights and camera distance. Keep it cheap enough to rerender after meaningful changes.

For Gaussian splats, PlayCanvas tooling or generated character assembly, read [splats-and-generated-characters.md](references/splats-and-generated-characters.md). Distinguish splat rendering, proxy collision and ordinary rigged meshes; prove the required route in the existing target engine.

For each loop:

1. Retain the relevant working baseline and define the intended improvement.
2. Implement the smallest coherent behavior or asset change.
3. Run the actual product and inspect the changed experience.
4. Record the observed defect, evidence and player impact; correct the most important defect.
5. Rerun affected checks and preserve the matching source, build and evidence.

After repeated unsuccessful fixes to the same defect, revisit the hypothesis or production method. Do not spend indefinite generations on a face or mesh that a different method would solve. Stop optional polish when the agreed slice passes, then continue remaining required work within the user's scope. A slice is an execution increment, not a replacement for the complete requested outcome.

## Verify what will be delivered

Read [verification-and-delivery.md](references/verification-and-delivery.md) for persistence changes, a release candidate or full production verification. Tests should demonstrate meaningful outcomes and failure recovery. Green checks cannot overrule unreadable menus or visibly broken geometry.

Keep these evidence types separate: concept image, authoring render, runtime capture, API-driven scenario, real input journey and target-device measurement. An API-restored scene can prove restoration without proving that a user can navigate the menu. A visually convincing mockup does not prove its controls work.

If the desktop, device or service is unavailable, continue build, source review, isolated API checks or asset work that remains possible. State which gate is unverified. Do not repeatedly request an unavailable action or simulate its success. Preserve a known working default when a new candidate has unmet promotion gates.

## Checkpoint and handoff

Track every requested deliverable and later correction. A checkpoint preserves progress; it is not a reason to end the turn while useful authorized work remains. If a gate cannot run, block only its dependent promotion or claim, complete independent requirements, and identify the exact missing evidence. Before the final response, reconcile the result against the whole request, including authorized delivery.

Update the existing project record with the decision, source/build identity, evidence, PASS/FAIL/UNVERIFIED gates, opening instructions and next bounded task. Mark N/A only with a reason. Do not call a design bible implemented or a slice the complete game.

Distinguish local source, pushed branch, merged PR, local package, published release and promoted default. Check actual repository visibility before external delivery; existing authorization must cover the destination. Merge only when authorized and verify its result. A merged candidate may still have open runtime gates.

Measure time, iteration cost and token usage only when reliable records exist or the user requests tracking. Separate elapsed time, active work, cached/uncached usage and actual monetary cost; do not infer a bill from a token total. Availability and pricing of models or asset services require current checks when material to a decision.

Finish with the concrete result, how to open/use it, material verification limits and the next useful step. Never imply continued background work without an actual running job.

For the origin of these rules and their limits, see [lessons-and-sources.md](references/lessons-and-sources.md). Read it when evaluating whether a historical lesson applies to a new project.
