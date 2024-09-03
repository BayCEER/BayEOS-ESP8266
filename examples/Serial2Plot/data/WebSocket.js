// WebSocket.js
// Define all Websocket stuff and command functions
// 
// Browser sends messages like "{"command":"save",...}
// ESP send messages like "{"event":"error", ... }

var live_mode_start;
var live_mode=0; //store live mode
var chart
var connection = new WebSocket('ws://' + location.hostname + ':81/',
		[ 'arduino' ]);


connection.onopen = function() {
	//on open we could send the time to the ESP.
	//here we do nothing
	var msg = {
			command : "getAll",
	};
	console.log(msg);
	connection.send(JSON.stringify(msg))
};


connection.onerror = function(error) {
	console.log('WebSocket Error ', error);
};


connection.onmessage = function(e) {
	console.log('Server:'+e.data);
	var msg = JSON.parse(''+e.data);
	switch(msg.event){
	case "error":
		$('#error').html(msg.text)
		$('#error_tr').show();
		setTimeout(function(){
		    $('#error_tr').fadeOut(500)
		}, 3000)
		break
	case "msg":
		$('#msg').html(msg.text)
		$('#msg_tr').show();
		setTimeout(function(){
		    $('#msg_tr').fadeOut(500)
		}, 3000)
		break
	case "conf":
		$('#version').html(msg.version)
		$('#ssid').val(msg.ssid)
		$('#password').val(msg.password)
		$('#baud').val(msg.baud)
		$('#max_time').val(msg.max_time)
		for(ch=1;ch<10;ch++){
		  $("#ch"+ch).val(msg["ch"+ch])
		}
		$(".config").prop('disabled', false); //enable inputs
		break;
		
		
	case "data":
		if(! live_mode) return;
		var shift = Date.now()-live_mode_start > parseInt($('#max_time').val())*1000;
		var s_nr=0;
	  	for(ch=1;ch<10;ch++){
	    	  if($("#ch"+ch).val()>''){
		    chart.series[s_nr].addPoint([ Date.now(), msg["ch"+ch]], false, shift)
		    s_nr++;
		    $("#v"+ch).html("<td>"+$("#ch"+ch).val()+"</td><td>"+msg["ch"+ch]+"</td>");
		    $("#v"+ch).show()
	          } else {
	            $("#v"+ch).hide()
	          }
	  	}
		chart.redraw()

		break
	}
};

connection.onclose = function() {
	console.log('WebSocket connection closed');
};



//Start live mode
function setLive(){
	if($("#live").val()=="Start Plotting"){
	  $("#live").val("Stop Plotting");
	  $(".config").prop('disabled', true);
	  chart = Highcharts.chart('container', {
		chart : {
			type : 'spline',
			animation: false
		},
		title : {
			text : 'Data'
		},
		xAxis : {
			type : 'datetime'
		},
		yAxis : {
			title : {
				text : 'Values'
			}
		},
		series : []
	})
	  live_mode=1;
	  live_mode_start=Date.now();
	  for(ch=1;ch<10;ch++){
	    if($("#ch"+ch).val()>''){
	      chart.addSeries({name: $("#ch"+ch).val(), data: []})
	    }
	  }
	  chart.redraw()	  
	} else {
	  $("#live").val("Start Plotting");
	  live_mode=0;
	  $(".config").prop('disabled', false);
	}
}


//Save config to EEPROM
function saveConf() {
	var max_time = parseInt($('#max_time').val())
	var baud = parseInt($('#baud').val())

	if (isNaN(baud) || isNaN(max_time) ) {
		alert("Baud, Max Runtime and Battery config must be a number!")
		return
	}
	var msg = {
			command : "setConf",
			ssid : $('#ssid').val(),
			password : $('#password').val(),
			baud : baud,
			max_time : max_time,
			ch1: $('#ch1').val(),
			ch2: $('#ch2').val(),
			ch3: $('#ch3').val(),
			ch4: $('#ch4').val(),
			ch5: $('#ch5').val(),
			ch6: $('#ch6').val(),
			ch7: $('#ch7').val(),
			ch8: $('#ch8').val(),
			ch9: $('#ch9').val()
		};
	console.log(msg);
	connection.send(JSON.stringify(msg));
	$(".config").prop('disabled', true);		
}


