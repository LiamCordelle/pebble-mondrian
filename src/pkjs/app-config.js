'use strict';
module.exports = {
  heading: 'Mondrian',
  storageKey: 'mondrian-kit-settings-v1',
  largeLabels: true,
  fullTimezones: true,
  faceSettings: [
    {type: 'toggle', messageKey: 'DARK_MODE', label: 'Dark mode', defaultValue: false},
    {type: 'text', defaultValue: 'Off keeps the original light theme. On uses black panels, white text and a dark grey grid, with the original colours.'}
  ]
};
