var chart;
var ms = 0;
var s_map = {};
var num_series = 0;
var channel_array=[];
var unit_array=[];

Highcharts.setOptions({
	global : {
		timezone : "Europe/Berlin"
	},
	lang : {
		months : [ "Januar", "Februar", "März", "April", "Mai", "Juni", "Juli",
				"August", "September", "Oktober", "November", "Dezember" ],
		weekdays : [ "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag",
				"Freitag", "Samstag" ]
	}
});
function parseFrame(data) {
	f = parseBayEOSFrame(data);
	if (typeof f.data === undefined)
		return;
	o = "";
	if (f.origin.length > 0)
		o = f.origin + "/";
	x = f.ts;
	for ( var ch in f.data) {
		l = o + ch;
		if (!s_map.hasOwnProperty(l)) {
			s_name=l
			if(typeof channel_array[num_series] === 'undefined') {
			  console.log("no channel name for series #"+num_series);
			} else {
			  s_name=channel_array[num_series]
			}
			if(typeof unit_array[num_series] === 'undefined') {
			  console.log("no unit for series #"+num_series);
			} else {
			  s_name=s_name+" ["+unit_array[num_series]+"]";
			}
			chart.addSeries({
				name : s_name,
				data : []
			}, !1);
			s_map[l] = num_series;
			num_series++;
			console.log(""+num_series+": "+s_name)
		}
		shift = chart.series[s_map[l]].data.length > 100;
		chart.series[s_map[l]].addPoint([ x, f.data[ch] ], !0, shift, !1)
	}

}
