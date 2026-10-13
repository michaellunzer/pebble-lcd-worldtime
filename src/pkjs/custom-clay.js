// Runs inside the Clay config webview: shows only the theme options
// that apply to the selected mode (single theme, auto at set times, or
// auto at sunrise / sunset).
module.exports = function (minified) {
  var clayConfig = this;

  function updateThemeMode() {
    var mode = clayConfig.getItemByMessageKey('THEME_MODE').get();
    var auto = mode === '1' || mode === '2';
    function toggle(item, on) {
      if (!item) { return; }
      if (on) { item.show(); } else { item.hide(); }
    }
    toggle(clayConfig.getItemByMessageKey('THEME_SEL'), !auto);
    toggle(clayConfig.getItemByMessageKey('THEME_DAY'), auto);
    toggle(clayConfig.getItemByMessageKey('THEME_NIGHT'), auto);
    toggle(clayConfig.getItemByMessageKey('DAY_START'), mode === '1');
    toggle(clayConfig.getItemByMessageKey('NIGHT_START'), mode === '1');
    toggle(clayConfig.getItemById('SUN_NOTE'), mode === '2');
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function () {
    // Listen first so a problem in the initial pass can't leave the
    // page without live updates.
    clayConfig.getItemByMessageKey('THEME_MODE').on('change', updateThemeMode);
    updateThemeMode();
  });
};
