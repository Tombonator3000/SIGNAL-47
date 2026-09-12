# Local compatibility — SIGNAL / 47, 2026-09-12

These are machine-specific observations, not requirements for other projects. Recheck paths if moved.

- Reviewed source: `/home/tombonator3000t/signal47-tools/blender-mcp-e8ac408`, commit `e8ac4088d4a3e0469d3eff9f9bb2dd717e899219`.
- Dedicated Python 3.12 environment: `/home/tombonator3000t/signal47-tools/blender-mcp-env/bin/python`.
- Installed MCP server: `/home/tombonator3000t/signal47-tools/blender-mcp-env/bin/blender-mcp`.
- Blender executable: `/home/tombonator3000t/signal47-tools/blender-4.5.13-linux-x64/blender` (4.5.13 LTS; embedded Python 3.11.15).
- Skill client: `/home/tombonator3000t/.codex/skills/blender-mcp/scripts/mcp_cli.py`.
- Evidence directory: `/home/tombonator3000t/signal47-tools/evidence/blender-mcp-20260912`.
- Dependency snapshot: `requirements-installed.txt` in that evidence directory. Installed upstream package 1.0.0, MCP SDK 1.30.0. MCP serverInfo reports the SDK version; it is not the upstream Git revision.

The system Python 3.14 lacked venv/ensurepip. Installation succeeded with the bundled Codex Python 3.12 in a dedicated venv. No system package, Blender version, add-on, global Codex MCP configuration or desktop setting was changed. Tools are usable now through the skill's stdio helper; they are not registered as native tool handles in this conversation.

## Actual asset probe

Input was a disposable copy of the original `Unity/Blender/SIGNAL47_workstation18.blend`. Original SHA-256: `8f8003b54605691ba2403187106fea45be8962d9dfa3305a1846a5476541eca8`.

| Capability | Observed result |
| --- | --- |
| Real MCP stdio initialize/list/call | PASS; 11 saved-file/documentation tools discovered |
| Data-block summary and mesh audit | PASS; 16 scene objects, 13 meshes, 31,732 evaluated triangles |
| Dedicated missing-files summary | FAIL on Blender 4.5.13: `BlendData.file_path_foreach` unavailable; raw failure retained |
| Generic runtime lookup of FBX operator | UNSUPPORTED (`found: false`), despite successful MCP transport |
| Runtime lookup of `bpy.utils.blend_paths` | PASS; reports the actual Blender/Python version and function description |
| Explicit FBX RNA query | PASS; forward/up enums and selection/unit defaults obtained from installed Blender |
| Dependency fallback | PASS for referenced file paths; injected missing image detected; scene had no external paths |
| Explicit checkpoint and fresh-process reopen | PASS; test scene property survives; all mesh counts, dimensions, materials and UV counts unchanged |
| Existing checkpoint rejection | PASS; rejected before supplied code ran |
| CPU render via execution tool | PASS; 640×400 PNG, Cycles CPU, 16 samples; image visually inspected |
| FBX export/import round trip | PASS; 13 meshes, 31,732 triangles; bounds differ by less than 1e-5 metres |
| Original source retention | PASS; original and input-copy SHA-256 unchanged |
| Client live-tool/output guards | PASS; live call rejected; existing evidence bytes retained |

Render: `iteration2/workstation-cpu.png`. It shows the existing CRT casing, keyboard and chair in their authoring layout. It is a Blender render, not a Unity screenshot or new game asset. The screen's live Unity overlay is not part of the Blender asset.

Raw requests/results, original failed iteration, scripts and the separate `compatibility.json` fallback/export test are in the evidence directory. `iteration2/summary.json` records hashes. Check returned semantic fields; an `isError: false` alone is insufficient evidence.

## Upstream tests and open gates

Selected upstream tests (`test_blender_cli`, `test_runtime_python_api_docs_toolcode`, `test_tool_listing`) ran 91 tests: 83 passed, 7 skipped, 1 failed. The tool-listing equality test differs from expected formatting/schema with this installed stack. The raw failure is retained in `upstream-tests.log`. These suites are not fully green; this skill uses the actual returned schemas rather than a copied tool catalog.

Not verified: live add-on/UI control, standalone bpy, CUDA image tool, exhaustive dependency contents, full upstream suite or benchmark. No Unity content was changed and no new gameplay, performance or native input verification is claimed.

Use the working saved-file route for future asset inspections and bounded edits. Preserve the current production source and engine checks when integrating real asset changes.
