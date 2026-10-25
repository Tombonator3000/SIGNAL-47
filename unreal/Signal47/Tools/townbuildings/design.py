"""Original Caliche production architecture. Source-only contract, no DCC launch.
Coordinates are Blender metres/Z up; worldThree=(rootX+Bx,rootY+Bz,rootZ-By).
Floors taken from immutable CORE gameplay proxy heights, not guessed terrain.
"""
REVISION='TB1-original-six-building-production'
PREFIX='SM_Town_'
BUILDINGS={
 'Sheriff':dict(root=[7984,7.192696381179047,2106],footprint=[-10,0,-6,6],height=3.5,front=1,door=[4.4,1.0,2.15],windows=[[-1.3,3.6,.8,2.4]],style='weathered tan mineral courthouse, barred oak office and records room'),
 'Laundry':dict(root=[7985,8.013043546669647,2133],footprint=[-10,0,-7,7],height=3.4,front=1,door=[-4.4,1.1,2.2],windows=[[4.1,3.1,.6,2.3],[.7,3.1,.6,2.3]],style='chipped cream concrete block, faded blue enamel and coin operated washing machines'),
 'Feed':dict(root=[7985,10.375556755065918,2202],footprint=[-16,0,-12,12],height=5.2,front=1,door=[.5,5.2,3.6],windows=[[-5.8,2.6,1.2,2.9],[6.4,2.6,1.2,2.9]],style='false-front stucco mercantile, corrugated awning, timber stock room and loading dock'),
 'Cinema':dict(root=[7985,8.987393474578857,2162],footprint=[-14,0,-10,10],height=6.2,front=1,door=[2.6,1.3,2.3],windows=[[1,1.3,.1,2.3],[-2.6,1.3,.1,2.3]],style='stepped Art Deco mineral facade, physical marquee, shuttered ticket booth and terrazzo lobby'),
 'Church':dict(root=[8021,6.315627861022949,2095.5],footprint=[0,14,-4.5,4.5],height=5.4,front=-1,door=[0,2.2,2.9],windows=[[2.9,.55,2.45,3.95],[-2.9,.55,2.45,3.95]],style='small weathered timber church, pitched shingle roof, real bell tower, pointed windows, pews and sacristy'),
 'Motel':dict(root=[-45.9,-2.0985683853163195,674.9],footprint=[-6.6,2.4,-45.1,2.9],height=2.8,front=1,door=[.8,1.4,2.2],windows=[[-1.3,2.04,.71,2.07]],style='Sierra Motor Court eight-room wing, teal frames, open reception and complete independent Room 6'),
}
ROOM6_ROOT=[0,0,-8000]
ROOM6_PROTECTED={'door':[-1.4,-3.04,1.05],'bed':[1.12,1.56,.43],'phone':[-.27,2.22,.66],'tv':[-1.87,.55,1.21],'photo':[-1.7,1.36,1.06],'table':[.7,-1.45,.775],'lamp':[1.18,-1.15,1.13],'nora':[.75,-2.27,.73],'fieldCard':[.33,-1.13,.835],'letter':[.76,-1.21,.835],'correction':[1.13,-1.53,.835],'walkie':[.27,-1.62,.86]}
MOTEL_PROTECTED={'register':[-.5,.85,1.048],'envelope':[-.5,-.2,1.027],'message':[-.5,-1.13,1.027],'keyBoard':[-2.07,1.67,1.67],'officePhone': [.23,2.51,1.43],'officePhoneHandset':[.43,2.52,1.57],'officeTv':[-1.87,-1.95,1.73]}
# Existing runtime objects/media/NPCs are kept by native integration, never authored twice.
PROTECTED_RUNTIME=['world.town.car','world.town.carDoor','world.town.woman','world.town.smoker','world.town.paper','world.town.cutout','world.motel.tv.group','world.motel.objs.envelope','world.motel.objs.message','world.motel.objs.key6Hook','world.motel.objs.officePhoneHandset','world.motel.objs.room6DoorLeaf']+[f'world.room6.objs.{n}'for n in ['door','phone','tv','photo','lamp','fieldCard','letter','correction','shoebox','walkie','noraHead']]
PARTS={}
for b in BUILDINGS:
 names=['Shell','Glass','Interior','Fixtures'] if b!='Motel' else ['Shell','Glass','Office','Fixtures','Room6Shell','Room6Interior','Room6Fixtures','Bathroom']
 for c in names:
  root=ROOM6_ROOT if c.startswith('Room6') or c=='Bathroom' else BUILDINGS[b]['root']
  PARTS[PREFIX+b+'_'+c]={'building':b,'chunk':c,'offset':[0,0,0],'site_world_three_m':root,'parent_handle':'world.room6.group' if root==ROOM6_ROOT else ('world.motel.group'if b=='Motel' else 'world.town.group'),'role':BUILDINGS[b]['style']+' / '+c}
