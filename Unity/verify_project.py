from pathlib import Path
import re
root=Path(__file__).resolve().parent
required=[
'Assets/Signal47/Runtime/Core/GameSession.cs','Assets/Signal47/Runtime/Signals/SignalConsole.cs','Assets/Signal47/Runtime/Events/PrologueDirector.cs','Assets/Signal47/Editor/Signal47SceneBuilder.cs',
'Assets/Signal47/Art/PrototypePort/SM_CRT_Terminal_A.obj','Assets/Signal47/Art/PrototypePort/SM_RadioDish_Bowl_A.obj']
missing=[p for p in required if not (root/p).exists()]
assert not missing, missing
text=(root/'Assets/Signal47/Runtime/Events/PrologueDirector.cs').read_text()
for token in ['3.6f','4.3f','16f','15f','16f','1.344f','2.976f']: assert token in text, token
console=(root/'Assets/Signal47/Runtime/Signals/SignalConsole.cs').read_text()
editor=(root/'Assets/Signal47/Editor/Signal47SceneBuilder.cs').read_text()
for token in ['1419.900f','1420.110f','1420.405f','-39 LY']: assert token in editor or token in console, token
for p in root.rglob('*.cs'):
    s=p.read_text(); assert s.count('{')==s.count('}'), f'brace mismatch {p}'
print('SIGNAL47 static project checks: PASS')
