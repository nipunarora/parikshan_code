
url:https://docs.google.com/document/d/1up7c0c1gm_jNxSvdJ5QVq0rwS-CVgPhbjmbQ-Cqw7As/pub

github-957 (Release)
[redis]github-957 Report


1. Symptom

Slave cannot sync with Master on big dbs.
1.1 Severity

Severe.
1.2 Was there exception thrown?

Yes.
Master:
Client %s scheduled to be closed ASAP for overcoming of output buffer limits.
Slave:
[83823] 25 Feb 15:21:47.022 # I/O error trying to sync with MASTER: connection lost
1.2.1 Were there multiple exceptions?

Yes.
1.3 Scope of the failure

Entire data cannot be replicated to any slaves--- master will reject all slave connections (client is not affected).
2. How to reproduce this failure

2.0 Version

2.6.11
2.1 Configuration

Standard config:
In particular, make sure the following is in config:
client-output-buffer-limit slave 256mb 64mb 60
Or you can set it even smaller.
2.2 Reproduction procedure

Slave <-> master
Load a very large DB into master (larger than 256MB).
2.2.1 Timing order

Single event, now particular order.
2.2.2 Events order externally controllable?

Yes
2.3 Can the logs tell how to reproduce the failure?

Yes
2.4 How many machines needed?

1 (1 slave + 1 master)
3. Diagnosis procedure

3.1 Detailed Symptom (where you start)

On slave:
[83823] 25 Feb 15:21:47.022 # I/O error trying to sync with MASTER: connection lost
3.2 Backward inference

Well, in fact this is a configuration error, and there is log:
Client %s scheduled to be closed ASAP for overcoming of output buffer limits.
in the masterâs log file. However, the user ignored this and the entire diagnosis went into a wild goose chase.
4. Root cause

Configuration error: the DB is too large, larger than client-output-buffer-limit
4.1 Category:

Config error
5. Fix

5.1 How?

Enlarge the limit.
Published by Google DriveâReport AbuseâUpdated automatically every 5 minutes
