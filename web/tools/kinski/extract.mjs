#!/usr/bin/env node
// Extract only the English text displayed by UI.document and Panels.talk.
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

const clips = new Map(), dynamic = [], documents = [], pages = [], talkSites = [];
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

const items = [...clips.values()];
const report = {
  version: 1,
  segmentation: 'Documents: transcript.split(/\\n\\s*\\n/), then whitespace normalization. Page first removes line-leading pencil markers (~ plus whitespace) and whole separator lines (at least three - or =, optionally spaced), then the same paragraphs. Talk: each lines[].text, same normalization. No titles, choices, status, reply or radio.',
  clips: items,
  summary: {
    clipCount: items.length,
    documentClipCount: items.filter((item) => item.sources.some((s) => s.kind === 'document')).length,
    pageClipCount: items.filter((item) => item.sources.some((s) => s.kind === 'page')).length,
    talkClipCount: items.filter((item) => item.sources.some((s) => s.kind === 'talk')).length,
    words: items.reduce((sum, item) => sum + item.text.split(/\s+/).length, 0),
    characters: items.reduce((sum, item) => sum + item.text.length, 0),
    transcriptDeclarations: documents.length,
    pageDeclarations: pages.length,
    staticPageVariants: pages.filter((p) => !p.dynamic && !p.empty).reduce((sum, p) => sum + p.variants, 0),
    talkSourceSites: talkSites.length,
  },
  documents,
  pages,
  unavailableDynamicDocuments: dynamic,
  availabilityPolicy: 'Runtime requires every paragraph/line in a requested reading to be present. Dynamic documents remain unavailable rather than silently skipping text.',
};
fs.mkdirSync(OUT, { recursive: true });
fs.writeFileSync(path.join(OUT, 'corpus.json'), JSON.stringify(report, null, 2) + '\n');
console.log(JSON.stringify({ ...report.summary, dynamicSourceSites: dynamic.map(({file,line,document}) => ({file,line,document})) }, null, 2));
