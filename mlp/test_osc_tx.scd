p = 9000;
a = NetAddr("localhost", p);

a.sendMsg('/tap', 0); // open/close
a.sendMsg('/tap', 1); // toggle dub
a.sendMsg('/tap', 2); // toggle mute
a.sendMsg('/tap', 3); // stop

// parameters presently affect the current layer only
a.sendMsg('/param', 0, -4.dbamp); // set preserve level (during overdub)
a.sendMsg('/param', 0, 1.0);
a.sendMsg('/param', 1, -6.dbamp); // set record level
a.sendMsg('/param', 1, 1.0);
a.sendMsg('/param', 2, 6.dbamp); // set playback level
a.sendMsg('/param', 2, 1.0);

/*
a.sendMsg('/quit');
*/

