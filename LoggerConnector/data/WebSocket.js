var connection = new WebSocket('ws://' + location.hostname + ':81/',
		[ 'arduino' ]);

connection.onopen = function() {
	t2000=new Date("2000-01-01T00:00:00.000Z")
	var msg = {
			command : "time",
			value : Math.floor((Date.now()-t2000.getUTCMilliseconds())/1000),
	};
	console.log(msg);
	connection.send(JSON.stringify(msg));
};
connection.onerror = function(error) {
	console.log('WebSocket Error ', error);
};


connection.onmessage = function(e) {
	console.log('Server:'+e.data);
	var msg = JSON.parse(''+e.data);
	switch(msg.event){
	case "wait":
		$('#status').html("Waiting for connection: "+msg.seconds+" sec")
		break
	case "connected":
		$('#status').html("connected")
		break;
	case "disconnected":
		$('#status').html("not connected")
		$('#buffer').html("unknown")
		$('#battery').html("unknown")
		$('#logger_name').val("")
		$('#logger_int').val("")		
		break;
	case "liveMode":
		$('#live').val("Stop live mode")
		$(".live").prop('disabled', false);		
		break;
	case "ready":
		$('#live').val("Start live mode")
		$('#status').html("logger ready")
		$(".logger").prop('disabled', false);
		$(".live").prop('disabled', false);				
		break;
	case "data":
		$('#status').html("live mode")
		parseFrame(msg.data)
		break;
	case "download":
		$('#status').html("Download: "+msg.sent+"/"+msg.total+" Bytes")
		var percent=Math.round(100*msg.sent/msg.total)
		dl_button.val("Download progress: "+percent+"%");
		break;
	case "getCon":
		$('#con').empty();
		$('#con_list').html("");
		$('#con').append("<option></option>")
		msg.connections.sort(function(a,b){
		    var nameA=a.name.toLowerCase(), nameB=b.name.toLowerCase()
		    if (nameA < nameB) //sort string ascending
		        return -1 
		    if (nameA > nameB)
		        return 1
		    return 0 //default return value (no sorting)
		})
		for(var i=0;i<msg.connections.length;i++){
			$('#con').append("<option>"+msg.connections[i].name+"</option>")
			$('#con_list').append("<tr><td>"+msg.connections[i].name+"</td><td>"+msg.connections[i].channel+
					"</td><td>"+msg.connections[i].pipe+
					"</td><td><input type=\"button\" value=\"delete\" onClick=\"if(confirm('Are you sure?')) delCon("+i+")\"></td></tr>")
		}	
		break;
	case "nameInt":
		$('#logger_name').val(msg.name)
		$('#logger_int').val(msg.int)
		break;
	case "buffer":
		var size=Math.round(msg.size/1024)
		var used=(msg.write-msg.end)%msg.size
		if( used>4196){
			used=Math.round(used/1024)
			used=''+used+'kb'
		} else used=''+used+'b'
		
		var unread=(msg.write-msg.read)%msg.size
		if( unread>4196){
			unread=Math.round(unread/1024)
			unread=''+unread+'kb'
		} else unread=''+unread+'b'
		
		var buffer="Size: "+size+"kb - Used: "+used+" - New: "+unread;
		$('#buffer').html(buffer)
		$("#dl").val("Download New ("+unread+")")
		$("#dl_full").val("Download Full ("+used+")")
		break;
	case "time":
		$('#sync_time').val("Sync Time (Current timeshift: "+msg.value+"sec)")
		break;
	case "battery":
		var bat=""+msg.value/1000+"V";
		if(msg.value<msg.warning) bat+=" LOW BATTERY"
		$('#battery').html(bat)
		break;		
	}
	
};
connection.onclose = function() {
	console.log('WebSocket connection closed');
};


//Add a new RF24 connection
function addCon() {
	var ch = parseInt($('#con_channel').val());
	var pipe = parseInt($('#con_pipe').val());
	if (isNaN(ch) || isNaN(pipe)) {
		alert("Channel and Pipe must be numbers!")
		return

	}
	var msg = {
		command : "addCon",
		name : $('#con_name').val(),
		channel : ch,
		pipe : $('#con_pipe').val()
	};
	console.log(msg);
	connection.send(JSON.stringify(msg));
}

//Delete a RF24 connection
function delCon(nr){
	var msg = {
			command : "delCon",
			id: nr
	};
	console.log(msg);
	connection.send(JSON.stringify(msg));
	
}

//Start a RF24 connection
function sendCon() {
	var msg = {
		command : "connect",
		name : $('#con option:selected').text(),
	};
	$(".logger").prop('disabled', true);		
	$(".live").prop('disabled', true);		
	console.log(msg);
	connection.send(JSON.stringify(msg));
	var c = $('#con').value;
}


//Start live mode
function modeLive(){
	var msg = {
			command : "modeLive",
	};
	$(".logger").prop('disabled', true);		
	$(".live").prop('disabled', true);		
	console.log(msg);
	connection.send(JSON.stringify(msg));
	
}

//Stop live mode
function modeStop(){
	var msg = {
			command : "modeStop",
	};
	$(".live").prop('disabled', true);		
	console.log(msg);
	connection.send(JSON.stringify(msg));
	
}

//Erase data from flash
function deleteData(){
	var msg = {
			command : "delete",
	};
	console.log(msg);
	connection.send(JSON.stringify(msg));
	
}

//Save config to EEPROM
function syncConf() {
	var logging_int = parseInt($('#logger_int').val())
	if (isNaN(logging_int) ) {
		alert("Logging interval must be a number!")
		return
	}
	var msg = {
			command : "save",
			name : $('#logger_name').val(),
			logging_int : logging_int
		};
	console.log(msg);
	connection.send(JSON.stringify(msg));
	$(".logger").prop('disabled', true);		
}

//Sync logger time
function syncTime() {
	t2000=new Date("2000-01-01T00:00:00.000Z")
	var msg = {
			command : "sync_time",
			value : Math.floor((Date.now()-t2000.getUTCMilliseconds())/1000),
	};
	console.log(msg);
	connection.send(JSON.stringify(msg));
	$(".logger").prop('disabled', true);		
}

//Start Download
function download(full=false){
	$(".logger").prop('disabled', true);		
	$(".live").prop('disabled', true);	
	
	var url='/download';
	if(full) dl_button=$("#dl_full")
	else dl_button=$("#dl")
	dl_button.val("Download progress: 0%");
	if(full) url+='Full'
	url+='?f='+$('#dl_format option:selected').val()
	
	var _iframe_dl = $('<iframe />')
	       .attr('src', url)
	       .hide()
	       .appendTo('body');	
}

