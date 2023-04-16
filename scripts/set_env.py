import os
import sys
import json
import cloudfiles.paths


def default_io_cmd(path):
    try:
        d = cloudfiles.paths.extract(path)
    except:
        return "cloudfiles cp -r"

    if d.protocol == "gs":
        return "gsutil -m cp -r"
    elif d.protocol == "s3":
        if d.host:
            return f"s5cmd --endpoint-url {d.host} cp"
        else:
            return "s5cmd cp"
    else:
        return "cloudfiles cp -r"


env = ["SCRATCH_PATH", "CHUNKMAP_INPUT", "CHUNKMAP_OUTPUT", "AFF_PATH", "AFF_MIP", "SEM_PATH", "SEM_MIP", "WS_PATH", "SEG_PATH", "WS_HIGH_THRESHOLD", "WS_LOW_THRESHOLD", "WS_SIZE_THRESHOLD", "AGG_THRESHOLD", "GT_PATH", "CLEFT_PATH", "MYELIN_THRESHOLD", "ADJUSTED_AFF_PATH", "CHUNKED_AGG_OUTPUT", "CHUNKED_SEG_PATH", "REDIS_SERVER", "REDIS_DB", "STATSD_HOST", "STATSD_PORT", "STATSD_PREFIX", "PARANOID", "BOTO_CONFIG", "UPLOAD_CMD", "DOWNLOAD_CMD", "IO_SCRATCH_PATH"]

with open(sys.argv[1]) as f:
    data = json.load(f)

data["STATSD_PREFIX"] = data["NAME"]

for s in ["SCRATCH", "WS", "SEG"]:
    prefix = "{}_PREFIX".format(s)
    path = "{}_PATH".format(s)
    if path not in data:
        data[path] = data[prefix]+data["NAME"]

if "UPLOAD_CMD" not in data:
    data["UPLOAD_CMD"] = data.get("DOWNLOAD_CMD", default_io_cmd(data["SCRATCH_PATH"]))

if "DOWNLOAD_CMD" not in data:
    data["DOWNLOAD_CMD"] = data.get("UPLOAD_CMD", default_io_cmd(data["SCRATCH_PATH"]))

if data["UPLOAD_CMD"].startswith("cloudfiles") and data["DOWNLOAD_CMD"].startswith("cloudfiles"):
    data["IO_SCRATCH_PATH"] = data["SCRATCH_PATH"]
else:
    extracted_path = cloudfiles.paths.extract(data["SCRATCH_PATH"])
    if extracted_path.alias or extracted_path.host:
        data["IO_SCRATCH_PATH"] = cloudfiles.paths.asprotocolpath(extracted_path._replace(alias=None, host=None))
    else:
        data["IO_SCRATCH_PATH"] = data["SCRATCH_PATH"]

if "CHUNKMAP_OUTPUT" in data:
    d = cloudfiles.paths.extract(data["CHUNKMAP_OUTPUT"])
    data["CHUNKMAP_OUTPUT"] = cloudfiles.paths.asprotocolpath(d._replace(alias=None, host=None))

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
    data["BOTO_CONFIG"] = "~/gsutil.boto"

for e in env:
    if e in data:
        print('export {}="{}"'.format(e, data[e]))
