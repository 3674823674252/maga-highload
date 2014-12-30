module.exports = function (credis, body, res) {
  if (!credis) {
    console.log('No redis, exiting..');
    return res.cantComplete();
  }

  var data = {
    lat: body.lat,
    lon: body.lon
  };

  credis.set(body.user, JSON.stringify(data), function (e) {
    if (e) {
      console.log('Error while PUTting:', e);
      return res.cantComplete();
    }

    res.ok();
  });
};