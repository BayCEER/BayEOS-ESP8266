var chart;
var ms = 0;
var s_map = {};
var num_series = 0;
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
		console.log(l);
		if (!s_map.hasOwnProperty(l)) {
			chart.addSeries({
				name : l,
				data : []
			}, !1);
			s_map[l] = num_series;
			num_series++;
			console.log(num_series)
		}
		shift = chart.series[s_map[l]].data.length > 100;
		chart.series[s_map[l]].addPoint([ x, f.data[ch] ], !0, shift, !1)
	}

}
