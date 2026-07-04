// Clay configuration page. Values land on the watch via AppMessage and
// are normalized in index.js before sending (lat/lon -> centi-degrees).

var THEME_OPTIONS = [
  { label: 'Positive (tan LCD)', value: '0' },
  { label: 'Negative (dark / amber)', value: '1' },
  { label: 'Mono (ink on paper)', value: '2' },
  { label: 'Accent (dark / teal)', value: '3' }
];

module.exports = [
  {
    type: 'heading',
    defaultValue: 'LCD Worldtime'
  },
  {
    type: 'text',
    defaultValue: 'Casio-flavoured worldtime face for Pebble Time 2.'
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Location' },
      {
        type: 'input',
        messageKey: 'LOC_LAT',
        defaultValue: '37.77',
        label: 'Latitude',
        description: 'Decimal degrees, north positive. SF = 37.77',
        attributes: { placeholder: '37.77', type: 'text' }
      },
      {
        type: 'input',
        messageKey: 'LOC_LON',
        defaultValue: '-122.42',
        label: 'Longitude',
        description: 'Decimal degrees, east positive. SF = -122.42',
        attributes: { placeholder: '-122.42', type: 'text' }
      },
      {
        type: 'toggle',
        messageKey: 'SHOW_DOT',
        defaultValue: true,
        label: 'Show location dot on map'
      },
      {
        type: 'toggle',
        messageKey: 'WX_GPS',
        defaultValue: true,
        label: 'Weather from phone GPS',
        description: 'Off: weather uses the coordinates above instead.'
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Theme' },
      {
        type: 'select',
        messageKey: 'THEME_MODE',
        defaultValue: '1',
        label: 'Theme mode',
        options: [
          { label: 'Single theme', value: '0' },
          { label: 'Auto: day / night', value: '1' }
        ]
      },
      {
        type: 'select',
        messageKey: 'THEME_SEL',
        defaultValue: '0',
        label: 'Theme (single mode)',
        options: THEME_OPTIONS
      },
      {
        type: 'select',
        messageKey: 'THEME_DAY',
        defaultValue: '0',
        label: 'Day theme (auto mode)',
        options: THEME_OPTIONS
      },
      {
        type: 'select',
        messageKey: 'THEME_NIGHT',
        defaultValue: '1',
        label: 'Night theme (auto mode)',
        options: THEME_OPTIONS
      },
      {
        type: 'slider',
        messageKey: 'DAY_START',
        defaultValue: 7,
        label: 'Day theme starts at (hour)',
        min: 0,
        max: 23,
        step: 1
      },
      {
        type: 'slider',
        messageKey: 'NIGHT_START',
        defaultValue: 20,
        label: 'Night theme starts at (hour)',
        min: 0,
        max: 23,
        step: 1
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Display' },
      {
        type: 'toggle',
        messageKey: 'SHOW_SECONDS',
        defaultValue: true,
        label: 'Seconds beside the time',
        description: 'Ignored when a bottom box shows seconds — the time ' +
          'grows to full width instead. All seconds off = once-a-minute ' +
          'wakeups and better battery.'
      },
      {
        type: 'select',
        messageKey: 'SLOT_LEFT',
        defaultValue: '0',
        label: 'Bottom left box',
        options: [
          { label: 'Weather', value: '0' },
          { label: 'Steps', value: '1' },
          { label: 'Heart rate', value: '2' },
          { label: 'Seconds', value: '3' },
          { label: 'Empty', value: '4' }
        ]
      },
      {
        type: 'select',
        messageKey: 'SLOT_RIGHT',
        defaultValue: '1',
        label: 'Bottom right box',
        options: [
          { label: 'Weather', value: '0' },
          { label: 'Steps', value: '1' },
          { label: 'Heart rate', value: '2' },
          { label: 'Seconds', value: '3' },
          { label: 'Empty', value: '4' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'USE_CELSIUS',
        defaultValue: false,
        label: 'Temperature in Celsius'
      },
      {
        type: 'slider',
        messageKey: 'STEP_GOAL',
        defaultValue: 10000,
        label: 'Step goal',
        min: 1000,
        max: 25000,
        step: 500
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
