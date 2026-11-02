'use strict';
var assert=require('assert');
var options=require('../src/pkjs/app-config');
var schema=require('../src/pkjs/kit/config')(options);
var settings=require('../src/pkjs/kit/settings')(options);
var pkg=require('../package.json');
assert.strictEqual(schema[0].defaultValue,pkg.pebble.displayName);
var expected=pkg.name==='pebble-mondrian'?'mondrian-kit-settings-v1':'underground-kit-settings-v1';
assert.strictEqual(options.storageKey,expected);
assert.strictEqual(options.largeLabels,pkg.name==='pebble-mondrian');
assert.strictEqual(options.fullTimezones,true);
assert(schema[1].items.find(function(item){return item.messageKey==='WORLD';}).options.length>500);
var display=schema[1].items;
assert.strictEqual(display.some(function(v){return v.messageKey==='LARGE_LABELS';}),options.largeLabels);
var memory={};memory[expected]=JSON.stringify({TIME_MODE:2,TEMP_UNIT:1,STEP_GOAL:18000});
var store={getItem:function(k){return memory[k]||null;},setItem:function(k,v){memory[k]=v;}};
assert.strictEqual(settings.load(store).STEP_GOAL,18000);
settings.save(store,settings.load(store));
assert.strictEqual(JSON.parse(memory[expected]).TEMP_UNIT,1);
assert.deepStrictEqual(pkg.pebble.messageKeys,['READY','REQUEST_WEATHER','WEATHER_ID','WEATHER',
  'WEATHER_ERROR','TIME_MODE','TEMP_UNIT','STEP_GOAL','WORLD','WEATHER_ENABLED','DEMO_FALLBACK','LARGE_LABELS','DARK_MODE']);
var dark=schema.find(function(section){return section.items && section.items.some(function(item){return item.messageKey==='DARK_MODE';});});
assert(dark);
assert.strictEqual(dark.items.find(function(item){return item.messageKey==='DARK_MODE';}).defaultValue,false);
assert.strictEqual(settings.normalize({}).DARK_MODE,0);
assert.strictEqual(settings.load(store).DARK_MODE,0);
[true,1,'true','1'].forEach(function(value){
  assert.strictEqual(settings.normalize({DARK_MODE:value}).DARK_MODE,1);
});
[false,0,'false','0',null,'invalid',2].forEach(function(value){
  assert.strictEqual(settings.normalize({DARK_MODE:value}).DARK_MODE,0);
});
var saved=settings.load(store);saved.DARK_MODE=1;settings.save(store,saved);
assert.strictEqual(settings.load(store).DARK_MODE,1);
assert.strictEqual(settings.load(store).STEP_GOAL,18000);
assert.strictEqual(settings.load(store).TIME_MODE,2);
assert.strictEqual(settings.load(store).TEMP_UNIT,1);
console.log(pkg.pebble.displayName+' consumer checks passed: UI, existing settings and message-key compatibility.');
