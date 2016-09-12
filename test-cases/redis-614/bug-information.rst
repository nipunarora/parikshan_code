[redis] github-614 (Release)

1. Symptom

When Master + Slave, and lua scripting is used, BRPush and BRPop on a list were not replicated correctly to slave (the master was still correct though, so client still sees correct data as long as the master is alive).

1.1 Severity

Severe

1.2 Was there exception thrown?

Yes

1.2.1 Were there multiple exceptions?

No

1.3 Scope of the failure

Just the slave is not replicating correctly and leaking memory. No visible symptom to client as long as the master is alive.

2. How to reproduce this failure

2.0 Version

2.6.0-RC4
2.1 Configuration

Master + slave
2.2 Reproduction procedure

I used the scripts provided by the reporter.
Basically:
0. setup master/slave replication
1. lpush some elements
2. brpoplpush
3. remove all elements (1 and 2 are sufficient to see the failure)

2.2.1 Timing order

Order doesn’t matter too much
2.2.2 Events order externally controllable?

Yes
2.3 Can the logs tell how to reproduce the failure?

Client log yes
2.4 How many machines needed?

2
3. Diagnosis procedure

3.1 Detailed Symptom (where you start)

You can notice the memory growth on slave log:
[3672] 01 Sep 21:39:59.596 - 1 clients connected (0 slaves), 557152 bytes in use
[3672] 01 Sep 21:40:04.642 - DB 8: 1 keys (0 volatile) in 4 slots HT.
[3672] 01 Sep 21:40:04.642 - 1 clients connected (0 slaves), 557312 bytes in use
[3672] 01 Sep 21:40:09.687 - DB 8: 1 keys (0 volatile) in 4 slots HT.
[3672] 01 Sep 21:40:09.687 - 1 clients connected (0 slaves), 557472 bytes in use
3.2 Backward inference

Hard to figure out why the memory is growing unboundedly. The root cause is that when scripting was pushing data that is waited by more than one node (client + slave in this case), the slave was not correctly served.
4. Root cause

The logic of handling script + synchronous blocking operations (BRPOP, BLPOP) are wrong.
4.1 Category:

Semantic
5. Fix

5.1 How?

Complete rewrite:
https://github.com/antirez/redis/commit/c0d87e0b55e70d5a4b6d92a842870fa638dfef20
Published by Google Drive–Report Abuse–Updated automatically every 5 minutes
