// Runs inside the Clay config webview: shows only the theme options
// that apply to the selected mode (single theme vs auto day/night).
module.exports = function (minified) {
  var clayConfig = this;

  function updateThemeMode() {
    var auto = clayConfig.getItemByMessageKey('THEME_MODE').get() === '1';
    var autoKeys = ['THEME_DAY', 'THEME_NIGHT', 'DAY_START', 'NIGHT_START'];
    for (var i = 0; i < autoKeys.length; i++) {
      var item = clayConfig.getItemByMessageKey(autoKeys[i]);
      if (auto) { item.show(); } else { item.hide(); }
    }
    var single = clayConfig.getItemByMessageKey('THEME_SEL');
    if (auto) { single.hide(); } else { single.show(); }
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function () {
    updateThemeMode();
    clayConfig.getItemByMessageKey('THEME_MODE').on('change', updateThemeMode);
  });
};
