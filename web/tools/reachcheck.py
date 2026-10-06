#!/usr/bin/env python3
"""Read-only movement/pick sampling through S47's existing hooks.

Run from web, or pass --url / S47_URL. No gameplay hooks or source are changed.
--self-test checks the checker with adversarial fixtures, not the real game.
Exit codes: 0 PASS, 1 a measured FAIL, 2 incomplete/UNVERIFIED without measured FAIL.
PASS is the regression guard, not complete interaction coverage: the documented
unbound diner and inactive chapter objects retain individual UNVERIFIED coverage.
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
from tempfile import gettempdir

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
  // Immutable 0.15 m baseline from the AABB-clear points in the 6097012 reports.
  // Boxes alone cannot whitelist a new pocket inside the same area. Row membership
  // and total count are checked as well. 1e-6 m only absorbs floating-point noise;
  // it is not a one-cell padding or a tolerance for enlarged geometry.
  const expectedPockets = {
    saro:{id:'b12_end',box:{minX:8.4,maxX:10.65,minZ:-19.2,maxZ:-17.55},max_count:56,
      reason:'Known unused pocket behind the B-12 sign at the end of the east walk; Claude accepted 2026-10-05 00:52 UTC.',
      rows:[[-19.2,8.4,16],[-19.05,8.4,5],[-18.9,8.4,5],[-18.75,8.4,5],[-18.6,8.4,5],[-18.45,8.4,5],[-18.3,8.4,5],
        [-18.15,8.4,2],[-18,8.4,2],[-17.85,8.4,2],[-17.7,8.4,2],[-17.55,8.4,2]]},
    station01:{id:'hut_corner',box:{minX:-12.6,maxX:-12.3,minZ:8002.65,maxZ:8003.1},max_count:5,
      reason:'Known unused corner by the station hut; Claude accepted 2026-10-05 00:52 UTC.',
      rows:[[8002.65,-12.45,1],[8002.8,-12.45,1],[8002.95,-12.45,1],[8003.1,-12.6,2]]},
    // the diner stands in the road area since 5 October (World.ts, DINER_ORIGIN 7979.95, 1999.95)
    diner:{id:'north_inside_corner',box:{minX:7979.55,maxX:7981.35,minZ:1991.55,maxZ:1992.3},max_count:72,
      local_box:{minX:-.4,maxX:1.4,minZ:-8.4,maxZ:-7.65},world_offset:{x:7979.95,z:1999.95},
      reason:'Known unused north interior corner of the diner; Claude accepted 2026-10-05 00:52 UTC.',
      rows:[[1991.55,7979.7,12],[1991.7,7979.7,12],[1991.85,7979.7,12],[1992,7979.7,12],[1992.15,7979.7,12],[1992.3,7979.7,12]]}
  };
  function classifyPockets(area,components,coords,spacing) {
    const spec=expectedPockets[area],tol=1e-6;
    const matches=t=>spec && t.x>=spec.box.minX-tol && t.x<=spec.box.maxX+tol && t.z>=spec.box.minZ-tol && t.z<=spec.box.maxZ+tol &&
      spec.rows.some(([z,x,n])=>Math.abs(t.z-z)<=tol && Array.from({length:n},(_,k)=>x+k*.15).some(v=>Math.abs(t.x-v)<=tol));
    let total=0;
    for(const c of components) {
      const points=c.indices.map(coords);
      c.bounds=points.reduce((b,t)=>({minX:Math.min(b.minX,t.x),maxX:Math.max(b.maxX,t.x),
        minZ:Math.min(b.minZ,t.z),maxZ:Math.max(b.maxZ,t.z)}),{minX:Infinity,maxX:-Infinity,minZ:Infinity,maxZ:-Infinity});
      c.expected=spacing===.15 && !!spec && c.count===points.length && c.count<=spec.max_count && points.every(matches);
      if(c.expected)total+=c.count;
    }
    if(spec && total>spec.max_count)for(const c of components)c.expected=false;
    for(const c of components) {
      c.status=c.expected?'EXPECTED':'FAIL';
      c.classification=c.expected?'documented unused baseline pocket':'new or expanded unreached AABB-clear component';
      if(c.expected)c.expectation={id:spec.id,reason:spec.reason,box:spec.box,local_box:spec.local_box??null,
        baseline_max_count:spec.max_count,spacing_m:.15,coordinate_tolerance_m:tol,
        rule:'Entire component is inside the box and every point matches an immutable baseline row; total expected points may not exceed the baseline.'};
      else delete c.expectation;
    }
    return {unexpected_count:components.filter(c=>!c.expected).reduce((n,c)=>n+c.count,0),
      expected_count:components.filter(c=>c.expected).reduce((n,c)=>n+c.count,0)};
  }
  const isSteep = delta => Math.abs(delta)>.12+1e-9;
  const reportStatus = report => {
    const checks=Object.values(report.checks??{}),statuses=checks.map(c=>c.guard_status??c.status);
    // A coverage exception can never hide an actually measured FAIL.
    if(checks.some(c=>c.status==='FAIL'))statuses.push('FAIL');
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
  // Bidirectional movement within an unvisited island cannot certify access.
  const joinStatus = (aPath,bPath) => aPath?.status==='PASS' && bPath?.status==='PASS' ? 'PASS' : 'FAIL';
  const dinerProxyIds=['booth','clipping','coffee','counter','door','driver','jukebox','menu','payphone','rig','sign','waitress','window'];
  const expectedDinerCoverage=(area,rows,proxyIds) => area==='diner' && rows.length===0 &&
    Array.isArray(proxyIds) && proxyIds.slice().sort().join('|')===dinerProxyIds.join('|');
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
        validity:'Player.walkable plus the exact axis-aligned square clearance condition used by Player.resolve',
        actual_physics:'Existing Player.place/update, including actual AABB collision resolution',
        graph:'Four cardinal neighbours; directed, tested parent tree. Reverse edges are never inferred.',
        floor:'Height changes are measured separately and never removed from the reach graph.',
        pick:'Actual inter.pick, full unmodified registry, exact target identity; legal camera pitch.',
        limitations:['Finite 0.15 m grid and finite aim samples are not a continuous-space proof.',
          'Only the immutable 0.15 m baseline points in three documented unused pockets are expected; new points still fail.',
          'PASS is the regression guard. Inactive objects and the known unbound diner retain UNVERIFIED interaction coverage.',
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
        walkable:p.walkable(anchor.x,anchor.z),aabb_clear:aabbClear(anchor.x,anchor.z,p.radius,cs),seed_witnesses:[]};
      const spacing=cfg.spacing,ox=Math.floor(Math.min(...zones.map(b=>b.minX))/spacing)*spacing,
        oz=Math.floor(Math.min(...zones.map(b=>b.minZ))/spacing)*spacing;
      const w=Math.ceil((Math.max(...zones.map(b=>b.maxX))-ox)/spacing)+1,
        h=Math.ceil((Math.max(...zones.map(b=>b.maxZ))-oz)/spacing)+1;
      if(w*h>cfg.max_cells)throw Error(`Grid ${w*h} exceeds explicit max-cells ${cfg.max_cells}; no subset tested`);
      const valid=new Uint8Array(w*h),heights=new Float64Array(w*h);
      const coords=id=>({x:ox+(id%w)*spacing,z:oz+Math.floor(id/w)*spacing});
      const validIds=[];
      for(let id=0;id<valid.length;id++) {
        if((id&8191)===0)guard();
        const t=coords(id);
        if(p.walkable(t.x,t.z) && aabbClear(t.x,t.z,p.radius,cs)) {
          valid[id]=1;validIds.push(id);heights[id]=floor(t.x,t.z);
          if(!Number.isFinite(heights[id]))throw Error('Non-finite floor sample');
        }
      }
      report.grid={origin:{x:ox,z:oz},spacing_m:spacing,width:w,height:h,total_cells:w*h,
        valid_indices:validIds,reached_indices:[],parents:[],valid_count:validIds.length,
        aabb_clear_count:validIds.length};
      if(!validIds.length)throw Error('No valid grid points');
      const mv=(a,b)=>movement(p,a,b);
      const local=validIds.filter(id=>{const t=coords(id);return Math.hypot(t.x-anchor.x,t.z-anchor.z)<=spacing*3;})
        .sort((a,b)=>{const u=coords(a),v=coords(b);return Math.hypot(u.x-anchor.x,u.z-anchor.z)-Math.hypot(v.x-anchor.x,v.z-anchor.z);});
      const seed=[];
      if(report.start.walkable && report.start.aabb_clear) {
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
        for(let k=0;k<q.length;k++){const id=q[k];
          const ix=id%w,iz=Math.floor(id/w),ns=[ix? id-1:-1,ix<w-1?id+1:-1,iz?id-w:-1,iz<h-1?id+w:-1];
          for(const n of ns)if(remaining.delete(n))q.push(n);}
        components.push({count:q.length,example:coords(first),indices:q});
      }
      const pocketSummary=classifyPockets(cfg.area,components,coords,spacing);
      report.grid.unreachable_components=components;
      report.checks.reachability={status:pocketSummary.unexpected_count?'FAIL':'PASS',unreachable_count:unreachable.length,...pocketSummary,
        explanation:'Exact engine square clearance; only whole components made of documented immutable baseline points are expected.'};
      for(const c of components)report.findings.push({check:'reachability',status:c.status,at:c.example,
        count:c.count,bounds:c.bounds,reason:c.classification,expectation:c.expectation??null});
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
      function startPath(index) {
        if(index==null||!reachedSet.has(index))return {status:'FAIL',reason:'No witnessed path from the exact start'};
        const reversed=[],seen=new Set();let id=index;
        while(id>=0) {
          if(seen.has(id)||!reachedSet.has(id))return {status:'FAIL',reason:'Invalid or cyclic start tree'};
          seen.add(id);reversed.push(id);id=graph.parents[id];
          if(id===-2)return {status:'FAIL',reason:'Unvisited predecessor'};
        }
        if(id!==-1)return {status:'FAIL',reason:'Path did not terminate at a start seed'};
        const ids=reversed.reverse(),seedWitness=report.start.seed_witnesses.find(w=>w.index===ids[0]&&w.pass);
        if(!seedWitness)return {status:'FAIL',reason:'No exact-start seed witness'};
        const points=ids.map(coords),turns=[points[0]];
        // Coalesce collinear sampled parent edges; every represented edge was
        // actually passed, without inferring reverse or diagonal movement.
        for(let k=1;k<points.length-1;k++) {
          const a=points[k-1],b=points[k],c=points[k+1];
          if(Math.abs((b.x-a.x)*(c.z-b.z)-(b.z-a.z)*(c.x-b.x))>1e-8 ||
            (b.x-a.x)*(c.x-b.x)+(b.z-a.z)*(c.z-b.z)<=0)turns.push(b);
        }
        if(points.length>1)turns.push(points[points.length-1]);
        return {status:'PASS',exact_start:anchor,seed_witness:seedWitness,target:coords(index),
          tested_parent_edges:ids.length-1,sampled_points:ids.length,forward_polyline:turns,
          direction:'exact start to target only; reverse path is not inferred'};
      }
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
        const startPaths=[startPath(aReached[0]),startPath(bReached[0])],connectionStatus=joinStatus(...startPaths);
        const bothReached=connectionStatus==='PASS';
        const jo={zones:[a.id??`zone_${ai}`,b.id??`zone_${bi}`],zone_indices:[ai,bi],
          overlap_m:{x:overlap.dx,z:overlap.dz},at:{x:cx,z:cz},status:connectionStatus,geometry_status:'NARROW',
          geometry_reason:'Positive raw overlap does not exceed 0.6 m in both dimensions; this alone is not a movement failure',
          direct_bidirectional_status:direct?'PASS':bestBlocked?'FAIL':'UNVERIFIED',
          direct_witness:direct??bestBlocked,connection_via_actual_start_tree:bothReached?'PASS':'FAIL',
          start_path_witnesses:startPaths,
          reached_zone_witnesses:[aReached[0]??null,bReached[0]??null],
          interpretation:bothReached&&direct?'Both sides have witnessed start paths and both direct directions work despite narrow raw overlap':bothReached?
            'Both zones reached through actual movement; blocked direct join may be an intentional wall or alternate door route':
            'Narrow raw join and at least one zone without an actual-start witness'};
        report.joins.push(jo);
        if(!bothReached)report.findings.push({check:'zone_joins',status:'FAIL',at:jo.at,zones:jo.zones,
          reason:jo.geometry_reason,connection_status:jo.connection_via_actual_start_tree});
      }
      report.checks.zone_joins={status:report.joins.some(j=>j.status==='FAIL')?'FAIL':'PASS',narrow_positive_overlaps:report.joins.length,
        connected_count:report.joins.filter(j=>j.status==='PASS').length,
        explanation:'A narrow join passes only with actual directed paths from the exact start to both sides; unvisited direct candidates never suffice. Zero overlaps, gaps and disabled zones are excluded.'};
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
      if(cfg.area==='diner'&&!report.objects.length) {
        const proxyIds=model?.proxies?Object.keys(model.proxies):null;
        report.checks.interactions.unbound_proxy_count=proxyIds?.length??null;
        report.checks.interactions.unbound_proxy_ids=proxyIds?.slice().sort()??null;
        if(expectedDinerCoverage(cfg.area,report.objects,proxyIds)) {
          report.checks.interactions.guard_status='PASS';
          report.checks.interactions.expected_coverage_gap={status:'UNVERIFIED',
            reason:'Documented diner geometry preview: all 13 named proxies exist but the chapter has no area-registered interactions yet. Coverage is not certified.',
            approved_utc:'2026-10-05T00:52:00Z',expected_area_registered_count:0,expected_proxy_ids:dinerProxyIds};
        }
      }
      report.interaction_coverage={status:summary.status==='FAIL'?'FAIL':
        summary.status==='UNVERIFIED'||summary.inactive_count?'UNVERIFIED':'PASS',
        active_status:summary.status,inactive_count:summary.inactive_count,
        reason:'Inactive and unbound interactions retain UNVERIFIED coverage even when the regression guard passes.'};
      report.completed=true;
    } catch(e) {report.error=String(e.stack??e);report.completed=false;
      report.checks.completeness={status:'UNVERIFIED',reason:String(e.message??e)};
    } finally {
      if(p&&saved){p.onStep=saved.onStep;p.pos.copy(saved.pos);p.yaw=saved.yaw;p.pitch=saved.pitch;
        p.floorY=saved.floorY;p.bob=saved.bob;p.stepDist=saved.stepDist;p.shake=saved.shake;
        s.camera.position.copy(saved.cameraPosition);s.camera.rotation.copy(saved.cameraRotation);s.camera.updateMatrixWorld(true);}
      report.elapsed_seconds=(performance.now()-started)/1000;
      report.status=reportStatus(report);
      report.guard_status=report.status;
    }
    return report;
  }
  return {run,circleClear,aabbClear,narrowJoin,joinStatus,movement,flood,acceptedPick,isSteep,interactionSummary,reportStatus,
    expectedPockets,classifyPockets,expectedDinerCoverage};
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
assert(e.aabbClear(-r,.5,r,c)&&e.aabbClear(1+r,.5,r,c)&&!e.aabbClear(-r+1e-8,.5,r,c),
  'exact Player.resolve square rule allows touching but rejects overlap');
const pc=points=>[{indices:points.map((_,i)=>i),count:points.length}];
const known=[{x:8.4,z:-19.2},{x:8.55,z:-19.2}],knownComp=pc(known);
assert(e.classifyPockets('saro',knownComp,i=>known[i],.15).expected_count===2 && knownComp[0].status==='EXPECTED',
  'known baseline pocket points are expected with coordinates and reason');
const insideNew=[{x:9.15,z:-19.05}],insideComp=pc(insideNew);
assert(e.classifyPockets('saro',insideComp,i=>insideNew[i],.15).unexpected_count===1,
  'new point inside the broad known box is not whitelisted');
const extended=[...known,{x:10.8,z:-19.2}],extendedComp=pc(extended);
assert(e.classifyPockets('saro',extendedComp,i=>extended[i],.15).expected_count===0 && extendedComp[0].status==='FAIL',
  'expanded component fails as a whole rather than hiding its old points');
const elsewhere=[{x:1,z:1}],elseComp=pc(elsewhere);
assert(e.classifyPockets('saro',elseComp,i=>elsewhere[i],.15).unexpected_count===1,
  'new pocket elsewhere fails');
const noisy=[{x:8.4+5e-7,z:-19.2-5e-7}],noisyComp=pc(noisy);
assert(e.classifyPockets('saro',noisyComp,i=>noisy[i],.15).expected_count===1,
  'baseline coordinates allow sub-micrometre floating-point noise only');
const shifted=[{x:8.4+.001,z:-19.2}],shiftComp=pc(shifted);
assert(e.classifyPockets('saro',shiftComp,i=>shifted[i],.15).unexpected_count===1,
  'one millimetre shift is not baseline sampling tolerance');
assert(e.classifyPockets('saro',pc(known),i=>known[i],.2).expected_count===0,
  'custom spacing cannot inherit 0.15 m pocket expectations');
const overflow=Array.from({length:57},()=>({x:8.4,z:-19.2})),overflowComp=pc(overflow);
assert(e.classifyPockets('saro',overflowComp,i=>overflow[i],.15).unexpected_count===57,
  'expected point count cannot exceed the frozen area baseline');
const yes={status:'PASS'},no={status:'FAIL'},directYes={forward:{pass:true},reverse:{pass:true}};
assert(e.joinStatus(yes,yes,null)==='PASS','alternate route with both exact-start paths passes narrow join');
assert(e.joinStatus(yes,no,directYes)==='FAIL','unvisited component cannot pass merely because its direct join works');
assert(e.joinStatus(no,no,directYes)==='FAIL','two unvisited components with bidirectional direct movement still fail');
assert(e.expectedDinerCoverage('diner',[],['booth','clipping','coffee','counter','door','driver','jukebox','menu','payphone','rig','sign','waitress','window']),
  'known diner geometry-only coverage gap has all thirteen named proxies');
assert(!e.expectedDinerCoverage('diner',[],['unknown']) && !e.expectedDinerCoverage('saro',[],[]),
  'missing or unexpected proxy inventory and other empty areas do not get a coverage exception');
assert(e.reportStatus({completed:true,checks:{interactions:{status:'UNVERIFIED',guard_status:'PASS'}}})==='PASS',
  'explicit expected diner coverage gap does not block a complete regression guard');
assert(e.reportStatus({completed:true,checks:{interactions:{status:'FAIL',guard_status:'PASS'}}})==='FAIL',
  'even an erroneous guard exception cannot hide a measured interaction FAIL');
assert(e.reportStatus({completed:true,checks:{interactions:{status:'UNVERIFIED'}}})==='UNVERIFIED',
  'unexplained interaction uncertainty still blocks the guard');
console.log('CHECKER SELF-TEST PASS (fixtures only; real runtime unverified)');
"""


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def aggregate(statuses: list[str]) -> str:
    return "FAIL" if "FAIL" in statuses else "UNVERIFIED" if "UNVERIFIED" in statuses else "PASS"


def report_status(report: dict) -> str:
    checks = list(report.get("checks", {}).values())
    statuses = [c.get("guard_status", c["status"]) for c in checks]
    if any(c["status"] == "FAIL" for c in checks):
        statuses.append("FAIL")
    if report.get("status") == "FAIL" or any(f.get("status") == "FAIL" for f in report.get("findings", [])):
        statuses.append("FAIL")
    if report.get("steep_edges"):
        statuses.append("FAIL")
    if not report.get("completed") or report.get("method", {}).get("spacing_m", .15) != .15:
        statuses.append("UNVERIFIED")
    return aggregate(statuses)


def guard_exit_code(status: str) -> int:
    return 1 if status == "FAIL" else 2 if status == "UNVERIFIED" else 0


def ensure_output_dir(out: Path) -> bool:
    try:
        out.mkdir(parents=True, exist_ok=True)
        return True
    except OSError as exc:
        print("UNVERIFIED output directory: " + str(exc), file=sys.stderr, flush=True)
        return False


def write_summary(summary: dict, out: Path, status: str) -> str:
    try:
        (out/"summary.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False)+"\n")
        return status
    except (OSError,TypeError,ValueError) as exc:
        print("UNVERIFIED summary output: "+str(exc),file=sys.stderr,flush=True)
        return aggregate([status,"UNVERIFIED"])


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
    left, top = 90, 70
    image_width = plot_w + left + 30
    legend=[("#41a95c","Reached"),("#62b9b2","Expected pocket"),("#de4545","New unreached"),("#ff8500","Passable >0.12 m"),("#ffe24b","Narrow join, see witness"),("#2375e8","Picked target"),("black","Unpicked active"),("#8a5bb0","Inactive / unknown")]
    legend_rows, legend_x = 1, left
    # Use exactly the drawing loop's wrapping rule, including its label spacing.
    for _, label in legend:
        if legend_x+len(label)*8+25>image_width-20:
            legend_x=left;legend_rows+=1
        legend_x+=len(label)*8+35
    footer=max(150,81+(legend_rows-1)*24+13+20)
    im = Image.new("RGB", (image_width, plot_h + top + footer), "white")
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
    expected = {i for c in grid.get("unreachable_components", []) if c.get("expected") for i in c.get("indices", [])}
    half = max(.6, spacing * scale * .48)
    for index in grid["valid_indices"]:
        x, z = cell(index)
        colour = "#41a95c" if index in reached else "#62b9b2" if index in expected else "#de4545" if report.get("completed") else "#b9bfc5"
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
    d.text((left, 14), f"{report['area']}   sampling {report['status']}   grid {spacing:g} m   {grid.get('reached_count',0)}/{grid['valid_count']} reached", font=font, fill="black")
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
    y += 20
    d.text((left,y),"Final delivery verdict: summary.json",fill="black",font=small)
    x=left;y+=26
    for colour,label in legend:
        if x+len(label)*8+25>im.width-20:x=left;y+=24
        d.rectangle((x,y,x+12,y+12),fill=colour);d.text((x+17,y-1),label,font=small,fill="black");x+=len(label)*8+35
    im.save(dest)


def compact_report(report: dict) -> dict:
    """Keep verdicts, coordinates and witnesses, not the large sampled arrays."""
    bulk_keys = {"valid_indices", "reached_indices", "parents", "indices"}

    def trim(value):
        if isinstance(value, dict):
            return {key: trim(item) for key, item in value.items() if key not in bulk_keys}
        if isinstance(value, list):
            return [trim(item) for item in value]
        return value

    result = trim(report)
    result["report_format"] = "compact"
    result["report_format_note"] = "Full sampled grid arrays are omitted; maps use the full in-memory grid. --full-grid writes a separate external report."
    return result


def validate_full_grid_dir(folder: Path) -> Path:
    """Resolve aliases/symlinks before rejecting the production tree."""
    resolved, production = folder.resolve(), (WEB/"production").resolve()
    if resolved == production or production in resolved.parents:
        raise ValueError("Full grids must be outside web/production, including symlink aliases")
    return resolved


def write_artifacts(report: dict, out: Path, full_grid_dir: Path | None = None) -> None:
    """Record output failures explicitly; old files never stand in for new evidence."""
    json_path, map_path = out/f"{report['area']}.json", out/f"{report['area']}.png"
    report["artifacts"] = {"json_written":False,"map_written":False,"map_sha256":None,
                           "full_grid_written":False,"full_grid_path":None,"full_grid_sha256":None}
    problems = []
    # The diagnostic map must be drawn before compacting the sampled arrays.
    try:
        draw_map(report,map_path)
        report["artifacts"]["map_written"] = True
        report["artifacts"]["map_sha256"] = sha(map_path.read_bytes())
    except Exception as exc:
        problems.append("Map output: "+str(exc))
    try:
        draft = compact_report(report)
        draft.setdefault("checks",{})["output_completeness"] = {"status":"UNVERIFIED",
            "errors":problems+["Draft report: final output publication is not complete"]}
        draft["status"] = report_status(draft)
        draft["guard_status"] = draft["status"]
        draft["report_format_note"] += " Draft: final artifact publication has not completed."
        json_path.write_text(json.dumps(draft,indent=1,ensure_ascii=False)+"\n")
        report["artifacts"]["json_written"] = True
    except (OSError, TypeError, ValueError) as exc:
        problems.append("JSON output: "+str(exc))
    if full_grid_dir is not None:
        try:
            full_grid_dir = validate_full_grid_dir(full_grid_dir)
            full_grid_dir.mkdir(parents=True, exist_ok=True)
            full_path = full_grid_dir/f"{report['area']}.full.json"
            # Detail files are sampling evidence, not a second output verdict.
            # Their write precedes final compact publication and therefore cannot
            # certify that publication, or embed a hash of themselves.
            detail = {key:value for key,value in report.items() if key not in {"status","guard_status","checks","artifacts"}}
            detail.update(report_format="full",sampling_checks=report.get("checks",{}),
                          authoritative_compact_report=str(json_path.resolve()),
                          report_format_note="Sample evidence only. The companion compact report is authoritative for guard status and artifact-output completeness; this file has no overall status or self hash.")
            full_path.write_text(json.dumps(detail,indent=1,ensure_ascii=False)+"\n")
            report["artifacts"].update(full_grid_written=True,full_grid_path=str(full_path),
                                       full_grid_sha256=sha(full_path.read_bytes()))
        except (OSError, TypeError, ValueError) as exc:
            problems.append("Full grid output: "+str(exc))
    report.setdefault("checks",{})["output_completeness"] = {"status":"UNVERIFIED" if problems else "PASS","errors":problems}
    report["status"] = report_status(report)
    report["guard_status"] = report["status"]
    try:
        json_path.write_text(json.dumps(compact_report(report),indent=1,ensure_ascii=False)+"\n")
        report["artifacts"]["json_written"] = True
    except (OSError, TypeError, ValueError) as exc:
        report["artifacts"]["json_written"] = False
        report["checks"]["output_completeness"] = {"status":"UNVERIFIED","errors":problems+["Final JSON output: "+str(exc)]}
        report["status"] = report_status(report)
        report["guard_status"] = report["status"]


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
    expect(report_status({**fixture("PASS"),"checks":{"interactions":{"status":"UNVERIFIED","guard_status":"PASS"}}}) == "PASS",
           "Python explicit expected coverage gap can pass the guard")
    expect(report_status({**fixture("PASS"),"checks":{"interactions":{"status":"FAIL","guard_status":"PASS"}}}) == "FAIL",
           "Python coverage exception cannot hide measured FAIL")
    expect(report_status({**fixture("PASS"),"checks":{"interactions":{"status":"UNVERIFIED"}}}) == "UNVERIFIED",
           "Python unknown interaction coverage without exception still blocks")
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
            expect(json.loads((out/"fixture.json").read_text())["status"] == expected,
                   "Python surviving draft after final JSON failure never claims PASS with " + measured)
        partial = deepcopy(fixture("PASS", completed=False))
        with patch(__name__ + ".draw_map", side_effect=lambda r, dest: dest.write_bytes(b"fixture only")):
            write_artifacts(partial, out)
        expect(partial["status"] == "UNVERIFIED", "Python successful outputs do not verify an incomplete run")
        full_fixture = fixture("PASS")
        full_fixture["grid"] = {"valid_indices":[0,1,2],"reached_indices":[0,1],"parents":[-1,0],
                                "unreachable_components":[{"indices":[2],"count":1,"example":{"x":.3,"z":0}}]}
        seen_full = False

        def full_map_only(report: dict, dest: Path):
            nonlocal seen_full
            seen_full = report["grid"]["valid_indices"] == [0,1,2]
            dest.write_bytes(b"fixture only")

        with patch(__name__ + ".draw_map", side_effect=full_map_only):
            write_artifacts(full_fixture, out)
        compact = json.loads((out/"fixture.json").read_text())
        expect(seen_full and "valid_indices" not in compact["grid"] and "parents" not in compact["grid"]
               and "indices" not in compact["grid"]["unreachable_components"][0]
               and full_fixture["grid"]["valid_indices"] == [0,1,2],
               "Python map sees full grid while default JSON omits all bulk arrays without mutating evidence")
        detail_dir = out/"external-detail"
        full_fixture = deepcopy(full_fixture)
        with patch(__name__ + ".draw_map", side_effect=full_map_only):
            write_artifacts(full_fixture, out, detail_dir)
        compact = json.loads((out/"fixture.json").read_text())
        detail = json.loads((detail_dir/"fixture.full.json").read_text())
        expect("valid_indices" not in compact["grid"] and detail["grid"]["valid_indices"] == [0,1,2]
               and full_fixture["artifacts"]["full_grid_written"]
               and full_fixture["artifacts"]["full_grid_sha256"] == sha((detail_dir/"fixture.full.json").read_bytes()),
               "Python optional external full-grid file preserves details and ordinary JSON stays compact")
        full_fixture = fixture("PASS")
        with patch(__name__ + ".draw_map", side_effect=OSError("fixture map write failure")):
            write_artifacts(full_fixture, out, detail_dir)
        detail = json.loads((detail_dir/"fixture.full.json").read_text())
        expect(full_fixture["status"] == "UNVERIFIED" and "status" not in detail and "guard_status" not in detail
               and "artifacts" not in detail and "authoritative_compact_report" in detail,
               "Python full sampling evidence never claims a green overall artifact verdict after map failure")
        for rejected in (WEB/"production", WEB/"production/reachcheck/detail"):
            try:
                validate_full_grid_dir(rejected)
            except ValueError:
                expect(True, "Python rejects full-grid directory in production: " + str(rejected.relative_to(WEB)))
            else:
                expect(False, "full-grid production directory was accepted")
        alias = out/"production-alias"
        alias.symlink_to(WEB/"production", target_is_directory=True)
        try:
            validate_full_grid_dir(alias/"detail")
        except ValueError:
            expect(True, "Python rejects symlink alias into production")
        else:
            expect(False, "full-grid symlink production alias was accepted")
        full_fixture = fixture("FAIL")
        # The rejected directory must not erase measured failures or claim it
        # wrote a detail artifact. This fake map needs no game grid.
        with patch(__name__ + ".draw_map", side_effect=lambda r,d:d.write_bytes(b"fixture only")):
            write_artifacts(full_fixture,out,WEB/"production/reachcheck")
        expect(full_fixture["status"] == "FAIL" and not full_fixture["artifacts"]["full_grid_written"],
               "Python rejected detail destination retains measured FAIL and never claims a file")
        with patch.object(Path,"write_text",side_effect=OSError("fixture summary write failure")):
            expect(guard_exit_code(write_summary({"status":"PASS"},out,"PASS")) == 2,
                   "Python pure summary output failure returns UNVERIFIED exit 2")
            expect(guard_exit_code(write_summary({"status":"FAIL"},out,"FAIL")) == 1,
                   "Python summary output failure preserves measured FAIL exit 1")
        with patch.object(Path,"mkdir",side_effect=OSError("fixture output directory failure")):
            expect(not ensure_output_dir(out),"Python output directory failure is caught before browser execution")
    print("PYTHON STATUS SELF-TEST PASS (fixtures only)", flush=True)


async def check(args: argparse.Namespace) -> int:
    if not ensure_output_dir(args.out_dir):
        return 2
    metadata = {"generated_utc":datetime.now(timezone.utc).isoformat(),"script_sha256":sha(Path(__file__).read_bytes()),
                "git_base":git_commit(),"source_url":args.url,"source_html_sha256":None,
                "browser":"Playwright Chromium, ANGLE swiftshader", "requested_areas":args.areas,
                "status_semantics":"Regression guard; interaction coverage is reported separately and can remain UNVERIFIED.",
                "report_format":"compact", "full_grid_requested":args.full_grid,
                "full_grid_dir":str(args.full_grid_dir) if args.full_grid else None}
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
                write_artifacts(report,args.out_dir,args.full_grid_dir if args.full_grid else None)
                print(f"{area}: guard {report['status']} ({report.get('grid',{}).get('reached_count',0)}/{report.get('grid',{}).get('valid_count',0)} reached; interaction coverage {report.get('interaction_coverage',{}).get('status','UNVERIFIED')})",flush=True)
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
            write_artifacts(report,args.out_dir,args.full_grid_dir if args.full_grid else None)
        print(aggregate([r["status"] for r in reports])+" "+str(exc),file=sys.stderr,flush=True)
    status=aggregate([r["status"] for r in reports])
    summary={**metadata,"status":status,"elapsed_seconds":time.monotonic()-started,
             "areas":[{"area":r["area"],"status":r["status"],"guard_status":r["status"],"completed":r["completed"],
                       "interaction_coverage_status":r.get("interaction_coverage",{}).get("status","UNVERIFIED"),
                       "report":f"{r['area']}.json" if r["artifacts"]["json_written"] else None,
                       "report_sha256":sha((args.out_dir/f"{r['area']}.json").read_bytes()) if r["artifacts"]["json_written"] else None,
                       "map":f"{r['area']}.png" if r["artifacts"]["map_written"] else None,
                       "map_sha256":r["artifacts"]["map_sha256"],
                       "full_grid":r["artifacts"]["full_grid_path"] if r["artifacts"]["full_grid_written"] else None,
                       "full_grid_sha256":r["artifacts"]["full_grid_sha256"],"findings":len(r.get('findings',[]))} for r in reports],
             "limitations":["PASS is the regression guard; expected unbound/inactive interactions retain UNVERIFIED coverage.",
                            "Checker uses finite samples, not a continuous-space proof.","Swiftshader does not verify hardware fps."]}
    return guard_exit_code(write_summary(summary,args.out_dir,status))


def main() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url",default=os.environ.get("S47_URL") or (WEB/"dist-single/index.html").as_uri())
    parser.add_argument("--out-dir",type=Path,default=WEB/"production/reachcheck")
    parser.add_argument("--full-grid",action="store_true",help="Write a separate detailed sampled-grid JSON outside web/production; ordinary reports stay compact")
    parser.add_argument("--full-grid-dir",type=Path,default=None,help="External detail directory, used only with --full-grid (default: temporary s47-reachcheck-full directory)")
    parser.add_argument("--areas",nargs="+",choices=AREAS,default=list(AREAS))
    parser.add_argument("--spacing",type=float,default=.15,help="Required full-check spacing is 0.15 m; overrides are UNVERIFIED unless a measured FAIL takes precedence")
    parser.add_argument("--timeout",type=float,default=540,help="Total browser/check budget in seconds (default 9 minutes)")
    parser.add_argument("--max-cells",type=int,default=3000000,help="Reject, never truncate, a grid above this safety limit")
    parser.add_argument("--self-test",action="store_true",help="Adversarial checker fixtures only; no real-game execution")
    args=parser.parse_args()
    if not math.isfinite(args.spacing) or not math.isfinite(args.timeout) or args.spacing<=0 or args.timeout<=0 or args.max_cells<1:
        parser.error("spacing, timeout and max-cells must be positive")
    if args.full_grid_dir is not None and not args.full_grid:
        parser.error("--full-grid-dir requires --full-grid")
    if args.full_grid:
        try:
            args.full_grid_dir=validate_full_grid_dir(args.full_grid_dir or Path(gettempdir())/"s47-reachcheck-full")
        except ValueError as exc:
            parser.error(str(exc))
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
