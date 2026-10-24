"""Original1986 diner PBR contract. Read-only list, no Blender dependency."""
def linear(rgb):
 return [((c/255)/12.92 if c/255<=.04045 else ((c/255+.055)/1.055)**2.4) for c in rgb]
def material_rows():
 rows=[]
 def row(name,rgb,metal,rough,label=None,normal=.2,**kw):
  d={'name':'M_Mesa_'+name,'base_color_linear':[*(linear(rgb) if not label else [1,1,1]),1],'metallic':metal,'roughness':rough,'specular':.32,'normal_strength':normal,'normal_convention':'DirectX','uv_scale':1,'emission':[0,0,0,1],'emission_strength':0,**kw}
  if label:d['textures']={role:'textures/'+label+'_'+role+'.png' for role in ['base_color','normal','roughness']}
  rows.append(d)
 row('Stucco',[216,203,177],0,.92,'Stucco',.32)
 row('PaintRed',[142,58,48],.05,.72,'PaintRed',.16)
 row('TealTile',[86,129,119],0,.34,'TealTile',.22)
 row('CheckerFloor',[162,153,131],0,.72,'CheckerFloor',.18)
 row('Vinyl',[103,29,26],0,.35,'Vinyl',.16)
 row('Formica',[195,180,147],0,.34,'Formica',.10)
 row('Wood',[82,57,35],0,.78,'Wood',.18)
 row('PrintAtlas',[220,209,175],0,.83,'PrintAtlas',.025)
 row('WallCream',[190,178,147],0,.85)
 row('Ceiling',[180,175,150],0,.93)
 row('TealPaint',[73,111,103],.12,.57)
 row('Chrome',[174,182,179],.94,.23)
 row('DarkMetal',[40,44,40],.75,.67)
 row('Rubber',[21,23,20],0,.91)
 row('Ceramic',[221,211,182],0,.24)
 row('Coffee',[42,23,11],0,.25)
 row('WindowGlass',[149,173,170],0,.10,alpha=.075,alpha_mode='translucent',two_sided=True)
 row('Headlamp',[218,211,170],0,.18)
 row('RedLens',[159,22,13],0,.25)
 row('AmberLens',[196,98,15],0,.25)
 row('TruckPaint',[125,149,135],.32,.61)
 row('SignTeal',[45,93,90],.35,.62)
 for name,rgb,strength in [('SignRedGlow',[255,67,32],3.2),('SignCyanGlow',[83,241,216],2.7),('PendantGlow',[255,222,148],1.8),('Fluorescent',[235,231,199],1.4)]:row(name,rgb,0,.3,emission=[*linear(rgb),1],emission_strength=strength)
 row('Asphalt',[73,71,65],0,.95)
 row('Concrete',[143,136,121],0,.93)
 row('Gravel',[133,120,98],0,.98)
 row('YardMarking',[181,164,114],0,.95)
 return rows
