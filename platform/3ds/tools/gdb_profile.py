#!/usr/bin/env python3
"""Statistical CPU profile of the 3DS game through a GDB remote stub.

Azahar's stub (Emulation > Configure > Debug > Enable GDB stub, port 24689) or
Luma3DS's (Rosalina menu > Debugger options) both work. `sample` interrupts
the game every few milliseconds and records the stopped thread's PC and LR
and, with --elf, the step of the frame the game thread marked last
(game/trace_3ds.c's s_stage). `report` names the functions, per step.
Emulator numbers are guest instructions (Azahar's tick is about one), not
hardware time.

  gdb_profile.py sample HOST:PORT SECONDS OUT [--interval S] [--warmup S] [--elf ELF]
  gdb_profile.py report ELF SAMPLES [--top N]
"""
import argparse
import collections
import os
import socket
import subprocess
import time

DEVKITARM = os.environ.get("DEVKITARM", "/opt/devkitpro/devkitARM")
ADDR2LINE = os.path.join(DEVKITARM, "bin", "arm-none-eabi-addr2line")
NM = os.path.join(DEVKITARM, "bin", "arm-none-eabi-nm")
STAGE_SYMBOL = "s_stage"
STAGE_NAME_BYTES = 48
REG_HEX = 8  # one 32-bit register in a 'g' reply
LR, PC = 14, 15
CONNECT_WAIT_S = 30


class Stub:
    """The few GDB remote packets a sampler needs."""

    def __init__(self, host, port, wait_s=CONNECT_WAIT_S):
        deadline = time.time() + wait_s
        while True:  # the emulator opens its stub a few seconds after launch
            try:
                self.sock = socket.create_connection((host, port), timeout=10)
                break
            except OSError:
                if time.time() > deadline:
                    raise
                time.sleep(0.5)
        self.buf = b""

    def packet(self):
        while True:
            start = self.buf.find(b"$")
            end = self.buf.find(b"#", start) if start >= 0 else -1
            if end >= 0 and len(self.buf) >= end + 3:
                data = self.buf[start + 1:end]
                self.buf = self.buf[end + 3:]
                self.sock.sendall(b"+")
                return data
            chunk = self.sock.recv(65536)
            if not chunk:
                raise EOFError("the stub closed the connection")
            self.buf += chunk

    def send(self, data):
        self.sock.sendall(b"$" + data + b"#" + b"%02x" % (sum(data) & 0xFF))

    def command(self, data):
        self.send(data)
        return self.packet()

    def interrupt(self):
        self.sock.sendall(b"\x03")
        return self.packet()

    def read_u32(self, addr):
        return le32(self.command(b"m%x,4" % addr).decode())

    def read_cstr(self, addr, size):
        raw = bytes.fromhex(self.command(b"m%x,%x" % (addr, size)).decode())
        return raw.split(b"\0")[0].decode(errors="replace")


def le32(hexstr):
    return int.from_bytes(bytes.fromhex(hexstr), "little")


def symbol_address(elf, name):
    out = subprocess.run([NM, elf], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == name:
            return int(parts[0], 16)
    raise SystemExit("%s: no symbol %s" % (elf, name))


def sample(args):
    host, port = args.target.rsplit(":", 1)
    stage_addr = symbol_address(args.elf, STAGE_SYMBOL) if args.elf else None
    stub = Stub(host, int(port))
    stub.command(b"?")
    stub.send(b"c")
    time.sleep(args.warmup)
    names, rows = {}, []
    end = time.time() + args.seconds
    while time.time() < end:
        time.sleep(args.interval)
        stub.interrupt()
        regs = stub.command(b"g").decode()
        if len(regs) >= (PC + 1) * REG_HEX:
            stage = "-"
            if stage_addr is not None:
                ptr = stub.read_u32(stage_addr)
                if ptr not in names:
                    names[ptr] = stub.read_cstr(ptr, STAGE_NAME_BYTES).replace(" ", "_") or "-"
                stage = names[ptr]
            rows.append((le32(regs[PC * REG_HEX:(PC + 1) * REG_HEX]), le32(regs[LR * REG_HEX:(LR + 1) * REG_HEX]), stage))
        stub.send(b"c")
    with open(args.out, "w") as f:
        for pc, lr, stage in rows:
            f.write("%08x %08x %s\n" % (pc, lr, stage))
    print("%d samples -> %s" % (len(rows), args.out))


def symbolize(elf, addrs):
    out = subprocess.run([ADDR2LINE, "-f", "-e", elf] + ["0x" + a for a in addrs],
                         capture_output=True, text=True, check=True).stdout.split("\n")
    return {a: (out[2 * i], out[2 * i + 1].split("/")[-1].split(":")[0]) for i, a in enumerate(addrs)}


def report(args):
    rows = [line.split() for line in open(args.samples) if line.strip()]
    names = symbolize(args.elf, sorted({r[0] for r in rows}))
    by_stage = collections.defaultdict(list)
    for r in rows:
        by_stage[r[2] if len(r) > 2 else "-"].append(r)
    print("%d samples" % len(rows))
    for stage, stage_rows in sorted(by_stage.items(), key=lambda kv: -len(kv[1])):
        print("=== %s: %d samples (%.0f%%)" % (stage, len(stage_rows), 100.0 * len(stage_rows) / len(rows)))
        counts = collections.Counter("%s  [%s]" % names[r[0]] for r in stage_rows)
        for name, n in counts.most_common(args.top):
            print("   %5.1f%% %4d %s" % (100.0 * n / len(stage_rows), n, name))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    s = sub.add_parser("sample")
    s.add_argument("target", help="HOST:PORT of the GDB stub")
    s.add_argument("seconds", type=float)
    s.add_argument("out")
    s.add_argument("--interval", type=float, default=0.05)
    s.add_argument("--warmup", type=float, default=0.0, help="seconds to let the game run first")
    s.add_argument("--elf", help="build/3ds/ac3ds.elf: tag samples with the frame step")
    r = sub.add_parser("report")
    r.add_argument("elf")
    r.add_argument("samples")
    r.add_argument("--top", type=int, default=15)
    args = parser.parse_args()
    (sample if args.command == "sample" else report)(args)


if __name__ == "__main__":
    main()
