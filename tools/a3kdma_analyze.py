#!/usr/bin/env python3
"""Classify an A3000 SDMAC DMA Transfer Test buffer dump (DiagROM menu 6, key D).

Feed it a minicom/serial log containing the "A3KDMA BEGIN" .. "A3KDMA END" block:

    python3 tools/a3kdma_analyze.py /tmp/diagrom.log [--map]

For every DMA destination row it compares each 32-bit longword with the PIO
reference and reports: lane damage, how many longwords are intact / half
written / untouched (poison = bytewise complement of the reference, which the
test pre-fills) / displaced (holding data that belongs further along the
stream), where the engine lost beats and how those losses are spaced.
"""
import re, sys
from collections import Counter

ANSI = re.compile(r'\x1b\[[0-9;?]*[A-Za-z]')
LINE = re.compile(r'^(REF|ROW\d) \$([0-9A-Fa-f]{8}):([0-9A-Fa-f]+)\s*$')
HDR  = re.compile(r'^(ROW\d) label=(.*?) buf=(\$[0-9A-Fa-f]+) acrend=(\d+)')

def parse(text):
    text = ANSI.sub('', text).replace('\r', '')
    bufs, meta = {}, {}
    inside = False
    for raw in text.split('\n'):
        line = raw.strip()
        if line.startswith('A3KDMA BEGIN'):
            inside = True; bufs, meta = {}, {}; continue   # keep the LAST dump in the log
        if line.startswith('A3KDMA END'):
            inside = False; continue
        if not inside: continue
        m = HDR.match(line)
        if m:
            meta[m.group(1)] = dict(label=m.group(2).strip(), buf=m.group(3), acrend=int(m.group(4)))
            continue
        m = LINE.match(line)
        if m:
            tag, off, hx = m.group(1), int(m.group(2), 16), m.group(3)
            b = bufs.setdefault(tag, bytearray(4096))
            data = bytes.fromhex(hx)
            b[off:off+len(data)] = data
    return bufs, meta

def lw(b, i): return int.from_bytes(b[i*4:i*4+4], 'big')

def analyze(ref, got, show_map=False):
    n = len(ref) // 4
    poison = bytes((~x) & 0xFF for x in ref)
    lanes = [0, 0, 0, 0]
    for i, (e, g) in enumerate(zip(ref, got)):
        if e != g: lanes[i & 3] += 1
    cls = []          # per longword: ok | upper-poison | lower-poison | poison | disp+k | other
    disp = {}
    for i in range(n):
        e, g = ref[i*4:i*4+4], got[i*4:i*4+4]
        p = poison[i*4:i*4+4]
        if g == e: cls.append('ok'); continue
        if g == p: cls.append('poison'); continue
        if g[:2] == p[:2] and g[2:] == e[2:]: cls.append('upper-poison'); continue
        if g[2:] == p[2:] and g[:2] == e[:2]: cls.append('lower-poison'); continue
        found = None
        for k in range(1, n - i):       # data from k longwords later landed here?
            if i + k < n and g == ref[(i+k)*4:(i+k)*4+4] and ref[(i+k)*4:(i+k)*4+4] != e:
                found = k; break
        if found:
            cls.append(f'disp+{found}'); disp[i] = found; continue
        # half-displaced: upper word poison, lower word from further along
        if g[:2] == p[:2]:
            for k in range(1, n - i):
                if i + k < n and g[2:] == ref[(i+k)*4+2:(i+k)*4+4]:
                    found = k; break
            if found: cls.append(f'upper-poison/lower-disp+{found}'); continue
        cls.append('other')
    c = Counter(x.split('/')[0].split('+')[0] if not x.startswith('upper-poison/') else 'upper-poison+lower-disp' for x in cls)
    print(f'  bytes differing by lane (offset&3): {lanes[0]}/{lanes[1]}/{lanes[2]}/{lanes[3]}')
    print(f'  longwords: ' + ', '.join(f'{k} {v}' for k, v in c.most_common()))
    # lost beats: where the displacement increases
    if disp:
        steps = []
        prev = 0
        for i in range(n):
            k = disp.get(i)
            if k is not None and k > prev:
                steps.append((i, k - prev)); prev = k
        print(f'  displacement reaches +{prev} longwords by the end; it grows at {len(steps)} places:')
        gaps = [b[0] - a[0] for a, b in zip(steps, steps[1:])]
        print('    first steps at LW ' + ', '.join(f'{i}(+{d})' for i, d in steps[:24]) + (' ...' if len(steps) > 24 else ''))
        if gaps:
            print(f'    spacing between steps: min {min(gaps)} max {max(gaps)} most common {Counter(gaps).most_common(5)}')
    runs = []
    cur, start = None, 0
    for i, x in enumerate(cls + [None]):
        key = None if x is None else x.split('+')[0]
        if key != cur:
            if cur is not None: runs.append((cur, start, i - start))
            cur, start = key, i
    bad_runs = [r for r in runs if r[0] != 'ok']
    print(f'  {len(runs)} runs; bad-run lengths: {Counter(r[2] for r in bad_runs).most_common(8)}')
    ok_runs = [r for r in runs if r[0] == 'ok']
    print(f'  intact-run lengths: {Counter(r[2] for r in ok_runs).most_common(8)}')
    if show_map:
        sym = {'ok': '.', 'poison': 'P', 'upper-poison': 'u', 'lower-poison': 'l', 'other': '?'}
        line = ''.join(sym.get(x.split('+')[0], 'd' if x.startswith('disp') else 'U') for x in cls)
        print('  map (one char per longword, 64 per line; . ok  u upper-word-poison  l lower  P all-poison  d displaced  U upper-poison+displaced-lower  ? other):')
        for i in range(0, n, 64):
            print(f'    {i*4:05X} {line[i:i+64]}')

def main():
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(2)
    text = open(sys.argv[1], encoding='latin-1').read()
    bufs, meta = parse(text)
    if 'REF' not in bufs:
        print('no A3KDMA dump with a REF buffer found in the log'); sys.exit(1)
    ref = bytes(bufs['REF'])
    for tag in sorted(t for t in bufs if t.startswith('ROW')):
        m = meta.get(tag, {})
        print(f"{tag} {m.get('label','')} buf={m.get('buf','?')} acrend={m.get('acrend','?')}")
        analyze(ref, bytes(bufs[tag]), '--map' in sys.argv)
        print()

if __name__ == '__main__':
    main()
