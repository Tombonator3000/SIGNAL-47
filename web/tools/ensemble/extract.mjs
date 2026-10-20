#!/usr/bin/env node
// Read authored character speech from TypeScript. No game module is executed.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import { fileURLToPath } from 'node:url';
import ts from 'typescript';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const normalize = text => text.replace(/\s+/g, ' ').trim();
const hash = text => crypto.createHash('sha256').update(text).digest('hex');
const files = ['src/story/caliche.ts', 'src/story/Chapter4.ts', 'src/core/phoneScripts.ts', 'src/story/Chapter3.ts'];
const parsed = new Map(files.map(file => [file, ts.createSourceFile(file, fs.readFileSync(path.join(ROOT, file), 'utf8'), ts.ScriptTarget.Latest, true)]));
const property = (node, name) => node.properties?.find(p => ts.isPropertyAssignment(p) && p.name.getText() === name);
const literal = node => node && (ts.isStringLiteral(node) || ts.isNoSubstitutionTemplateLiteral(node));
const price = 'Reference west. No. East. Hold the last reading.';
const clips = new Map();
function add(voice, text, source) {
  text = normalize(text);
  const id = hash(voice + '\0' + text).slice(0, 16), prior = clips.get(id);
  if (prior) { assert.equal(prior.text, text); assert.equal(prior.voice, voice); prior.sources.push(source); }
  else clips.set(id, {id, voice, text, sources: [source]});
}
for (const [file, sf] of parsed) {
  const source = (node, kind, extra = {}) => ({file, line: sf.getLineAndCharacterOfPosition(node.getStart(sf)).line + 1, kind, ...extra});
  const helpers = file.endsWith('caliche.ts') ? {A: ['stanton', 'THE ATTENDANT'], DP: ['stack', 'THE DEPUTY'], SM: ['lynch', 'THE SOUVENIR MAN']}
    : file.endsWith('Chapter4.ts') ? {n: ['bacall', 'N. VEGA']} : {};
  const validated = new Set();
  function walk(node) {
    if (ts.isVariableDeclaration(node) && helpers[node.name.getText(sf)]) {
      const [voice, who] = helpers[node.name.getText(sf)], arrow = node.initializer;
      assert.ok(ts.isArrowFunction(arrow) && ts.isParenthesizedExpression(arrow.body));
      const value = property(arrow.body.expression, 'who')?.initializer;
      if (voice === 'bacall') {
        assert.ok(ts.isIdentifier(value) && value.text === 'N');
        assert.ok(sf.text.includes("const N = 'N. VEGA'"));
      } else assert.equal(value?.text, who);
      validated.add(node.name.getText(sf));
    }
    if (ts.isCallExpression(node) && helpers[node.expression.getText(sf)]) {
      assert.equal(node.arguments.length, 1);
      assert.ok(literal(node.arguments[0]), 'Actor dialogue must remain a static literal');
      add(helpers[node.expression.getText(sf)][0], node.arguments[0].text, source(node, 'dialogue', {helper: node.expression.getText(sf)}));
    }
    if (file.endsWith('phoneScripts.ts') && ts.isPropertyAssignment(node) && ['nora', 'fieldNora'].includes(node.name.getText(sf))) {
      assert.ok(ts.isArrayLiteralExpression(node.initializer));
      for (const cue of node.initializer.elements) {
        assert.ok(ts.isObjectLiteralExpression(cue));
        if (property(cue, 'aside')?.initializer.kind === ts.SyntaxKind.TrueKeyword) continue;
        const text = property(cue, 'text')?.initializer;
        assert.ok(literal(text)); assert.equal(property(cue, 'by')?.initializer.text, 'N. VEGA');
        add('bacall', text.text, source(cue, 'phone', {declaration: node.name.getText(sf)}));
      }
    }
    if (file.endsWith('Chapter4.ts') && ts.isCallExpression(node) && node.expression.getText(sf) === 'this.d.motel.cb') {
      assert.ok(ts.isArrayLiteralExpression(node.arguments[0]));
      for (const cue of node.arguments[0].elements) {
        assert.ok(ts.isObjectLiteralExpression(cue));
        const text = property(cue, 'text')?.initializer;
        assert.ok(literal(text)); assert.equal(property(cue, 'who')?.initializer.text, 'woman');
        add('bacall', text.text, source(cue, 'cb'));
      }
    }
    if (file.endsWith('Chapter4.ts') && literal(node)) {
      const match = /^N\. VEGA: "([\s\S]*)"$/.exec(node.text);
      if (match) add('bacall', match[1], source(node, 'captioned-dialogue'));
    }
    if (literal(node) && node.text === price) add('price', price, source(node, 'recorded-field-line'));
    ts.forEachChild(node, walk);
  }
  walk(sf);
  assert.deepEqual([...validated].sort(), Object.keys(helpers).sort());
}
const items = [...clips.values()];
const counts = Object.fromEntries(['stanton', 'bacall', 'stack', 'lynch', 'price'].map(role => {
  const list = items.filter(item => item.voice === role);
  return [role, {clips: list.length, words: list.reduce((n, item) => n + item.text.split(' ').length, 0)}];
}));
assert.deepEqual(Object.fromEntries(Object.entries(counts).map(([k,v]) => [k,v.clips])), {stanton:18, bacall:49, stack:19, lynch:13, price:1});
assert.equal(items.length, 100);
const corpus = {version: 1,
  scope: 'Eksisterende A/DP/SM i Caliche, alle n()-linjer fra Nora i rom 6, Nora-telefonene og CB samt hennes ni sitat-/hintlinjer. Price bruker bare den eksisterende T. Vega-feltlinjen. Ingen nye replikker.',
  normalization: 'Whitespace collapse and trim; ID = SHA-256(role + NUL + normalized UTF-8 text).slice(0,16).',
  sourceFiles: files.map(file => ({file, sha256: hash(fs.readFileSync(path.join(ROOT, file)))})),
  summary: {clips: items.length, words: items.reduce((n,c) => n + c.text.split(' ').length, 0), roles: counts}, clips: items};
const out = path.join(ROOT, 'production/ensemble/corpus.json');
fs.mkdirSync(path.dirname(out), {recursive: true});
fs.writeFileSync(out, JSON.stringify(corpus, null, 2) + '\n');
console.log(JSON.stringify(corpus.summary, null, 2));
