import os
import sys

from cloudfiles import CloudFiles

try:
    import redis
except ImportError:
    redis = None


TASK_KEY = sys.argv[1]

done = False
if redis is not None and "REDIS_SERVER" in os.environ and "REDIS_DB" in os.environ:
    try:
        r = redis.Redis(host=os.environ["REDIS_SERVER"], db=int(os.environ["REDIS_DB"]))
        if r.exists(TASK_KEY) and r.get(TASK_KEY).decode() == "DONE":
            done = True
    except Exception:
        pass

if not done:
    cf = CloudFiles(os.environ["SCRATCH_PATH"])
    if cf.exists(f"done/{TASK_KEY}.txt"):
        done = True

if not done:
    sys.exit(1)
