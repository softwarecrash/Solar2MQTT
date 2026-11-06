'use strict';

// Keep offsets while hiding C++ comments and literals. Braces inside a log
// message, character, raw string or comment must not terminate a function.
function maskLiterals(source) {
  const output = source.split('');
  const hide = (start, end) => {
    for (let i = start; i < end; ++i) if (source[i] !== '\n') output[i] = ' ';
  };
  for (let i = 0; i < source.length;) {
    if (source.startsWith('//', i)) {
      const end = source.indexOf('\n', i);
      const next = end < 0 ? source.length : end;
      hide(i, next); i = next; continue;
    }
    if (source.startsWith('/*', i)) {
      const end = source.indexOf('*/', i + 2);
      if (end < 0) throw Error('Unterminated C++ comment');
      hide(i, end + 2); i = end + 2; continue;
    }
    const raw = source.slice(i).match(/^R"([^\s()\\]{0,16})\(/);
    if (raw) {
      const closing = ')' + raw[1] + '"';
      const end = source.indexOf(closing, i + raw[0].length);
      if (end < 0) throw Error('Unterminated C++ raw string');
      hide(i, end + closing.length); i = end + closing.length; continue;
    }
    const quote = source[i];
    const digitSeparator = quote === "'" && /[\w]/.test(source[i - 1] || '') && /[\w]/.test(source[i + 1] || '');
    if ((quote === '"' || quote === "'") && !digitSeparator) {
      let end = i + 1;
      while (end < source.length && source[end] !== quote) end += source[end] === '\\' ? 2 : 1;
      if (end >= source.length) throw Error('Unterminated C++ literal');
      hide(i, end + 1); i = end + 1; continue;
    }
    ++i;
  }
  return output.join('');
}

function balancedEnd(text, start) {
  const closing = { '(': ')', '{': '}', '[': ']' }[text[start]];
  if (!closing) throw Error('Expected a C++ opening delimiter');
  let depth = 1;
  for (let i = start + 1; i < text.length; ++i) {
    if (text[i] === text[start]) ++depth;
    if (text[i] === closing) {
      --depth;
      if (depth === 0) return i + 1;
    }
  }
  throw Error('Unbalanced C++ delimiters');
}

function extract(source, signature) {
  const masked = maskLiterals(source);
  const escaped = signature.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  const matches = [...masked.matchAll(new RegExp('^[ \\t]*' + escaped + '\\s*\\(', 'gm'))];
  if (matches.length !== 1) throw Error((matches.length ? 'Ambiguous' : 'Missing') + ' production function ' + signature);
  const start = matches[0].index + matches[0][0].indexOf(signature);
  let cursor = balancedEnd(masked, masked.indexOf('(', start));
  const whitespace = () => { while (/\s/.test(masked[cursor] || '') && cursor < masked.length) ++cursor; };
  while (cursor < masked.length) {
    whitespace();
    if (masked[cursor] === ';') throw Error('Missing production function body ' + signature);
    if (masked[cursor] === '{') return source.slice(start, balancedEnd(masked, cursor));
    if (masked[cursor] === ':' && masked[cursor + 1] !== ':' && masked[cursor - 1] !== ':') {
      // Constructors may have parenthesized or braced member initializers.
      ++cursor;
      for (;;) {
        while (cursor < masked.length && !'({'.includes(masked[cursor])) ++cursor;
        cursor = balancedEnd(masked, cursor);
        whitespace();
        if (masked[cursor] !== ',') break;
        ++cursor;
      }
      continue;
    }
    if ('(['.includes(masked[cursor])) cursor = balancedEnd(masked, cursor);
    else ++cursor;
  }
  throw Error('Missing production function body ' + signature);
}

module.exports = { extract, maskLiterals };
