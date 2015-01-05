node.js
================
starting test, test config is located at ./node/load.ini ..
test load is line(1, 10000, 2m) (copied from groupmate config)
using ammo file located at ./node/ammo.txt

starting node (cwd = ./node):

REDIS_AUTH=<your redis auth> node server.js

starting test in another tab (cwd = ./node):

yandex-tank ammo.txt

test results are saved to:

./node/2015-01-05_00-42-59.HzJt70/ - html report

https://loadosophia.org/gui/201348/#tab=tabTimelines - loadosophia url

from test results we notice that the average load node.js could hold is ~1000rps.

c
================
starting test, test config is located at ./c/load.ini
test load is line(1, 10000, 2m) (copied from groupmate config)
using ammo file located at ./c/ammo.txt

so, test setup is the same as for node.js

starting fastcgi (cwd = ./c, note that redis auth is hardcoded into the code, for simplicity):

make

starting test in the same tab (cwd = ./c, the same tab is reused cause nginx is a daemon):

yandex-tank ammo.txt

unfortunately, initially the test for c version wasn't fancy. there was an attempt to run line(1, 1000, 1m) (milder version of node.js test), but very soon nginx stopped responding (it can be seen from answ log).

the second test attempt will be run where tcp/ip socket is going to replace unix socket /tmp/fcgiapp.socket

second test (dA7sok) brought no ease as well. the only difference is that there appeared some 500s that were absent from the first test. (first test only had 502s)

third test is going to employ not 4 threads, but 100 threads. and even milder line(1, 100, 1m). judging by ongoing results, it (wpq0eH) is no better than the second test. third test was interesting because answ_ log contained NO 500s or 502s and server was responsive after the test. yet all requests were 71 net errors, according to tank.

fourth test is a "giving-up" test. if that one does not work - i have no idea why it does not work. it is const(1, 1m).

that one (tQVtJo) also failed. that means i am doing something wrong.

fifth test is replacing my code with someone else's code (from: http://habrahabr.ru/post/154187/)
and that test (vlaJp4) was succesful. means the problem is in my code.

the error was that i was setting Status: 200 header in code, and that was rejected by nginx.

next test is going to be line(1, 10000, 2m), same as for node.js.

this test (2ENNLZ) ran at about ~700rps, lagging behind node.js

to ensure number of threads (100) didn't cap the possibility to process requests, the other test was performed, with number of threads equal to 1000. but this test (bS72h0) has very large number of failures.