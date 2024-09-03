// WebSocket.js
// Define all Websocket stuff and command functions
// 
// Browser sends messages like "{"command":"save",...}
// ESP send messages like "{"event":"error", ... }

var connection = new WebSocket('ws://' + location.hostname + ':81/',
		[ 'arduino' ]);

connection.onopen = function() {
	//on open we could send the time to the ESP.
	//here we do nothing
	/*
	t2000=new Date("2000-01-01T00:00:00.000Z")
	var msg = {
			command : "time",
			value : Math.floor((Date.now()-t2000.getUTCMilliseconds())/1000),
	};
	console.log(msg);
	connection.send(JSON.stringify(msg));
	*/
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
	case "config":
		$('#name').val(msg.name)
		$('#uint8_val').val(msg.uint8_val)
		$('#long_val').val(msg.long_val)
		$('#float_val').val(msg.float_val)
		$(".config").prop('disabled', false); //enable inputs
		break;
	//TODO: add further events to handle
	}
};
connection.onclose = function() {
	console.log('WebSocket connection closed');
};


//Save config to EEPROM
function saveConf() {
	var uint8_val = parseInt($('#uint8_val').val())
	var long_val = parseInt($('#long_val').val())
	var float_val = parseFloat($('#float_val').val())
	if (isNaN(uint8_val) || isNaN(long_val) ||isNaN(float_val) ) {
		alert("Uint8, long and float must be a number!")
		return
	}
	var msg = {
			command : "save",
			name : $('#name').val(),
			uint8_val : uint8_val,
			long_val : long_val,
			float_val: float_val
		};
	console.log(msg);
	connection.send(JSON.stringify(msg));
	$(".config").prop('disabled', true);		
}
