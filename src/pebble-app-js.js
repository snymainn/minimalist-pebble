
Pebble.addEventListener('showConfiguration', function() {
  var url = 'https://rawgit.com/nguyer/pebble-engineering/1.5/config/index.html';
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
		navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
	}

	var toggleDict = {};

	toggleDict.SHOW_NUMBERS = configData.show_numbers;
	toggleDict.SHOW_DATE = configData.show_date;

	var colorDict = {};

	colorDict.COLOR_BACKGROUND = parseInt(configData.background_color.substring(0), 16);
	colorDict.COLOR_LABEL = parseInt(configData.label_color.substring(0), 16);
	colorDict.COLOR_HOUR_MARKS = parseInt(configData.hour_mark_color.substring(0), 16);
	colorDict.COLOR_MINUTE_MARKS = parseInt(configData.minute_mark_color.substring(0), 16);
	colorDict.COLOR_HOUR_HAND = parseInt(configData.hour_hand_color.substring(0), 16);
	colorDict.COLOR_MINUTE_HAND = parseInt(configData.minute_hand_color.substring(0), 16);

	Pebble.sendAppMessage(toggleDict, function() {
		console.log('Send toggles successful: ' + JSON.stringify(toggleDict));

		Pebble.sendAppMessage(colorDict, function() {
			console.log('Send colors successful: ' + JSON.stringify(colorDict));
		}, function() {
			console.log('Send failed!');
		});

	}, function() {
		console.log('Send failed!');
	});

