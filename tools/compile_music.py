#!/usr/bin/env python3
"""Compile a .uge tracker file to C via uge2source and patch empty arrays for SDCC."""
import sys
import subprocess
import re

if len(sys.argv) < 5:
    print("Usage: compile_music.py <uge_file> <bank> <symbol> <out_c>")
    sys.exit(1)

uge_file, bank, symbol, out_c = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]

subprocess.run(["uge2source", uge_file, "-b", str(bank), symbol, out_c], check=True)

with open(out_c, "r", encoding="utf-8") as f:
    src = f.read()

# SDCC error 286: empty initializer on array of unknown size
src = re.sub(r"static const hUGENoiseInstr_t noise_instruments\[\] = \{\s*\};",
              "static const hUGENoiseInstr_t noise_instruments[1] = {{0, 0, 0, 0, 0}};", src)
src = re.sub(r"static const hUGEDutyInstr_t duty_instruments\[\] = \{\s*\};",
              "static const hUGEDutyInstr_t duty_instruments[1] = {{0, 0, 0, 0, 0}};", src)
src = re.sub(r"static const hUGEWaveInstr_t wave_instruments\[\] = \{\s*\};",
              "static const hUGEWaveInstr_t wave_instruments[1] = {{0, 0, 0, 0, 0}};", src)

with open(out_c, "w", encoding="utf-8") as f:
    f.write(src)
