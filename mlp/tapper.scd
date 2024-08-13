p = 9000;
a = NetAddr("localhost", p);

a.sendMsg('/tap', 0); // open/close
a.sendMsg('/tap', 1); // toggle dub
a.sendMsg('/tap', 2); // toggle mute
a.sendMsg('/tap', 3); // stop

/*
a.sendMsg('/quit');
*/

