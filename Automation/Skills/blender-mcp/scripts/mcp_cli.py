#!/usr/bin/env python3
"""One real MCP/stdio request against saved files; no global client configuration.

Original helper. Requires the MCP Python SDK in the invoking environment.
The reserved, unlistened port prevents upstream implicit live-scene syncing.
This helper is not a sandbox for Python supplied to execution tools.
"""
import argparse
import asyncio
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import socket
import sys

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client


def allowed(name):
    return name.endswith('_for_cli') or name in {
        'search_api_docs', 'search_manual_docs', 'get_python_api_docs'
    }


async def run(args, request, log):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as reserved:
        reserved.bind(('127.0.0.1', 0))
        env = dict(os.environ)
        env.update(BLENDER_MCP_CLI_BACKEND='blender', BLENDER_PATH=str(args.blender),
                   BLENDER_MCP_HOST='127.0.0.1',
                   BLENDER_MCP_PORT=str(reserved.getsockname()[1]))
        params = StdioServerParameters(command=str(args.server),
                                      args=['--transport', 'stdio'], env=env)
        async with asyncio.timeout(150):
            async with stdio_client(params, errlog=log) as (read, write):
                async with ClientSession(read, write) as session:
                    init = await session.initialize()
                    listing = await session.list_tools()
                    available = {t.name: t for t in listing.tools if allowed(t.name)}
                    if request is None:
                        value = {'tools': [t.model_dump(mode='json') for t in available.values()]}
                    else:
                        if request['tool'] not in available:
                            raise ValueError('Tool is unavailable on the saved-file route: ' + request['tool'])
                        response = await session.call_tool(request['tool'], request.get('arguments', {}))
                        value = response.model_dump(mode='json')
                    return {'timestamp_utc': datetime.now(timezone.utc).isoformat(),
                            'server': str(args.server), 'blender': str(args.blender),
                            'transport': 'stdio', 'live_sync_disabled': True,
                            'server_info': init.serverInfo.model_dump(mode='json'),
                            'request': request, 'response': value}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server', type=Path, required=True)
    parser.add_argument('--blender', type=Path, required=True)
    parser.add_argument('--request', type=Path, help='JSON {"tool": name, "arguments": {...}}; omit to list')
    parser.add_argument('--output', type=Path, required=True, help='Fresh result JSON; stderr saved beside it')
    args = parser.parse_args()
    for path in (args.server, args.blender):
        if not path.is_absolute() or not path.is_file() or not os.access(path, os.X_OK):
            parser.error('Executable must be an existing absolute executable path: ' + str(path))
    request = json.loads(args.request.read_text()) if args.request else None
    if request is not None and (not isinstance(request, dict) or
                               not allowed(str(request.get('tool', ''))) or
                               not isinstance(request.get('arguments', {}), dict)):
        parser.error('Request must name a saved-file or documentation tool with object arguments')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    log_path = args.output.with_suffix('.stderr.log')
    if args.output.exists() or log_path.exists():
        parser.error('Choose fresh result and log paths')
    with args.output.open('x', encoding='utf-8') as result_file:
        try:
            with log_path.open('x', encoding='utf-8') as log:
                result = asyncio.run(run(args, request, log))
        except Exception as exc:
            json.dump({'failed': True, 'error': str(exc), 'request': request}, result_file, indent=2)
            print('MCP request failed; inspect ' + str(args.output), file=sys.stderr)
            return 1
        json.dump(result, result_file, indent=2, ensure_ascii=False)
        result_file.write('\n')
    failed = bool(result['response'].get('isError'))
    print(json.dumps({'output': str(args.output), 'isError': failed}))
    return int(failed)


if __name__ == '__main__':
    sys.exit(main())
