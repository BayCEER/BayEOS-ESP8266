const BayEOS_DataFrame=1,BayEOS_Command=2,BayEOS_CommandResponse=3,BayEOS_Message=4,BayEOS_ErrorMessage=5,BayEOS_RoutedFrame=6,BayEOS_DelayedFrame=7,BayEOS_RoutedFrameRSSI=8,BayEOS_TimestampFrame=9,BayEOS_BinaryFrame=10,BayEOS_OriginFrame=11,BayEOS_MillisecondTimestampFrame=12,BayEOS_RoutedOriginFrame=13,BayEOS_ChecksumFrame=15,BayEOS_DelayedSecondFrame=16,BayEOS_WithoutOffset=32,BayEOS_ChannelNumber=64,BayEOS_ChannelLabel=96,BayEOS_DATATYP_MASK=15,BayEOS_OFFSETTYP_MASK=240;function parseBayEOSFrame(e){var a=base64js.toByteArray(e),t=new DataView(a.buffer,0),r={cks:0,origin:"",ts:Date.now()};return function e(a){var n;switch(t.getUint8(a)){case BayEOS_DataFrame:data=function(e){if(1==t.getUint8(e)){r.data={},e++;var a=t.getUint8(e)&BayEOS_DATATYP_MASK,n=t.getUint8(e)&BayEOS_OFFSETTYP_MASK,i=0;for(0==n&&(e++,i=t.getUint8(e)),e++;e<t.byteLength-r.cks;){if(ch_label="",value=0,n==BayEOS_ChannelLabel)for(i=t.getUint8(e)+e+1,e++;e<t.byteLength-r.cks&&e<i;)ch_label+=String.fromCharCode(t.getUint8(e)),e++;else n==BayEOS_ChannelNumber?(i=t.getUint8(e),e++):i++,ch_label="CH"+i;switch(a){case 1:value=t.getFloat32(e,1),e+=4;break;case 2:value=t.getInt32(e,1),e+=4;break;case 3:value=t.getInt16(e,1),e+=2;break;case 4:value=t.getInt8(e),e++}r.data[ch_label]=value}}}(a);break;case BayEOS_RoutedFrame:r.origin.length&&(r.origin+="/"),r.origin+="XBee"+t.getInt16(a+1,1)+"/"+t.getInt16(a+3,1),e(a+5);break;case BayEOS_RoutedFrameRSSI:r.origin.length&&(r.origin+="/"),r.origin+="XBee"+t.getInt16(a+1,1)+"/"+t.getInt16(a+3,1),r.rssi=t.getUint8(a+5),e(a+6);break;case BayEOS_OriginFrame:case BayEOS_RoutedOriginFrame:for(t.getUint8(a)!=BayEOS_RoutedOriginFrame&&(r.origin=""),r.origin.length&&(r.origin+="/"),a++,n=t.getUint8(a),a++;n>0;)r.origin+=String.fromCharCode(t.getUint8(a)),a++,n--;e(a);break;case BayEOS_DelayedFrame:r.ts-=t.getUint32(a+1,1),e(a+5);break;case BayEOS_DelayedSecondFrame:r.ts-=1e3*t.getUint32(a+1,1),e(a+5);break;case BayEOS_TimestampFrame:r.ts=Date.UTC(2e3,0,1,0,0,0).valueOf()+1e3*t.getUint32(a+1,1),e(a+5);break;case BayEOS_Message:case BayEOS_ErrorMessage:for(a++,r.ms="";a<t.byteLength-r.cks;)r.ms+=String.fromCharCode(t.getUint8(a)),a++;break;case BayEOS_ChecksumFrame:r.cks=2,e(++a)}}(0),r}