import os
import sys
import json

env = ["SCRATCH_PATH","AFF_PATH","AFF_MIP","WS_PATH", "WS_MIP", "SEG_PATH","SEG_MIP", "WS_HIGH_THRESHOLD", "WS_LOW_THRESHOLD", "WS_SIZE_THRESHOLD", "AGG_THRESHOLD"]

with open(sys.argv[1]) as f:
    data = json.load(f)

for e in env:
    if e in data:
        print('export {}="{}"'.format(e, data[e]))
