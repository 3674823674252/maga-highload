module.exports = function (credis, user, res) {
  if (!credis) {
    // console.log('No redis, exiting..');
    return res.cantComplete();
  }

  credis.del(user, function (e) {
    if (e) {
      // console.log('Error while DELETing..', e);
      return res.cantComplete();
    }

    res.ok();
  });
};