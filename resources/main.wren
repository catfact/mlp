System.print("hello from main.wren (embedded)")

class Output {
  foreign static bang(a, b)
}

class MidiIn {
  static noteOn(data) {
    var num = data.bytes[0]
    var vel = data.bytes[1]
    System.print("wren noteon: %(num), %(vel)")
    Output.bang(num, vel)
  }

  static noteOff(data) {
    var num = data.bytes[0]
    var vel = data.bytes[1]
    System.print("wren noteoff: %(num), %(vel)")
    Output.bang(num, vel)
  }
}
