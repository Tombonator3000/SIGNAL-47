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
      nearest: t.magFilter === T.NearestFilter, canvas: !!(t.isCanvasTexture || (t.image && t.image.getContext)), premultiplied: !!t.premultiplyAlpha,
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
    // the near mesas' (world/horizon.ts hazed): unfogged, but the dawn's haze on them (World.update)
    if (m.customProgramCacheKey && m.customProgramCacheKey() === 'horizon-haze') d.hazed = true;
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

  // world/flora.ts pairShape: a pair's mesh holds the second kind's corners in aPosB, aUvB, aColB and
  // aSwayB, which its shader takes for the copies past the split. Unreal draws that kind with a mesh of
  // its own: those corners as a geometry (the first kind's normals and triangles), made once per pair.
  const pairB = new Map();
  function pairGeo(g) {
    if (pairB.has(g.uuid)) return pairB.get(g.uuid);
    const b = new T.BufferGeometry();
    b.setAttribute('position', g.attributes.aPosB);
    b.setAttribute('normal', g.attributes.normal);
    b.setAttribute('uv', g.attributes.aUvB);
    b.setAttribute('color', g.attributes.aColB);
    b.setAttribute('aSway', g.attributes.aSwayB);
    b.setIndex(g.index);
    pairB.set(g.uuid, b);
    return b;
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
        // a pair of flora kinds in one mesh: the second kind's shape (pairGeo)
        if (o.geometry.attributes.aPosB) n.geoB = geoId(pairGeo(o.geometry));
      }
      // world/flora.ts: the ground cover's materials sway, shrink with distance and take the sky
      // less inside a plant (the C++ port's flora materials): which kind of mesh they are on
      if (o.name && o.name.startsWith('flora-')) for (const id of n.mats) if (id >= 0) mats[id].flora = o.name.slice(6);
      n.castShadow = !!o.castShadow;
    } else if (o.isPoints) {
      skipped.points++;
      const p = o.geometry.attributes.position, c = o.geometry.attributes.aCol || o.geometry.attributes.color, s = o.geometry.attributes.aSize;
      const pts = [];
      for (let k = 0; k < p.count; k++) pts.push([p.getX(k), p.getY(k), p.getZ(k), c ? c.getX(k) : 1, c ? c.getY(k) : 1, c ? c.getZ(k) : 1, s ? s.getX(k) : (o.material.size || 1)]);
      n.points = pts;
      n.pointMat = matId(o.material);
      // world/glow.ts: each point's blink (aBlink: rate, phase; rate 0 steady), and Sky.ts's stars'
      // twinkle (aPh), flat, a point after another
      const bl = o.geometry.attributes.aBlink, ph = o.geometry.attributes.aPh;
      if (bl) { const b = []; for (let k = 0; k < p.count; k++) b.push(bl.getX(k), bl.getY(k)); n.blink = b; }
      if (ph) { const q = []; for (let k = 0; k < p.count; k++) q.push(ph.getX(k)); n.ph = q; }
    } else if (o.isLine) {
      skipped.lines++;
      // Each segment as two thin quads crossed along it (1.2 cm wide), so it reads as a wire from any side.
      // (with the line's own colours, where it has them: the power lines' vertexColors)
      const p = o.geometry.attributes.position, C = o.geometry.attributes.color, step = o.isLineSegments ? 2 : 1, W = 0.006;
      const pos = [], idx = [], col = [];
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
          if (C) for (const j of [k, k, k + 1, k + 1]) col.push(C.getX(j), C.getY(j), C.getZ(j));
        }
      }
      const g = new T.BufferGeometry();
      g.setAttribute('position', new T.Float32BufferAttribute(pos, 3));
      if (C) g.setAttribute('color', new T.Float32BufferAttribute(col, 3));
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
    // a texture three uploads with its colours premultiplied by its alpha (the painted horizon's two
    // pictures, backdrop.ts): written so, so its smaller sizes average as three's do (a transparent
    // texel black, not white) and the shader reads it as three's does
    if (t.premultiplyAlpha) {
      const g = cv.getContext('2d'), d = g.getImageData(0, 0, w, h), px = d.data;
      for (let k = 0; k < px.length; k += 4) { const a = px[k + 3] / 255; px[k] = Math.round(px[k] * a); px[k + 1] = Math.round(px[k + 1] * a); px[k + 2] = Math.round(px[k + 2] * a); }
      g.putImageData(d, 0, 0);
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
      // (the shooting star's strip is laid out each frame it shows, Sky.meteorStep: all its corners are
      // at the origin until then, and a mesh of nothing but points is dropped on import. Its corners
      // go round the eye instead, 50 m out, so it is a mesh; its material puts them where they belong.)
      const strip = g.attributes.aT && g.attributes.aS;
      for (let k = 0; k < n; k++) {
        const x = strip ? (g.attributes.aT.getX(k) - 0.5) * 50 : P.getX(k), y = strip ? g.attributes.aS.getX(k) * 50 : P.getY(k), z = strip ? (g.attributes.aT.getX(k) - 0.5) * 50 : P.getZ(k);
        pos[k * 3] = x; pos[k * 3 + 1] = y; pos[k * 3 + 2] = z;
        mn[0] = Math.min(mn[0], x); mn[1] = Math.min(mn[1], y); mn[2] = Math.min(mn[2], z);
        mx[0] = Math.max(mx[0], x); mx[1] = Math.max(mx[1], y); mx[2] = Math.max(mx[2], z);
      }
      const attrs = { POSITION: acc(pos, 'VEC3', 5126, n, [mn, mx]) };
      let N = g.attributes.normal;
      if (!N) { const c = g.clone(); c.computeVertexNormals(); N = c.attributes.normal; }
      const nor = new Float32Array(n * 3);
      for (let k = 0; k < n; k++) {
        let x = N.getX(k), y = N.getY(k), z = N.getZ(k);
        // (a corner with no normal to speak of, as on the shooting star's strip: straight up)
        if (!(Math.hypot(x, y, z) > 1e-8)) { x = 0; y = 1; z = 0; }
        const l = Math.hypot(x, y, z); nor[k * 3] = x / l; nor[k * 3 + 1] = y / l; nor[k * 3 + 2] = z / l;
      }
      attrs.NORMAL = acc(nor, 'VEC3', 5126, n);
      for (const [name, key] of [['uv', 'TEXCOORD_0'], ['uv1', 'TEXCOORD_1']]) {
        const U = g.attributes[name]; if (!U) continue;
        const uv = new Float32Array(n * 2);
        for (let k = 0; k < n; k++) { uv[k * 2] = U.getX(k); uv[k * 2 + 1] = U.getY(k); }
        attrs[key] = acc(uv, 'VEC2', 5126, n);
      }
      // What the C++ port's own materials read off a corner, in the spare UV channels (the shaders'
      // attributes, which a glTF has no place for): a flora shape's sway and which of two plants it is
      // (world/flora.ts aSway, aPart: channel 1), the shooting star's strip (Sky.ts aT, aS), a cone's
      // place down it and its lamp (airlight.ts aV, aLamp), a haze layer (aLayer), a window's light
      // (windowLight.ts aUvq, aBlur: channels 0 to 2, with the corner's index for its colour).
      const two = (a, b, key) => {
        const A = g.attributes[a], B = b ? g.attributes[b] : null;
        if (!A || attrs[key]) return;
        const uv = new Float32Array(n * 2);
        for (let k = 0; k < n; k++) { uv[k * 2] = A.getX(k); uv[k * 2 + 1] = B ? B.getX(k) : 0; }
        attrs[key] = acc(uv, 'VEC2', 5126, n);
      };
      two('aSway', 'aPart', 'TEXCOORD_1');
      two('aT', 'aS', 'TEXCOORD_0');
      two('aV', 'aLamp', 'TEXCOORD_0');
      two('aLayer', null, 'TEXCOORD_0');
      if (g.attributes.aUvq && g.attributes.aBlur) {
        const Q = g.attributes.aUvq, Bl = g.attributes.aBlur, u0 = new Float32Array(n * 2), u1 = new Float32Array(n * 2), u2 = new Float32Array(n * 2);
        for (let k = 0; k < n; k++) {
          u0[k * 2] = Q.getX(k); u0[k * 2 + 1] = Q.getY(k);
          u1[k * 2] = Q.getZ(k); u1[k * 2 + 1] = Bl.getX(k);
          u2[k * 2] = Bl.getY(k); u2[k * 2 + 1] = k;
        }
        attrs.TEXCOORD_0 = acc(u0, 'VEC2', 5126, n); attrs.TEXCOORD_1 = acc(u1, 'VEC2', 5126, n); attrs.TEXCOORD_2 = acc(u2, 'VEC2', 5126, n);
      }
      if (!attrs.TEXCOORD_0) attrs.TEXCOORD_0 = acc(new Float32Array(n * 2), 'VEC2', 5126, n);
      const C = g.attributes.color || g.attributes.aCol;
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
    // Every scene object, material, texture and light the game's modules hold, by the path the
    // code reaches it with ("room.objs.mug", "world.site.hut.door", "game.ch1.yard..."), so the
    // C++ port can name the same things. Down to five levels; the first path found wins.
    // (the world before the game: the chapters hold the world's parts too, under longer paths)
    const owners = { room: S.room, ext: S.ext, yard: S.yard, annex: S.annex, sky: S.sky, backdrop: S.backdrop, fcam: S.fcam };
    const w = S.world;
    if (w) for (const [k, v] of Object.entries(w)) if (v && typeof v === 'object' && !(v instanceof Element) && k !== 'd') owners['world.' + k] = v;
    if (w && w.d) owners['world.d'] = w.d;
    owners.doors = S.doors; owners.game = S.game;
    const seen = new Set();
    const skip = (v) => v instanceof Element || v instanceof Node || ArrayBuffer.isView(v) || v instanceof ArrayBuffer || v.isBufferGeometry || v.isBufferAttribute
      || (typeof AudioNode !== 'undefined' && v instanceof AudioNode) || (typeof AudioContext !== 'undefined' && v instanceof AudioContext) || v instanceof Map || v instanceof Set || v.isCamera && !v.parent;
    const put = (path, v) => {
      if (handles[path]) return;
      if (v.isLight) { const i = idx(v); handles[path] = { light: v.uuid, ...(i >= 0 ? { node: i } : {}) }; }
      else if (v.isObject3D) { const i = idx(v); if (i >= 0) handles[path] = { node: i }; }
      else if (v.isTexture) handles[path] = { tex: texIndex.has(v.uuid) ? texIndex.get(v.uuid) : -1 };
      else if (v.isMaterial) handles[path] = { mat: matIndex.has(v.uuid) ? matIndex.get(v.uuid) : -1 };
      else if (v.isVector3) handles[path] = { vec: [v.x, v.y, v.z] };
      else if (isRect(v)) handles[path] = { rect: [v.minX, v.maxX, v.minZ, v.maxZ] };
      else if (isPose(v)) handles[path] = { pose: [v.x, v.z, v.yaw] };
    };
    // a place to stand: {x, z, yaw} (an area's anchors)
    const isPose = (v) => typeof v.x === 'number' && typeof v.z === 'number' && typeof v.yaw === 'number' && !v.isVector3;
    // a walk zone, a room's bounds: four numbers; an anchor: a Vector3 (zones and anchors the code reads)
    const isRect = (v) => typeof v.minX === 'number' && typeof v.maxX === 'number' && typeof v.minZ === 'number' && typeof v.maxZ === 'number';
    const scan = (prefix, obj, depth) => {
      if (!obj || depth > 5 || seen.has(obj)) return;
      seen.add(obj);
      let entries;
      try { entries = Object.entries(obj); } catch (e) { return; }
      for (const [k, v] of entries) {
        // (a module's d is what it was handed: the other modules, found under their own names)
        if (!v || typeof v !== 'object' || k === 'parent' || k === 'children' || k === 'd' && prefix !== 'world') continue;
        if (v.isObject3D || v.isTexture || v.isMaterial || v.isVector3 || isRect(v) || isPose(v)) { put(prefix + '.' + k, v); continue; }
        if (Array.isArray(v)) {
          v.slice(0, 400).forEach((e, n) => {
            if (!e || typeof e !== 'object') return;
            if (e.isObject3D || e.isTexture || e.isMaterial || e.isVector3 || isRect(e) || isPose(e)) put(`${prefix}.${k}[${n}]`, e);
            else if (!skip(e)) scan(`${prefix}.${k}[${n}]`, e, depth + 1);
          });
          continue;
        }
        if (skip(v)) continue;
        scan(prefix + '.' + k, v, depth + 1);
      }
    };
    for (const [p, o] of Object.entries(owners)) scan(p, o, 0);
    // world/archive047.ts keeps the switchboard's two line lamps and the call recorder's strip in its
    // closure (lamp(), setStrip()), out of the scan's reach: found under annex.arc.group by what they
    // are. The lamps are the two unlit boxes that are not tone mapped (trk1 the one at the face's left
    // as seen from the room, the room's -z side), the strip the mesh with the 64 x 512 canvas.
    const arc = S.annex && S.annex.arc;
    if (arc && arc.group) {
      const lamps = [];
      let strip = null;
      arc.group.traverse((o) => {
        if (!o.isMesh) return;
        const m = o.material;
        if (m && m.isMeshBasicMaterial && m.toneMapped === false && m.visible !== false) lamps.push(o);
        if (m && m.map && m.map.image && m.map.image.width === 64 && m.map.image.height === 512) strip = o;
      });
      const zOf = (o) => new T.Box3().setFromObject(o).getCenter(new T.Vector3()).z;
      lamps.sort((a, b) => zOf(a) - zOf(b));
      if (lamps[0]) put('annex.arc.lamps.trk1', lamps[0]);
      if (lamps[1]) put('annex.arc.lamps.trk2', lamps[1]);
      if (strip) { put('annex.arc.strip', strip); put('annex.arc.stripTex', strip.material.map); }
    }
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
        // every slot, the unused ones too: the code switches a lamp by its index (kit.ts setFlood)
        const set = (S.floodSets || []).find((f) => f.pos === P);
        floods.push({ key: set ? set.key : '', scale: sh.uniforms.uFloodScale ? sh.uniforms.uFloodScale.value : 1, pools: P.map((v, k) => ({ x: v.x, y: v.y, z: v.z, r: v.w, col: lin(Cc[k]) })),
          // the slots in use, and those never handed to a real light (they move: core/ultra.ts, windowLight.ts)
          count: set ? set.count : P.length, skip: set ? Array.from(set.skip) : [] });
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
    // the drives (drive/legs.ts, World.ts trips): what the C++ drive needs that only exists while
    // the game runs. Each leg's obstacles as the builders made them (posts, poles, rocks: circles
    // [x, z, r]; fences, walls, the diner's and the town's: boxes [minX, maxX, minZ, maxZ]; world
    // terms), its lots, the flood set and slots of its headlight pools; each truck's cab flood set.
    // (World keeps these in private fields; they are plain properties at run time.)
    const setIndex = (s) => (s && s.pos && setOf.has(s.pos) ? setOf.get(s.pos) : -1);
    const drive = { legs: {}, trucks: {} };
    if (w && w.legs) for (const [id, L] of Object.entries(w.legs)) {
      if (!L || !L.area) continue;
      const a = L.area;
      drive.legs[id] = {
        obstacles: (a.obstacles || []).map((o) => (o.kind === 'circle' ? [o.x, o.z, o.r] : [o.minX, o.maxX, o.minZ, o.maxZ])),
        lots: (a.lots || []).map((l) => ({ box: [l.box.minX, l.box.maxX, l.box.minZ, l.box.maxZ], y: l.y })),
        head: a.headlights ? { set: setIndex(a.headlights.set), slots: Array.from(a.headlights.slots) } : null,
        truck: idx(L.truck && L.truck.group),
      };
    }
    for (const k of ['saroTruck', 'truck', 'parked', 'oldTruck']) {
      const t = w && w[k];
      if (t) drive.trucks[k] = { group: idx(t.group), cab: setIndex(t.cabFlood), flood: setIndex(t.floods) };
    }
    const life = lifeOf(idx, setIndex);
    // the player and the camera where the night starts
    const player ={ x: S.player.pos.x, y: S.player.pos.y, z: S.player.pos.z, yaw: S.player.yaw, pitch: S.player.pitch, eye: S.player.eye, radius: S.player.radius, speed: S.player.speed };
    const cam = { fov: S.camera.fov, near: S.camera.near, far: S.camera.far };
    const fog = S.scene.fog ? { color: lin(S.scene.fog.color), density: S.scene.fog.density } : null;
    const bg = S.scene.background && S.scene.background.isColor ? lin(S.scene.background) : null;
    const L = S.room && S.room.lights;
    const roomLights = L ? { ceiling: L.ceiling.map((l) => l.uuid), lamps: L.lamps.map((l) => l.uuid), crt: L.crt.uuid, hemi: L.hemi.uuid, moon: L.moon.uuid } : null;
    const spawn = S.room && S.room.spawn ? { x: S.room.spawn.x, z: S.room.spawn.z, yaw: S.room.spawn.yaw } : null;
    return { roomLights, spawn, doors, handles, dishes, dishParts, items, walks, areas, floods, drive, life, player, cam, fog, bg, toneMappingExposure: S.renderer.toneMappingExposure };
  };

  // ---------- the world's own life (the C++ port's S47WorldLife) ----------
  // What the world's modules change every frame and keep in private fields, plain properties at run
  // time: the sky's parts and pictures (Sky.ts, backdrop.ts), the lamps that light each other's places
  // (World.shareLights), the light in the air (airlight.ts), the life in the night (nightlife.ts) and
  // the windows' light on the ground (windowLight.ts). A lamp that is read off a glow point (a cone's,
  // a moth's) names it: the points node and the point's index there.
  function lifeOf(idx, setIndex) {
    const w = S.world;
    // every glow point's world position, to find a lamp's by
    const pts = [];
    S.scene.updateMatrixWorld(true);
    S.scene.traverse((o) => {
      if (!o.isPoints || !byObj.has(o)) return;
      const p = o.geometry.attributes.position, c = o.geometry.attributes.aCol;
      if (!p || !c) return;
      const v = new T.Vector3();
      for (let k = 0; k < p.count; k++) { v.set(p.getX(k), p.getY(k), p.getZ(k)).applyMatrix4(o.matrixWorld); pts.push({ node: byObj.get(o), i: k, x: v.x, y: v.y, z: v.z, r: c.getX(k) }); }
    });
    const glowAt = (wp) => {
      let best = null, bd = 1e-4;
      for (const q of pts) { const d = (q.x - wp.x) ** 2 + (q.y - wp.y) ** 2 + (q.z - wp.z) ** 2; if (d < bd) { bd = d; best = q; } }
      return best;
    };
    const v3 = (v) => [v.x, v.y, v.z];
    const out = {};
    // the sky: the dome, the stars, the comet, the shooting star and the painted horizon in the sky's
    // group, and what their shaders read (Sky.ts milkyWay, holeSky; backdrop.ts's two pictures)
    const sk = S.sky, bd = S.backdrop;
    if (sk) {
      const dome = sk.group.children.find((c) => c.isMesh && c.material && c.material.uniforms && c.material.uniforms.uPanorama);
      const stars = sk.group.children.find((c) => c.isPoints);
      const u = dome ? dome.material.uniforms : null;
      out.sky = {
        group: idx(sk.group), dome: idx(dome), stars: idx(stars), comet: idx(sk.comet), meteor: idx(sk.meteor), band: bd ? idx(bd.mesh) : -1,
        panorama: u ? texId(u.uPanorama.value) : -1, holeSky: u ? lin(u.uHoleSky.value) : [0.006, 0.008, 0.02],
        night: bd ? texId(bd.uniforms.uNight.value) : -1, day: bd ? texId(bd.uniforms.uDay.value) : -1, texel: bd ? bd.uniforms.uTexel.value : 1 / 128,
      };
    }
    // World.shareLights: the driven truck's headlights in the motel's, the diner's and the town's own
    // sets (their head slots), the town's lamps in the road's (townSlots), the diner's neon in the
    // road's and the old road's (neonSlots); the set each is in
    const byKey = (k) => (S.floodSets || []).find((f) => f.key === k);
    const share = {};
    if (w) {
      if (w.motel && w.motel.head) share.motel = { set: setIndex(byKey('court')), head: Array.from(w.motel.head) };
      if (w.diner && w.diner.head) share.diner = { set: setIndex(byKey('diner')), head: Array.from(w.diner.head), red: w.diner.slots.red, cyan: w.diner.slots.cyan };
      if (w.town && w.town.head) share.town = { set: setIndex(byKey('town')), head: Array.from(w.town.head), lamps: w.town.lamps.map((l) => l.slot), canopy: w.town.slots.canopy ?? -1, blink: w.town.slots.blink ?? -1 };
      share.townSlots = Array.from(w.townSlots || []);
      share.neon = { road: Array.from((w.neonSlots && w.neonSlots.road) || []), old: Array.from((w.neonSlots && w.neonSlots.old) || []) };
    }
    out.share = share;
    // airlight.ts: each LightCones (its mesh, its lamps and the glow point each one is read off: k() is
    // that point's red over k0), the haze's mesh, and the lamps that tint the haze per area
    const air = w && w.air;
    if (air) {
      const cone = (c) => ({
        node: idx(c.mesh), lamps: c.lamps.map((L) => {
          const at = new T.Vector3(L.x, L.y + 0.15, L.z);
          if (c.mesh.parent) c.mesh.parent.localToWorld(at);
          const g = glowAt(at), k = L.k();
          return { x: L.x, y: L.y, z: L.z, ground: L.ground, r: L.r, col: lin(L.col), glow: g ? [g.node, g.i] : null, k0: g && k > 0 ? g.r / k : 1 };
        }),
      });
      const hl = (L) => ({ p: v3(L.p), r: L.r, col: lin(L.col), k: L.k });
      out.air = { cones: air.cones.map(cone), town: air.town ? air.cones.indexOf(air.town) : -1, haze: air.haze ? idx(air.haze.mesh) : -1,
        lamps: { saro: air.lamps.saro.map(hl), road: air.lamps.road.map(hl) } };
    }
    // nightlife.ts: per area its moths and bats (the cloud's node, every point's attributes, its lamps
    // with the glow point each is read off and the wall a porch light is on), its far lights (the glow
    // points they are, which windows and where), its rolling tumbleweed (and the materials of the lamp
    // sets it is lit by), the ones lying about; the dust of the beams
    const L = w && w.life;
    if (L) {
      const attr = (g, k) => (g.attributes[k] ? Array.from(g.attributes[k].array) : []);
      const areas = {};
      for (const [id, A] of Object.entries(L.areas || {})) {
        const a = {};
        const M = A.moths;
        if (M) {
          const g = M.points.geometry, par = M.points.parent;
          a.moths = {
            node: idx(M.points), n: M.n,
            pos: attr(g, 'position'), seed: attr(g, 'aSeed'), lamp: attr(g, 'aLamp'), kind: attr(g, 'aKind'), col: attr(g, 'aCol'), wall: attr(g, 'aWall'),
            lamps: M.lamps.map((P) => {
              const at = P.p.clone(); if (par) par.localToWorld(at);
              const q = glowAt(at), k = P.k();
              return { p: v3(P.p), col: lin(P.col), big: P.big, h: P.h, glow: q ? [q.node, q.i] : null, k0: q && k > 0 ? q.r / k : 1, wall: P.wall ? { n: v3(P.wall.n), rest: P.wall.rest.map(v3) } : null };
            }),
          };
        }
        const F = A.far;
        if (F) a.far = { node: idx(F.glow.points), win: F.win.map((W) => ({ i: W.i, ph: W.ph, p: v3(W.p), x: W.w.x, z: W.w.z, y: W.w.y, size: W.w.size, col: W.w.col, on: W.w.on, k: W.w.k ?? 1, tv: !!W.w.tv })), car: Array.from(F.car) };
        const Wd = A.weed;
        if (Wd) a.weed = { node: idx(Wd.mesh), base: matIndex.has(Wd.base.uuid) ? matIndex.get(Wd.base.uuid) : matId(Wd.base), floods: Wd.floods.map((f) => matIndex.has(f.mat.uuid) ? matIndex.get(f.mat.uuid) : matId(f.mat)) };
        a.resting = (A.resting || []).map((R) => {
          const g = R.mesh.geometry; if (!g.boundingSphere) g.computeBoundingSphere();
          return { node: idx(R.mesh), low: R.low, centre: v3(g.boundingSphere.center), radius: g.boundingSphere.radius };
        });
        areas[id] = a;
      }
      out.life = { areas, flash: L.flash ? idx(L.flash.points) : -1, head: L.head ? idx(L.head.points) : -1 };
    }
    // windowLight.ts: every area's window lights: the mesh (its corners' picture coordinates, blur and
    // index are in its UV channels), the set its lamps are in, the slots that are its own rooms', the
    // floods' fall-off, where its group stands, every window (its slot, level, colour, corners) and each
    // corner's fall-off and ground tint
    if (S.windowLights) out.windows = S.windowLights.map((W) => ({
      name: W.name, node: idx(W.mesh), set: setIndex(W.set), own: Array.from(W.own || []), fall: W.fall, origin: v3(W.origin),
      wins: W.wins.map((x) => ({ id: x.id, slot: x.slot ?? -1, k: x.k, w0: x.w0, level: x.level, col: lin(x.col), at: x.at, n: x.n, fall: Array.from(x.fall) })),
      pos: Array.from(W.pos), tints: Array.from(W.tints), map: W.mat && W.mat.uniforms.uMap.value ? texId(W.mat.uniforms.uMap.value) : -1,
      mapSize: W.mat ? [W.mat.uniforms.uMapSize.value.x, W.mat.uniforms.uMapSize.value.y] : [1, 1],
    }));
    // The web version's own numbers for the C++ port's self test (S47WorldLife::SelfTest): some cells
    // of each flora patch as Flora.make draws them (with the patch's own cover rules), and the sky's
    // uniforms at a few clocks (Sky.setClock; put back to the game's clock after)
    const tests = {};
    const fl = w && w.flora && w.flora.patches ? [...w.flora.patches] : [];
    if (w && w.oldRoad && w.oldRoad.flora) fl.push(w.oldRoad.flora);
    const CELLS = { saro: [[3, -38], [0, -41], [5, -30], [2, -20], [-10, -50], [3, 20]], road: [[0, 50], [-1, 10], [2, 250], [-3, 260], [1, 85]],
      station01: [[-70, 66], [-60, 70], [-50, 60]], oldroad: [[0, 0], [10, -5], [-20, 3]] };
    tests.flora = fl.map((f) => ({
      name: f.spec.name, cells: (CELLS[f.spec.name] || []).flatMap(([i, j]) => [0, 1, 2, 3, 4, 5, 6].map((ki) => {
        const c = f.make(i, j, ki);
        return { i, j, ki, n: c.n, first: c.n ? Array.from(c.m.slice(0, 16)) : null, last: c.n ? Array.from(c.m.slice((c.n - 1) * 16, c.n * 16)) : null,
          c: c.n ? Array.from(c.c.slice(0, 3)) : null, rank: c.n ? c.rank[0] : null };
      })),
    }));
    if (sk) {
      const u = sk.uniforms, keep = S.game ? S.game.clock : 0;
      tests.sky = { lift: sk.dawnLift, holeFrom: sk.holeFrom, at: [2 * 3600, 2 * 3600 + 16 * 60, 4 * 3600 + 50 * 60, 5 * 3600 + 20 * 60, 5 * 3600 + 28 * 60, 5 * 3600 + 40 * 60].map((c) => {
        sk.setClock(c);
        return { clock: c, dawn: u.uDawn.value, sun: u.uSun.value, sunDir: v3(u.uSunDir.value), hole: u.uHole.value, fog: lin(sk.fogColor) };
      }) };
      sk.setClock(keep);
    }
    out.tests = tests;
    return out;
  }
})();
