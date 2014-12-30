module.exports = function (credis, user, res) {
  if (!credis) {
    console.log('No redis, exiting..');
    return res.cantComplete();
  }

  credis.get(user, function (e, data) {
    if (e) {
      console.log('Error while GETting,', e);
      return res.cantComplete();
    }

    if (!data) {
      console.log('No data returned from key', user);
      return res.wrongParams();
    }

    res.sendData(data);
  });
};