// Runs inside the web game (dist-single/index.html?dev) and takes the whole scene apart into
// plain data the Unreal importer can rebuild: the node tree, one entry per geometry (written as
// glb by the caller), every material and texture, the real lights, the fake floodlights, the glow
// points, the lines, the interactable things, the walk zones and colliders, and named handles
// for the things the story moves (the lever, the mug, the dishes). Nothing in the game changes.
// The caller (export.py) asks for the pieces one at a time so no single message is huge.
(() => {
  const S = window.S47, T = S.three;
  const X = (window.S47X = {});

  // ---------- the node tree ----------
  const nodes = [], byObj = new Map();
  const geos = [], geoIndex = new Map();
  const mats = [], matIndex = new Map();
  // floodlit() can sit under other wrappers (desertGround, wet), so its uniforms are looked for
  // by running the material's onBeforeCompile on a dummy shader
  const floodOf = (m) => {
    if (!m.onBeforeCompile || m.onBeforeCompile === T.Material.prototype.onBeforeCompile) return false;
    const sh = { uniforms: {}, vertexShader: '', fragmentShader: '', defines: {} };
    try { m.onBeforeCompile(sh, S.renderer); } catch (e) { return false; }
    return !!sh.uniforms.uFloodPos;
  };
  const texs = [], texIndex = new Map();
  const skipped = { shader: 0, points: 0, lines: 0, sprites: 0 };

  const m4 = (m) => Array.from(m.elements);
  const lin = (c) => (c ? [c.r, c.g, c.b] : [1, 1, 1]);

  function texId(t) {
    if (!t || !t.isTexture) return -1;
    if (texIndex.has(t.uuid)) return texIndex.get(t.uuid);
    t.updateMatrix && t.updateMatrix();
    const id = texs.length;
    texIndex.set(t.uuid, id);
    texs.push({ id, t, name: t.name || '', srgb: t.colorSpace === T.SRGBColorSpace, flipY: !!t.flipY,
      wrapS: t.wrapS === T.RepeatWrapping ? 'repeat' : t.wrapS === T.MirroredRepeatWrapping ? 'mirror' : 'clamp',
      wrapT: t.wrapT === T.RepeatWrapping ? 'repeat' : t.wrapT === T.MirroredRepeatWrapping ? 'mirror' : 'clamp',
      nearest: t.magFilter === T.NearestFilter, canvas: !!(t.isCanvasTexture || (t.image && t.image.getContext)),
      w: t.image ? (t.image.width || (t.image.data && t.image.width) || 0) : 0, h: t.image ? (t.image.height || 0) : 0 });
    return id;
  }
  const uvRows = (t) => { if (!t || !t.matrix) return [1, 0, 0, 0, 1, 0]; const e = t.matrix.elements; return [e[0], e[3], e[6], e[1], e[4], e[7]]; };

  function matId(m) {
    if (!m) return -1;
    if (matIndex.has(m.uuid)) return matIndex.get(m.uuid);
    const id = mats.length;
    matIndex.set(m.uuid, id);
    const d = { id, type: m.type, name: m.name || '', color: lin(m.color), opacity: m.opacity ?? 1, transparent: !!m.transparent,
      alphaTest: m.alphaTest || 0, side: m.side === T.DoubleSide ? 'double' : m.side === T.BackSide ? 'back' : 'front',
      vertexColors: !!m.vertexColors, toneMapped: m.toneMapped !== false, fog: m.fog !== false, depthWrite: m.depthWrite !== false,
      additive: m.blending === T.AdditiveBlending, flood: floodOf(m),
      visible: m.visible !== false };
    if (m.isMeshStandardMaterial || m.isMeshPhysicalMaterial || m.isMeshLambertMaterial || m.isMeshPhongMaterial) {
      d.lit = true;
      d.roughness = m.roughness ?? 1; d.metalness = m.metalness ?? 0;
      d.emissive = lin(m.emissive); d.emissiveIntensity = m.emissiveIntensity ?? 1;
    } else d.lit = false;
    for (const k of ['map', 'emissiveMap', 'alphaMap', 'normalMap', 'roughnessMap', 'metalnessMap', 'aoMap', 'bumpMap']) {
      if (m[k]) { d[k] = texId(m[k]); d[k + 'UV'] = uvRows(m[k]); }
    }
    if (m.normalScale) d.normalScale = [m.normalScale.x, m.normalScale.y];
    if (m.isShaderMaterial) {
      // keep what can be told about it: its colour-like uniforms and any texture
      d.shader = true; d.uniforms = {};
      for (const [k, u] of Object.entries(m.uniforms || {})) {
        const v = u && u.value;
        if (v && v.isColor) d.uniforms[k] = lin(v);
        else if (typeof v === 'number') d.uniforms[k] = v;
        else if (v && v.isVector3) d.uniforms[k] = [v.x, v.y, v.z];
        else if (v && v.isTexture) d.uniforms[k] = { tex: texId(v) };
      }
    }
    mats.push(d);
    return id;
  }

  function geoId(g) {
    if (geoIndex.has(g.uuid)) return geoIndex.get(g.uuid);
    const id = geos.length;
    geoIndex.set(g.uuid, id);
    geos.push(g);
    return id;
  }

  function walk(o, parent) {
    if (o.isLight || o.isCamera) return; // lights go to their own list
    const i = nodes.length;
    const n = { i, p: parent, name: o.name || '', type: o.type, m: m4(o.matrix), vis: o.visible !== false };
    byObj.set(o, i);
    nodes.push(n);
    if (o.isMesh && !o.isSkinnedMesh) {
      const ms = [].concat(o.material);
      if (ms.some((x) => x && x.isShaderMaterial && !x.isRawShaderMaterial && x.type === 'ShaderMaterial')) skipped.shader++;
      n.geo = geoId(o.geometry);
      n.mats = ms.map(matId);
      n.groups = (o.geometry.groups || []).map((g) => g.materialIndex ?? 0);
      if (o.isInstancedMesh) {
        const a = o.instanceMatrix.array, out = [];
        for (let k = 0; k < o.count; k++) out.push(Array.from(a.slice(k * 16, k * 16 + 16)));
        n.inst = out;
        if (o.instanceColor) n.instColor = Array.from(o.instanceColor.array.slice(0, o.count * 3));
      }
      n.castShadow = !!o.castShadow;
    } else if (o.isPoints) {
      skipped.points++;
      const p = o.geometry.attributes.position, c = o.geometry.attributes.aCol || o.geometry.attributes.color, s = o.geometry.attributes.aSize;
      const pts = [];
      for (let k = 0; k < p.count; k++) pts.push([p.getX(k), p.getY(k), p.getZ(k), c ? c.getX(k) : 1, c ? c.getY(k) : 1, c ? c.getZ(k) : 1, s ? s.getX(k) : (o.material.size || 1)]);
      n.points = pts;
      n.pointMat = matId(o.material);
    } else if (o.isLine) {
      skipped.lines++;
      // Each segment as two thin quads crossed along it (1.2 cm wide), so it reads as a wire from any side.
      const p = o.geometry.attributes.position, step = o.isLineSegments ? 2 : 1, W = 0.006;
      const pos = [], idx = [];
      const a = new T.Vector3(), b = new T.Vector3(), d = new T.Vector3(), u = new T.Vector3(), v = new T.Vector3(), up = new T.Vector3(0, 1, 0), side = new T.Vector3(1, 0, 0);
      for (let k = 0; k + 1 < p.count; k += step) {
        a.set(p.getX(k), p.getY(k), p.getZ(k)); b.set(p.getX(k + 1), p.getY(k + 1), p.getZ(k + 1));
        d.subVectors(b, a); if (d.lengthSq() < 1e-12) continue; d.normalize();
        u.crossVectors(d, Math.abs(d.y) > 0.9 ? side : up).normalize().multiplyScalar(W);
        v.crossVectors(d, u).normalize().multiplyScalar(W);
        for (const w of [u, v]) {
          const base = pos.length / 3;
          pos.push(a.x - w.x, a.y - w.y, a.z - w.z, a.x + w.x, a.y + w.y, a.z + w.z, b.x + w.x, b.y + w.y, b.z + w.z, b.x - w.x, b.y - w.y, b.z - w.z);
          idx.push(base, base + 1, base + 2, base, base + 2, base + 3);
        }
      }
      const g = new T.BufferGeometry();
      g.setAttribute('position', new T.Float32BufferAttribute(pos, 3));
      g.setIndex(idx);
      g.computeVertexNormals();
      n.geo = geoId(g);
      n.mats = [matId(o.material)];
      n.groups = [];
      n.wire = true;
    } else if (o.isSprite) {
      skipped.sprites++;
      n.sprite = { mat: matId(o.material) };
    }
    for (const c of o.children) walk(c, i);
  }

  // ---------- collect ----------
  X.collect = () => {
    S.scene.updateMatrixWorld(true);
    for (const c of S.scene.children) walk(c, -1);
    const lights = [];
    S.scene.traverse((o) => {
      if (!o.isLight) return;
      const w = new T.Vector3(); o.getWorldPosition(w);
      const L = { uuid: o.uuid, type: o.type, name: o.name || '', color: lin(o.color), intensity: o.intensity, pos: [w.x, w.y, w.z], vis: o.visible, castShadow: !!o.castShadow,
        parent: o.parent ? (byObj.get(o.parent) ?? -1) : -1, local: m4(o.matrix) };
      if (o.isPointLight || o.isSpotLight) { L.distance = o.distance; L.decay = o.decay; }
      if (o.isSpotLight) { L.angle = o.angle; L.penumbra = o.penumbra; }
      if (o.isSpotLight || o.isDirectionalLight) { const t = new T.Vector3(); o.target.getWorldPosition(t); L.target = [t.x, t.y, t.z]; }
      if (o.isHemisphereLight) L.ground = lin(o.groundColor);
      lights.push(L);
    });
    return { nodes: nodes.length, geos: geos.length, mats: mats.length, texs: texs.length, lights: lights.length, skipped, lightsList: lights };
  };

  // ---------- the node list, the materials, the textures' metadata ----------
  X.nodes = (from, to) => nodes.slice(from, to);
  X.materials = () => mats;
  X.textureMeta = () => texs.map(({ t, ...rest }) => rest);

  // A texture as a PNG data URL, flipped when three flips it, so the importer can use the raw UVs.
  X.texture = (id) => {
    const t = texs[id].t, img = t.image;
    if (!img) return null;
    let w = img.width || 0, h = img.height || 0;
    const cv = document.createElement('canvas');
    if (img.data && !(img instanceof HTMLImageElement) && !(img.getContext)) {
      // a DataTexture: RGBA or RGB or single channel, bytes or floats
      w = img.width; h = img.height;
      cv.width = w; cv.height = h;
      const ctx = cv.getContext('2d'), out = ctx.createImageData(w, h), src = img.data;
      const ch = Math.round(src.length / (w * h)) || 4, isF = !(src instanceof Uint8Array || src instanceof Uint8ClampedArray);
      for (let k = 0; k < w * h; k++) {
        for (let c = 0; c < 4; c++) {
          let v = c < ch ? src[k * ch + c] : (c === 3 ? (isF ? 1 : 255) : src[k * ch]);
          if (isF) v = Math.max(0, Math.min(255, v * 255));
          out.data[k * 4 + c] = v;
        }
      }
      ctx.putImageData(out, 0, 0);
    } else {
      if (!w || !h) return null;
      cv.width = w; cv.height = h;
      cv.getContext('2d').drawImage(img, 0, 0, w, h);
    }
    if (t.flipY) {
      const f = document.createElement('canvas'); f.width = w; f.height = h;
      const fx = f.getContext('2d'); fx.translate(0, h); fx.scale(1, -1); fx.drawImage(cv, 0, 0);
      return f.toDataURL('image/png');
    }
    return cv.toDataURL('image/png');
  };

  // ---------- geometry as glb, a batch of geometries per file ----------
  // Each geometry becomes one glTF mesh "g<id>" on its own node at the origin, one primitive per
  // material group, with placeholder materials "slot<k>" so the importer makes the slots in order.
  X.glb = (from, to) => {
    const bin = [], views = [], accs = [], meshes = [], gnodes = [];
    let off = 0;
    const push = (arr, target) => {
      const bytes = new Uint8Array(arr.buffer, arr.byteOffset, arr.byteLength);
      const pad = (4 - (bytes.length % 4)) % 4;
      bin.push(bytes); if (pad) bin.push(new Uint8Array(pad));
      views.push({ buffer: 0, byteOffset: off, byteLength: bytes.length, ...(target ? { target } : {}) });
      off += bytes.length + pad;
      return views.length - 1;
    };
    const acc = (arr, type, comp, count, minmax) => {
      const v = push(arr, comp === 5125 ? 34963 : 34962);
      const a = { bufferView: v, componentType: comp, count, type };
      if (minmax) { a.min = minmax[0]; a.max = minmax[1]; }
      accs.push(a); return accs.length - 1;
    };
    const slots = 8;
    for (let gi = from; gi < Math.min(to, geos.length); gi++) {
      const g = geos[gi], P = g.attributes.position;
      if (!P || P.count === 0) { meshes.push(null); continue; }
      const n = P.count, pos = new Float32Array(n * 3);
      const mn = [Infinity, Infinity, Infinity], mx = [-Infinity, -Infinity, -Infinity];
      for (let k = 0; k < n; k++) {
        const x = P.getX(k), y = P.getY(k), z = P.getZ(k);
        pos[k * 3] = x; pos[k * 3 + 1] = y; pos[k * 3 + 2] = z;
        mn[0] = Math.min(mn[0], x); mn[1] = Math.min(mn[1], y); mn[2] = Math.min(mn[2], z);
        mx[0] = Math.max(mx[0], x); mx[1] = Math.max(mx[1], y); mx[2] = Math.max(mx[2], z);
      }
      const attrs = { POSITION: acc(pos, 'VEC3', 5126, n, [mn, mx]) };
      let N = g.attributes.normal;
      if (!N) { const c = g.clone(); c.computeVertexNormals(); N = c.attributes.normal; }
      const nor = new Float32Array(n * 3);
      for (let k = 0; k < n; k++) { let x = N.getX(k), y = N.getY(k), z = N.getZ(k); const l = Math.hypot(x, y, z) || 1; nor[k * 3] = x / l; nor[k * 3 + 1] = y / l; nor[k * 3 + 2] = z / l; }
      attrs.NORMAL = acc(nor, 'VEC3', 5126, n);
      for (const [name, key] of [['uv', 'TEXCOORD_0'], ['uv1', 'TEXCOORD_1']]) {
        const U = g.attributes[name]; if (!U) continue;
        const uv = new Float32Array(n * 2);
        for (let k = 0; k < n; k++) { uv[k * 2] = U.getX(k); uv[k * 2 + 1] = U.getY(k); }
        attrs[key] = acc(uv, 'VEC2', 5126, n);
      }
      if (!attrs.TEXCOORD_0) attrs.TEXCOORD_0 = acc(new Float32Array(n * 2), 'VEC2', 5126, n);
      const C = g.attributes.color;
      if (C) {
        const col = new Float32Array(n * 4);
        for (let k = 0; k < n; k++) { col[k * 4] = C.getX(k); col[k * 4 + 1] = C.getY(k); col[k * 4 + 2] = C.getZ(k); col[k * 4 + 3] = C.itemSize > 3 ? C.getW(k) : 1; }
        attrs.COLOR_0 = acc(col, 'VEC4', 5126, n);
      }
      let idx;
      if (g.index) { idx = new Uint32Array(g.index.count); for (let k = 0; k < g.index.count; k++) idx[k] = g.index.getX(k); }
      else { idx = new Uint32Array(n); for (let k = 0; k < n; k++) idx[k] = k; }
      const groups = g.groups && g.groups.length ? g.groups : [{ start: 0, count: idx.length, materialIndex: 0 }];
      const prims = [];
      for (const gr of groups) {
        const s = gr.start, c = Math.min(gr.count === Infinity ? idx.length - s : gr.count, idx.length - s);
        if (c <= 0) continue;
        const part = idx.slice(s, s + c);
        prims.push({ attributes: attrs, indices: acc(part, 'SCALAR', 5125, part.length), material: Math.min(gr.materialIndex ?? 0, slots - 1), mode: 4 });
      }
      meshes.push({ name: 'g' + gi, primitives: prims });
    }
    const gmeshes = [];
    for (let k = 0; k < meshes.length; k++) {
      if (!meshes[k]) continue;
      gmeshes.push(meshes[k]);
      gnodes.push({ name: meshes[k].name, mesh: gmeshes.length - 1 });
    }
    const json = {
      asset: { version: '2.0', generator: 'SIGNAL-47 web export' },
      scene: 0, scenes: [{ nodes: gnodes.map((_, k) => k) }], nodes: gnodes, meshes: gmeshes,
      materials: Array.from({ length: slots }, (_, k) => ({ name: 'slot' + k, pbrMetallicRoughness: { baseColorFactor: [1, 1, 1, 1], metallicFactor: 0, roughnessFactor: 1 } })),
      accessors: accs, bufferViews: views, buffers: [{ byteLength: off }],
    };
    let js = new TextEncoder().encode(JSON.stringify(json));
    const jpad = (4 - (js.length % 4)) % 4;
    const total = 12 + 8 + js.length + jpad + 8 + off;
    const out = new Uint8Array(total), dv = new DataView(out.buffer);
    dv.setUint32(0, 0x46546c67, true); dv.setUint32(4, 2, true); dv.setUint32(8, total, true);
    dv.setUint32(12, js.length + jpad, true); dv.setUint32(16, 0x4e4f534a, true);
    out.set(js, 20); for (let k = 0; k < jpad; k++) out[20 + js.length + k] = 0x20;
    let p = 20 + js.length + jpad;
    dv.setUint32(p, off, true); dv.setUint32(p + 4, 0x004e4942, true); p += 8;
    for (const b of bin) { out.set(b, p); p += b.length; }
    // base64 in chunks (btoa on huge strings is slow)
    let s = '';
    for (let k = 0; k < out.length; k += 0x8000) s += String.fromCharCode.apply(null, out.subarray(k, k + 0x8000));
    return btoa(s);
  };
  X.geoCount = () => geos.length;

  // The sky (stars, moon, the band at the horizon) and the painted horizon as one equirectangular
  // picture, from the eye at `at`, rendered by the game's own renderer with only those two groups
  // shown. Six 90 degree views into a float target, then resampled. Returns a PNG (sRGB).
  X.sky = (at, W = 2048) => {
    const r = S.renderer, sc = S.scene;
    const keep = new Set([S.sky && S.sky.group, S.backdrop && S.backdrop.group].filter(Boolean));
    const vis = sc.children.map((c) => c.visible);
    sc.children.forEach((c) => { c.visible = keep.has(c); });
    const bg = sc.background, fog = sc.fog;
    sc.fog = null;
    // a cube face as fine as the picture (the stars are points of a few pixels, as on screen)
    const F = Math.round(W / 4), rt = new T.WebGLRenderTarget(F, F, { type: T.FloatType });
    const cam = new T.PerspectiveCamera(90, 1, 0.1, 200000);
    cam.position.set(at[0], at[1], at[2]);
    const dirs = [[1, 0, 0, 0, -1, 0], [-1, 0, 0, 0, -1, 0], [0, 1, 0, 0, 0, 1], [0, -1, 0, 0, 0, -1], [0, 0, 1, 0, -1, 0], [0, 0, -1, 0, -1, 0]];
    const faces = [];
    const prevTM = r.toneMapping; r.toneMapping = T.NoToneMapping;
    for (const d of dirs) {
      cam.up.set(d[3], d[4], d[5]);
      cam.lookAt(at[0] + d[0], at[1] + d[1], at[2] + d[2]);
      cam.updateMatrixWorld();
      r.setRenderTarget(rt); r.clear(); r.render(sc, cam);
      const px = new Float32Array(F * F * 4); r.readRenderTargetPixels(rt, 0, 0, F, F, px);
      faces.push({ px, cam: cam.clone() });
    }
    r.setRenderTarget(null); r.toneMapping = prevTM;
    sc.children.forEach((c, k) => { c.visible = vis[k]; });
    sc.fog = fog; sc.background = bg;
    rt.dispose();
    const H = W / 2, cv = document.createElement('canvas'); cv.width = W; cv.height = H;
    const ctx = cv.getContext('2d'), img = ctx.createImageData(W, H);
    const v = new T.Vector3();
    const enc = (x) => { x = Math.max(0, x); const s = x <= 0.0031308 ? 12.92 * x : 1.055 * Math.pow(x, 1 / 2.4) - 0.055; return Math.min(255, Math.round(s * 255)); };
    for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
      // longitude 0 at -Z (north), increasing to the east (+X); latitude +90 at the top
      const lon = (x + 0.5) / W * Math.PI * 2, lat = Math.PI / 2 - (y + 0.5) / H * Math.PI;
      v.set(Math.sin(lon) * Math.cos(lat), Math.sin(lat), -Math.cos(lon) * Math.cos(lat));
      let best = 0, bd = -2;
      for (let k = 0; k < 6; k++) { const dd = dirs[k]; const dot = v.x * dd[0] + v.y * dd[1] + v.z * dd[2]; if (dot > bd) { bd = dot; best = k; } }
      const f = faces[best], p = v.clone().add(f.cam.position).project(f.cam);
      const fx = Math.min(F - 1, Math.max(0, Math.floor((p.x * 0.5 + 0.5) * F))), fy = Math.min(F - 1, Math.max(0, Math.floor((p.y * 0.5 + 0.5) * F)));
      const o = (fy * F + fx) * 4, q = (y * W + x) * 4;
      img.data[q] = enc(f.px[o]); img.data[q + 1] = enc(f.px[o + 1]); img.data[q + 2] = enc(f.px[o + 2]); img.data[q + 3] = 255;
    }
    ctx.putImageData(img, 0, 0);
    return cv.toDataURL('image/png');
  };

  // ---------- the things the story needs by name ----------
  X.extras = () => {
    const idx = (o) => (o && byObj.has(o) ? byObj.get(o) : -1);
    const handles = {};
    const owners = { room: S.room, ext: S.ext, yard: S.yard, annex: S.annex, sky: S.sky, backdrop: S.backdrop, fcam: S.fcam };
    const w = S.world;
    for (const k of ['site', 'road', 'diner', 'town', 'room6', 'oldRoad', 'truck', 'saroTruck', 'parked', 'oldTruck', 'motel', 'stationTruck'])
      if (w && w[k]) owners['world.' + k] = w[k];
    const seen = new Set();
    const scan = (prefix, obj, depth) => {
      if (!obj || depth > 2 || seen.has(obj)) return;
      seen.add(obj);
      for (const [k, v] of Object.entries(obj)) {
        if (!v || typeof v !== 'object') continue;
        if (v.isObject3D) { const i = idx(v); if (i >= 0) handles[prefix + '.' + k] = { node: i }; }
        else if (v.isTexture) handles[prefix + '.' + k] = { tex: texIndex.has(v.uuid) ? texIndex.get(v.uuid) : -1 };
        else if (v.isMaterial) handles[prefix + '.' + k] = { mat: matIndex.has(v.uuid) ? matIndex.get(v.uuid) : -1 };
        else if (Array.isArray(v)) v.forEach((e, n) => { if (e && e.isObject3D) { const i = idx(e); if (i >= 0) handles[`${prefix}.${k}[${n}]`] = { node: i }; } else if (e && e.isLight) handles[`${prefix}.${k}[${n}]`] = { light: e.uuid }; });
        else if (!(v instanceof Element) && depth < 2) scan(prefix + '.' + k, v, depth + 1);
      }
    };
    for (const [p, o] of Object.entries(owners)) scan(p, o, 0);
    // the CRT screens: their canvas textures, by name
    for (const k of ['crtLeft', 'crtCenter', 'crtRight', 'crtOps']) {
      const c = S.room && S.room[k];
      if (c && c.tex) handles['room.' + k + '.tex'] = { tex: texIndex.has(c.tex.uuid) ? texIndex.get(c.tex.uuid) : -1, mat: c.mat && matIndex.has(c.mat.uuid) ? matIndex.get(c.mat.uuid) : -1 };
    }
    // the dishes: their pointing, and which instanced meshes draw which level for which dish
    const dishes = (S.ext && S.ext.dishes || []).map((d) => ({ id: d.id, x: d.x, z: d.z, scale: d.scale, az: d.curAz, el: d.curEl, lod: d.lod }));
    const dishParts = [];
    const arr = S.ext && S.ext.dishArray;
    if (arr && arr.parts) for (const part of arr.parts) dishParts.push({ node: idx(part.mesh), level: part.level, dishes: part.dishes.map((d) => S.ext.dishes.indexOf(d)) });
    // the interactable things
    const inter = (S.game && S.game.d && S.game.d.inter) || null;
    const items = inter ? inter.items.map((it) => {
      const b = new T.Box3().setFromObject(it.object);
      return { id: it.id, node: idx(it.object), range: it.range ?? 2.4, look: !!it.look, min: b.isEmpty() ? null : [b.min.x, b.min.y, b.min.z], max: b.isEmpty() ? null : [b.max.x, b.max.y, b.max.z] };
    }) : [];
    // the walk zones and colliders per area
    const walks = {};
    const saro = w && w.d && w.d.saro;
    if (saro) walks.saro = { zones: saro.zones, colliders: saro.colliders };
    for (const [k, a] of [['station01', w && w.site], ['room6', w && w.room6], ['diner', w && w.diner]]) if (a && a.zones) walks[k] = { zones: a.zones, colliders: a.colliders };
    if (w && w.roadWalk) walks.road = { zones: w.roadWalk.zones, colliders: w.roadWalk.colliders };
    // which top-level nodes belong to which area
    const areas = {};
    const add = (k, o) => { const i = idx(o); if (i >= 0) (areas[k] = areas[k] || []).push(i); };
    if (saro) for (const g of saro.groups) add('saro', g);
    if (w) {
      if (w.site) add('station01', w.site.group); if (w.road) add('road', w.road.group); if (w.diner) add('diner', w.diner.group);
      if (w.town) add('diner', w.town.group); if (w.room6) add('room6', w.room6.group); if (w.oldRoad) add('roswell', w.oldRoad.group);
      if (w.truck) add('road', w.truck.group); if (w.saroTruck) add('saro', w.saroTruck.group); if (w.parked) add('station01', w.parked.group); if (w.oldTruck) add('roswell', w.oldTruck.group);
    }
    // the fake floodlights: their world positions (w = reach), colours and strength
    // floodlit() keeps its set in the material's onBeforeCompile; calling it on a dummy shader
    // hands over the set's arrays (positions with reach in w, colours, scale, falloff).
    const floods = [], setOf = new Map();
    for (const d of mats) d.floodSet = -1;
    for (const [uuid, id] of matIndex) {
      const d = mats[id];
      if (!d.flood) continue;
      let m = null;
      S.scene.traverse((o) => { if (!m && o.material) for (const x of [].concat(o.material)) if (x && x.uuid === uuid) m = x; });
      if (!m) continue;
      const sh = { uniforms: {}, vertexShader: '', fragmentShader: '', defines: {} };
      try { m.onBeforeCompile(sh, S.renderer); } catch (e) { continue; }
      const P = sh.uniforms.uFloodPos && sh.uniforms.uFloodPos.value, Cc = sh.uniforms.uFloodCol && sh.uniforms.uFloodCol.value;
      if (!P || !Cc) continue;
      if (!setOf.has(P)) {
        setOf.set(P, floods.length);
        floods.push({ scale: sh.uniforms.uFloodScale ? sh.uniforms.uFloodScale.value : 1, pools: P.map((v, k) => ({ x: v.x, y: v.y, z: v.z, r: v.w, col: lin(Cc[k]) })).filter((q) => q.r > 0) });
      }
      d.floodSet = setOf.get(P);
      d.floodFall = sh.uniforms.uFloodFall ? sh.uniforms.uFloodFall.value : 0.012;
    }
    // the doors: each one's leaf as it stands shut and open (Doors.ts set(k) moves it), the
    // doorway zones it opens and the collider its open leaf adds, by index in its area's lists
    const doors = [];
    const dm = S.doors && S.doors.doors;
    if (dm) for (const [id, door] of dm) {
      const sp = door.spec, before = new Map();
      const snap = () => { const out = new Map(); S.scene.traverse((o) => { if (byObj.has(o)) out.set(byObj.get(o), Array.from(o.matrix.elements)); }); return out; };
      sp.set(0); S.scene.updateMatrixWorld(true); const shut = snap();
      sp.set(1); S.scene.updateMatrixWorld(true); const open = snap();
      sp.set(door.k); S.scene.updateMatrixWorld(true);
      const moved = [];
      for (const [i, m] of shut) { const o = open.get(i); if (o && o.some((v, k) => Math.abs(v - m[k]) > 1e-6)) moved.push({ node: i, shut: m, open: o }); }
      let area = null, zones = [];
      for (const [a, wk] of Object.entries(walks)) { const z = sp.zones.map((q) => wk.zones.indexOf(q)).filter((k) => k >= 0); if (z.length) { area = a; zones = z; } }
      doors.push({ id, inter: sp.inter, name: sp.name, range: sp.range ?? 2.4, sound: sp.sound || 'heavy', open: door.open, proxy: idx(sp.proxy), moved, area, zones, leaf: sp.leaf || null });
    }
    // the player and the camera where the night starts
    const player = { x: S.player.pos.x, y: S.player.pos.y, z: S.player.pos.z, yaw: S.player.yaw, pitch: S.player.pitch, eye: S.player.eye, radius: S.player.radius, speed: S.player.speed };
    const cam = { fov: S.camera.fov, near: S.camera.near, far: S.camera.far };
    const fog = S.scene.fog ? { color: lin(S.scene.fog.color), density: S.scene.fog.density } : null;
    const bg = S.scene.background && S.scene.background.isColor ? lin(S.scene.background) : null;
    const L = S.room && S.room.lights;
    const roomLights = L ? { ceiling: L.ceiling.map((l) => l.uuid), lamps: L.lamps.map((l) => l.uuid), crt: L.crt.uuid, hemi: L.hemi.uuid, moon: L.moon.uuid } : null;
    const spawn = S.room && S.room.spawn ? { x: S.room.spawn.x, z: S.room.spawn.z, yaw: S.room.spawn.yaw } : null;
    return { roomLights, spawn, doors, handles, dishes, dishParts, items, walks, areas, floods, player, cam, fog, bg, toneMappingExposure: S.renderer.toneMappingExposure };
  };
})();
