# Source assessment — 2026-09-12

Reviewed repository: https://github.com/bpy-dev/blender-mcp

Pinned source: `e8ac4088d4a3e0469d3eff9f9bb2dd717e899219` (2026-09-11). Source review covered architecture, dependency metadata, tool discovery and execution, CLI process handling, live synchronization, runtime documentation, rendering, representative tests, provenance and benchmark publication material. This is a targeted implementation review and compatibility probe, not an exhaustive security audit.

## Decision

Useful as an optional authoring and inspection layer alongside an existing Blender pipeline. Highest-value additions for SIGNAL / 47: queries against actual saved scenes, exact API discovery, missing-dependency detection, isolated saved checkpoints and repeatable visual checks. Existing parameterized modeling scripts and Unity FBX imports remain the production route. No general image-to-3D generator or Unity bridge is implemented here.

## Evidence and constraints

| Finding | Practical consequence | Primary source |
| --- | --- | --- |
| Independent enhanced Blender Lab distribution; developer preview | Pin a reviewed checkout and test local compatibility | [readme](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/readme.md), [notice](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/NOTICE.md) |
| Saved-file tools accept a Blender executable or separate bpy runtime | Existing Blender is sufficient; no mandatory version migration | [backend guide](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/readme_bpy_backend.rst) |
| Exact runtime lookup supplements bundled RST docs | Query uncertain operator signatures/enums on the installed version | [runtime docs tool](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/mcp/blmcp/tools/get_runtime_python_api_docs.py) |
| Several CLI wrappers synchronize dirty live files before opening a subprocess | For exact saved-file provenance, disable implicit live connection or explicitly document its use | [CLI helper](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/mcp/blmcp/tools_helpers/blender_cli.py) |
| Declared output checkpoints must be new and differ from input bytes | Save explicitly; verify the original separately because arbitrary Python still has write access | [execution tool](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/mcp/blmcp/tools/execute_blender_code.py) |
| Saved-image tool hardcodes CUDA and requires Cycles | Use a reviewed CPU render or established Eevee route when those requirements do not fit | [render implementation](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/mcp/blmcp/tools/execute_blender_code.py) |
| 120-second CLI timeout; subprocess is not an OS sandbox | Size preview renders appropriately and scope filesystem writes | [security](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/SECURITY.md) |
| Optional HTTP mode uses permissive CORS and disables DNS rebinding protection | Local stdio avoids needing a listening HTTP server for this workflow | [server entrypoint](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/mcp/blmcp/__init__.py) |

Repository metadata and notices identify GPL-3.0-or-later. The local installation retains these notices. The new skill/helper is original workflow code; no upstream implementation is copied into the game runtime. Generated assets and third-party inputs require their own provenance records.

## Benchmark interpretation

The repository reports 27 tasks over 270 rounds, covering camera and attribute matching. CLIP values express image embedding similarity; the tables report the best round per task. This does not establish game-ready topology, rigging, collision, Unity materials, production time or superiority on our assets. We did not rerun that benchmark or launch its Codex evaluation agents. Their model, approval and runtime configuration is a benchmark harness, not a prerequisite for normal Blender use.

Primary sources: [results](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/benchmarks/blenderbench_direct/publication/results-summary.json), [benchmark scope](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/benchmarks/blenderbench_direct/README.md), [direct launcher](https://github.com/bpy-dev/blender-mcp/blob/e8ac4088d4a3e0469d3eff9f9bb2dd717e899219/tools/direct_codex.md).
