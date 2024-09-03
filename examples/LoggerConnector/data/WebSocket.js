var connection = new WebSocket('ws://' + location.hostname + ':81/',
		[ 'arduino' ]);

var bytePerFrame

connection.onopen = function() {
	t2000=new Date("2000-01-01T00:00:00.000Z")
	var msg = {
			command : "time",
			value : Math.floor((Date.now()-t2000.getTime())/1000),
	};
	console.log(msg);
	connection.send(JSON.stringify(msg));
};
connection.onerror = function(error) {
	console.log('WebSocket Error ', error);
	if(confirm('Verbindung unterbrochen! Soll die Seite neu geladen werden?'))
		   location.reload(); 
};


connection.onmessage = function(e) {
	console.log('Server:'+e.data);
	var msg = JSON.parse(''+e.data);
	switch(msg.event){
	case "bat":
		$('#bat').html(''+msg.value.toFixed(2)+'V')
		if(msg.value>4.0) $('#bat_img').attr('src','bat1.gif')
		else if(msg.value>3.85) $('#bat_img').attr('src','bat2.gif')
		else if(msg.value>3.7) $('#bat_img').attr('src','bat3.gif')
		else $('#bat_img').attr('src','bat4.gif')
		$('#bat_div').show();
		break
	case "logging_disabled": /* v1.6 or more */
		if(msg.value){
		  $('#logging').val("Logging is STOPPED! - Start logging");
		  $("#logging").css("color","#f00");
		  $("#logging").css("font-weight","bold");		
		} else {
		  $('#logging').val("Logging is active - Stop logging");
		  $("#logging").css("color","#080");
		  $("#logging").css("font-weight","normal");		
	        }
		$('#logging_tr').show();
		$("#dl_format option").filter(function() { 
		    return this.text == "Metadata File"; 
		}).remove();
		$("#dl_format").append('<option value="2">Metadata File</option>')
		//$('#dl_meta_tr').show();
		break
	case "wait":
		if(msg.tx_error) 
		  $('#status').html("Lost connection! Reconnecting: "+msg.seconds+" sec")
		else {
		  $('#status').html("Waiting for connection: "+msg.seconds+" sec")
		  $('#buffer').html("unknown")
		  $('#battery').html("unknown")
		  $('#logger_version').html("unknown")
		  $('#logger_name').val("")
		  $('#logger_int').val("")
		}
		break
	case "connected":
		$('#status').html("Got RX! Trying to fetch Metadata")
		if(msg.tx_error || msg.rx_error) $('#status').html("Communication failure: TX "+msg.tx_error+" - RX "+msg.rx_error)

		break;
	case "disconnected":
		$('#status').html("not connected")
		$('#buffer').html("unknown")
		$('#battery').html("unknown")
		$('#logger_version').html("unknown")
		$('#logger_name').val("")
		$('#logger_int').val("")		
		break;
	case "liveMode":
		$('#live').val("Stop live mode")
		$(".live").prop('disabled', false);		
		break;
	case "ready":
		$('#live').val("Start live mode")
		if(msg.tx_error){
		$('#status').html("Last command failed. Connection may be lost!")		
		} else {
		$('#status').html("logger ready")
		}
		$('#dl_meta').val("Download Metadata");
		$(".logger").prop('disabled', false);
		$(".live").prop('disabled', false);				
		break;
	case "data":
		$('#status').html("live mode")
		parseFrame(msg.data)
		break;
	case "download":
		$('#status').html("Download: "+msg.sent+"/"+msg.total+" Bytes")
		$("#dl").val("Download progress: "+(100*msg.sent/msg.total).toFixed(1)+"%");
		break;
	case "getCon":
		$('#version').html(msg.version);
		$('#con').empty();
		$('#con_list').html("");
		$('#con').append("<option></option>")
		if(msg.serial){
			for(baud of [9600,38400,57600]){
				$('#con').append("<option value="+baud+">Serial ("+baud+" baud)</option>")
			}

		}
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
		bytePerFrame=msg.framesize
		var size=Math.round(msg.size/1024)
		var used=(msg.write-msg.end)
		if(used<0) used=used+msg.size
		var used_dp=''+used+'b'
		if( used>4196) used_dp=''+Math.round(used/1024)+'kb'
		
		var unread=(msg.write-msg.read)
		var unread_dp=''+unread+'b'
		if(unread<0) unread=unread+msg.size	
		if( unread>4196) unread_dp=''+Math.round(unread/1024)+'kb'
		
		if(msg.framesize>0){
			$("#dl_size_span").show()
			var bytePerDay=24*3600*msg.framesize/msg.loggingint
			$("#dl_new_l").html("Download New ("+unread_dp+" - "+(1.0*unread/bytePerDay).toFixed(2)+" days)")
			$("#dl_full_l").html("Download Full ("+used_dp+" - "+(1.0*used/bytePerDay).toFixed(2)+" days)")
			if( bytePerDay>4196){
				bytePerDay=Math.round(bytePerDay/1024)
				bytePerDay=''+bytePerDay+'kb'
			} else bytePerDay=''+bytePerDay+'b'
			$("#dl_size_l").html(bytePerDay+" per day")
		} else {
			$("#dl_new_l").html("Download New ("+unread+")")
			$("#dl_full_l").html("Download Full ("+used+")")
			$("#dl_size_span").hide()
		}
		var buffer="Size: "+size+"kb - Used: "+used+" - New: "+unread;
		$('#buffer').html(buffer)
		break;
	case "time":
		$('#sync_time').val("Sync Time (Current timeshift: "+msg.value+"sec)")
		break;
	case "metadata":
		var bat=""+msg.bat/1000+"V";
		if(msg.bat<msg.bat_warning) bat+=" LOW BATTERY"
		$('#battery').html(bat)
		$('#logger_version').html(msg.version);
		var version=msg.version.split('.')
		if(parseInt(version[0])<=1 && parseInt(version[1])<=5){
		 $('#logging_tr').hide();
		 $("#dl_format option").filter(function() { 
			    return this.text == "Metadata File"; 
			}).remove();
	//	 $('#dl_meta_tr').hide();
		} else {
		  channel_array=msg.channel.split(";");
		  unit_array=msg.unit.split(";");
		}
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
	baud=parseInt($('#con option:selected').val())
	
	var msg = {
		command : "connect",
		name : (isNaN(baud)?$('#con option:selected').text():$('#con option:selected').val()),
	};
	$(".logger").prop('disabled', true);
	$(".live").prop('disabled', true);		
	console.log(msg);
	connection.send(JSON.stringify(msg));

	//reset chart
	while(chart.series.length>0){
  		chart.series[0].remove(false) //false = don't redraw
	}
	chart.redraw() // update the visual aspect
	ms = 0;
	s_map = {};
	num_series = 0;
	channel_array=[];
	unit_array=[];
	
	$('#buffer').html("unknown")
	$('#battery').html("unknown")
	$('#logger_version').html("unknown")
	$('#logger_name').val("")
	$('#logger_int').val("")		

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
			value : Math.floor((Date.now()-t2000.getTime())/1000),
	};
	console.log(msg);
	connection.send(JSON.stringify(msg));
	$(".logger").prop('disabled', true);		
}

//Start Download
function download(){
	var fileFormat=parseInt($('#dl_format option:selected').val())
	var dlSize= parseInt($("input[name='dl_size']:checked").val())
	if(dlSize>0){
		dlSize=bytePerFrame*Math.round(3600*parseInt($('#dl_size_s option:selected').val())*
		parseInt($('#dl_size_u option:selected').val())/parseInt($('#logger_int').val()))
		if(isNaN(dlSize) || dlSize<=0){
			alert('Invalid download size:' +dlSize)
			return;
		}
	}
	
	var url='/download?f='+fileFormat+'&s='+dlSize;
	$("#dl").val("Download progress: 0%");
	$(".logger").prop('disabled', true);		
	$(".live").prop('disabled', true);	
	
	var _iframe_dl = $('<iframe />')
	       .attr('src', url)
	       .hide()
	       .appendTo('body');	
}

//Start and Stop Logging
function setLogging(){

	if($("#logging").val()=="Logging is active - Stop logging"){
	   msg = { command : "logging_disabled", value: true }
	} else {
	   msg = { command : "logging_disabled", value: false }
        }
	console.log(msg);
	connection.send(JSON.stringify(msg));
	$(".logger").prop('disabled', true); 	
	
}
