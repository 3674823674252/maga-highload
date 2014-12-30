var http = require('http');
var putLoc = require('./putLoc');
var getLoc = require('./getLoc');
var delLoc = require('./delLoc');

var redis = require('redis');

var credis;

var rhost = process.env.REDIS_HOST || 'localhost';
var rport = process.env.REDIS_PORT || 6379;

var server = http.createServer(function (req, res) {
  var url = req.url;
  var method = req.method;
  var body = '';

  req.on('data', function (chunk) {
    body += chunk;
  });

  req.on('end', function (chunk) {
    body += chunk || '';

    handle(url, method, res, body);
  });

  console.log('Serving', url, 'for', method);

  res.cantComplete = function () {
    this.status = 500;
    this.write(JSON.stringify({
      status: 500,
      copy: 'server error'
    }));
    this.end();
  };

  res.wrongParams = function () {
    this.status = 401;
    this.write(JSON.stringify({
      status: 401,
      copy: 'malformed params'
    }));
    this.end();
  };

  res.fourOhFour = function() {
    this.status = 404;
    this.write(JSON.stringify({
      status: 404,
      copy: 'no such user or other 404 cause'
    }));
    this.end();
  };

  res.ok = function () {
    this.status = 200;
    this.write(JSON.stringify({
      status: 200
    }));
    this.end();
  };

  res.sendData = function (data) {
    this.status = 200;
    this.write(data);
    this.end();
  };
});

credis = redis.createClient(rport, rhost);

var rauth = process.env.REDIS_AUTH;

if (rauth) {
  credis.auth(rauth, function (e) {
    if (e) {
      console.log('Wrong auth for redis! Exiting..');
      return;
    }

    console.log('Authenticated with redis');

    server.listen(3000);
  });
}

var PUT_REGEX = /\/loc\/put\/?/gmi;
var DEL_REGEX = /\/loc\/del\/?/gmi;
var GET_REGEX = /\/loc\/get\/([^\/]+)\/?/gmi;

function handle(url, method, res, body) {
  if (url.match(PUT_REGEX) && method == 'PUT') {
    console.log('LOC: attempting to process PUT');

    try {
      body = JSON.parse(body);
      return putLoc(credis, body, res);
    } catch (e) {
      console.log('Error while parsing req.json', e);
      return res.wrongParams();
    }

  }

  if (url.match(GET_REGEX) && method == 'GET') {
    console.log('LOC: attempting to process GET');
    return getLoc(credis, new RegExp(GET_REGEX).exec(url)[1], res);
  }

  if (url.match(DEL_REGEX) && method == 'DELETE') {
    console.log('LOC: attempting to process DELETE');

    try {
      body = JSON.parse(body);
      return delLoc(credis, body.user, res);
    } catch (e) {
      console.log('Error while parsing req.json', e);
      return res.wrongParams();
    }
  }

  return res.fourOhFour();
}