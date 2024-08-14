p = 9000;
a = NetAddr("localhost", p);

~tap_id = Dictionary.with([
	\setLoop, 0,
	\toggleOverdub, 1,
	\toggleMute, 2,
	\stop, 3
	\reset, 4
]);

~tap = { arg key; var id = ~tap_id[key]; a.sendMsg(\tap, id); };

~fparam_id = Dictionary.with([
	\preserveLevel,
	\recordLevel,
	\playbackLevel,
	\fadeIncrement
]);

~fparam = { arg key, value; var id = ~fparam_id[key]; s.sendMsg(\fparam, id, value); };

~iparam_id = Dictionary.with([
	\selectLayer,
	\resetLayer,
	\loopStartFrame,
	\loopEndFrame,
	\loopResetFrame
]);

~iparam = { arg key, value; var id = ~iparam_id[key]; s.sendMsg(\iparam, id, value); };


~bparam_id = Dictionary.with([
	\syncLastLayer,
	\loopEnabled
]);

~bparam = { arg key, value; var id = ~bparam_id[key]; s.sendMsg(\bparam, id, value); };
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

