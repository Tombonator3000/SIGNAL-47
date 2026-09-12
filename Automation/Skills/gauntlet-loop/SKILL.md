---
name: gauntlet-loop
description: Build or advance a project through the Universal Gauntlet workflow, comparing an actual working slice with its intended experience and verifying user journey, visuals, performance and delivery. Use when the user invokes Gauntlet or explicitly requests this complete production and verification loop.
---

# Universal Gauntlet

Turn the user's concept or existing project into a working, inspectable result through small verified increments. Adapt the user's Universal Gauntlet master prompt; respond in Norwegian unless they select another language. This skill is an operating workflow, not a tool connection or a model selector.

## Establish the current slice

Resolve AUTO choices from the conversation, project decisions and files. For production requests, start with a representative vertical slice and continue through the remaining requested outcome; the first slice is a checkpoint, not a scope reduction. Respect assessment-only or concept-only requests. Preserve the existing stack and working source unless evidence justifies a change. Read applicable AGENTS.md, project references, relevant skills, dependency versions, current source, recent changes and validation evidence. Identify canonical editable source versus generated output.

Describe the core experience concretely: what the user does, what responds and why it matters. State 3–5 observable acceptance criteria. Separate required outcomes from optional ideas. Make distinct decisions for representation (document/data, ordinary UI, 2D, 2.5D, 3D, or combination) and delivery (browser, desktop, mobile, engine build, export). A browser can deliver real 3D; a 3D-looking product may use prerendered 2D.

Record controls, screen sizes, accessibility, target hardware, offline/network needs, persistence and constraints relevant to the slice. For a new project compare at most 2–3 credible stacks. Ask only questions that materially change concept, platform, cost or architecture; complete useful unblocked work first. Do not expand scope or invent a spending allowance.

## Verify the production path

Use a compact stack table: responsibility, selected tool/version, input, output, integration, availability and reason. Every tool needs a concrete handoff. Verify current compatibility through installed versions and official/maintainer documentation. Prefer one canonical source, naming convention and reproducible build path. Prove a representative asset or input reaches the actual product.

Classify required capabilities as VERIFIED WORKING, AVAILABLE BUT UNTESTED, SETUP REQUIRED or UNAVAILABLE. Verify editing, building, launching, inspection, screenshots and measurements separately. Installing an engine or package does not prove its connection works. A cloud localhost is not the user's PC. Report requested-model mismatch if verifiable; never infer or invent model identity.

For Blender, inspect the installation and available integration. A third-party MCP bridge requires current transport/setup checks, an actual host connection, scene inspection and a reversible disposable edit before being called working. Use reproducible executed Blender scripts as an accurately labeled alternative when appropriate. Prove save/export/engine import. Do not install an unnecessary bridge merely to fill a tool list. Do not expose control servers publicly or imply new authorization from this skill.

For Unity or another engine, verify project loading, compilation, scene execution, logs and output separately through available MCP, editor automation or CLI. Give the minimum exact local step only when user action is actually required; continue independent work.

## Establish a visual target

For visual work read [references/visual-and-performance.md](references/visual-and-performance.md). Use the available imagegen skill/tool to produce normally 2–3 separate concept previews before substantial new visual production. Existing approved references may be reused; do not restart an established art direction. If generation is unavailable, preserve executable briefs and label images missing. Written prompts are not generated images.

For persistent Unity desktop investigations or Kubuntu/XWayland settings, consult [the tested chapter validation workflow](references/unity-linux-chapter-validation.md) when applicable. Its version-specific findings require fresh verification in another environment.

Inspect concept outputs, select a provisional direction when clear, and record named reference files and features that must survive implementation. Do not call the selection user-approved. Keep concept previews, authoring renders and runtime evidence distinct. For nonvisual work use representative input/output acceptance examples instead.

## Run one complete loop

1. **Define:** intended improvement and observable acceptance criteria.
2. **Inspect:** reproduce current behavior and retain a relevant baseline.
3. **Implement:** the smallest coherent correction, preserving core behavior.
4. **Run:** build and exercise the actual changed product through its real user path.
5. **Review:** inspect functionality, visuals, usability and relevant measurements.
6. **Correct:** address the highest-impact remaining defect; rerun affected checks.
7. **Checkpoint:** preserve revision, build, evidence, results and remaining gaps.

Review product experience, visual direction, correctness, performance and delivery as relevant. Label sequential agent review as self-review. Use subagents only when authorized and available; never claim independent reviewers that did not run.

Findings need an observed issue, evidence, user impact, proposed correction and verification method. Green automated checks do not overrule broken screenshots or user reports. Programmatic state setup helps repeatable captures but does not prove the real journey. A game needs central action, response, progression outcome and restart/continuation. An app must complete one meaningful task with necessary persistence/errors. A document must demonstrate actual content and final presentation.

Prioritize broken core behavior, defining visual mismatches, serious usability and failed performance gates. After three unsuccessful attempts at the same issue revise the hypothesis or inspect the limiting dependency. Reuse valid evidence. Stop optional polish when the slice passes; advance another bounded slice only within existing authorization. Never imply work continues between sessions without a running job.

## Checkpoint and deliver

Carry existing authorization forward; invoking Gauntlet or completing a slice does not require fresh permission. Track all requested outcomes and continue the next required increment when its dependencies permit. An unavailable gate blocks its dependent claim or promotion, not independent development. Do not end with only a plan when implementation was requested, or treat one successful loop as completion of a larger task.

Maintain project truth in existing records rather than creating a large documentation system. Associate evidence with source revision/build, device, resolution, preset, procedure and known limits. Reverify checks affected by later changes.

Assign each applicable gate PASS, FAIL or UNVERIFIED; N/A requires a reason:

- Real user journey and required outcomes demonstrated.
- Explicit visual criteria met in inspected runtime evidence.
- Performance thresholds met on the stated target and delivered preset.
- Relevant checks pass without hidden blocking regressions.
- Accessible artifact/preview and accurate opening instructions supplied.

An unmeasured performance gate stays UNVERIFIED. Do not erase failures with an overall score. Make a focused branch/commit and prepare a PR when workflow/access permit; keep it unmerged without merge authorization. Publish only to an authorized destination/access level.

Finish briefly in Norwegian: what works, how to open it, concept-versus-runtime evidence, measured performance and conditions, material gaps and next bounded step. Label the scope prototype, verified slice or complete requested project accurately.
