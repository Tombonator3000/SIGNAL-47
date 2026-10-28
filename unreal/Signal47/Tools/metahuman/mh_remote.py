#!/usr/bin/env python3
"""Run Python inside the open MHStaging editor through its file command queue (init_unreal.py).

  mh_remote.py -c 'code'            run literal code (several statements allowed)
  mh_remote.py FILE.py              run a file
  --timeout N                       seconds to wait for the result (default 600)

Only the MHStaging editor reads this queue, so no other Unreal editor can receive the commands.
"""
import json
import os
import sys
import time
import uuid

ROOT = '/home/tom/scratch-bm/Signal47-metahuman-20261026/cmd'


def main():
    args = sys.argv[1:]
    timeout = 600
    if '--timeout' in args:
        i = args.index('--timeout'); timeout = float(args[i + 1]); del args[i:i + 2]
    if not args:
        raise SystemExit(__doc__)
    code = args[1] if args[0] == '-c' else open(args[0]).read()
    os.makedirs(ROOT + '/inbox', exist_ok=True)
    cid = time.strftime('%H%M%S') + '-' + uuid.uuid4().hex[:6]
    tmp = f'{ROOT}/inbox/{cid}.tmp'
    open(tmp, 'w').write(code)
    os.replace(tmp, f'{ROOT}/inbox/{cid}.py')
    result = f'{ROOT}/outbox/{cid}.json'
    end = time.time() + timeout
    while not os.path.exists(result):
        if time.time() > end:
            if os.path.exists(f'{ROOT}/inbox/{cid}.py'):
                os.remove(f'{ROOT}/inbox/{cid}.py')
                raise SystemExit(f'no MHStaging editor took the command within {timeout:.0f}s')
            raise SystemExit(f'command still running after {timeout:.0f}s; result will land in {result}')
        time.sleep(0.25)
    r = json.load(open(result))
    if r['output']:
        print(r['output'].rstrip())
    if not r['ok']:
        print(r['error'].rstrip(), file=sys.stderr)
        raise SystemExit(1)


if __name__ == '__main__':
    main()
