# Disassembles guest code from the dumped image: python -I tools\dis.py image.bin <addr> [count]
import struct, sys
from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN
BASE = 0x92000000
img = open(sys.argv[1], 'rb').read()
addr, n = int(sys.argv[2], 16), int(sys.argv[3]) if len(sys.argv) > 3 else 32
md = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)
for i in range(n):
    a = addr + 4 * i
    raw = img[a - BASE:a - BASE + 4]
    insn = next(md.disasm(raw, a), None)
    print(f'{a:08X}  {raw.hex()}  {insn.mnemonic + " " + insn.op_str if insn else "?"}')
