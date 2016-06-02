Instructions to recreate the bug:
========

Clone redis:

	git clone https://github.com/antirez/redis.git

Checkout the correct version of redis:

	git checkout 2ac546e00cd4000506558e23d11acafb4ce654b7

Install redis:

	make

Create two copies of the installed folders:

       redis and redis2 

Instructions on how to run proxy for redis-487
=========

1. Run first redis service:

   cd redis
   ./src/redis-server --loglevel verbose

2. Run second redis service:

   cd redis2
   ./src/redis-server --port 7777 --loglevel verbose

3. Start proxy clone:

   ./proxyClone -l 5555 -h 127.0.0.1 -p 6379 -x 127.0.0.1 -d 7777 -a

4. Start redis client:

   

cat bug.txt| ./redis/src/redis-cli -p 5555
