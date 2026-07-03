// PebbleKit JS: fetches weather from Open-Meteo (no API key required)
// using the phone's location, and pushes it to the watch over AppMessage.
//
// Condition codes shared with src/c/main.c:
//   0 = sun, 1 = cloud, 2 = rain, 3 = moon (clear night)

var REFRESH_MS = 30 * 60 * 1000;

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
    '&temperature_unit=fahrenheit&forecast_days=1&timezone=auto';

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
  navigator.geolocation.getCurrentPosition(function (pos) {
    fetchWeather(pos.coords.latitude, pos.coords.longitude);
  }, function (err) {
    console.log('Geolocation failed: ' + err.message);
  }, { timeout: 15000, maximumAge: 60 * 60 * 1000 });
}

Pebble.addEventListener('ready', function () {
  updateWeather();
  setInterval(updateWeather, REFRESH_MS);
});
