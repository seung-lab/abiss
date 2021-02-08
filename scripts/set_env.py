import os
import sys
import json

env = ["SCRATCH_PATH", "CHUNKMAP_INPUT", "CHUNKMAP_OUTPUT", "AFF_PATH", "AFF_MIP", "SEM_PATH", "SEM_MIP", "WS_PATH", "SEG_PATH", "WS_HIGH_THRESHOLD", "WS_LOW_THRESHOLD", "WS_SIZE_THRESHOLD", "AGG_THRESHOLD", "GT_PATH", "MYELIN_THRESHOLD", "ADJUSTED_AFF_PATH"]

with open(sys.argv[1]) as f:
    data = json.load(f)

for s in ["SCRATCH", "WS", "SEG"]:
    prefix = "{}_PREFIX".format(s)
    path = "{}_PATH".format(s)
    if path not in data:
         data[path] = data[prefix]+data["NAME"]

if "CHUNKMAP_INPUT" not in data:
    data["CHUNKMAP_INPUT"] = os.path.join(data["SCRATCH_PATH"], "ws", "chunkmap")

for e in env:
    if e in data:
        print('export {}="{}"'.format(e, data[e]))
