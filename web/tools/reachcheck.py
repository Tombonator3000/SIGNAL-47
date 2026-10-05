#!/usr/bin/env python3
"""Read-only movement/pick sampling through S47's existing hooks.

Run from web, or pass --url / S47_URL. No gameplay hooks or source are changed.
--self-test checks the checker with adversarial fixtures, not the real game.
Exit codes: 0 PASS, 1 a measured FAIL, 2 incomplete/UNVERIFIED without measured FAIL.
"""
from __future__ import annotations

import argparse
import asyncio
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
from urllib.parse import unquote, urlparse

WEB = Path(__file__).resolve().parents[1]
AREAS = ("saro", "station01", "room6", "diner")

# Shared by browser execution and the adversarial Node fixtures below. Physics
# and raycasting are supplied by the game; these helpers never replace them.
ENGINE_JS = r"""
function makeReachEngine() {
  const EPS = 1e-4, STEP = .04;
  const rect = b => ({id:b.id ?? null,minX:b.minX,maxX:b.maxX,minZ:b.minZ,maxZ:b.maxZ,enabled:b.enabled !== false});
  const circleClear = (x,z,r,cs) => cs.every(c => {
    const dx=x-Math.max(c.minX,Math.min(c.maxX,x));
    const dz=z-Math.max(c.minZ,Math.min(c.maxZ,z));
    return dx*dx+dz*dz >= r*r-1e-12;
  });
  const aabbClear = (x,z,r,cs) => cs.every(c => x+r<=c.minX || x-r>=c.maxX || z+r<=c.minZ || z-r>=c.maxZ);
  const isSteep = delta => Math.abs(delta)>.12+1e-9;
  const reportStatus = report => {
    const statuses=Object.values(report.checks??{}).map(c=>c.status);
    if(report.status==='FAIL'||(report.findings??[]).some(f=>f.status==='FAIL'))statuses.push('FAIL');
    if(report.steep_edges?.length)statuses.push('FAIL');
    if(!report.completed||(report.method?.spacing_m??.15)!==.15)statuses.push('UNVERIFIED');
    return statuses.includes('FAIL')?'FAIL':statuses.includes('UNVERIFIED')?'UNVERIFIED':'PASS';
  };
  const interactionSummary = rows => {
    const active=rows.filter(o=>o.label),failed=active.filter(o=>o.status==='FAIL'),
      unknown=rows.filter(o=>o.label_error || (o.label&&o.status==='UNVERIFIED'));
    return {status:failed.length?'FAIL':!active.length||unknown.length?'UNVERIFIED':'PASS',
      area_registered_count:rows.length,active_count:active.length,
      reachable_active_count:active.filter(o=>o.status==='PASS').length,
      inactive_count:rows.filter(o=>!o.label&&!o.label_error).length,unknown_count:unknown.length};
  };
  const narrowJoin = (a,b) => {
    const dx=Math.min(a.maxX,b.maxX)-Math.max(a.minX,b.minX);
    const dz=Math.min(a.maxZ,b.maxZ)-Math.max(a.minZ,b.minZ);
    return {dx,dz,narrow:dx>1e-9 && dz>1e-9 && (dx<=.6+1e-9 || dz<=.6+1e-9),
      touching:dx>=-1e-9 && dz>=-1e-9};
  };
  function movement(p,a,b) {
    const dx=b.x-a.x,dz=b.z-a.z,d=Math.hypot(dx,dz);
    p.place(a.x,a.z,0); p.shake=0; p.bob=0;
    if (!Number.isFinite(p.speed) || p.speed<=0) throw Error('Invalid Player.speed');
    const n=Math.max(1,Math.ceil(d/STEP));
    for(let i=0;i<n;i++) p.update(d/n/p.speed,d?dx/d:0,d?dz/d:0,false);
    const endpoint={x:p.pos.x,z:p.pos.z,floorY:p.floorY};
    const error=Math.hypot(endpoint.x-b.x,endpoint.z-b.z);
    return {pass:error<=EPS,from:{x:a.x,z:a.z},to:{x:b.x,z:b.z},endpoint,
      error_m:error,steps:n,max_step_m:d/n};
  }
  function flood(valid,w,h,coords,seed,move,guard) {
    const parents=new Int32Array(valid.length).fill(-2),q=[];
    for(const s of seed) {if(valid[s.index] && parents[s.index]===-2){parents[s.index]=-1;q.push(s.index);}}
    let tested=0,blocked=0;
    for(let head=0;head<q.length;head++) {
      if((head&255)===0) guard();
      const id=q[head],ix=id%w,iz=Math.floor(id/w);
      const ns=[];
      if(ix>0)ns.push(id-1); if(ix<w-1)ns.push(id+1);
      if(iz>0)ns.push(id-w); if(iz<h-1)ns.push(id+w);
      for(const to of ns) {
        if(!valid[to] || parents[to]!==-2)continue;
        const result=move(coords(id),coords(to));tested++;
        if(result.pass){parents[to]=id;q.push(to);}else blocked++;
      }
    }
    return {parents,reached:q,tested,blocked};
  }
  const acceptedPick=(inter,camera,it) => inter.pick(camera)===it;
  async function run(cfg) {
    const started=performance.now(),deadline=started+cfg.budget_ms;
    const guard=()=>{if(performance.now()>deadline)throw Error('TIME_LIMIT: area sampling incomplete');};
    const s=globalThis.S47,report={area:cfg.area,status:'UNVERIFIED',completed:false,checks:{},findings:[],
      method:{spacing_m:cfg.spacing,endpoint_tolerance_m:EPS,max_movement_step_m:STEP,
        validity:'Player.walkable plus Euclidean circle/rectangle clearance, as requested',
        actual_physics:'Existing Player.place/update, including actual AABB collision resolution',
        graph:'Four cardinal neighbours; directed, tested parent tree. Reverse edges are never inferred.',
        floor:'Height changes are measured separately and never removed from the reach graph.',
        pick:'Actual inter.pick, full unmodified registry, exact target identity; legal camera pitch.',
        limitations:['Finite 0.15 m grid and finite aim samples are not a continuous-space proof.',
          'Circle-clear corners may be rejected by the actual AABB resolver; those remain in the requested grid.',
          'No hardware performance or complete chapter playthrough is verified.']}};
    let p,saved;
    if(cfg.spacing!==.15){
      report.checks.required_spacing={status:'UNVERIFIED',reason:'Custom spacing does not satisfy the required 0.15 m full check.'};
      report.method.limitations.push(report.checks.required_spacing.reason);
    }
    try {
      const needed=['player','world','camera','scene','game'];
      if(!s || needed.some(k=>!s[k]) || !s.game.d?.inter || typeof s.jump!=='function' || typeof s.tick!=='function')
        throw Error('Missing existing S47 hooks');
      p=s.player;
      if(typeof p.place!=='function'||typeof p.update!=='function'||typeof p.walkable!=='function')throw Error('Missing Player hooks');
      if(![p.radius,p.eye,p.speed].every(v=>Number.isFinite(v)&&v>0))throw Error('Player radius/eye/speed must be finite and positive');
      if(typeof s.world.prepare!=='function'||typeof s.world.enter!=='function')throw Error('Missing World hooks');
      s.hold=true;
      s.jump(cfg.area==='station01'?'chapter3':cfg.area==='room6'?'chapter4':'chapter1');
      s.tick(.1);
      if(cfg.area==='saro') {
        if(!s.doors?.set)throw Error('Missing DoorRegistry.set');
        for(const id of ['east','south','lab','exit','records'])s.doors.set(id,true,true);
      }
      await s.world.prepare(cfg.area);s.world.enter(cfg.area);
      const model=cfg.area==='station01'?s.world.site:cfg.area==='room6'?s.world.room6:cfg.area==='diner'?s.world.diner:null;
      const anchor=cfg.area==='saro'?{x:0,z:2}:model?.anchors?.arrive;
      if(!anchor || !Number.isFinite(anchor.x)||!Number.isFinite(anchor.z))throw Error('Missing finite arrive anchor');
      if(!Array.isArray(p.zones)||!Array.isArray(p.colliders))throw Error('Missing zone/collider arrays');
      const zones=p.zones.filter(z=>z.enabled!==false).map(rect),cs=p.colliders.map(rect);
      if(!zones.length)throw Error('No enabled zones');
      if([...zones,...cs].some(b=>![b.minX,b.maxX,b.minZ,b.maxZ].every(Number.isFinite)))throw Error('Non-finite rectangle');
      if(cfg.area==='saro' && typeof s.world.grounds?.floorAt!=='function')throw Error('Missing SARO floorAt hook');
      if(p.floor!=null && typeof p.floor!=='function')throw Error('Player.floor is neither null nor a function');
      const floor=cfg.area==='saro'?s.world.grounds.floorAt:p.floor??(()=>0);
      saved={onStep:p.onStep,bob:p.bob,stepDist:p.stepDist,shake:p.shake,pos:p.pos.clone(),yaw:p.yaw,pitch:p.pitch,
        floorY:p.floorY,cameraPosition:s.camera.position.clone(),cameraRotation:s.camera.rotation.clone()};
      p.onStep=undefined;p.shake=0;
      s.scene.updateMatrixWorld(true);
      report.phase=s.game.phase ?? s.game.state?.phase ?? null;
      report.zones=zones;report.colliders=cs;report.player={radius_m:p.radius,eye_m:p.eye,speed_mps:p.speed};
      report.start={x:anchor.x,z:anchor.z,floor:floor(anchor.x,anchor.z),
        walkable:p.walkable(anchor.x,anchor.z),circle_clear:circleClear(anchor.x,anchor.z,p.radius,cs),seed_witnesses:[]};
      const spacing=cfg.spacing,ox=Math.floor(Math.min(...zones.map(b=>b.minX))/spacing)*spacing,
        oz=Math.floor(Math.min(...zones.map(b=>b.minZ))/spacing)*spacing;
      const w=Math.ceil((Math.max(...zones.map(b=>b.maxX))-ox)/spacing)+1,
        h=Math.ceil((Math.max(...zones.map(b=>b.maxZ))-oz)/spacing)+1;
      if(w*h>cfg.max_cells)throw Error(`Grid ${w*h} exceeds explicit max-cells ${cfg.max_cells}; no subset tested`);
      const valid=new Uint8Array(w*h),heights=new Float64Array(w*h),aabb=new Uint8Array(w*h);
      const coords=id=>({x:ox+(id%w)*spacing,z:oz+Math.floor(id/w)*spacing});
      const validIds=[];
      for(let id=0;id<valid.length;id++) {
        if((id&8191)===0)guard();
        const t=coords(id);
        if(p.walkable(t.x,t.z) && circleClear(t.x,t.z,p.radius,cs)) {
          valid[id]=1;validIds.push(id);heights[id]=floor(t.x,t.z);
          if(!Number.isFinite(heights[id]))throw Error('Non-finite floor sample');
          aabb[id]=aabbClear(t.x,t.z,p.radius,cs)?1:0;
        }
      }
      report.grid={origin:{x:ox,z:oz},spacing_m:spacing,width:w,height:h,total_cells:w*h,
        valid_indices:validIds,reached_indices:[],parents:[],valid_count:validIds.length,
        aabb_clear_count:validIds.filter(id=>aabb[id]).length};
      if(!validIds.length)throw Error('No valid grid points');
      const mv=(a,b)=>movement(p,a,b);
      const local=validIds.filter(id=>{const t=coords(id);return Math.hypot(t.x-anchor.x,t.z-anchor.z)<=spacing*3;})
        .sort((a,b)=>{const u=coords(a),v=coords(b);return Math.hypot(u.x-anchor.x,u.z-anchor.z)-Math.hypot(v.x-anchor.x,v.z-anchor.z);});
      const seed=[];
      if(report.start.walkable && report.start.circle_clear) {
        // Seed only witnessed moves from the exact anchor, never a teleport snap.
        for(const index of local) {const witness=mv(anchor,coords(index));if(witness.pass){seed.push({index});
          report.start.seed_witnesses.push({index,...witness});break;}}
      }
      if(!seed.length)throw Error('Exact start did not reach a nearby valid grid point');
      report.checks.start={status:'PASS',evidence:'Exact anchor to grid movement witness recorded'};
      const graph=flood(valid,w,h,coords,seed,mv,guard),reachedSet=new Set(graph.reached);
      report.grid.reached_indices=graph.reached;report.grid.parents=graph.reached.map(id=>graph.parents[id]);
      report.grid.reached_count=graph.reached.length;report.grid.unreachable_count=validIds.length-graph.reached.length;
      report.movement={tested_directed_attempts:graph.tested,blocked_attempts:graph.blocked,
        successful_parent_edges:graph.reached.length-seed.length,
        parent_format:'parents[n] is predecessor of reached_indices[n]; -1 is exact-start witness seed',
        first_parent_witnesses:graph.reached.filter(id=>graph.parents[id]>=0).slice(0,8).map(id=>mv(coords(graph.parents[id]),coords(id)))};
      const unreachable=validIds.filter(id=>!reachedSet.has(id)),remaining=new Set(unreachable),components=[];
      while(remaining.size) {
        const first=remaining.values().next().value,q=[first];remaining.delete(first);
        let corners=0;for(let k=0;k<q.length;k++){const id=q[k];if(!aabb[id])corners++;
          const ix=id%w,iz=Math.floor(id/w),ns=[ix? id-1:-1,ix<w-1?id+1:-1,iz?id-w:-1,iz<h-1?id+w:-1];
          for(const n of ns)if(remaining.delete(n))q.push(n);}
        components.push({count:q.length,circle_clear_but_aabb_overlap_count:corners,
          example:coords(first),indices:q,classification:corners===q.length?'circle/AABB corner discrepancy':'unreached valid component'});
      }
      report.grid.unreachable_components=components;
      report.checks.reachability={status:unreachable.length?'FAIL':'PASS',unreachable_count:unreachable.length,
        explanation:'Every valid point is required. Small corner discrepancies are identified, not whitelisted.'};
      for(const c of components)report.findings.push({check:'reachability',status:'FAIL',at:c.example,
        count:c.count,reason:c.classification});
      report.steep_edges=[];let blockedHeightBoundaries=0;
      for(const id of validIds) {
        if((id&2047)===0)guard();
        const ix=id%w,iz=Math.floor(id/w),ns=[ix<w-1?id+1:-1,iz<h-1?id+w:-1];
        for(const to of ns)if(to>=0&&valid[to]&&isSteep(heights[id]-heights[to])) {
          const forward=mv(coords(id),coords(to)),reverse=mv(coords(to),coords(id));
          if(forward.pass||reverse.pass) report.steep_edges.push({a:{...coords(id),floor:heights[id]},
            b:{...coords(to),floor:heights[to]},delta_m:Math.abs(heights[id]-heights[to]),forward,reverse,
            reachable_from_start:reachedSet.has(id)||reachedSet.has(to)});
          else blockedHeightBoundaries++;
        }
      }
      report.checks.floor_steps={status:report.steep_edges.length?'FAIL':'PASS',passable_above_0_12m:report.steep_edges.length,
        blocked_height_boundaries:blockedHeightBoundaries};
      for(const e of report.steep_edges)report.findings.push({check:'floor_steps',status:'FAIL',at:e.a,delta_m:e.delta_m});
      report.joins=[];
      const inside=(b,t)=>t.x>=b.minX+p.radius-1e-6&&t.x<=b.maxX-p.radius+1e-6&&t.z>=b.minZ+p.radius-1e-6&&t.z<=b.maxZ-p.radius+1e-6;
      const zoneIndices=zones.map(b=>validIds.filter(id=>inside(b,coords(id))));
      for(let ai=0;ai<zones.length;ai++)for(let bi=ai+1;bi<zones.length;bi++) {
        guard();const a=zones[ai],b=zones[bi],overlap=narrowJoin(a,b);if(!overlap.narrow)continue;
        const aReached=zoneIndices[ai].filter(id=>reachedSet.has(id)),bReached=zoneIndices[bi].filter(id=>reachedSet.has(id));
        const cx=(Math.max(a.minX,b.minX)+Math.min(a.maxX,b.maxX))/2,
          cz=(Math.max(a.minZ,b.minZ)+Math.min(a.maxZ,b.maxZ))/2;
        const near=ids=>ids.slice().sort((u,v)=>{const x=coords(u),y=coords(v);return Math.hypot(x.x-cx,x.z-cz)-Math.hypot(y.x-cx,y.z-cz);}).slice(0,12);
        let direct=null,bestBlocked=null;
        outer:for(const u of near(zoneIndices[ai]))for(const v of near(zoneIndices[bi])) {
          const x=coords(u),y=coords(v);if(Math.hypot(x.x-y.x,x.z-y.z)>1.5)continue;
          const f=mv(x,y),r=mv(y,x),proof={a_index:u,b_index:v,forward:f,reverse:r};
          if(!bestBlocked)bestBlocked=proof;if(f.pass&&r.pass){direct=proof;break outer;}
        }
        const bothReached=aReached.length>0&&bReached.length>0;
        const jo={zones:[a.id??`zone_${ai}`,b.id??`zone_${bi}`],zone_indices:[ai,bi],
          overlap_m:{x:overlap.dx,z:overlap.dz},at:{x:cx,z:cz},geometry_status:'FAIL',
          geometry_reason:'Positive overlap does not exceed 0.6 m in both dimensions',
          direct_bidirectional_status:direct?'PASS':bestBlocked?'FAIL':'UNVERIFIED',
          direct_witness:direct??bestBlocked,connection_via_actual_start_tree:bothReached?'PASS':'FAIL',
          reached_zone_witnesses:[aReached[0]??null,bReached[0]??null],
          interpretation:direct?'Both directions work despite narrow raw overlap':bothReached?
            'Both zones reached through actual movement; blocked direct join may be an intentional wall or alternate door route':
            'Narrow raw join and at least one zone without an actual-start witness'};
        report.joins.push(jo);
        report.findings.push({check:'zone_joins',status:'FAIL',at:jo.at,zones:jo.zones,
          reason:jo.geometry_reason,connection_status:jo.connection_via_actual_start_tree});
      }
      report.checks.zone_joins={status:report.joins.length?'FAIL':'PASS',narrow_positive_overlaps:report.joins.length,
        explanation:'Raw geometry rule and real connection evidence are separate. Zero overlap, gaps and disabled zones are not failures.'};
      const inter=s.game.d.inter,cam=s.camera;
      if(!Array.isArray(inter.items)||typeof inter.pick!=='function')throw Error('Missing Interaction.items/pick');
      // Inspect actual mesh bounds without removing hidden/inert proxies or changing range.
      function bounds(object) {
        const b={minX:Infinity,minY:Infinity,minZ:Infinity,maxX:-Infinity,maxY:-Infinity,maxZ:-Infinity};
        let count=0;object.traverse(o=>{const g=o.geometry;if(!g)return;
          if(!g.boundingBox)g.computeBoundingBox();const bb=g.boundingBox;if(!bb)return;
          for(const x of [bb.min.x,bb.max.x])for(const y of [bb.min.y,bb.max.y])for(const z of [bb.min.z,bb.max.z]) {
            const v=p.pos.clone().set(x,y,z).applyMatrix4(o.matrixWorld);count++;
            b.minX=Math.min(b.minX,v.x);b.maxX=Math.max(b.maxX,v.x);b.minY=Math.min(b.minY,v.y);
            b.maxY=Math.max(b.maxY,v.y);b.minZ=Math.min(b.minZ,v.z);b.maxZ=Math.max(b.maxZ,v.z);
          }});return count?b:null;
      }
      report.objects=[];report.excluded_other_area_items=[];
      for(const it of inter.items) {
        guard();const bb=bounds(it.object),range=it.range??2.4;
        if(bb&&!zones.some(z=>bb.maxX>=z.minX-range&&bb.minX<=z.maxX+range&&bb.maxZ>=z.minZ-range&&bb.minZ<=z.maxZ+range)) {
          report.excluded_other_area_items.push({id:it.id,uuid:it.object.uuid});continue;
        }
        let label;try{label=it.label();}catch(e){report.objects.push({id:it.id,status:'UNVERIFIED',label_error:true,reason:'label threw: '+e.message});continue;}
        const row={id:it.id,uuid:it.object.uuid,label,range_m:range,bounds:bb,status:'UNVERIFIED',
          at:bb?{x:(bb.minX+bb.maxX)/2,z:(bb.minZ+bb.maxZ)/2}:null};
        report.objects.push(row);
        if(!label){row.reason='No current label in the activated chapter/area; later or conditional interaction not tested';continue;}
        if(!bb){row.reason='No inspectable geometry bounds';continue;}
        const cx=(bb.minX+bb.maxX)/2,cz=(bb.minZ+bb.maxZ)/2,cy=(bb.minY+bb.maxY)/2;
        const aimPoints=[];
        for(const x of [cx,bb.minX+(bb.maxX-bb.minX)*.2,bb.minX+(bb.maxX-bb.minX)*.8])
          for(const z of [cz,bb.minZ+(bb.maxZ-bb.minZ)*.2,bb.minZ+(bb.maxZ-bb.minZ)*.8])
            for(const y of [cy,bb.minY+(bb.maxY-bb.minY)*.2,bb.minY+(bb.maxY-bb.minY)*.8])aimPoints.push({x,y,z});
        const candidates=graph.reached.filter(id=>{const t=coords(id),eye=heights[id]+p.eye;
          const dx=t.x-Math.max(bb.minX,Math.min(bb.maxX,t.x)),dz=t.z-Math.max(bb.minZ,Math.min(bb.maxZ,t.z)),
            dy=eye-Math.max(bb.minY,Math.min(bb.maxY,eye));return Math.hypot(dx,dy,dz)<=range+1e-6;})
          .sort((a,b)=>{const x=coords(a),y=coords(b);return Math.hypot(x.x-cx,x.z-cz)-Math.hypot(y.x-cx,y.z-cz);});
        row.candidate_reached_points=candidates.length;row.aim_attempts=0;row.blocker_examples=[];
        search:for(const index of candidates) {
          guard();const from=coords(index);p.place(from.x,from.z,0);
          for(const aim of aimPoints) {
            const eye=heights[index]+p.eye,dx=aim.x-from.x,dz=aim.z-from.z,dy=aim.y-eye;
            const pitch=Math.atan2(dy,Math.hypot(dx,dz));if(pitch< -1.35||pitch>1.25)continue;
            const yaw=Math.atan2(-dx,-dz);
            cam.position.set(from.x,eye,from.z);cam.rotation.set(pitch,yaw,0,'YXZ');cam.updateMatrixWorld(true);
            const hit=inter.pick(cam);row.aim_attempts++;
            if(hit===it) {
              const ray=inter.ray,hits=ray?.intersectObjects?.(inter.meshes,true),first=hits?.find(h=>h.object.userData.interactId);
              row.status='PASS';row.proof={grid_index:index,from:{...from,floor:heights[index]},eye_y:eye,
                aim,yaw,pitch,actual_pick:{id:hit.id,uuid:hit.object.uuid,same_registered_object:true},
                first_ray_hit:first?{distance_m:first.distance,interact_id:first.object.userData.interactId,point:first.point.toArray()}:null};
              break search;
            }
            if(row.blocker_examples.length<3)row.blocker_examples.push({grid_index:index,aim,
              actual_pick:hit?{id:hit.id,uuid:hit.object.uuid}:null});
          }
        }
        if(row.status!=='PASS'){row.status='FAIL';row.reason='No reached grid point and sampled legal aim picked this actual registered target';
          report.findings.push({check:'interactions',status:'FAIL',id:it.id,at:row.at,reason:row.reason});}
      }
      const summary=interactionSummary(report.objects);
      report.checks.interactions={...summary,
        reason:!summary.active_count?'No active registered area interactions; geometry proxies alone cannot verify gameplay coverage':
          'Current active labels sampled; inactive conditional/later labels remain individually UNVERIFIED'};
      if(cfg.area==='diner'&&!report.objects.length)report.checks.interactions.unbound_proxy_count=model?.proxies?Object.keys(model.proxies).length:null;
      report.completed=true;
    } catch(e) {report.error=String(e.stack??e);report.completed=false;
      report.checks.completeness={status:'UNVERIFIED',reason:String(e.message??e)};
    } finally {
      if(p&&saved){p.onStep=saved.onStep;p.pos.copy(saved.pos);p.yaw=saved.yaw;p.pitch=saved.pitch;
        p.floorY=saved.floorY;p.bob=saved.bob;p.stepDist=saved.stepDist;p.shake=saved.shake;
        s.camera.position.copy(saved.cameraPosition);s.camera.rotation.copy(saved.cameraRotation);s.camera.updateMatrixWorld(true);}
      report.elapsed_seconds=(performance.now()-started)/1000;
      report.status=reportStatus(report);
    }
    return report;
  }
  return {run,circleClear,aabbClear,narrowJoin,movement,flood,acceptedPick,isSteep,interactionSummary,reportStatus};
}
"""

SELFTEST_JS = r"""
const assert=(ok,msg)=>{if(!ok)throw Error(msg);console.log('PASS '+msg);};
const e=makeReachEngine(),r=.27,c=[{minX:0,maxX:1,minZ:0,maxZ:1}];
assert(e.circleClear(1+.8*r,1+.8*r,r,c)&&!e.aabbClear(1+.8*r,1+.8*r,r,c),'circle-clear diagonal is distinct from actual AABB overlap');
assert(!e.circleClear(1+.6*r,1+.6*r,r,c),'circle-collider diagonal rejection');
function fake(move){return {speed:1.9,pos:{x:0,z:0},floorY:0,place(x,z){this.pos={x,z};},update(dt,mx,mz){move(this,dt,mx,mz);}};}
const frozen=fake(()=>{}),moving=fake((p,dt,mx,mz)=>{p.pos.x+=mx*p.speed*dt;p.pos.z+=mz*p.speed*dt;});
assert(!e.movement(frozen,{x:0,z:0},{x:.15,z:0}).pass,'0.15 m edge cannot PASS without update progress');
assert(e.movement(moving,{x:0,z:0},{x:.15,z:0}).pass,'short-step movement reaches exact endpoint');
const oneWay=fake((p,dt,mx,mz)=>{if(mx>0)p.pos.x+=mx*p.speed*dt;});
assert(e.movement(oneWay,{x:0,z:0},{x:.15,z:0}).pass&&!e.movement(oneWay,{x:.15,z:0},{x:0,z:0}).pass,'directed edge is not mirrored');
const valid=new Uint8Array([1,1,0,1,1]),xy=id=>({x:id*.15,z:0});
const f=e.flood(valid,5,1,xy,[{index:0}],(a,b)=>e.movement(moving,a,b),()=>{});
assert(f.reached.join(',')==='0,1'&&f.parents[4]===-2,'disconnected valid island stays unreachable');
const active={id:'target'},inert={id:'inert'},camera={};
assert(!e.acceptedPick({items:[inert,active],pick(){return null;}},camera,active),'inert front proxy remains a blocker in actual pick');
assert(!e.acceptedPick({pick(){return {id:'target'};}},camera,active),'same id is not proof of exact registered target identity');
assert(e.acceptedPick({pick(){return active;}},camera,active),'actual target pick accepted');
const a={minX:0,maxX:1,minZ:0,maxZ:1},b={minX:.4,maxX:2,minZ:0,maxZ:1};
assert(e.narrowJoin(a,b).narrow,'overlap exactly 0.60 m violates strictly greater than 0.60');
assert(!e.narrowJoin(a,{...b,minX:.399}).narrow,'overlap 0.601 m in both dimensions is sufficient');
assert(!e.narrowJoin(a,{...b,minX:1}).narrow,'mere touching without positive overlap is not narrow-join failure');
assert(e.isSteep(.121)&&e.isSteep(-.121)&&!e.isSteep(.119),'actual floor helper distinguishes 0.121 and 0.119 m in both directions');
assert(e.flood(new Uint8Array([1,1]),2,1,xy,[{index:0}],(a,b)=>e.movement(moving,a,b),()=>{}).reached.length===2,
  'height QA does not remove movement edges');
assert(e.interactionSummary([{label:'active',status:'PASS'},{label_error:true,status:'UNVERIFIED'}]).status==='UNVERIFIED',
  'successful active target cannot hide label exception');
assert(e.interactionSummary([{label:'active',status:'FAIL'},{label_error:true,status:'UNVERIFIED'}]).status==='FAIL',
  'label exception cannot hide a measured interaction failure');
assert(e.interactionSummary([]).status==='UNVERIFIED','empty area interactions are not 0/0 PASS');
assert(e.reportStatus({completed:true,method:{spacing_m:.2},checks:{reachability:{status:'FAIL'}}})==='FAIL',
  'custom spacing preserves a measured failure');
assert(e.reportStatus({completed:true,method:{spacing_m:.2},checks:{reachability:{status:'PASS'}}})==='UNVERIFIED',
  'custom spacing without failures stays unverified');
assert(e.reportStatus({completed:false,checks:{completeness:{status:'UNVERIFIED'}},findings:[{status:'FAIL'}]})==='FAIL',
  'incomplete run preserves already measured findings');
assert(e.reportStatus({completed:false,checks:{completeness:{status:'UNVERIFIED'}}})==='UNVERIFIED',
  'incomplete run without measured failures never passes');
assert(e.reportStatus({completed:true,method:{spacing_m:.15},checks:{reachability:{status:'PASS'}}})==='PASS',
  'complete required spacing passes when all checks pass');
assert(e.reportStatus({completed:false,steep_edges:[{delta_m:.44}]})==='FAIL',
  'interrupted floor scan retains an already measured steep edge');
console.log('CHECKER SELF-TEST PASS (fixtures only; real runtime unverified)');
"""


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def aggregate(statuses: list[str]) -> str:
    return "FAIL" if "FAIL" in statuses else "UNVERIFIED" if "UNVERIFIED" in statuses else "PASS"


def report_status(report: dict) -> str:
    statuses = [c["status"] for c in report.get("checks", {}).values()]
    if report.get("status") == "FAIL" or any(f.get("status") == "FAIL" for f in report.get("findings", [])):
        statuses.append("FAIL")
    if report.get("steep_edges"):
        statuses.append("FAIL")
    if not report.get("completed") or report.get("method", {}).get("spacing_m", .15) != .15:
        statuses.append("UNVERIFIED")
    return aggregate(statuses)


def git_commit() -> str | None:
    try:
        return subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=WEB, text=True).strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def draw_map(report: dict, dest: Path) -> None:
    """Annotated diagnostic only; does not edit any production art."""
    from PIL import Image, ImageDraw, ImageFont
    grid = report.get("grid")
    font = ImageFont.load_default(size=16)
    small = ImageFont.load_default(size=13)
    if not grid:
        im = Image.new("RGB", (1000, 220), "white")
        ImageDraw.Draw(im).text((20, 20), f"{report['area']} UNVERIFIED\nNo complete grid: {report.get('error', '')[:160]}", fill="black", font=font)
        im.save(dest)
        return
    spacing, width, height = grid["spacing_m"], grid["width"], grid["height"]
    ox, oz = grid["origin"]["x"], grid["origin"]["z"]
    scale = min(12 / spacing, 1700 / max(width * spacing, height * spacing))
    plot_w, plot_h = max(240, math.ceil(width * spacing * scale)), max(240, math.ceil(height * spacing * scale))
    left, top, footer = 90, 70, 150
    im = Image.new("RGB", (plot_w + left + 30, plot_h + top + footer), "white")
    d = ImageDraw.Draw(im)

    def point(x: float, z: float) -> tuple[float, float]:
        return left + (x - ox) * scale, top + (z - oz) * scale

    def cell(index: int) -> tuple[float, float]:
        return point(ox + (index % width) * spacing, oz + (index // width) * spacing)

    for c in report.get("colliders", []):
        d.rectangle((*point(c["minX"], c["minZ"]), *point(c["maxX"], c["maxZ"])), fill="#dadada")
    for b in report.get("zones", []):
        d.rectangle((*point(b["minX"], b["minZ"]), *point(b["maxX"], b["maxZ"])), outline="#aab0b6")
    reached = set(grid.get("reached_indices", []))
    half = max(.6, spacing * scale * .48)
    for index in grid["valid_indices"]:
        x, z = cell(index)
        colour = "#41a95c" if index in reached else "#de4545" if report.get("completed") else "#b9bfc5"
        d.rectangle((x-half, z-half, x+half, z+half), fill=colour)
    for edge in report.get("steep_edges", []):
        d.line((*point(edge["a"]["x"], edge["a"]["z"]), *point(edge["b"]["x"], edge["b"]["z"])), fill="#ff8500", width=4)
    for join in report.get("joins", []):
        x, z = point(join["at"]["x"], join["at"]["z"])
        d.ellipse((x-5, z-5, x+5, z+5), outline="#b29b00", fill="#ffe24b", width=2)
    for item in report.get("objects", []):
        if not item.get("at"):
            continue
        x, z = point(item["at"]["x"], item["at"]["z"])
        colour = "#2375e8" if item["status"] == "PASS" else "black" if item["status"] == "FAIL" else "#8a5bb0"
        d.ellipse((x-4,z-4,x+4,z+4), fill=colour)
        d.text((x+5,z+2),item["id"],fill=colour,font=small)
    anchor = report.get("start", {})
    if "x" in anchor:
        x, z = point(anchor["x"], anchor["z"])
        d.line((x-8,z,x+8,z),fill="white",width=3);d.line((x,z-8,x,z+8),fill="white",width=3)
        d.text((x+9,z-14),"START",font=small,fill="black")
    d.text((left, 14), f"{report['area']}   {report['status']}   grid {spacing:g} m   {grid.get('reached_count',0)}/{grid['valid_count']} reached", font=font, fill="black")
    for axis, count, origin in (("x", width, ox), ("z", height, oz)):
        span = (count-1)*spacing
        tick = max(1, 5 * math.ceil(span/50))
        first = math.ceil(origin/tick)*tick
        for value in range(first, math.floor((origin+span)/tick)*tick+1, tick):
            if axis == "x":
                px, _ = point(value, oz); d.text((px-12, top+plot_h+7), f"{value:g}", fill="black", font=small)
            else:
                _, pz = point(ox, value); d.text((5, pz-7), f"{value:g}", fill="black", font=small)
    y = top+plot_h+35
    completion = "COMPLETE" if report.get("completed") else "INCOMPLETE / UNVERIFIED: grey cells have no final reach verdict"
    d.text((left,y),completion+"; world x,z in metres, north / -z is up",fill="black",font=small)
    legend=[("#41a95c","Reached"),("#de4545","Valid unreached"),("#ff8500","Passable >0.12 m"),("#ffe24b","Narrow raw join"),("#2375e8","Picked target"),("black","Unpicked active"),("#8a5bb0","Inactive / unknown")]
    x=left;y+=26
    for colour,label in legend:
        if x+len(label)*8+25>im.width-20:x=left;y+=24
        d.rectangle((x,y,x+12,y+12),fill=colour);d.text((x+17,y-1),label,font=small,fill="black");x+=len(label)*8+35
    im.save(dest)


def write_artifacts(report: dict, out: Path) -> None:
    """Record output failures explicitly; old files never stand in for new evidence."""
    json_path, map_path = out/f"{report['area']}.json", out/f"{report['area']}.png"
    report["artifacts"] = {"json_written":False,"map_written":False,"map_sha256":None}
    problems = []
    try:
        json_path.write_text(json.dumps(report,indent=1,ensure_ascii=False)+"\n")
        report["artifacts"]["json_written"] = True
    except (OSError, TypeError, ValueError) as exc:
        problems.append("JSON output: "+str(exc))
    try:
        draw_map(report,map_path)
        report["artifacts"]["map_written"] = True
        report["artifacts"]["map_sha256"] = sha(map_path.read_bytes())
    except Exception as exc:
        problems.append("Map output: "+str(exc))
    report.setdefault("checks",{})["output_completeness"] = {"status":"UNVERIFIED" if problems else "PASS","errors":problems}
    report["status"] = report_status(report)
    try:
        json_path.write_text(json.dumps(report,indent=1,ensure_ascii=False)+"\n")
        report["artifacts"]["json_written"] = True
    except (OSError, TypeError, ValueError) as exc:
        report["artifacts"]["json_written"] = False
        report["checks"]["output_completeness"] = {"status":"UNVERIFIED","errors":problems+["Final JSON output: "+str(exc)]}
        report["status"] = report_status(report)


def self_test_status() -> None:
    """Exercise Python status/output failures; fixture maps are not game evidence."""
    from copy import deepcopy
    from tempfile import TemporaryDirectory
    from unittest.mock import patch

    def expect(ok: bool, name: str) -> None:
        if not ok:
            raise AssertionError(name)
        print("PASS " + name, flush=True)

    def fixture(status: str, *, completed: bool = True, spacing: float = .15) -> dict:
        return {"area":"fixture", "status":"UNVERIFIED", "completed":completed,
                "method":{"spacing_m":spacing}, "checks":{"reachability":{"status":status}}, "findings":[]}

    cases = [
        (fixture("PASS"), "PASS", "Python complete required spacing passes"),
        (fixture("FAIL", spacing=.2), "FAIL", "Python custom spacing preserves measured FAIL"),
        (fixture("PASS", spacing=.2), "UNVERIFIED", "Python custom spacing without FAIL stays unverified"),
        (fixture("PASS", completed=False), "UNVERIFIED", "Python incomplete checks never pass"),
        ({**fixture("PASS", completed=False), "findings":[{"status":"FAIL"}]}, "FAIL", "Python partial findings retain FAIL"),
        ({**fixture("PASS", completed=False), "checks":{"runtime_errors":{"status":"FAIL"}}}, "FAIL", "Python incomplete runtime failure retains FAIL"),
        ({**fixture("PASS", completed=False), "steep_edges":[{"delta_m":.44}]}, "FAIL", "Python interrupted floor scan retains measured FAIL"),
    ]
    for report, expected, name in cases:
        expect(report_status(report) == expected, name)
    expect(aggregate(["UNVERIFIED", "FAIL"]) == "FAIL", "Python summary retains FAIL beside UNVERIFIED")
    with TemporaryDirectory(prefix="s47-status-fixtures-") as folder:
        out = Path(folder)
        for measured, expected in (("FAIL", "FAIL"), ("PASS", "UNVERIFIED")):
            report = fixture(measured)
            with patch(__name__ + ".draw_map", side_effect=OSError("fixture map write failure")):
                write_artifacts(report, out)
            expect(report["status"] == expected and not report["artifacts"]["map_written"]
                   and json.loads((out/"fixture.json").read_text())["status"] == expected,
                   "Python map write failure with " + measured + " yields " + expected)
        for measured, expected in (("FAIL", "FAIL"), ("PASS", "UNVERIFIED")):
            report = fixture(measured)
            original_write = Path.write_text
            writes = 0

            def fail_final_json(path: Path, *args, **kwargs):
                nonlocal writes
                writes += 1
                if writes == 2:
                    raise OSError("fixture final JSON write failure")
                return original_write(path, *args, **kwargs)

            with patch(__name__ + ".draw_map", side_effect=lambda r, dest: dest.write_bytes(b"fixture only")), \
                 patch.object(Path, "write_text", fail_final_json):
                write_artifacts(report, out)
            expect(report["status"] == expected and not report["artifacts"]["json_written"],
                   "Python final JSON failure with " + measured + " yields " + expected)
        partial = deepcopy(fixture("PASS", completed=False))
        with patch(__name__ + ".draw_map", side_effect=lambda r, dest: dest.write_bytes(b"fixture only")):
            write_artifacts(partial, out)
        expect(partial["status"] == "UNVERIFIED", "Python successful outputs do not verify an incomplete run")
    print("PYTHON STATUS SELF-TEST PASS (fixtures only)", flush=True)


async def check(args: argparse.Namespace) -> int:
    args.out_dir.mkdir(parents=True, exist_ok=True)
    metadata = {"generated_utc":datetime.now(timezone.utc).isoformat(),"script_sha256":sha(Path(__file__).read_bytes()),
                "git_base":git_commit(),"source_url":args.url,"source_html_sha256":None,
                "browser":"Playwright Chromium, ANGLE swiftshader", "requested_areas":args.areas}
    reports: list[dict] = []
    started = time.monotonic()
    browser = None
    pg = None
    errors: list[str] = []
    try:
        from playwright.async_api import async_playwright
        async with async_playwright() as pw:
            browser = await pw.chromium.launch(args=["--use-angle=swiftshader","--enable-unsafe-swiftshader","--ignore-gpu-blocklist"])
            pg = await browser.new_page(viewport={"width":1280,"height":800})
            pg.set_default_timeout(min(60000, args.timeout * 1000))
            pg.on("pageerror", lambda e: errors.append(str(e)))
            await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
            response = await pg.goto(args.url, wait_until="load")
            parsed = urlparse(args.url)
            if parsed.scheme == "file":
                metadata["source_html_sha256"] = sha(Path(unquote(parsed.path)).read_bytes())
            elif response:
                metadata["source_html_sha256"] = sha(await response.body())
            await pg.wait_for_selector("button[data-a=start]")
            await pg.click("button[data-a=start]")
            await pg.wait_for_function("globalThis.S47 && S47.started && S47.started()")
            await pg.evaluate("S47.hold = true")
            for area in args.areas:
                remaining = args.timeout - (time.monotonic() - started)
                if remaining < 2:
                    report = {"area":area,"status":"UNVERIFIED","completed":False,"error":"Total time budget exhausted before area", "checks":{},"findings":[]}
                else:
                    try:
                        report = await asyncio.wait_for(pg.evaluate("async cfg => {"+ENGINE_JS+"; return await makeReachEngine().run(cfg);}",
                            {"area":area,"spacing":args.spacing,"max_cells":args.max_cells,"budget_ms":max(1,(remaining-1)*1000)}),timeout=remaining)
                    except Exception as exc:
                        report = {"area":area,"status":"UNVERIFIED","completed":False,"error":str(exc),"checks":{},"findings":[]}
                report["provenance"] = dict(metadata)
                report["runtime_page_errors"] = list(errors)
                report["checks"]["runtime_errors"] = {"status":"FAIL" if errors else "PASS","page_errors":list(errors)}
                report["status"] = report_status(report)
                reports.append(report)
                write_artifacts(report,args.out_dir)
                print(f"{area}: {report['status']} ({report.get('grid',{}).get('reached_count',0)}/{report.get('grid',{}).get('valid_count',0)} reached)",flush=True)
                if report.get("error"):
                    print(report["error"].splitlines()[0], flush=True)
            await browser.close()
    except Exception as exc:
        for area in args.areas:
            if area in {r["area"] for r in reports}:
                continue
            report={"area":area,"status":"UNVERIFIED","completed":False,"error":str(exc),
                    "checks":{"runtime_errors":{"status":"FAIL" if errors else "PASS","page_errors":list(errors)}},
                    "runtime_page_errors":list(errors),"findings":[],"provenance":dict(metadata)}
            report["status"] = report_status(report)
            reports.append(report)
            write_artifacts(report,args.out_dir)
        print(aggregate([r["status"] for r in reports])+" "+str(exc),file=sys.stderr,flush=True)
    status=aggregate([r["status"] for r in reports])
    summary={**metadata,"status":status,"elapsed_seconds":time.monotonic()-started,
             "areas":[{"area":r["area"],"status":r["status"],"completed":r["completed"],
                       "report":f"{r['area']}.json" if r["artifacts"]["json_written"] else None,
                       "report_sha256":sha((args.out_dir/f"{r['area']}.json").read_bytes()) if r["artifacts"]["json_written"] else None,
                       "map":f"{r['area']}.png" if r["artifacts"]["map_written"] else None,
                       "map_sha256":r["artifacts"]["map_sha256"],"findings":len(r.get('findings',[]))} for r in reports],
             "limitations":["Checker uses finite samples, not a continuous-space proof.","Swiftshader does not verify hardware fps."]}
    (args.out_dir/"summary.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False)+"\n")
    return 1 if status=="FAIL" else 2 if status=="UNVERIFIED" else 0


def main() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url",default=os.environ.get("S47_URL") or (WEB/"dist-single/index.html").as_uri())
    parser.add_argument("--out-dir",type=Path,default=WEB/"production/reachcheck")
    parser.add_argument("--areas",nargs="+",choices=AREAS,default=list(AREAS))
    parser.add_argument("--spacing",type=float,default=.15,help="Required full-check spacing is 0.15 m; overrides are UNVERIFIED unless a measured FAIL takes precedence")
    parser.add_argument("--timeout",type=float,default=540,help="Total browser/check budget in seconds (default 9 minutes)")
    parser.add_argument("--max-cells",type=int,default=3000000,help="Reject, never truncate, a grid above this safety limit")
    parser.add_argument("--self-test",action="store_true",help="Adversarial checker fixtures only; no real-game execution")
    args=parser.parse_args()
    if args.spacing<=0 or args.timeout<=0 or args.max_cells<1:
        parser.error("spacing, timeout and max-cells must be positive")
    if args.self_test:
        node=shutil.which("node")
        if not node:
            candidate=Path("/home/tombonator3000t/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node")
            node=str(candidate) if candidate.is_file() else None
        if not node:
            print("UNVERIFIED: Node unavailable for embedded checker fixtures",file=sys.stderr)
            return 2
        code = subprocess.run([node],input=ENGINE_JS+SELFTEST_JS,text=True,timeout=30).returncode
        if code:
            return code
        self_test_status()
        return 0
    return asyncio.run(check(args))


if __name__=="__main__":
    raise SystemExit(main())
