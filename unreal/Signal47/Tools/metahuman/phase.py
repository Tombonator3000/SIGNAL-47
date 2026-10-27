"""Commandlet entry: run named s47mh steps in order, e.g. `phase.py probe studio audition`.
A step may carry Python literal args: `make_waitress:'Grace'`."""
import ast
import sys

import unreal
import s47mh

for spec in sys.argv[1:]:
    name, _, arg = spec.partition(':')
    args = ast.literal_eval('(' + arg + ',)') if arg else ()
    unreal.log(f'[S47MH] phase step {name}{args}')
    getattr(s47mh, name)(*args)
unreal.log('[S47MH] phase complete')
