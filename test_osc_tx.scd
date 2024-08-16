p = 9000;
a = NetAddr("localhost", p);

~maxLoopFrames = 1 << 25;

~tap_id = [
	\setLoop,
	\toggleOverdub,
	\toggleMute,
	\stop,
	\reset,
];

~tap = { arg id; a.sendMsg(\tap, id); };

~fparam_id =[
	[\preserveLevel, ControlSpec(0, 100, \lin, 0.1)],
	[\recordLevel, ControlSpec(0, 100, \lin, 0.1)],
	[\playbackLevel, ControlSpec(0, 100, \lin, 0.1)],
	[\fadeIncrement, ControlSpec(0, 1, \lin, 0.01)],
];

~fparam = { arg id, v; a.sendMsg(\fparam, id, v); };

~iparam_id = [
	[\selectLayer, ControlSpec(0, 3, \lin, 1) ],
	[\resetLayer, ControlSpec(0, 3, \lin, 1) ],
	[\loopStartFrame, ControlSpec(0, ~maxLoopFrames-1, \lin, 1) ],
	[\loopEndFrame, ControlSpec(0, ~maxLoopFrames-1, \lin, 1) ],
	[\loopResetFrame, ControlSpec(0, ~maxLoopFrames-1, \lin, 1) ]
];

~iparam = { arg id, v; a.sendMsg(\iparam, id, v); };

~bparam_id = [
	\syncLastLayer,
	\loopEnabled
];

~bparam = { arg id, v; a.sendMsg(\bparam, id, v); };

//--------------------------------------------------------
//--- gui

w = Window("mlp-ctl", Rect(100, 100, 600, 400));
w.front;
f = FlowLayout.new(w.view.bounds);
w.view.decorator = f;
~tap_id.do({ arg k, i;
	Button(w, 100@30)
	.states_([[k.asString, Color.black, Color.white]])
	.action_({ ~tap.value(i); })
});
f.nextLine;

~bparam_id.do({ arg k;
	Button(w, 140@30)
	.states_([
		[k.asString ++ " ON", Color.black, Color.white],
		[k.asString ++ " OFF", Color.white, Color.gray]
	])
	.action_({ ~bparam.value(i); })
});
f.nextLine;

~fparam_id.do({ arg el, i;
	var k = el[0];
	var spec = el[1];
	k.postln;
	EZKnob(w, 100@100, k.asString, spec, {
		arg sl;
		[k, sl.value].postln;
		// NB: re-scaling here because EZKnob truncates displayed values :(
		~fparam.value(i, sl.value * 0.01);
	});
});
f.nextLine;

~iparam_id.do({ arg el, i;
	var k = el[0];
	var spec = el[1];
	k.postln;
	EZKnob(w, 100@100, k.asString, spec, {
		arg sl;
		[k, sl.value].postln;
		~iparam.value(i, sl.value);
	});
});


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

