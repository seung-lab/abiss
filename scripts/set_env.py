import os
import sys
import json

env = ["SCRATCH_PATH", "CHUNKMAP_INPUT", "CHUNKMAP_OUTPUT", "AFF_PATH", "AFF_MIP", "SEM_PATH", "SEM_MIP", "WS_PATH", "SEG_PATH", "WS_HIGH_THRESHOLD", "WS_LOW_THRESHOLD", "WS_SIZE_THRESHOLD", "AGG_THRESHOLD", "GT_PATH", "CLEFT_PATH", "MYELIN_THRESHOLD", "ADJUSTED_AFF_PATH", "CHUNKED_AGG_OUTPUT", "CHUNKED_SEG_PATH", "REDIS_SERVER", "REDIS_DB", "STATSD_HOST", "STATSD_PORT", "STATSD_PREFIX", "PARANOID", "BOTO_CONFIG"]

with open(sys.argv[1]) as f:
    data = json.load(f)

data["STATSD_PREFIX"] = data["NAME"]

for s in ["SCRATCH", "WS", "SEG"]:
    prefix = "{}_PREFIX".format(s)
    path = "{}_PATH".format(s)
    if path not in data:
        data[path] = data[prefix]+data["NAME"]

if "CHUNKMAP_INPUT" not in data:
    data["CHUNKMAP_INPUT"] = os.path.join(data["SCRATCH_PATH"], "ws", "chunkmap")

if data.get("CHUNKED_AGG_OUTPUT", False):
    data["CHUNKED_AGG_OUTPUT"] = 1
else:
    data["CHUNKED_AGG_OUTPUT"] = 0

if data.get("PARANOID", False):
    data["PARANOID"] = 1
else:
    data["PARANOID"] = 0

if "gsutil-secret.json" in data.get("MOUNT_SECRETS", []):
    data["BOTO_CONFIG"]="~/gsutil.boto"

for e in env:
    if e in data:
        print('export {}="{}"'.format(e, data[e]))
