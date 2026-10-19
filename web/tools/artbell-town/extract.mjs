#!/usr/bin/env node
// Only Earl's authored in-person speech, including both greeting branches.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import { fileURLToPath } from 'node:url';
import ts from 'typescript';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const file = 'src/story/caliche.ts';
const raw = fs.readFileSync(path.join(ROOT, file), 'utf8');
const sf = ts.createSourceFile(file, raw, ts.ScriptTarget.Latest, true);
const normalize = text => text.replace(/\s+/g, ' ').trim();
const sha = text => crypto.createHash('sha256').update(text).digest('hex');
const property = (node, name) => node.properties?.find(p => ts.isPropertyAssignment(p) && p.name.getText(sf) === name);
const literal = node => node && (ts.isStringLiteral(node) || ts.isNoSubstitutionTemplateLiteral(node));
function declaration(name) {
  const found = [];
  function walk(node) {
    if (ts.isVariableDeclaration(node) && node.name.getText(sf) === name) found.push(node);
    ts.forEachChild(node, walk);
  }
  walk(sf);
  assert.equal(found.length, 1, `Expected exactly one ${name} declaration`);
  return found[0];
}
const helper = declaration('EA').initializer;
assert.ok(ts.isArrowFunction(helper) && ts.isParenthesizedExpression(helper.body));
assert.equal(property(helper.body.expression, 'who')?.initializer.text, 'EARL');
assert.equal(property(helper.body.expression, 'text'), undefined, 'EA must keep its text shorthand');
assert.ok(helper.body.expression.properties.some(p => ts.isShorthandPropertyAssignment(p) && p.name.text === helper.parameters[0].name.getText(sf)));
const people = declaration('PEOPLE').initializer;
assert.ok(ts.isObjectLiteralExpression(people));
const host = property(people, 'host')?.initializer;
assert.ok(host && ts.isObjectLiteralExpression(host));
const clips = new Map();
function walk(node) {
  if (ts.isCallExpression(node) && node.expression.getText(sf) === 'EA') {
    assert.ok(node.pos >= host.pos && node.end <= host.end, 'Unreviewed Earl call outside PEOPLE.host');
    assert.equal(node.arguments.length, 1);
    assert.ok(literal(node.arguments[0]), 'Earl NPC lines must remain static literals');
    let ancestor = node.parent, topic;
    while (ancestor && ancestor !== host) {
      if (ts.isObjectLiteralExpression(ancestor) && property(ancestor, 'id')) {
        const value = property(ancestor, 'id').initializer;
        assert.ok(literal(value)); topic = value.text; break;
      }
      if (ts.isPropertyAssignment(ancestor) && ancestor.name.getText(sf) === 'open') { topic = 'open'; break; }
      ancestor = ancestor.parent;
    }
    assert.ok(['open', 'calls', 'visitors', 'tape', 'boy'].includes(topic));
    const text = normalize(node.arguments[0].text), id = sha(text).slice(0, 16);
    const source = {file, line: sf.getLineAndCharacterOfPosition(node.getStart(sf)).line + 1,
      kind: 'static-npc', character: 'EARL', topic};
    const prior = clips.get(id);
    if (prior) { assert.equal(prior.text, text); prior.sources.push(source); }
    else clips.set(id, {id, text, sources: [source]});
  }
  ts.forEachChild(node, walk);
}
walk(sf);
const items = [...clips.values()];
const topics = Object.fromEntries(['open', 'calls', 'visitors', 'tape', 'boy'].map(t => [t, items.filter(c => c.sources.some(s => s.topic === t)).length]));
const words = items.reduce((n, c) => n + c.text.split(' ').length, 0);
assert.deepEqual(topics, {open: 2, calls: 4, visitors: 2, tape: 4, boy: 4});
assert.equal(items.length, 16); assert.equal(words, 150);
const corpus = {version: 1,
  scope: 'Kun statisk EA-tale under PEOPLE.host: første møte, gjenbesøk og alle fire samtaleemner. Spillersvar, asider, knapper og vitnepapirer er utelatt.',
  normalization: 'Whitespace collapse and trim; ID = SHA-256(normalized UTF-8 text).slice(0,16).',
  source: {file, sha256: sha(raw)},
  summary: {clips: items.length, words, topics}, clips: items};
const out = path.join(ROOT, 'production/artbell-town/corpus.json');
fs.mkdirSync(path.dirname(out), {recursive: true});
fs.writeFileSync(out, JSON.stringify(corpus, null, 2) + '\n');
console.log(JSON.stringify(corpus.summary));
