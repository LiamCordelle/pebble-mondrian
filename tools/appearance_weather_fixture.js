'use strict';
// Face-owned sunrise/sunset scenarios; encoding and transport stay in the kit.
var fs=require('fs'),path=require('path');
var fixture=require('../shared-kit/tools/emulator_weather_fixture');
var weather=require('../src/pkjs/kit/weather');
var output=process.argv[2],phase=process.argv[3],now=Math.floor(Date.now()/1000);
var offset=-new Date(now*1000).getTimezoneOffset()*60;
var data=fixture(now,offset),base=data.daily.time[0];
var boundary=(Math.floor(now/60)+(now%60>40?2:1))*60;
var day=Math.floor((boundary-base)/86400),minute=(boundary-base-day*86400)/60;
if(minute===0 || (phase==='sunrise' && minute>=1438)) {
  boundary=base+(minute===0?day:day+1)*86400+60;
  day=Math.floor((boundary-base)/86400);minute=1;
}
if(phase==='sunset') {
  data.daily.sunrise[day]=base+day*86400+Math.max(0,minute-60)*60;
  data.daily.sunset[day]=boundary;
} else if(phase==='sunrise') {
  data.daily.sunrise[day]=boundary;
  data.daily.sunset[day]=base+day*86400+Math.min(1439,minute+60)*60;
} else throw new Error('Expected sunrise or sunset');
fs.mkdirSync(output,{recursive:true});
fs.writeFileSync(path.join(output,'boundary.json'),JSON.stringify({phase:phase,now:now,boundary:boundary,offset:offset},null,2)+'\n');
fs.writeFileSync(path.join(output,'boundary-weather.bin'),Buffer.from(weather.encode(data,now)));
