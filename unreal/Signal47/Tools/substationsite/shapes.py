"""Reuse accepted original geometry helpers without path or source mutation."""
from pathlib import Path
import importlib.util
_path=Path(__file__).resolve().parent.parent/'substation'/'meshkit.py'
_spec=importlib.util.spec_from_file_location('s47_substation_original_meshkit',_path)
_original=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_original)
add,sub,mul,dot,cross,unit=(_original.add,_original.sub,_original.mul,_original.dot,_original.cross,_original.unit)
class Mesh(_original.Mesh):
 def face(self,ps,mat,uv=None,smooth=False):
  super().face(ps,mat,uv,smooth)
  self.mat[-1]=self.mat[-1].replace('M_Substation_','M_SubstationSite_',1)
