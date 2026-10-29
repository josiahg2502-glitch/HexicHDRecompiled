# Extracts an Xbox 360 STFS package (CON/LIVE/PIRS) to a folder.
# usage: python -I tools\stfs.py <package> <out_dir>
# shortcut: reads files as contiguous block runs only (fails loudly otherwise); add hash-chain walking if a package needs it.
import os, struct, sys

def u24le(b): return b[0] | b[1] << 8 | b[2] << 16

def main(pkg, out):
    d = open(pkg, 'rb').read()
    assert d[:4] in (b'CON ', b'LIVE', b'PIRS'), 'not an STFS package'
    header_size = struct.unpack_from('>I', d, 0x340)[0]
    vd = d[0x379:0x379 + 0x24]
    read_only = vd[2] & 1
    ft_blocks = struct.unpack_from('<H', vd, 3)[0]
    ft_start = u24le(vd[5:8])
    base_off = (header_size + 0xFFF) & ~0xFFF
    shift = 0 if read_only else 1

    def block_off(i):
        blk, base = i, 0xAA
        for _ in range(3):
            blk += ((i + base) // base) << shift
            if i < base: break
            base *= 0xAA
        return base_off + (blk << 12)

    def read_run(start, count, size=None):
        data = b''.join(d[block_off(start + k):block_off(start + k) + 0x1000] for k in range(count))
        return data if size is None else data[:size]

    table = read_run(ft_start, ft_blocks)
    entries = []
    for o in range(0, len(table), 0x40):
        e = table[o:o + 0x40]
        n = e[0x28] & 0x3F
        if n == 0: break
        entries.append(dict(name=e[:n].decode('ascii'), dir=bool(e[0x28] & 0x80), contig=bool(e[0x28] & 0x40),
                            blocks=u24le(e[0x2C:0x2F]), start=u24le(e[0x2F:0x32]),
                            parent=struct.unpack_from('>h', e, 0x32)[0], size=struct.unpack_from('>I', e, 0x34)[0]))

    def path(i):
        p = []
        while i != -1:
            p.append(entries[i]['name']); i = entries[i]['parent']
        return os.path.join(out, *reversed(p))

    for i, e in enumerate(entries):
        p = path(i)
        if e['dir']:
            os.makedirs(p, exist_ok=True); continue
        os.makedirs(os.path.dirname(p), exist_ok=True)
        if not e['contig'] and e['blocks'] > 1:
            sys.exit(f"{e['name']}: non-contiguous file, needs hash-chain support")
        with open(p, 'wb') as f:
            f.write(read_run(e['start'], (e['size'] + 0xFFF) >> 12, e['size']))
        print(f"{e['size']:>10}  {os.path.relpath(p, out)}")

if __name__ == '__main__':
    main(*sys.argv[1:3])
