import os
import sys

from cloudfiles import CloudFiles

try:
    import redis
except ImportError:
    redis = None


TASK_KEY = sys.argv[1]
STATE = sys.argv[2]

updated = False
if redis is not None and "REDIS_SERVER" in os.environ and "REDIS_DB" in os.environ:
    try:
        r = redis.Redis(host=os.environ["REDIS_SERVER"], db=int(os.environ["REDIS_DB"]))
        r.set(TASK_KEY, STATE)
        updated = True
    except Exception:
        pass

if not updated:
    cf = CloudFiles(os.environ["SCRATCH_PATH"])
    if STATE == "DONE":
        cf.put(f'done/{TASK_KEY}.txt', b"")
