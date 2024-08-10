s.waitForBoot {

	//--- constants
	// mono input flag (read once at launch)
	~in_mono = true;

	// buffer duration in seconds
	~buf_dur = 60 * 60;
	~buf_frames = ~buf_dur * s.sampleRate;

	//--- The Buffer
	~buf = Buffer.alloc(s, ~buf_frames, 2);

	//===============================================================
	//====== DSP

	//--- busses, collected
	b = ();
	// write-head position
	b.wr_pos = Bus.audio(s, 1);
	// read-head position
	b.rd_pos = Bus.audio(s, 1);

	//--- synthdefs
	SynthDef.new(\simplePhasor, {
		var x = Phasor.ar(\reset.tr, rate: \rate.kr(1) * SampleDur.ir);
		Out.ar(\out.kr(0), x);
	}.send(s);

	// wait for all defs to be compiled and sent to server
	s.sync;

	//--- synths, collected
	z = ();

	// write-head phasor
	z.wr_phase = {
		///....
	}.play(s);

	// read-head phasor
	z.rd_phase = {
		//....
	}.play(s);

	// writer
	z.wr = {
	};

	// reader
	z.rd = {
	};

	//===============================================================
	//====== controller / logic

	// last tap time
	t = -1;

	// controller state
	c = (
		// true if we are recording the base loop
		rec: false,
		// true if we are playing back
		play: false,
		// true if we are overdubbing
		dub: false,
		// true if we are setting the multiple length (mid-tap)
		mul_set: false
	);


	// action handlers,
	a = ();
	// handle a tap on the "main" play/loop/dub switch
	a.tap_loop = {
	};
	// handle a tap on the "mul" switch
	a.tap_mul = {
	}
	// handle a tap on the "stop/clear" switch
	a.tap_stop = {
	};


	// midi responder
	//		MIDIClient.

	};