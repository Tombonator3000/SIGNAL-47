# Setup and saved-file use

Use this when MCP tools are not already callable. `scripts/mcp_cli.py` launches one real stdio MCP session, discovers the schema, makes one call and retains its response and stderr. It does not register a global MCP server or change an open Blender scene. It requires Python 3.11+ and the MCP SDK; the tested environment uses Python 3.12.

## Installation on another machine

Clone https://github.com/bpy-dev/blender-mcp into a tools directory outside the game. Select and inspect a specific revision; the review here used `e8ac4088d4a3e0469d3eff9f9bb2dd717e899219`. Create a dedicated Python environment and install the clone's `mcp/` directory with that environment's pip. Record `pip freeze` and the Git revision. Do not install a generic package named `blender-mcp` from a package index and assume it is this distribution.

Use the existing Blender executable, not a Python executable, as `BLENDER_PATH`. The local helper supplies `BLENDER_MCP_CLI_BACKEND=blender`. This needs no Blender add-on for the saved-file route. Installation into a venv avoids replacing the project's Blender or system packages.

For the paths already tested on this machine, see [local-verification.md](local-verification.md).

## List and call

Supply absolute executable paths and fresh evidence output names:

```sh
/path/to/venv/bin/python /path/to/skill/scripts/mcp_cli.py \
  --server /path/to/venv/bin/blender-mcp \
  --blender /path/to/blender \
  --output /path/to/evidence/tools.json
```

Then write a request JSON file, using the returned `inputSchema`. Example:

```json
{
  "tool": "get_runtime_python_api_docs_for_cli",
  "arguments": {
    "blend_file": "/path/to/source-copy.blend",
    "identifier": "bpy.utils.blend_paths"
  }
}
```

Run the same command with `--request /path/to/request.json` and a new `--output`. Results contain MCP `response.content`, optional `response.structuredContent`, and `response.isError`; process exit 0 means MCP did not report a tool error. Also inspect returned semantic results: a missing API can be a successful call with `found: false`. The helper refuses live tool names and existing result/log paths. It does not restrict what arbitrary Blender Python can do.

Available CLI tools at the reviewed revision include data-block/missing-file/library/path summaries, runtime API docs, Python execution and CUDA rendering. Object summaries and viewport screenshots are live tools; for saved files, use a compact `execute_blender_code_for_cli` query instead. Do not guess that every live tool has a CLI variant.

## Editing and rendering

Blender 4.5.13 compatibility fallbacks, exercised locally:

```python
# Dependency references (not exhaustive validation of sequences/cache contents).
import bpy, os
paths = bpy.utils.blend_paths(absolute=True, packed=False)
result = {'paths': paths, 'missing': [p for p in paths if not os.path.exists(p)]}
```

```python
# Explicit trusted operator, when the generic runtime lookup says unsupported.
import bpy
p = bpy.ops.export_scene.fbx.get_rna_type().properties
result = {'axis_forward': [v.identifier for v in p['axis_forward'].enum_items],
          'axis_up': [v.identifier for v in p['axis_up'].enum_items],
          'use_selection_default': p['use_selection'].default}
```

Do not interpret an unsupported generic lookup as proof that the Blender operator is unavailable. Do not dynamically evaluate arbitrary identifiers supplied by scene metadata.

For `execute_blender_code_for_cli`, assign JSON-serializable data to `result`. Each call opens a fresh subprocess; only explicitly saved state survives. Use Python code stored in a request file, not shell interpolation of arbitrary text.

For an edit, include `expected_output_blend` in the arguments and explicitly call `bpy.ops.wm.save_as_mainfile(filepath=...)` with that same fresh output. Reopen it in a subsequent call. Compare input/output hashes and the actual asset properties, not just `saved: true`.

The direct image tool at this revision requires NVIDIA CUDA and an existing Cycles scene. Portable preview rendering can run through the execution tool using an already configured scene camera:

```python
import bpy
s = bpy.context.scene
s.render.engine = 'CYCLES'
s.cycles.device = 'CPU'
s.cycles.samples = 16
s.render.resolution_x, s.render.resolution_y = 640, 400
s.render.resolution_percentage = 100
s.render.image_settings.file_format = 'PNG'
s.render.filepath = '/absolute/fresh/output.png'
bpy.ops.render.render(write_still=True)
result = {'rendered': True, 'device': s.cycles.device}
```

These are transient preview settings: do not save them over the authoring source. Confirm the PNG exists, inspect it, and record the source hash. For Eevee or long renders, use the project's proven context/display and batch renderer; a successful CPU preview does not certify those routes.

## Live or standalone modes

If live scene editing is requested, install/enable the reviewed add-on in the intended Blender session and configure a local connection using upstream documentation. Our helper deliberately excludes this route. Unsaved-scene identity and UI context matter; do not send input to a locked desktop.

Standalone `bpy` is independent of Blender's embedded module. Upstream's documented example uses a third-party 5.2.1 wheel and CPython 3.13. It has not been validated by this skill on the SIGNAL / 47 machine. Retain the executable backend unless an actual need justifies a separate compatibility probe.
