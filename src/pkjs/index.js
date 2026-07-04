// PebbleKit JS: Clay settings page + weather from Open-Meteo (no API key).
// Weather source is the phone's GPS or the user-configured coordinates,
// in the user's unit; pushed to the watch over AppMessage.
//
// Condition codes shared with src/c/main.c:
//   0 = sun, 1 = cloud, 2 = rain, 3 = moon (clear night)

var Clay = require('pebble-clay');
var clayConfig = require('./config');
var customClay = require('./custom-clay');
var messageKeys = require('message_keys');

var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false });

var REFRESH_MS = 30 * 60 * 1000;

// Weather-relevant settings, mirrored locally so fetches between config
// opens use the latest values.
var prefs = {
  celsius: false,
  gps: true,
  lat: 37.77,
  lon: -122.42
};
try {
  var stored = JSON.parse(localStorage.getItem('wxPrefs'));
  if (stored && typeof stored === 'object') {
    prefs.celsius = !!stored.celsius;
    prefs.gps = stored.gps !== false;
    if (isFinite(stored.lat)) prefs.lat = Number(stored.lat);
    if (isFinite(stored.lon)) prefs.lon = Number(stored.lon);
  }
} catch (e) {}

function conditionFor(weatherCode, isDay) {
  // WMO weather interpretation codes (Open-Meteo `weather_code`).
  if (weatherCode <= 1) return isDay ? 0 : 3; // clear / mainly clear
  if (weatherCode <= 48) return 1;            // cloudy / fog
  return 2;                                   // drizzle, rain, snow, storms
}

function sendWeather(payload) {
  Pebble.sendAppMessage(payload, function () {
    console.log('Weather sent: ' + JSON.stringify(payload));
  }, function (e) {
    console.log('Weather send failed: ' + JSON.stringify(e));
  });
}

function fetchWeather(lat, lon) {
  var url = 'https://api.open-meteo.com/v1/forecast' +
    '?latitude=' + lat.toFixed(4) +
    '&longitude=' + lon.toFixed(4) +
    '&current=temperature_2m,weather_code,is_day' +
    '&daily=temperature_2m_max,temperature_2m_min' +
    '&temperature_unit=' + (prefs.celsius ? 'celsius' : 'fahrenheit') +
    '&forecast_days=1&timezone=auto';

  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    try {
      var data = JSON.parse(xhr.responseText);
      sendWeather({
        TEMPERATURE: Math.round(data.current.temperature_2m),
        TEMP_HI: Math.round(data.daily.temperature_2m_max[0]),
        TEMP_LO: Math.round(data.daily.temperature_2m_min[0]),
        CONDITION: conditionFor(data.current.weather_code,
                                data.current.is_day === 1)
      });
    } catch (err) {
      console.log('Weather parse failed: ' + err);
    }
  };
  xhr.onerror = function () { console.log('Weather request failed'); };
  xhr.open('GET', url);
  xhr.send();
}

function updateWeather() {
  if (!prefs.gps) {
    fetchWeather(prefs.lat, prefs.lon);
    return;
  }
  navigator.geolocation.getCurrentPosition(function (pos) {
    fetchWeather(pos.coords.latitude, pos.coords.longitude);
  }, function (err) {
    console.log('Geolocation failed (' + err.message + '), using configured coords');
    fetchWeather(prefs.lat, prefs.lon);
  }, { timeout: 15000, maximumAge: 60 * 60 * 1000 });
}

Pebble.addEventListener('ready', function () {
  updateWeather();
  setInterval(updateWeather, REFRESH_MS);
});

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) return;
  var dict = clay.getSettings(e.response);

  // Lat/lon arrive as free-text strings — convert to the centi-degree
  // ints the watch expects, falling back to the previous good values.
  var lat = parseFloat(dict[messageKeys.LOC_LAT]);
  var lon = parseFloat(dict[messageKeys.LOC_LON]);
  if (!isFinite(lat) || lat < -90 || lat > 90) lat = prefs.lat;
  if (!isFinite(lon) || lon < -180 || lon > 180) lon = prefs.lon;
  dict[messageKeys.LOC_LAT] = Math.round(lat * 100);
  dict[messageKeys.LOC_LON] = Math.round(lon * 100);

  var tag = String(dict[messageKeys.CITY_TAG] || '')
    .toUpperCase().replace(/[^A-Z0-9]/g, '').substring(0, 3);
  dict[messageKeys.CITY_TAG] = tag || 'SF';

  // Clay select values are strings — the watch expects ints.
  ['THEME_MODE', 'THEME_SEL', 'THEME_DAY', 'THEME_NIGHT',
   'SLOT_LEFT', 'SLOT_RIGHT'].forEach(function (k) {
    dict[messageKeys[k]] = parseInt(dict[messageKeys[k]], 10) || 0;
  });

  prefs = {
    celsius: !!Number(dict[messageKeys.USE_CELSIUS]),
    gps: !!Number(dict[messageKeys.WX_GPS]),
    lat: lat,
    lon: lon
  };
  localStorage.setItem('wxPrefs', JSON.stringify(prefs));

  Pebble.sendAppMessage(dict, function () {
    console.log('Settings sent');
    updateWeather(); // unit or location may have changed
  }, function (err) {
    console.log('Settings send failed: ' + JSON.stringify(err));
  });
});
