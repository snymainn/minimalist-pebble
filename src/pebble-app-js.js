
Pebble.addEventListener('showConfiguration', function() {
  var url = 'https://cdn.rawgit.com/snymainn/minimalist-pebble/0.5/config/index.html';
  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
	var configData = JSON.parse(decodeURIComponent(e.response));
	console.log('Configuration page returned: ' + JSON.stringify(configData));

	var toggleDict = {};

	toggleDict.INVERSE_WHEN_DISCONNECTED = configData.inverse_colors_when_bluetooth_disconnected;
	toggleDict.SHOW_BATTERY_STATUS = configData.show_battery_status;
	toggleDict.VIBRATE_ON_DISCONNECT = configData.vibrate_on_disconnect;

	Pebble.sendAppMessage(toggleDict, function() {
		console.log('Send toggles successful: ' + JSON.stringify(toggleDict));
	}, function() {
		console.log('Send failed!');
	});
});  

