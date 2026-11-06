'use strict';
module.exports = {
  heading: 'Mondrian',
  storageKey: 'mondrian-kit-settings-v1',
  largeLabels: true,
  fullTimezones: true,
  faceSettings: [
    {type: 'select', messageKey: 'DARK_MODE', label: 'Appearance', defaultValue: '0', min: 0, max: 2,
      options: [{label: 'Light', value: '0'}, {label: 'Dark', value: '1'},
                {label: 'Automatic (sunset to sunrise)', value: '2'}]},
    {type: 'select', messageKey: 'DARK_GRID', label: 'Dark grid', defaultValue: '0', min: 0, max: 1,
      options: [{label: 'Grey', value: '0'}, {label: 'White', value: '1'}]},
    {type: 'text', defaultValue: 'Light is the default. Automatic follows local sunset and sunrise using weather data, and falls back to Light when sun times are unavailable or weather is disabled. The grid choice applies to dark mode.'}
  ]
};
