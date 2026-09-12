---
name: blender-mcp
description: Inspect, modify, and verify Blender assets using the bpy-dev/blender-mcp distribution, including saved-file headless execution and runtime API lookup. Use for Blender asset work or evaluating this bridge; retain the project's Blender version and export pipeline.
---

# Blender MCP

Use this bridge where scene inspection, exact API lookup or iterative authoring improves the existing Blender workflow. A skill supplies instructions; a callable MCP server is a separate dependency. Discover available tools first. If absent, use the tested local stdio client described in [setup-and-use.md](references/setup-and-use.md), or the existing Blender CLI for operations the bridge cannot handle.

This skill concerns **bpy-dev/blender-mcp**, an independent enhanced distribution of Blender Lab MCP. Similarly named packages can be different projects. Read [assessment.md](references/assessment.md) when choosing or updating the installation; it records the reviewed revision, source links and limitations. Read [local-verification.md](references/local-verification.md) for this machine's tested paths and evidence.

## Choose the execution route

- **Saved asset or unattended work:** use `_for_cli` with the project's Blender executable. Start from an explicit saved file, retain its hash, and inspect the output in a new process. This works without controlling the desktop.
- **Authorized edits to an open, unsaved scene:** use the connected add-on's live tools only after identifying the actual scene. Do not assume saved-file tools reflect exactly the disk state: several wrappers call `synced_blend_for_cli`, which can save a temporary copy of the live dirty scene. Our supplied CLI client reserves an unlistened local port to disable this implicit live connection.
- **Standalone bpy:** optional, only when its version, wheel provenance and operators meet the project requirements. Do not replace a working Blender installation to follow the repository's custom-wheel example.

Keep authoring scripts and the engine's established export format. The bridge does not provide a new 3D generation model, retopology service or Unity integration. Magnific/image-to-3D outputs remain candidates for Blender cleanup; use connected services only within the current request and authorization.

## Work in a verifiable loop

1. Identify the intended asset improvement and the saved source. Inspect version, objects, scale, mesh/material counts and relevant missing dependencies. Use summary tools or compact `bpy` data queries; avoid dumping entire scenes.
2. For uncertain API details, use `search_api_docs` for discovery and `get_runtime_python_api_docs_for_cli` for an exact identifier in the actual runtime. Check `found`, not just MCP `isError`. On tested Blender 4.5.13, function lookup works but the FBX operator lookup returns `unsupported`; use an explicit operator RNA query through the execution tool. Reuse established results. Static bundled documentation can differ from installed Blender.
3. Make a bounded change using the data API where possible. `bpy.ops` may depend on active object, mode, selection and UI context. Preserve unrelated geometry, materials, cameras and lights.
4. Save explicitly to a fresh output `.blend`. Supply `expected_output_blend` to receive checkpoint hashes. The guard validates the declared output; it does not stop arbitrary Python from writing elsewhere. Independently check that the original source hash is unchanged.
5. Reopen that output, verify the intended change and inspect a render tied to its hash. Use front/side/rear or gameplay-distance views as relevant. On this reviewed revision, `get_render_as_image_for_cli` hardcodes CUDA and requires an existing Cycles scene. For CPU or Eevee use reviewed `execute_blender_code_for_cli` rendering code with explicit settings and a fresh PNG output. A render timeout is not evidence of a successful render.
6. Export with the project's axis, unit, pivot and material conventions. Check the imported asset in the actual engine before calling it integrated. Blender renders demonstrate authoring output, not gameplay or performance.

Record useful counts and defects rather than maximizing a similarity score. CLIP similarity in the upstream benchmark is an image metric, not a measure of topology, collision, usability or asset quality.

## Operational limits

On tested Blender 4.5.13, the dedicated missing-files summary fails because `bpy.data.file_path_foreach` is absent. Use `bpy.utils.blend_paths(absolute=True, packed=False)` plus existence checks for referenced file paths; image sequences/cache contents can need specialized validation. The tested fallback and its negative fixture are recorded in [local-verification.md](references/local-verification.md).

The server executes Python with the process's file/network permissions. The weak sandbox and subprocess timeouts are not OS isolation. Use reviewed code, scoped output paths and copies of important files. Keep local stdio as the default; an HTTP listener is unnecessary for saved-file work.

CLI execution is limited to 120 seconds in the reviewed server. Use smaller previews or the existing Blender batch pipeline for long renders; do not launch indefinite retries. Inspect the exception and existing outputs before retrying a mutation. This is a diagnostic checkpoint: resolve the cause or use a verified alternative route and continue the authorized task. A failed helper does not make every Blender operation unavailable.

Preserve upstream license notices if copying or redistributing its code. Keep the external tool installation separate from game runtime code and independently record the provenance of generated or imported assets. No external model purchase, desktop unlock, global configuration rewrite or benchmark agent run is implied by invoking this skill.
