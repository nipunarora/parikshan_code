==================
SANDBOX TESTING
=================

:version: 1.1
:source: https://github.com/nipunarora/sandbox-testing/wiki/Parikshan:-A-Testing-Harness-for-In-Vivo-Sandbox-Testing
:keywords: how-to, sandbox testing

Decription
===========

One of the biggest problems faced by developers testing large scale systems is replicating the deployed environment to figure out errors.
In recent years there has been a lot of work in record-and-replay systems which captures traces from live production systems, and replays them.
However, most such record-replay systems have a high recording overhead and are still not practical to be used in production environments without paying a penalty in terms of overhead.

In this work we present a testing harness for production systems which allows the capabilities of running test-cases in a sandbox environment in the wild at any point in the execution of an integrated application. 
The paper levarages, User-Space Containers(OpenVZ/LXCs) to launch test instances in a container cloned and migrated from a running instances of an application. 
The test-container provides a sandbox environment, for safe execution of test-cases provided by the users without disturbing the execution environment. 
Test cases are initiated using user-defined probe points which launch test-cases using the execution context of the probe point. 
Our sandboxes provide a seperate namespace for the processes executing the test cases, replicate and copy inputs to the parent application, safetly discard all outputs, and manage the file system such that existing and newly created file descriptors are safetly managed.

We believe our tool provides a mechanism for practical testing of large scale multi-tier and cloud applications. 

1. Installing OpenVZ
=============

Install OpenVZ via the following script: https://github.com/nipunarora/sandbox-testing/blob/master/openvz-boot.sh

Install OpenVZ in Debian Wheezy: http://www.howtoforge.com/installing-and-using-openvz-on-debian-wheezy-amd64

Install Ubuntu 13.04 instructions: http://www.howtoforge.com/installing-and-using-openvz-on-ubuntu-13.04-amd64

Install OpenVZ in Centos 6.4: http://www.howtoforge.com/installing-and-using-openvz-on-centos-6.4

Install vzdump http://chrisschuld.com/2009/11/installing-vzdump-for-openvz-on-centos/

Installing OpenVZPanel http://owp.softunity.com.ru
