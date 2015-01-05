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

make; nginx -s reload;

starting test in the same tab (cwd = ./c, the same tab is reused cause nginx is a daemon):

yandex-tank ammo.txt

test-results are saved to:
