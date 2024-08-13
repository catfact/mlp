# mlp

multi-layer loop pedal

(current state: early development / POC)

---

mlp is a multi-layer loop pedal, loosely inspired by the "unquantized multiply" function on the 1994 gibson/oberheim "echoplex digital pro." 

at present, this repo includes `mlp-cli`, a standalone, cross-platform audio/OSC program.

## mlp-cli usage

the present interface is extremely primitive. launching `mlp-cli` will do the following:

- attach to the default system audio device(s) for input and output
- open a UDP socket on port 9000 for receiving OSC messages

and it responds to the following OSC messages:

- `/tap {int}`: tap a virtual button. the integer argument is the button number:
  - 0: open/close a loop on the current layer. on closing, the next layer is selected.
  - 1: toggle overdubbing on the current layer.
  - 2: toggle muting of the current layer (which continues to update its loop position and trigger syncs.)
  - 3: stop the current layer
  
- `/quit`: stop the program

the primary feature of `mlp` is that layers can trigger each other. the current behavior is that when a layer reaches the endpoint of its loop, it triggers a reset to the start of the loop for the layer below it. this is a simple way to create a "multiply" effect, where the topmost layer acts as a "leader" and specifies the loop length for the layers below it.

the "stop" command stops the topmost layer playing. this has the interesting effect of also changing the total loop length - the next-lowest layer is now the leader.

## roadmap

i intend to continue adding features to `mlp` and to port it to other platforms. there is no particular timeline for this, nor are my final goals very clear. but as a devlopment exercise, i intend to work quickly to put it in a musically useful state, and to focus on refinements that i find most necessary. the following goals are fairly definite, and not overly ambitious:

### audio features

- per-layer and global varispeed options (at least for playback - not sure if i want to deal with varispeed recording for this project... hm)
- per-layer stereo field manipulation and spatialization
- per-layer filtering and saturation
- per-layer time offset control with smoothing

### control features

- more parameter control (preserve/record levels, time offsets, fade durations, etc)
- explicit layer selection
- MIDI mapping 
- configuration saving/loading

### platform support / integration

- DAW plugin wrapper
- daisy seed/patch/petal wrapper
- norns script