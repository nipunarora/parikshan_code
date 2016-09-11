url:https://docs.google.com/document/d/1Q60AlJyPYOvbi-RM3df9H5PM65H6a0vwYzcC2tIpsQ0/pub

github-761 (Release)
 [redis]github-761 Report
https://github.com/antirez/redis/issues/761
1. Symptom

Redis server crashes on a large integer input to âzinterstoreâ command
1.1 Severity

Severe
1.2 Was there exception thrown?

Yes: crash
1.2.1 Were there multiple exceptions?

No
1.3 Scope of the failure

Single server
2. How to reproduce this failure

2.0 Version

2.6.0-rc7
2.1 Configuration

Standard
2.2 Reproduction procedure

 zinterstore out 9223372036854775807 zset zset2 (feature start)
2.2.1 Timing order

Single event
2.2.2 Events order externally controllable?

Yes
2.3 Can the logs tell how to reproduce the failure?

yes
2.4 How many machines needed?

1
3. Diagnosis procedure

3.1 Detailed Symptom (where you start)

Crash, and the crash log contains the command and stack trace
3.2 Backward inference

Itâs quite easy to notice the wrong thing from the crash report:
=r cmd=zinterstore
[17998] 12 Sep 16:35:58.838 # argv[0]: 'zinterstore'
[17998] 12 Sep 16:35:58.838 # argv[1]: 'out'
[17998] 12 Sep 16:35:58.838 # argv[2]: '9223372036854775807'
[17998] 12 Sep 16:35:58.838 # argv[3]: 'zset'
[17998] 12 Sep 16:35:58.838 # argv[4]: 'zset2'
And this will manifest as âsetnumâ here:
 /* test if the expected number of keys would overflow */
    if (3+setnum > c->argc) {
        addReply(c,shared.syntaxerr);
        return;
    }
If you add 3, there will be integer overflowâ¦ c->argc is 5 and setnum is 9223372036854775807. They should have rejected the error here, but when the integer overflowed, and they passed this condition...
4. Root cause

Integer overflow.
4.1 Category:

Semantic (integer overflow)
5. Fix

5.1 How?

 /* test if the expected number of keys would overflow */
-    if (3+setnum > c->argc) {
+    if (setnum > c->argc-3) {
        addReply(c,shared.syntaxerr);
        return;
    }
thub-761 (Release)
 [redis]github-761 Report
https://github.com/antirez/redis/issues/761
1. Symptom

Redis server crashes on a large integer input to âzinterstoreâ command
1.1 Severity

Severe
1.2 Was there exception thrown?

Yes: crash
1.2.1 Were there multiple exceptions?

No
1.3 Scope of the failure

Single server
2. How to reproduce this failure

2.0 Version

2.6.0-rc7
2.1 Configuration

Standard
2.2 Reproduction procedure

 zinterstore out 9223372036854775807 zset zset2 (feature start)
2.2.1 Timing order

Single event
2.2.2 Events order externally controllable?

Yes
2.3 Can the logs tell how to reproduce the failure?

yes
2.4 How many machines needed?

1
3. Diagnosis procedure

3.1 Detailed Symptom (where you start)

Crash, and the crash log contains the command and stack trace
3.2 Backward inference

Itâs quite easy to notice the wrong thing from the crash report:
=r cmd=zinterstore
[17998] 12 Sep 16:35:58.838 # argv[0]: 'zinterstore'
[17998] 12 Sep 16:35:58.838 # argv[1]: 'out'
[17998] 12 Sep 16:35:58.838 # argv[2]: '9223372036854775807'
[17998] 12 Sep 16:35:58.838 # argv[3]: 'zset'
[17998] 12 Sep 16:35:58.838 # argv[4]: 'zset2'
And this will manifest as âsetnumâ here:
 /* test if the expected number of keys would overflow */
    if (3+setnum > c->argc) {
        addReply(c,shared.syntaxerr);
        return;
    }
If you add 3, there will be integer overflowâ¦ c->argc is 5 and setnum is 9223372036854775807. They should have rejected the error here, but when the integer overflowed, and they passed this condition...
4. Root cause

Integer overflow.
4.1 Category:

Semantic (integer overflow)
5. Fix

5.1 How?

 /* test if the expected number of keys would overflow */
-    if (3+setnum > c->argc) {
+    if (setnum > c->argc-3) {
        addReply(c,shared.syntaxerr);
        return;
    }


