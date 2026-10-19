#!/usr/bin/env node
// Extract the English document, conversation and authored telephone speech.
// The AST is inspected, never evaluated. Radio and gameplay code are not imported.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';
import ts from 'typescript';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const OUT = path.join(ROOT, 'production/kinski');
const normalize = (text) => text.replace(/\s+/g, ' ').trim();
// Keep identical to documentReadingParagraphs() in core/readAloud.ts. Only the
// paper's rendering marks are removed; descriptions and dialogue stay exact.
const pageParagraphs = (text) => text.replace(/^~[ \t]+/gm, '').replace(/^[ \t]*(?:[-=][ \t]*){3,}$/gm, '')
  .split(/\n\s*\n/).map(normalize).filter(Boolean);
const DYNAMIC = '\u0001RUNTIME_VALUE\u0001';
const helpers = {
  'Chapter4.ts': ['n', 'you', 'aside'],
  'Chapter5.ts': ['W', 'D', 'E', 'you', 'aside'],
  'caliche.ts': ['A', 'DP', 'EA', 'RW', 'SM', 'you', 'aside'],
};

function variants(node) {
  if (!node) return [DYNAMIC];
  if (ts.isStringLiteral(node) || ts.isNoSubstitutionTemplateLiteral(node)) return [node.text];
  if (ts.isParenthesizedExpression(node)) return variants(node.expression);
  if (ts.isConditionalExpression(node)) return [...variants(node.whenTrue), ...variants(node.whenFalse)];
  if (ts.isTemplateExpression(node)) {
    let out = [node.head.text];
    for (const span of node.templateSpans)
      out = out.flatMap((prefix) => variants(span.expression).map((value) => prefix + value + span.literal.text));
    return out;
  }
  if (ts.isBinaryExpression(node) && node.operatorToken.kind === ts.SyntaxKind.PlusToken)
    return variants(node.left).flatMap((a) => variants(node.right).map((b) => a + b));
  return [DYNAMIC];
}

// Resolve only literal page data from the same source file. No game module runs.
// The scope check matters: Prologue has several unrelated local `lines` arrays.
function pageReader(sf) {
  const declarations = new Map();
  function index(node) {
    if (ts.isVariableDeclaration(node) && ts.isIdentifier(node.name) && node.initializer && (node.parent.flags & ts.NodeFlags.Const)) {
      let scope = node.parent;
      while (scope && !ts.isBlock(scope) && !ts.isSourceFile(scope)) scope = scope.parent;
      const list = declarations.get(node.name.text) ?? [];
      list.push({ node: node.initializer, scope }); declarations.set(node.name.text, list);
    }
    ts.forEachChild(node, index);
  }
  index(sf);
  const binding = (node) => (declarations.get(node.text) ?? [])
    .filter(({ scope }) => scope.pos <= node.pos && scope.end >= node.end)
    .sort((a, b) => (a.scope.end - a.scope.pos) - (b.scope.end - b.scope.pos))[0]?.node;
  function unwrap(node, seen = new Set()) {
    if (!node || seen.has(node)) return null;
    seen = new Set([...seen, node]);
    if (ts.isIdentifier(node)) return unwrap(binding(node), seen);
    if (ts.isParenthesizedExpression(node)) return unwrap(node.expression, seen);
    return node;
  }
  function read(node, choices = new Map(), seen = new Set()) {
    if (!node || seen.has(node)) return [DYNAMIC];
    seen = new Set([...seen, node]);
    if (ts.isStringLiteral(node) || ts.isNoSubstitutionTemplateLiteral(node)) return [node.text];
    if (ts.isIdentifier(node)) return read(binding(node), choices, seen);
    if (ts.isParenthesizedExpression(node)) return read(node.expression, choices, seen);
    if (ts.isConditionalExpression(node)) {
      const choice = choices.get(node.condition.getText(sf));
      return choice === undefined ? [...read(node.whenTrue, choices, seen), ...read(node.whenFalse, choices, seen)]
        : read(choice ? node.whenTrue : node.whenFalse, choices, seen);
    }
    const combine = (prefixes, suffixes, join = '') => prefixes.flatMap((a) => suffixes.map((b) => a + join + b));
    if (ts.isTemplateExpression(node)) {
      let out = [node.head.text];
      for (const span of node.templateSpans) out = combine(out, read(span.expression, choices, seen)).map((s) => s + span.literal.text);
      return out;
    }
    if (ts.isBinaryExpression(node) && node.operatorToken.kind === ts.SyntaxKind.PlusToken)
      return combine(read(node.left, choices, seen), read(node.right, choices, seen));
    if (ts.isPropertyAccessExpression(node) || ts.isElementAccessExpression(node)) {
      const object = unwrap(node.expression);
      if (!object || !ts.isObjectLiteralExpression(object)) return [DYNAMIC];
      const key = ts.isPropertyAccessExpression(node) ? node.name.text
        : node.argumentExpression && ts.isStringLiteral(node.argumentExpression) ? node.argumentExpression.text : null;
      return object.properties.filter(ts.isPropertyAssignment)
        .filter((p) => key === null || p.name.getText(sf).replace(/^['"]|['"]$/g, '') === key)
        .flatMap((p) => read(p.initializer, choices, seen));
    }
    if (ts.isCallExpression(node) && ts.isPropertyAccessExpression(node.expression)) {
      const call = node.expression;
      if (call.name.text === 'join') {
        const array = unwrap(call.expression);
        if (!array || !ts.isArrayLiteralExpression(array)) return [DYNAMIC];
        const elements = (arr) => arr.elements.flatMap((e) => {
          if (!ts.isSpreadElement(e)) return [e];
          const spread = unwrap(e.expression);
          return spread && ts.isArrayLiteralExpression(spread) ? elements(spread) : [null];
        });
        const separators = node.arguments.length ? read(node.arguments[0], choices, seen) : [','];
        return separators.flatMap((separator) => elements(array).reduce((out, item, i) => combine(out, read(item, choices, seen), i ? separator : ''), ['']));
      }
      if (call.expression.kind === ts.SyntaxKind.ThisKeyword && !node.arguments.length) {
        let owner = node.parent;
        while (owner && !ts.isClassDeclaration(owner)) owner = owner.parent;
        const method = owner?.members.find((m) => ts.isMethodDeclaration(m) && m.name.getText(sf) === call.name.text);
        const returned = method?.body?.statements.find(ts.isReturnStatement)?.expression;
        if (!returned) return [DYNAMIC];
        // Repeated predicates in reportText() must choose the same branch in
        // every paragraph, preserving the actual passive/active reports.
        const predicates = new Map();
        function collect(n) {
          if (ts.isConditionalExpression(n)) { const key = n.condition.getText(sf); predicates.set(key, (predicates.get(key) ?? 0) + 1); }
          ts.forEachChild(n, collect);
        }
        collect(returned);
        let contexts = [choices];
        for (const [key, count] of predicates) if (count > 1 && !choices.has(key))
          contexts = contexts.flatMap((context) => [new Map([...context, [key, true]]), new Map([...context, [key, false]])]);
        return contexts.flatMap((context) => read(returned, context, seen));
      }
    }
    return [DYNAMIC];
  }
  return (node) => [...new Set(read(node))];
}

const clips = new Map(), dynamic = [], documents = [], pages = [], talkSites = [], phoneSites = [];
function add(text, source) {
  text = normalize(text);
  if (!text || text.includes(DYNAMIC)) return;
  const id = crypto.createHash('sha256').update(text, 'utf8').digest('hex').slice(0, 16);
  const existing = clips.get(id);
  if (existing && existing.text !== text) throw new Error(`Hash collision: ${id}`);
  if (existing) existing.sources.push(source);
  else clips.set(id, { id, text, sources: [source] });
}

for (const name of fs.readdirSync(path.join(ROOT, 'src/story')).filter((name) => name.endsWith('.ts')).sort()) {
  const file = 'src/story/' + name;
  const sf = ts.createSourceFile(file, fs.readFileSync(path.join(ROOT, file), 'utf8'), ts.ScriptTarget.Latest, true);
  const pageVariants = pageReader(sf);
  const where = (node, kind) => ({ file, line: sf.getLineAndCharacterOfPosition(node.getStart(sf)).line + 1, kind });
  function walk(node) {
    if (ts.isPropertyAssignment(node) && node.name.getText(sf) === 'transcript') {
      const source = where(node, 'document');
      const parent = node.parent;
      const id = parent.properties?.find((p) => ts.isPropertyAssignment(p) && p.name.getText(sf) === 'id');
      const title = parent.properties?.find((p) => ts.isPropertyAssignment(p) && p.name.getText(sf) === 'title');
      source.document = id?.initializer.getText(sf) ?? title?.initializer.getText(sf) ?? 'document factory';
      const values = [...new Set(variants(node.initializer))];
      const entry = { ...source, variants: values.length, dynamic: values.some((v) => v.includes(DYNAMIC)) };
      documents.push(entry);
      for (const text of values) {
        const paragraphs = text.split(/\n\s*\n/).filter((text) => normalize(text));
        paragraphs.forEach((paragraph, index) => add(paragraph, { ...source, paragraph: index + 1 }));
      }
      if (entry.dynamic) dynamic.push({ ...entry, expression: node.initializer.getText(sf) });
    }
    if (ts.isPropertyAssignment(node) && node.name.getText(sf) === 'page') {
      const parent = node.parent;
      const prop = (key) => parent.properties?.find((p) => ts.isPropertyAssignment(p) && p.name.getText(sf) === key);
      // BINDER.sheets repeat the complete page and are not separate documents.
      if (!(prop('paper') && !prop('transcript') && !prop('id') && !prop('title'))) {
        const source = { ...where(node, 'page'), document: prop('id')?.initializer.getText(sf) ?? prop('title')?.initializer.getText(sf) ?? 'document factory' };
        const values = pageVariants(node.initializer);
        const entry = { ...source, variants: values.length, empty: values.every((v) => !normalize(v)), dynamic: values.some((v) => v.includes(DYNAMIC)) };
        pages.push(entry);
        for (const text of values) {
          // A runtime value makes the entire page unavailable; do not generate
          // a partial reading with its dates or measurements silently removed.
          if (text.includes(DYNAMIC)) continue;
          pageParagraphs(text)
            .forEach((paragraph, index) => add(paragraph, { ...source, paragraph: index + 1 }));
        }
        if (entry.dynamic) dynamic.push({ ...entry, expression: node.initializer.getText(sf) });
      }
    }
    if (helpers[name] && ts.isCallExpression(node) && helpers[name].includes(node.expression.getText(sf))) {
      const source = { ...where(node, 'talk'), helper: node.expression.getText(sf) };
      talkSites.push(source);
      for (const text of variants(node.arguments[0])) {
        if (text.includes(DYNAMIC)) throw new Error(`Unresolved dialogue: ${file}:${source.line}`);
        add(text, source);
      }
    }
    // Room 6's THE LINE uses a literal TalkLine instead of one of the helpers.
    // CB/radio utterances have `by` and are deliberately excluded.
    if (helpers[name] && ts.isObjectLiteralExpression(node)) {
      const prop = (key) => node.properties.find((p) => ts.isPropertyAssignment(p) && p.name.getText(sf) === key);
      const hasBy = node.properties.some((p) => p.name?.getText(sf) === 'by');
      if (prop('who') && prop('text') && !hasBy && !ts.isIdentifier(prop('text').initializer)) {
        const source = { ...where(node, 'talk'), helper: 'literal TalkLine' };
        talkSites.push(source);
        for (const text of variants(prop('text').initializer)) {
          if (text.includes(DYNAMIC)) throw new Error(`Unresolved literal dialogue: ${file}:${source.line}`);
          add(text, source);
        }
      }
    }
    ts.forEachChild(node, walk);
  }
  walk(sf);
}

// Canonical telephone scripts are plain literal data. Read the AST without
// importing game code, and exclude the player's unsaid caption/aside.
{
  const file = 'src/core/phoneScripts.ts';
  const sf = ts.createSourceFile(file, fs.readFileSync(path.join(ROOT, file), 'utf8'), ts.ScriptTarget.Latest, true);
  const source = (node, declaration) => ({ file, line: sf.getLineAndCharacterOfPosition(node.getStart(sf)).line + 1, kind: 'phone', declaration });
  function cue(node) {
    if (ts.isObjectLiteralExpression(node)) {
      const prop = (name) => node.properties.find((p) => ts.isPropertyAssignment(p) && p.name.getText(sf) === name);
      const text = prop('text')?.initializer;
      if (text && prop('aside')?.initializer.kind !== ts.SyntaxKind.TrueKeyword) {
        if (!ts.isStringLiteral(text)) throw new Error(`Nonliteral telephone cue: ${file}:${source(text).line}`);
        const where = source(text, 'PHONE_CALLS');
        phoneSites.push(where); add(text.text, where);
      }
    }
    ts.forEachChild(node, cue);
  }
  let calls = false, texts = false;
  for (const statement of sf.statements) if (ts.isVariableStatement(statement)) {
    for (const declaration of statement.declarationList.declarations) {
      if (declaration.name.getText(sf) === 'PHONE_CALLS') {
        calls = true; cue(declaration.initializer);
      }
      if (declaration.name.getText(sf) === 'PHONE_TEXTS') {
        texts = true;
        if (!ts.isArrayLiteralExpression(declaration.initializer)) throw new Error('PHONE_TEXTS must remain a literal array');
        for (const text of declaration.initializer.elements) {
          if (!ts.isStringLiteral(text)) throw new Error('PHONE_TEXTS must contain only literal speech strings');
          const where = source(text, 'PHONE_TEXTS');
          phoneSites.push(where); add(text.text, where);
        }
      }
    }
  }
  if (!calls || !texts) throw new Error('Missing canonical telephone declarations');
}

// Description roots across the complete runtime source tree. This is a bounded
// literal interpreter, never an import/eval of gameplay code. Unknown runtime
// values remain explicit boundaries; their surrounding words become exact
// fragments for the resolver, while numbers use the small spoken-number bank.
const descriptionSites = [], dynamicDescriptions = [];
const sourceFiles = (dir) => fs.readdirSync(dir, { withFileTypes: true }).flatMap((e) =>
  e.isDirectory() ? sourceFiles(path.join(dir, e.name)) : e.name.endsWith('.ts') ? [path.join(dir, e.name)] : []);
function descriptionReader(sf) {
  const bindings = new Map();
  function index(node) {
    if (ts.isVariableDeclaration(node) && ts.isIdentifier(node.name) && node.initializer) {
      let scope = node.parent;
      while (scope && !ts.isBlock(scope) && !ts.isSourceFile(scope)) scope = scope.parent;
      const values = bindings.get(node.name.text) ?? [];
      values.push({ scope, value: node.initializer }); bindings.set(node.name.text, values);
    }
    ts.forEachChild(node, index);
  }
  index(sf);
  const binding = (node) => (bindings.get(node.text) ?? []).filter(({ scope }) => scope.pos <= node.pos && scope.end >= node.end)
    .sort((a, b) => a.scope.end - a.scope.pos - (b.scope.end - b.scope.pos))[0]?.value;
  function unwrap(node, seen = new Set()) {
    if (!node || seen.has(node)) return null;
    seen = new Set([...seen, node]);
    if (ts.isIdentifier(node)) return unwrap(binding(node), seen);
    if (ts.isParenthesizedExpression(node) || ts.isAsExpression(node) || ts.isSatisfiesExpression(node)) return unwrap(node.expression, seen);
    return node;
  }
  const combine = (a, b) => a.length * b.length > 128 ? [DYNAMIC] : a.flatMap((x) => b.map((y) => x + y));
  function read(node, seen = new Set()) {
    if (!node || seen.has(node) || seen.size > 25) return [DYNAMIC];
    seen = new Set([...seen, node]);
    if (ts.isStringLiteral(node) || ts.isNoSubstitutionTemplateLiteral(node)) return [node.text];
    if (node.kind === ts.SyntaxKind.NullKeyword || node.kind === ts.SyntaxKind.FalseKeyword) return [''];
    if (ts.isIdentifier(node)) return node.text === 'CREDITS' ? [DYNAMIC] : read(binding(node), seen);
    if (ts.isArrayLiteralExpression(node)) return node.elements.flatMap((e) => read(e, seen));
    if (ts.isParenthesizedExpression(node) || ts.isAsExpression(node) || ts.isSatisfiesExpression(node)) return read(node.expression, seen);
    if (ts.isConditionalExpression(node)) return [...read(node.whenTrue, seen), ...read(node.whenFalse, seen)];
    if (ts.isTemplateExpression(node)) {
      let out = [node.head.text];
      for (const span of node.templateSpans) out = combine(out, read(span.expression, seen)).map((s) => s + span.literal.text);
      return out;
    }
    if (ts.isBinaryExpression(node) && node.operatorToken.kind === ts.SyntaxKind.PlusToken) return combine(read(node.left, seen), read(node.right, seen));
    if (ts.isBinaryExpression(node) && [ts.SyntaxKind.QuestionQuestionToken, ts.SyntaxKind.BarBarToken].includes(node.operatorToken.kind)) return [...read(node.left, seen), ...read(node.right, seen)];
    if (ts.isBinaryExpression(node) && node.operatorToken.kind === ts.SyntaxKind.AmpersandAmpersandToken) return ['', ...read(node.right, seen)];
    if (ts.isArrowFunction(node) || ts.isFunctionExpression(node) || ts.isMethodDeclaration(node)) {
      if (!ts.isBlock(node.body)) return read(node.body, seen);
      const out = [];
      function returns(n) { if (ts.isReturnStatement(n) && n.expression) out.push(...read(n.expression, seen)); else if (n === node.body || !ts.isFunctionLike(n)) ts.forEachChild(n, returns); }
      returns(node.body); return out.length ? out : [DYNAMIC];
    }
    if (ts.isPropertyAccessExpression(node) || ts.isElementAccessExpression(node)) {
      const object = unwrap(node.expression);
      const key = ts.isPropertyAccessExpression(node) ? node.name.text : node.argumentExpression && (ts.isStringLiteral(node.argumentExpression) || ts.isNumericLiteral(node.argumentExpression)) ? node.argumentExpression.text : null;
      if (object && ts.isObjectLiteralExpression(object)) return object.properties.filter(ts.isPropertyAssignment)
        .filter((p) => key === null || p.name.getText(sf).replace(/^['"]|['"]$/g, '') === key).flatMap((p) => read(p.initializer, seen));
      if (object && ts.isArrayLiteralExpression(object)) return (key !== null && /^\d+$/.test(key) ? [object.elements[Number(key)]] : [...object.elements]).flatMap((e) => read(e, seen));
      return [DYNAMIC];
    }
    if (ts.isCallExpression(node)) {
      const name = node.expression.getText(sf);
      if (['esc', 'escape', 'pencil', 'say'].includes(name)) return read(node.arguments[0], seen);
      if (ts.isPropertyAccessExpression(node.expression) && node.expression.expression.kind === ts.SyntaxKind.ThisKeyword) {
        let owner = node.parent; while (owner && !ts.isClassDeclaration(owner)) owner = owner.parent;
        const method = owner?.members.find((m) => ts.isMethodDeclaration(m) && m.name.getText(sf) === node.expression.name.text);
        if (method) return read(method, seen);
      }
      if (ts.isPropertyAccessExpression(node.expression) && node.expression.name.text === 'join') {
        const arr = unwrap(node.expression.expression);
        if (arr && ts.isArrayLiteralExpression(arr)) {
          const separator = node.arguments[0] && ts.isStringLiteral(node.arguments[0]) ? node.arguments[0].text : ',';
          return arr.elements.reduce((out, e, i) => combine(out, read(e, seen).map((s) => (i ? separator : '') + s)), ['']);
        }
      }
    }
    return [DYNAMIC];
  }
  return (node) => [...new Set(read(node))];
}
const entities = (s) => s.replace(/&(?:amp|lt|gt|quot|apos|nbsp);/g, (e) => ({'&amp;':'&','&lt;':'<','&gt;':'>','&quot;':'"','&apos;':"'",'&nbsp;':' '})[e]);
function visibleBlocks(html) {
  const bodyParts = [];
  for (const m of html.matchAll(/<(button|ol)\b[^>]*class=["'][^"']*(?:lp-card|lp-steps)[^"']*["'][^>]*>([\s\S]*?)<\/\1>/gi))
    for (const span of m[2].matchAll(/<span\b[^>]*>([\s\S]*?)<\/span>/gi)) bodyParts.push(span[1]);
  const clean = html.replace(/<(button|script|style)\b[^>]*>[\s\S]*?<\/\1>/gi, '');
  const blocks = bodyParts;
  const selected = /<(p|div|span)\b[^>]*class=["'][^"']*(?:lp-text|lp-small|empty|board-how|nb-note|kit-note|person-note)[^"']*["'][^>]*>([\s\S]*?)<\/\1>/gi;
  for (const m of clean.matchAll(selected)) blocks.push(m[2]);
  for (const m of clean.matchAll(/<p\b[^>]*>([\s\S]*?)<\/p>/gi)) blocks.push(m[1]);
  return blocks.flatMap((s) => s.replace(/<br\s*\/?\s*>/gi, '\n\n').replace(/<[^>]*>/g, '').split(/\n\s*\n/)).map(entities);
}
for (const absolute of sourceFiles(path.join(ROOT, 'src')).sort()) {
  const file = path.relative(ROOT, absolute), name = path.basename(file);
  const worldSource = file.startsWith('src/world/');
  if (['DevMenu.ts', 'SaveMenu.ts', 'debug.ts', 'readAloud.ts'].includes(name) || worldSource && !['World.ts','Doors.ts'].includes(name) || /(?:radio|tripRadio|cb|phoneScripts)\.ts$/.test(name)) continue;
  const sf = ts.createSourceFile(file, fs.readFileSync(absolute, 'utf8'), ts.ScriptTarget.Latest, true);
  const read = descriptionReader(sf), emitted = new Set();
  const where = (node, trigger) => ({ file, line: sf.getLineAndCharacterOfPosition(node.getStart(sf)).line + 1, kind: 'description', trigger });
  function emit(node, trigger) {
    if (!node) return;
    for (let owner = node.parent; owner; owner = owner.parent)
      if (ts.isFunctionDeclaration(owner) && /^dev(?:Groups|Start)$/.test(owner.name?.getText(sf) ?? '')) return;
    const key = node.pos + ':' + trigger; if (emitted.has(key)) return; emitted.add(key);
    const source = where(node, trigger), values = read(node);
    descriptionSites.push(source);
    if (values.some((s) => s.includes(DYNAMIC) && /[A-Za-z0-9]/.test(s.replaceAll(DYNAMIC, '')))) dynamicDescriptions.push({ ...source, expression: node.getText(sf) });
    for (let value of values) {
      const blocks = /<\/?[a-z][^>]*>/i.test(value) ? visibleBlocks(value) : trigger === 'page' ? pageParagraphs(value) : value.split(/\n\s*\n/);
      for (const block of blocks) for (const fragment of block.split(DYNAMIC)) {
        const text = normalize(fragment);
        // Punctuation alone has no spoken content. Keep exact letter fragments,
        // including short ones, because dynamic values can adjoin them.
        if (/^[a-z][a-z0-9]*(?:[._][a-z0-9]+)+$/.test(text)) continue;
        if (/^(?:Autosave failed:|That save could not|SIGNAL 47 has been updated|Not here:|No county road seen|No open ground for one|Free drive\. The developer menu)/.test(text)) continue;
        if (/[A-Za-z0-9]/.test(text)) add(text, { ...source, fragment: block.includes(DYNAMIC) });
      }
    }
  }
  function walk(node) {
    if (ts.isCallExpression(node)) {
      const call = node.expression.getText(sf), leaf = ts.isPropertyAccessExpression(node.expression) ? node.expression.name.text : call;
      if (['toast', 'note'].includes(leaf)) emit(node.arguments[0], leaf);
      if (['cap', 'say'].includes(call)) emit(node.arguments[call === 'cap' ? 1 : 0], call);
      if (name === 'Panels.ts' && call === 'paper') emit(node.arguments[0], 'panel-paper');
      if (name === 'looks.ts') {
        if (call === 'looks') emit(node.arguments[1], 'looks');
        if (call === 'road') emit(node.arguments[7], 'road-description');
        if (call === 'motel') emit(node.arguments[1], 'motel-description');
      }
      if (name === 'caliche.ts' && ['once', 'say'].includes(call)) emit(node.arguments[call === 'once' ? 1 : 0], 'town-description');
      if (name === 'Chapter4.ts' && call === 'extra') emit(node.arguments[2], 'motel-description');
      if (name === 'Chapter3.ts' && call === 'frame') { emit(node.arguments[3], 'board-photo-description'); emit(node.arguments[4], 'board-photo-where'); }
      if (name === 'albumSubjects.ts' && call === 'add') emit(node.arguments[0], 'album-subject');
    }
    if (ts.isBinaryExpression(node) && node.operatorToken.kind === ts.SyntaxKind.EqualsToken && ts.isPropertyAccessExpression(node.left) && node.left.name.text === 'examine') emit(node.right, 'assigned-examine');
    if (!worldSource && ts.isPropertyAssignment(node)) {
      const key = node.name.getText(sf).replace(/^['"]|['"]$/g, '');
      if (['text','transcript','page','examine','note','full','empty','hint','hints','detail','caption','recap','description','role','body','odd','closed','where','question','prompt','elsewhere','done','intro','review','extra','sheet'].includes(key) || key === 'state' && name === 'main.ts') emit(node.initializer, key);
      if (name === 'main.ts' && key === 'name') emit(node.initializer, 'inventory-person-name');
      if (name === 'caliche.ts' && key === 'name' && ts.isStringLiteral(node.initializer) && ['The attendant','The deputy','Earl','The woman on the bench','The souvenir man'].includes(node.initializer.text)) emit(node.initializer, 'town-person-name');
      if (name === 'Signal.ts' && key === 'card') emit(node.initializer, 'receiver-refcard');
    }
    // HTML prose is data embedded in templates; take only readable body blocks,
    // not button labels, attributes, CSS, headings or debug strings.
    if ((ts.isStringLiteral(node) || ts.isNoSubstitutionTemplateLiteral(node) || ts.isTemplateExpression(node)) && /(?:lp-text|lp-small|class=["'](?:empty|board-how)|<p\b)/.test(node.getText(sf))) emit(node, 'html-prose');
    if (name === 'Board.ts' && ts.isBinaryExpression(node) && node.operatorToken.kind === ts.SyntaxKind.EqualsToken && node.left.getText(sf) === 'howEl.textContent') emit(node.right, 'board-instruction');
    if (name === 'MapOverlay.ts' && ts.isBinaryExpression(node) && node.operatorToken.kind === ts.SyntaxKind.EqualsToken && node.left.getText(sf) === 'status.textContent') emit(node.right, 'map-status');
    if (name === 'RxConsole.ts' && ts.isBinaryExpression(node) && node.operatorToken.kind === ts.SyntaxKind.EqualsToken && node.left.getText(sf) === 'card.textContent') emit(node.right, 'receiver-refcard');
    // Album subject tuples have a non-prose object key followed by the caption
    // name. Antenna description arrays are likewise displayed indirectly.
    if (ts.isArrayLiteralExpression(node) && name === 'albumSubjects.ts' && node.elements.length >= 2 && ts.isStringLiteral(node.elements[0]) && ts.isStringLiteral(node.elements[1])) emit(node.elements[1], 'album-subject');
    if (ts.isArrayLiteralExpression(node) && name === 'albumSubjects.ts' && node.elements.length >= 2 && ts.isStringLiteral(node.elements[0]) && ts.isConditionalExpression(node.elements[1])) emit(node.elements[1], 'album-subject');
    if (ts.isReturnStatement(node) && node.expression) {
      let owner = node.parent; while (owner && !ts.isFunctionLike(owner)) owner = owner.parent;
      if (owner && ['placeName','rollText','caption','areaMapDescription'].includes(owner.name?.getText(sf))) emit(node.expression, 'description-function');
    }
    if (name === 'areaMap.ts' && ts.isVariableDeclaration(node) && ['MAP_READING','MAP_POSITION'].includes(node.name.getText(sf))) {
      function literals(n) { if (ts.isStringLiteral(n) && !(ts.isPropertyAssignment(n.parent) && n.parent.name === n)) emit(n, 'map-description'); ts.forEachChild(n, literals); }
      literals(node.initializer);
    }
    if (name === 'areaMap.ts' && ts.isCallExpression(node) && node.expression.getText(sf) === 'text.push') node.arguments.forEach((e) => emit(e, 'map-description'));
    if (name === 'looks.ts' && ts.isVariableDeclaration(node) && node.name.getText(sf) === 'DISH' && node.initializer && ts.isArrayLiteralExpression(node.initializer)) node.initializer.elements.forEach((e) => emit(e, 'antenna-description'));
    if (name === 'Panels.ts' && ts.isVariableDeclaration(node) && node.name.getText(sf) === 'steps' && node.initializer && ts.isArrayLiteralExpression(node.initializer))
      node.initializer.elements.filter(ts.isArrayLiteralExpression).forEach((e) => emit(e.elements[1], 'wet-bench-step'));
    if (ts.isVariableDeclaration(node) && ['placeName','rollText','NOBODY'].includes(node.name.getText(sf))) emit(node.initializer, 'description-data');
    if (name === 'nightshift.ts' && ts.isVariableDeclaration(node) && node.name.getText(sf) === 'TELEX_EVENING') emit(node.initializer, 'telex-paper');
    if (name === 'album.ts' && file.startsWith('src/core/') && ts.isVariableDeclaration(node) && node.name.getText(sf) === 'main') emit(node.initializer, 'album-caption');
    ts.forEachChild(node, walk);
  }
  walk(sf);
}
const numberWords = ['zero','one','two','three','four','five','six','seven','eight','nine','ten','eleven','twelve','thirteen','fourteen','fifteen','sixteen','seventeen','eighteen','nineteen','twenty','thirty','forty','fifty','sixty','seventy','eighty','ninety','hundred','thousand','million','minus','plus','point','percent'];
for (const text of numberWords) add(text, { file: 'tools/kinski/extract.mjs', kind: 'number', purpose: 'Complete runtime numeric and clock pronunciation' });
const identifierWords = [...'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'frame'];
for (const text of identifierWords) add(text, { file: 'tools/kinski/extract.mjs', kind: 'identifier', purpose: 'Exact short game identifier prefix or letter pronunciation' });

const items = [...clips.values()];
const report = {
  version: 1,
  segmentation: 'Full normalized text first. Documents and plain description data split at blank paragraphs; page-only pencil markers and separator lines are removed. Narrative descriptions include displayed toast/note, examine, panel prose, board facts/hints, notebook names/roles/state/notes, album captions, map prose and chapter recaps. Finite branches are literal variants. Runtime templates retain every static fragment, resolved as a complete sequence with cardinal/decimal/clock words and strict identifier letters. No skipping unknown words. Telephone uses exact PHONE_CALLS speech except aside and PHONE_TEXTS. Button labels, development menu, credits and broadcast radio scripts are excluded.',
  clips: items,
  summary: {
    clipCount: items.length,
    documentClipCount: items.filter((item) => item.sources.some((s) => s.kind === 'document')).length,
    pageClipCount: items.filter((item) => item.sources.some((s) => s.kind === 'page')).length,
    talkClipCount: items.filter((item) => item.sources.some((s) => s.kind === 'talk')).length,
    phoneClipCount: items.filter((item) => item.sources.some((s) => s.kind === 'phone')).length,
    descriptionClipCount: items.filter((item) => item.sources.some((s) => s.kind === 'description')).length,
    numberClipCount: numberWords.length,
    identifierClipCount: identifierWords.length,
    words: items.reduce((sum, item) => sum + item.text.split(/\s+/).length, 0),
    characters: items.reduce((sum, item) => sum + item.text.length, 0),
    transcriptDeclarations: documents.length,
    pageDeclarations: pages.length,
    staticPageVariants: pages.filter((p) => !p.dynamic && !p.empty).reduce((sum, p) => sum + p.variants, 0),
    talkSourceSites: talkSites.length,
    phoneSourceSites: phoneSites.length,
  },
  documents,
  pages,
  phoneSites,
  descriptionSites,
  dynamicDescriptions,
  numberWords,
  identifierWords,
  dynamicDocumentTemplates: dynamic.map((source) => ({ ...source, resolution: 'Exact source fragments plus complete runtime number/clock/identifier composition; runtime rejects any unresolved words.' })),
  unavailableDynamicDocuments: [],
  availabilityPolicy: 'No blanket exclusion for dynamic documents. Runtime first uses exact whole-text clips, then only a complete sequence of exact fragments and spoken numeric/identifier components. Any unknown word makes the whole requested block unavailable. No silent omission or unrestricted word synthesis.',
};
fs.mkdirSync(OUT, { recursive: true });
fs.writeFileSync(path.join(OUT, 'corpus.json'), JSON.stringify(report, null, 2) + '\n');
console.log(JSON.stringify({ ...report.summary, dynamicSourceSites: dynamic.map(({file,line,document}) => ({file,line,document})) }, null, 2));
