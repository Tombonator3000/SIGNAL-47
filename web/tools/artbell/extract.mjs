#!/usr/bin/env node
// Reproduce the exact 1420 Night Line host bank from authored TypeScript.
// No game modules, radio playback, callers or NPC conversations are executed.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import vm from 'node:vm';
import assert from 'node:assert/strict';
import { fileURLToPath } from 'node:url';
import ts from 'typescript';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const OUT = path.join(ROOT, 'production/artbell/corpus.json');
const normalize = (text) => text.replace(/\s+/g, ' ').trim();
const hash = (text) => crypto.createHash('sha256').update(text, 'utf8').digest('hex').slice(0, 16);
const clips = new Map(), staticLines = [], templates = [], excludedNpc = [];
const sourceFiles = ['src/core/radio.ts', 'src/story/tripRadio.ts', 'src/story/caliche.ts'];
const parsed = new Map(sourceFiles.map((file) => [file, ts.createSourceFile(file, fs.readFileSync(path.join(ROOT, file), 'utf8'), ts.ScriptTarget.Latest, true)]));
const lineOf = (sf, node) => sf.getLineAndCharacterOfPosition(node.getStart(sf)).line + 1;
const property = (node, name) => node.properties?.find((p) => ts.isPropertyAssignment(p) && p.name.getText() === name);

function add(text, source) {
  text = normalize(text);
  if (!text) return null;
  const id = hash(text), prior = clips.get(id);
  if (prior) { assert.equal(prior.text, text, `Hash collision: ${id}`); prior.sources.push(source); }
  else clips.set(id, { id, text, sources: [source] });
  return id;
}

function declaration(sf, name) {
  let result;
  function walk(node) {
    if (ts.isVariableDeclaration(node) && node.name.getText(sf) === name) result = node;
    ts.forEachChild(node, walk);
  }
  walk(sf);
  assert.ok(result, `Missing declaration ${name} in ${sf.fileName}`);
  return result;
}

function owner(node) {
  while (node && !ts.isVariableDeclaration(node)) node = node.parent;
  return node?.name.getText();
}

for (const file of sourceFiles) {
  const sf = parsed.get(file);
  if (file !== 'src/story/caliche.ts') {
    const helper = declaration(sf, 'H').initializer;
    assert.ok(ts.isArrowFunction(helper) && ts.isParenthesizedExpression(helper.body));
    assert.equal(property(helper.body.expression, 'who')?.initializer.text, 'host', 'H must remain the Night Line host');
  }
  function walk(node) {
    if (!ts.isCallExpression(node)) { ts.forEachChild(node, walk); return; }
    const helper = node.expression.getText(sf);
    if (file === 'src/story/caliche.ts') {
      if (helper === 'EA') excludedNpc.push({ file, line: lineOf(sf, node), reason: 'In-person Earl remains outside Art Bell radio scope.' });
      ts.forEachChild(node, walk); return;
    }
    if (helper !== 'H') { ts.forEachChild(node, walk); return; }
    const group = owner(node), source = { file, line: lineOf(sf, node), station: 'caliche', voice: 'host', group };
    if (file === 'src/core/radio.ts')
      assert.ok(['NIGHT_LINE', 'BOY_CALL', 'SIGN_OFF_SEQ'].includes(group), `Unreviewed H scope: ${group}`);
    else {
      assert.equal(group, 'PROGRAMME');
      let mark = node.parent;
      while (mark && !(ts.isObjectLiteralExpression(mark) && property(mark, 'on'))) mark = mark.parent;
      assert.equal(property(mark, 'on')?.initializer.text, 'caliche', 'Trip host must be on 1420');
      source.trip = property(mark, 'trip')?.initializer.text;
    }
    const value = node.arguments[0];
    if (ts.isStringLiteral(value) || ts.isNoSubstitutionTemplateLiteral(value)) {
      const id = add(value.text, { ...source, kind: 'static-host' });
      staticLines.push({ ...source, id, text: normalize(value.text) });
    } else {
      assert.ok(ts.isArrowFunction(value) && ts.isTemplateExpression(value.body), `Unsupported host expression: ${file}:${source.line}`);
      const body = value.body;
      assert.equal(body.templateSpans.length, 1, 'Only the existing single sayTime slot is in scope');
      let clock = body.templateSpans[0].expression;
      const capitalize = ts.isCallExpression(clock) && clock.expression.getText(sf) === 'cap';
      if (capitalize) clock = clock.arguments[0];
      assert.ok(ts.isCallExpression(clock) && clock.expression.getText(sf) === 'sayTime');
      assert.equal(clock.arguments[0].getText(sf), value.parameters[0].name.getText(sf));
      const prefix = body.head.text, suffix = body.templateSpans[0].literal.text;
      const before = add(prefix, { ...source, kind: 'time-prefix' });
      const after = add(suffix, { ...source, kind: 'time-suffix' });
      const parts = [
        ...(before ? [{ kind: 'clip', id: before, text: normalize(prefix), raw: prefix }] : []),
        { kind: 'time', capitalize },
        ...(after ? [{ kind: 'clip', id: after, text: normalize(suffix), raw: suffix }] : []),
      ];
      templates.push({ ...source, id: hash(`${prefix}{sayTime:${capitalize ? 'cap' : 'plain'}}${suffix}`), prefix, suffix, capitalize,
        ...(before ? { before } : {}), ...(after ? { after } : {}), parts });
    }
    ts.forEachChild(node, walk);
  }
  walk(sf);
}

const radio = parsed.get('src/core/radio.ts');
const onesNode = declaration(radio, 'ONES'), tensNode = declaration(radio, 'TENS');
const literalArray = (node) => {
  assert.ok(ts.isArrayLiteralExpression(node.initializer));
  return node.initializer.elements.map((element) => { assert.ok(ts.isStringLiteral(element)); return element.text; });
};
const ones = literalArray(onesNode), tens = literalArray(tensNode);
const sayTimeNode = radio.statements.find((node) => ts.isFunctionDeclaration(node) && node.name?.text === 'sayTime');
assert.ok(sayTimeNode, 'Missing sayTime formatter');
const timeSource = { file: 'src/core/radio.ts', line: lineOf(radio, sayTimeNode), station: 'caliche', voice: 'host', kind: 'time-bank' };
// Twelve hour words and sixty minute phrases share ten/eleven/twelve. The
// minute's hyphens and o'clock are exactly the ones sayTime() renders.
const timeBank = {
  hours: Array.from({ length: 12 }, (_, i) => {
    const value = i + 1, text = ones[value];
    return { value, id: add(text, { ...timeSource, component: 'hour', value }), text };
  }),
  minutes: Array.from({ length: 60 }, (_, value) => {
    const text = value === 0 ? "o'clock" : value < 10 ? `oh-${ones[value]}`
      : value < 20 ? ones[value] : tens[Math.floor(value / 10)] + (value % 10 ? '-' + ones[value % 10] : '');
    return { value, id: add(text, { ...timeSource, component: 'minute', value }), text };
  }),
};

// Evaluate only the isolated, AST-selected string formatter to verify parity.
// This never imports radio.ts or runs any game/audio code.
const formatterSource = `const ONES = ${onesNode.initializer.getText(radio)};\nconst TENS = ${tensNode.initializer.getText(radio)};\n${sayTimeNode.getText(radio)}`;
const formatterExports = {};
vm.runInNewContext(ts.transpileModule(formatterSource, { compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS } }).outputText,
  { exports: formatterExports }, { timeout: 100 });
const sayTime = formatterExports.sayTime;
let checks = 0;
for (let minute = 0; minute < 24 * 60; minute++) {
  const hour = Math.floor(minute / 60) % 12 || 12, m = minute % 60;
  const clock = timeBank.hours.find((entry) => entry.value === hour).text + (m > 0 && m < 10 ? '-' : ' ') + timeBank.minutes[m].text;
  assert.equal(clock, sayTime(minute * 60 + 37));
  for (const template of templates) {
    const rendered = template.prefix + (template.capitalize ? clock[0].toUpperCase() + clock.slice(1) : clock) + template.suffix;
    const composed = template.parts.map((part) => part.kind === 'clip' ? part.raw : part.capitalize ? clock[0].toUpperCase() + clock.slice(1) : clock).join('');
    assert.equal(rendered, composed); checks++;
  }
}
const items = [...clips.values()];
assert.equal(staticLines.length, 38, 'Unexpected static host scope; review the authored changes');
assert.equal(templates.length, 6, 'Unexpected dynamic host scope; review the authored changes');
assert.equal(items.length, 115, 'Unexpected bank size; review before regenerating assets');
for (const clip of items) assert.equal(clip.id, hash(clip.text));
const report = {
  version: 1,
  scope: 'Art Bell synthetic radio voice for who=host on 1420 Caliche / Night Line, including the boy call, sign-off and trip programme. Callers, 1230 Roswell, news, CB and in-person Earl are excluded.',
  normalization: 'text.replace(/\\s+/g, " ").trim(); id = SHA-256(normalized UTF-8 text).slice(0,16). Template prefix/suffix/raw preserve original whitespace.',
  clips: items, staticLines, templates, timeBank,
  summary: { clipCount: items.length, staticHostLines: staticLines.length, dynamicHostTemplates: templates.length,
    timeFragmentClips: items.filter((clip) => clip.sources.some((source) => /^time-(?:prefix|suffix)$/.test(source.kind))).length,
    timeBankClips: items.filter((clip) => clip.sources.some((source) => source.kind === 'time-bank')).length,
    words: items.reduce((sum, clip) => sum + clip.text.split(/\s+/).length, 0),
    characters: items.reduce((sum, clip) => sum + clip.text.length, 0), excludedInPersonEarlLines: excludedNpc.length },
  coverage: { kind: 'text-only AST extraction and isolated formatter verification; no audio playback', minutesChecked: 1440, dynamicRenderedChecks: checks, idsChecked: items.length },
  excludedNpc,
};
fs.mkdirSync(path.dirname(OUT), { recursive: true });
fs.writeFileSync(OUT, JSON.stringify(report, null, 2) + '\n');
console.log(JSON.stringify({ ...report.summary, ...report.coverage }, null, 2));
