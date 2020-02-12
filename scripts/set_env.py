import sys
import json

env = ["SCRATCH_PATH", "CHUNKMAP_PATH", "AFF_PATH", "AFF_MIP", "WS_PATH", "SEG_PATH", "WS_HIGH_THRESHOLD", "WS_LOW_THRESHOLD", "WS_SIZE_THRESHOLD", "AGG_THRESHOLD", "GT_PATH"]

with open(sys.argv[1]) as f:
    data = json.load(f)

for s in ["SCRATCH", "WS", "SEG"]:
    prefix = "{}_PREFIX".format(s)
    path = "{}_PATH".format(s)
    if path not in data:
         data[path] = data[prefix]+data["NAME"]

if "CHUNKMAP_PATH" not in data:
    data["CHUNKMAP_PATH"] = data["SCRATCH_PATH"]+"/chunkmap"

for e in env:
    if e in data:
        print('export {}="{}"'.format(e, data[e]))
