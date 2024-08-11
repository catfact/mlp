p = 9000;

//a = NetAddr("osc.udp://eear", p);

a = NetAddr("localhost", p);

a.sendMsg('/tap', 0);
a.sendMsg("/quit");

a.sendMsg('tap', 0);
a.sendMsg('quit');


a.sendMsg('example');
a.sendMsg('quit');
