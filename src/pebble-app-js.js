
Pebble.addEventListener('showConfiguration', function() {
  var url = 'https://rawgit.com/snymainn/minimalist-pebble/0.1/config/index.html';
  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
	var configData = JSON.parse(decodeURIComponent(e.response));
	console.log('Configuration page returned: ' + JSON.stringify(configData));

	if (configData.units && configData.units !== units) {
		// update units
		units = configData.units;
		localStorage.units = configData.units;
	}

	var toggleDict = {};

	toggleDict.INVERSE_WHEN_DISCONNECTED = configData.inverse_colors_when_bluetooth_disconnected;
	toggleDict.SHOW_BATTERY_STATUS = configData.show_battery_status;

	Pebble.sendAppMessage(toggleDict, function() {
		console.log('Send toggles successful: ' + JSON.stringify(toggleDict));
	}, function() {
		console.log('Send failed!');
	});

