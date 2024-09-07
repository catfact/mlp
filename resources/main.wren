System.print("hello from main.wren (embedded)")

//-----------------------------------------------------------
//--- wren classes (customizable)

class MidiIn {

  //--- customize these method definitions

  static noteOn(channel, note, velocity) {
    System.print("wren noteon: %(note), %(velocity)")
  }

  static noteOff(channel, note, velocity) {
    System.print("wren noteoff: %(note), %(velocity)")
  }

  static controlChange(channel, number, value) {
    System.print("wren cc: %(number), %(value)")
  }

  static pitchBend(channel, lsb, msb) {
    var val = (msb << 7) | lsb
    System.print("wren pitchbend: %(val)")
  }
}

//-----------------------------------------------------------
//--- FFI output class (don't edit)

class Mlp {
    foreign static BangSet()
    foreign static BangStop()
    foreign static BangReset()

    foreign static ParamLayerPreserveLevel(layer, value)
    foreign static ParamLayerRecordLevel(layer, value)
    foreign static ParamLayerPlayLevel(layer, value)
    foreign static ParamLayerFadeTime(layer, value)
    foreign static ParamLayerSwitchTime(layer, value)

    foreign static GlobalMode(index)
    foreign static LayerSelect(index)
    foreign static LayerReset(index)
    foreign static LayerRestart(index)

    foreign static LayerWrite(layer, state)
    foreign static LayerClear(layer, state)
    foreign static LayerRead(layer, state)
    foreign static LayerLoop(layer, state)

// NYI
//    foreign static LayerMode(layer, value)
//    foreign static LayerLoopStartFrame(layer, value)
//    foreign static LayerLoopEndFrame(layer, value)
//    foreign static LayerResetFrame(layer, value)
//    foreign static LayerTriggerFrame(layer, value)


}

System.print("done; Mlp = %(Mlp)")