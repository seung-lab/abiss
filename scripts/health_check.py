import sys
import json
import requests
import psutil
import redis
from datetime import datetime

METADATA_URL = "http://metadata.google.internal/computeMetadata/v1/instance/"
METADATA_HEADERS = {'Metadata-Flavor': 'Google'}

def get_hostname():
    data = requests.get(METADATA_URL + 'hostname', headers=METADATA_HEADERS).text
    return data.split(".")[0]


with open("/run/secrets/param") as f:
    data = json.load(f)
    if "REDIS_SERVER" in data:
        hostname = get_hostname()
        r = redis.Redis(host=data["REDIS_SERVER"])
        r.set(hostname, datetime.now().timestamp())


cpu_percent = sum(psutil.cpu_percent(5, percpu=True))

if cpu_percent > 5:
    sys.exit(0)
else:
    sys.exit(1)
