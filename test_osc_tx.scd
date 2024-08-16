p = 9000;
a = NetAddr("localhost", p);

~tap_id = Dictionary.newFrom([
	\setLoop, 0,
	\toggleOverdub, 1,
	\toggleMute, 2,
	\stop, 3,
	\reset, 4
]);

~tap = { arg k; var id = ~tap_id[k]; a.sendMsg(\tap, id); };

~fparam_id = Dictionary.newFrom([
	\preserveLevel, 0,
	\recordLevel, 1,
	\playbackLevel, 2,
	\fadeIncrement, 3,
]);

~fparam = { arg k, v; var id = ~fparam_id[k]; s.sendMsg(\fparam, id, v); };

~iparam_id = Dictionary.newFrom([
	\selectLayer, 0,
	\resetLayer, 1,
	\loopStartFrame, 2,
	\loopEndFrame, 3,
	\loopResetFrame, 4
]);

~iparam = { arg k, v; var id = ~iparam_id[k]; s.sendMsg(\iparam, id, v); };


~bparam_id = Dictionary.newFrom([
	\syncLastLayer, 0,
	\loopEnabled, 1
]);

~bparam = { arg k, v; var id = ~bparam_id[k]; s.sendMsg(\bparam, id, v); };

//--------------------------------------------------------
//--- gui

w = Window("mlp-ctl", Rect(100, 100, 600, 200));
w.front;
f = FlowLayout.new(w.view.bounds);
w.view.decorator = f;
~tap_id.keys.size.do({ arg i;
	var k = ~tap_id.findKeyForValue(i);
	Button(w, 100@30)
	.states_([[k.asString, Color.black, Color.white]])
	.action_({ ~tap.value(k); })
});
f.nextLine;


//
// a.sendMsg('/tap', 0); // open/close
// a.sendMsg('/tap', 1); // toggle dub
// a.sendMsg('/tap', 2); // toggle mute
// a.sendMsg('/tap', 3); // stop
//
// // parameters presently affect the current layer only
// a.sendMsg('/fparam', 0, -4.dbamp); // set preserve level (during overdub)
// a.sendMsg('/fparam', 0, 1.0);
// a.sendMsg('/fparam', 1, -6.dbamp); // set record level
// a.sendMsg('/fparam', 1, 1.0);
// a.sendMsg('/fparam', 2, 6.dbamp); // set playback level
// a.sendMsg('/fparam', 2, 1.0);

/*
a.sendMsg('/quit');
*/

