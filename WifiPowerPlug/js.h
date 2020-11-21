const char bayeosParser_js[] PROGMEM = "const BayEOS_DataFrame=1,BayEOS_Command=2,BayEOS_CommandResponse=3,BayEOS_Message=4,BayEOS_ErrorMessage=5,BayEOS_RoutedFrame=6,BayEOS_DelayedFrame=7,BayEOS_RoutedFrameRSSI=8,BayEOS_TimestampFrame=9,BayEOS_BinaryFrame=10,BayEOS_OriginFrame=11,BayEOS_MillisecondTimestampFrame=12,BayEOS_RoutedOriginFrame=13,BayEOS_ChecksumFrame=15,BayEOS_DelayedSecondFrame=16,BayEOS_WithoutOffset=32,BayEOS_ChannelNumber=64,BayEOS_ChannelLabel=96,BayEOS_DATATYP_MASK=15,BayEOS_OFFSETTYP_MASK=240;function parseBayEOSFrame(e){var a=base64js.toByteArray(e),t=new DataView(a.buffer,0),r={cks:0,origin:\"\",ts:Date.now()};return function e(a){var n;switch(t.getUint8(a)){case BayEOS_DataFrame:data=function(e){if(1==t.getUint8(e)){r.data={},e++;var a=t.getUint8(e)&BayEOS_DATATYP_MASK,n=t.getUint8(e)&BayEOS_OFFSETTYP_MASK,i=0;for(0==n&&(e++,i=t.getUint8(e)),e++;e<t.byteLength-r.cks;){if(ch_label=\"\",value=0,n==BayEOS_ChannelLabel)for(i=t.getUint8(e)+e+1,e++;e<t.byteLength-r.cks&&e<i;)ch_label+=String.fromCharCode(t.getUint8(e)),e++;else n==BayEOS_ChannelNumber?(i=t.getUint8(e),e++):i++,ch_label=\"CH\"+i;switch(a){case 1:value=t.getFloat32(e,1),e+=4;break;case 2:value=t.getInt32(e,1),e+=4;break;case 3:value=t.getInt16(e,1),e+=2;break;case 4:value=t.getInt8(e),e++}r.data[ch_label]=value}}}(a);break;case BayEOS_RoutedFrame:r.origin.length&&(r.origin+=\"/\"),r.origin+=\"XBee\"+t.getInt16(a+1,1)+\"/\"+t.getInt16(a+3,1),e(a+5);break;case BayEOS_RoutedFrameRSSI:r.origin.length&&(r.origin+=\"/\"),r.origin+=\"XBee\"+t.getInt16(a+1,1)+\"/\"+t.getInt16(a+3,1),r.rssi=t.getUint8(a+5),e(a+6);break;case BayEOS_OriginFrame:case BayEOS_RoutedOriginFrame:for(t.getUint8(a)!=BayEOS_RoutedOriginFrame&&(r.origin=\"\"),r.origin.length&&(r.origin+=\"/\"),a++,n=t.getUint8(a),a++;n>0;)r.origin+=String.fromCharCode(t.getUint8(a)),a++,n--;e(a);break;case BayEOS_DelayedFrame:r.ts-=t.getUint32(a+1,1),e(a+5);break;case BayEOS_DelayedSecondFrame:r.ts-=1e3*t.getUint32(a+1,1),e(a+5);break;case BayEOS_TimestampFrame:r.ts=Date.UTC(2e3,0,1,0,0,0).valueOf()+1e3*t.getUint32(a+1,1),e(a+5);break;case BayEOS_Message:case BayEOS_ErrorMessage:for(a++,r.ms=\"\";a<t.byteLength-r.cks;)r.ms+=String.fromCharCode(t.getUint8(a)),a++;break;case BayEOS_ChecksumFrame:r.cks=2,e(++a)}}(0),r}";
const char base64_min_js[] PROGMEM = "(function(r){if(typeof exports===\"object\"&&typeof module!==\"undefined\"){module.exports=r()}else if(typeof define===\"function\"&&define.amd){define([],r)}else{var e;if(typeof window!==\"undefined\"){e=window}else if(typeof global!==\"undefined\"){e=global}else if(typeof self!==\"undefined\"){e=self}else{e=this}e.base64js=r()}})(function(){var r,e,n;return function(){function r(e,n,t){function o(f,i){if(!n[f]){if(!e[f]){var u=\"function\"==typeof require&&require;if(!i&&u)return u(f,!0);if(a)return a(f,!0);var v=new Error(\"Cannot find module '\"+f+\"'\");throw v.code=\"MODULE_NOT_FOUND\",v}var d=n[f]={exports:{}};e[f][0].call(d.exports,function(r){var n=e[f][1][r];return o(n||r)},d,d.exports,r,e,n,t)}return n[f].exports}for(var a=\"function\"==typeof require&&require,f=0;f<t.length;f++)o(t[f]);return o}return r}()({\"/\":[function(r,e,n){\"use strict\";n.byteLength=d;n.toByteArray=h;n.fromByteArray=p;var t=[];var o=[];var a=typeof Uint8Array!==\"undefined\"?Uint8Array:Array;var f=\"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\";for(var i=0,u=f.length;i<u;++i){t[i]=f[i];o[f.charCodeAt(i)]=i}o[\"-\".charCodeAt(0)]=62;o[\"_\".charCodeAt(0)]=63;function v(r){var e=r.length;if(e%4>0){throw new Error(\"Invalid string. Length must be a multiple of 4\")}var n=r.indexOf(\"=\");if(n===-1)n=e;var t=n===e?0:4-n%4;return[n,t]}function d(r){var e=v(r);var n=e[0];var t=e[1];return(n+t)*3/4-t}function c(r,e,n){return(e+n)*3/4-n}function h(r){var e;var n=v(r);var t=n[0];var f=n[1];var i=new a(c(r,t,f));var u=0;var d=f>0?t-4:t;for(var h=0;h<d;h+=4){e=o[r.charCodeAt(h)]<<18|o[r.charCodeAt(h+1)]<<12|o[r.charCodeAt(h+2)]<<6|o[r.charCodeAt(h+3)];i[u++]=e>>16&255;i[u++]=e>>8&255;i[u++]=e&255}if(f===2){e=o[r.charCodeAt(h)]<<2|o[r.charCodeAt(h+1)]>>4;i[u++]=e&255}if(f===1){e=o[r.charCodeAt(h)]<<10|o[r.charCodeAt(h+1)]<<4|o[r.charCodeAt(h+2)]>>2;i[u++]=e>>8&255;i[u++]=e&255}return i}function s(r){return t[r>>18&63]+t[r>>12&63]+t[r>>6&63]+t[r&63]}function l(r,e,n){var t;var o=[];for(var a=e;a<n;a+=3){t=(r[a]<<16&16711680)+(r[a+1]<<8&65280)+(r[a+2]&255);o.push(s(t))}return o.join(\"\")}function p(r){var e;var n=r.length;var o=n%3;var a=[];var f=16383;for(var i=0,u=n-o;i<u;i+=f){a.push(l(r,i,i+f>u?u:i+f))}if(o===1){e=r[n-1];a.push(t[e>>2]+t[e<<4&63]+\"==\")}else if(o===2){e=(r[n-2]<<8)+r[n-1];a.push(t[e>>10]+t[e>>4&63]+t[e<<2&63]+\"=\")}return a.join(\"\")}},{}]},{},[])(\"/\")});";
const char HIGHCHART_JS1[] PROGMEM = "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script><script src=\"https://code.highcharts.com/highcharts.js\"></script><script src=\"https://code.highcharts.com/modules/data.js\"></script><script src=\"https://code.highcharts.com/modules/exporting.js\"></script><script src=\"https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.22.2/moment.min.js\"></script><script src=\"https://cdnjs.cloudflare.com/ajax/libs/moment-timezone/0.5.21/moment-timezone-with-data-2012-2022.min.js\"></script><script src=\"base64js.min.js\"></script><script src=\"bayeosParser.js\"></script><style>#container{min-width:310px;max-width:1200px;height:400px;margin:0 auto}</style><script>";
const char HIGHCHART_JS2[] PROGMEM = "var chart;var ms=0;var s_map={};var num_series=0;Highcharts.setOptions({global:{timezone:\"Europe/Berlin\"},lang:{months:[\"Januar\",\"Februar\",\"März\",\"April\",\"Mai\",\"Juni\",\"Juli\",\"August\",\"September\",\"Oktober\",\"November\",\"Dezember\"],weekdays:[\"Sonntag\",\"Montag\",\"Dienstag\",\"Mittwoch\",\"Donnerstag\",\"Freitag\",\"Samstag\"]}});function requestData(){$.ajax({url:'/bin',data:{m:ms},success:function(points){for(i=0;i<points.length;i++){ms=points[i][0];f=parseBayEOSFrame(points[i][1]);if(typeof f.data===undefined)continue;o=\"\";if(f.origin.length>0)o=f.origin+\"/\";x=f.ts;for(var ch in f.data){l=o+ch;console.log(l);if(!s_map.hasOwnProperty(l)){chart.addSeries({name:l,data:[]},!1);s_map[l]=num_series;num_series++;console.log(num_series)}shift=chart.series[s_map[l]].data.length>100;chart.series[s_map[l]].addPoint([x,f.data[ch]],!0,shift,!1)}}setTimeout(requestData,2000)},cache:!1})}document.addEventListener('DOMContentLoaded',function(){chart=Highcharts.chart('container',{chart:{type:'spline',events:{load:requestData}},title:{text:'Data'},xAxis:{type:'datetime'},yAxis:{minPadding:0.2,maxPadding:0.2,title:{text:'Value'}},series:[]})});</script>";
const char HIGHCHART_BUTTON[] PROGMEM = "<script>var button=$(\"#button\");button.click(function(){\"Hide series\"==button.html()?($(chart.series).each(function(){this.setVisible(!1,!1)}),chart.redraw(),button.html(\"Show series\")):($(chart.series).each(function(){this.setVisible(!0,!1)}),chart.redraw(),button.html(\"Hide series\"))});</script>";
