# Scope, story and production decisions

Use this for a design bible, substantial new content or a roadmap. Scale down for a small slice. The aim is to remove production guesses, not to generate a large document for every change.

## Define an executable content envelope

Capture the player's role, central actions, intended experience, platform, audience, target duration and constraints. Separate established canon, accepted working decisions, new proposals and implemented behavior. For an existing game, start from playable content before adding more.

Budget locations, interiors, unique characters/rigs, mechanics, puzzles, dialogue, cutscenes and endings. Each unique environment or system has an integration and verification cost. Prefer reuse with meaningful changed context when it serves the game; repetition alone does not create worthwhile duration. Project-specific limits such as three locations or 5–6 hours are examples, never defaults for another game.

For each chapter/sequence, record only what helps implementation:

| Decision | Required clarity |
| --- | --- |
| Entry state | What the player has seen, learned and retained |
| Objective | What they are trying to establish or accomplish |
| Action | What they actually do using the game's controls |
| Evidence/feedback | What supports the conclusion or signals success/failure |
| Change | Knowledge, world state or capability gained |
| Exit and recovery | What unlocks next and how interruption/failure is handled |
| Dependencies | Assets, text, systems and preceding tasks needed |
| Duration | Estimate, assumptions and eventual measured result |

Use stable task/evidence IDs if cross-references make them useful. Make the dependency graph explicit enough to detect circular prerequisites and unreachable conclusions. Check whether the opening teaches the actions required later.

## Story as a chain the player can understand

Write the complete underlying explanation and ending consequences for production. Then trace what the player can know at each moment. Separate mystery from missing information: an intended deduction needs accessible supporting clues. Avoid source text, labels or UI highlighting that accidentally gives away a comparison puzzle.

For an investigation puzzle, provide the actual player-facing documents or dialogue and a separate facilitator explanation. A paper prototype can expose broken reasoning before asset production. A self-review is not a blind comprehension test. If no new reader is available, retain that gate as UNVERIFIED and proceed only with work that does not depend on its success.

State what each branch changes. Budget shared scenes and distinct outcomes explicitly; do not imply two fully separate campaigns when only the ending differs. Draft the required text, not merely a list of documents someone must later invent.

## Maps and UI must describe usable paths

Choose map type deliberately: topology, traversal blockout, measured layout or world overview. Label it. Include entry/exit, landmarks, interactions, prerequisites and meaningful return routes. Do not present schematic coordinates as engine geometry.

Design important UI states together: first launch, existing progress, pause, load selection, save in progress, success, failure, recovery, canceled action and settings. Show focus/keyboard behavior when applicable and identify which buttons are enabled. A static mockup cannot prove navigation or persistence.

Distinguish checkpoint, manual slot, separate playthrough and immutable evidence archive. Decide what "New", "Continue" and "Load" mean before implementing their storage behavior.

## Roadmap by dependency and evidence

A useful default sequence to adapt is:

- Prove the central interaction and any uncertain story deduction.
- Establish the necessary persistence and transition contract.
- Produce one integrated sequence with representative art and UI.
- Expand content using the proven asset/system families.
- Tune pacing, feedback, accessibility and performance; verify the delivered build.

For each milestone name the deliverable, prerequisites, completion evidence and deliberate cuts. Maintain an asset list linked to the tasks that need each item, with reuse strategy and acceptance criteria. Reserve explicit polish and verification effort; do not promise a fixed percentage or another creator's schedule.

Duration arithmetic is a planning check. Measure first-time play separately from retries, optional exploration, pauses and expert knowledge. Do not pad travel or repeated chores merely to hit an hour count. Update the estimate after observed play.
