#!/usr/bin/env node
// Extract only the authored caller and fictional 1947 bulletin speech.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import { fileURLToPath } from 'node:url';
import ts from 'typescript';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const files = ['src/core/radio.ts', 'src/story/tripRadio.ts', 'src/story/nightEvents.ts'];
const roles = new Set(['woman', 'man', 'old', 'boy', 'news']);
const helpers = { W: 'woman', M: 'man', O: 'old' };
const normalize = s => s.replace(/\s+/g, ' ').trim();
const clips = new Map();
const prop = (n, key) => n.properties?.find(p => ts.isPropertyAssignment(p) && p.name.getText() === key);
const literal = n => n && (ts.isStringLiteral(n) || ts.isNoSubstitutionTemplateLiteral(n));
const group = n => { while (n && !ts.isVariableDeclaration(n)) n = n.parent; return n?.name.getText(); };
function add(voice, text, source) {
  text = normalize(text);
  const id = crypto.createHash('sha256').update(voice + '\0' + text).digest('hex').slice(0, 16);
  const old = clips.get(id);
  if (old) { assert.equal(old.text, text); assert.equal(old.voice, voice); old.sources.push(source); }
  else clips.set(id, { id, voice, text, sources: [source] });
}
for (const file of files) {
  const sf = ts.createSourceFile(file, fs.readFileSync(path.join(ROOT, file), 'utf8'), ts.ScriptTarget.Latest, true);
  const source = n => ({file, line: sf.getLineAndCharacterOfPosition(n.getStart(sf)).line + 1, group: group(n)});
  const foundHelpers = new Set();
  function visit(n) {
    if (file.endsWith('tripRadio.ts') && ts.isVariableDeclaration(n) && helpers[n.name.getText()]) {
      const body = n.initializer?.body;
      assert.ok(body && ts.isParenthesizedExpression(body));
      assert.equal(prop(body.expression, 'who')?.initializer.text, helpers[n.name.getText()]);
      foundHelpers.add(n.name.getText());
    }
    if (ts.isObjectLiteralExpression(n)) {
      const who = prop(n, 'who')?.initializer, text = prop(n, 'text')?.initializer;
      if (literal(who) && roles.has(who.text) && literal(text)) {
        const context = source(n);
        assert.ok(file.endsWith('nightEvents.ts') ? context.group === 'BULLETIN' : ['NIGHT_LINE', 'BOY_CALL'].includes(context.group));
        add(who.text, text.text, context);
      }
    }
    if (file.endsWith('tripRadio.ts') && ts.isCallExpression(n) && helpers[n.expression.getText()]) {
      assert.ok(literal(n.arguments[0]), 'Caller speech must remain static');
      const context = source(n);
      assert.equal(context.group, 'PROGRAMME');
      add(helpers[n.expression.getText()], n.arguments[0].text, context);
    }
    ts.forEachChild(n, visit);
  }
  visit(sf);
  if (file.endsWith('tripRadio.ts')) assert.deepEqual([...foundHelpers].sort(), Object.keys(helpers).sort());
}
const values = [...clips.values()];
const counts = Object.fromEntries([...roles].map(role => [role, values.filter(c => c.voice === role).length]));
assert.equal(values.length, 28, 'Review source scope before changing the 28-clip bank');
assert.equal(counts.news, 6);
const corpus = {version: 1, normalization: 'Whitespace collapse and trim; ID is first 16 hexadecimal SHA-256 characters of role + NUL + normalized text.',
  sourceFiles: files, summary: {clips: values.length, words: values.reduce((n,c) => n + c.text.split(' ').length, 0), roles: counts}, clips: values};
const out = path.join(ROOT, 'production/radiocast/corpus.json');
fs.mkdirSync(path.dirname(out), {recursive: true});
fs.writeFileSync(out, JSON.stringify(corpus, null, 2) + '\n');
console.log(JSON.stringify(corpus.summary));
