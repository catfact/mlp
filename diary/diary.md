
# 2024/8/10

thinking about loopers again. i'd like to try one with a "multiply" feature. this is an exercise, so  approach it minimally, focusing on supporting only the feature under investigation. (at first.) 

the "multiply" operation on a loop is perhaps most familiar / defining as appears in the oberheim/gibson "echoplex digital pro." various flavors of the feature have appeared in loop pedals, but rarely with much flexibility. so this looper will focus on managing multiple loop layers with various heuristics for synchronization and layer management.

began conceptualizing this in supercollider, added `mlp.scd` with stubs. but now i'm not sure SC seems right: things like data transfer between .ar/.kr/logic parts of code, are clunky and imprecise. leaning towards either a graph-based prototype, or straight to c++.

(_softcut_ is also fairly well suited for this sort of thing, but the ability to synchronize voices precisely is not quite up to the requirements. someday i would like to improve on it or build up a new system with all its features; maybe this could be a start...)

this application is also a good candidate for a daisy/petal port: it's simple, has low CPU requirements, and i think a 4-switch interface is about right. so... i'll go straight for the c++ implementation.

end of day: pushed first draft of untested c++ implem. (initially had all the low-jitter boilerplate in there, but too complicated/unnecessary for POC.)

**time:** ~2hrs

# 2024/8/11

first run of the first-draft program, fixed issues with basic audio setup.

ready to test actual processing, i stepped in a rabbit hole: 
- liblo suddenly seems to not work on my machine, at least during debug (macbook air m1. oddly i don't seem to have used liblo before in any projects on it?) the symptom is frustrating - messages just disappear. 
- i don't want to debug this, so maybe it's time to investigate other options, like `oscpack`
- with `oscpack` i can receive messages (yay.) but, it triggers an assertion because numerical type definitions are wrong for this architecture (boo)
- so i have quickly forked `oscpack` and hacked in a fix. now i must waste a little more time learning its API and porting the POC...

... finished the port, OSC reception seems ok. crashing somewhere...

... fixed crash, fixed layer-reset logic, fixed layer-stop logic. things seem to be working as expected; next up is building out more features (particularly de-clicking crossfades.) 

**NTS**: design-wise, i would love to be able to dynamically re-define things like layer-sync behavior, tap logic etc., to be definable in lua or some other scripting language. but, it's a special challenge to have user-defined script executed on realtime audio thread, and i've yet to build a system that does this well. (have looked at e.g. cockos EEL2, and the possibility of lua with granular control of GC, but the fact is that i don't really enjoy software enough to want to solve those problems.)

**time**: ~3hr

# 2024/8/12

implemented crossfading, which seems to be working after a couple bugs. next issues:

- (1) when closing a loop, we immediately update the frame offset for the previous layer, causing a click
- (2) enabling/disabling write/read for a layer is still done by setting a boolean, causing a click
- (3) stopping a layer likewise is a boolean operation that causes a click
- 
... ok, (1) seems fixed
... ok actually threw in a fix for (3) as well.

(2) calls for the general addition of smoothed parameters, probably updated at vector rate, so that seems like a good starting place for next session.

... ok, added "smoothed switches" for overdubbing and muting layers. didn't yet get to more general smoothed parameters or exposing more OSC controls. probably next time.

also wrote a readme and spent a bit more time on this diary.

**time**: ~3.5hr

# 2024/8/13

messed around with project configuration so that `mlp-cli` can be self-contained: added rtaudio as submodule so we can force it to build static libraries. also got codesigning set up. posted a v0.0.0 release with the bare minimium of features, and a signed macOS build. (mostly to test if that works for people.)

added setters and OSC controls for parameters. (preserve level during overdub, record level, playback level.) these affect the current layer only.

next block tasks, in no particular order:
- add explicit layer selection
- add at least minimal stereo positioning
- tweak the behavior of "set loop" tap when all layers are used (wrap and re-set?)

and likely the next block after that:
- timed taps (optionally map duration to fade time)
- MIDI / keybinding
- conf file
- varispeed
- iplug2 or juce wrapper

**time**: ~1.5hr

# 2024/8/14

### some random thoughts

- i don't want to futz around with more platforms at the moment, i want to focus on the osc/cli version, to nail down the functionality. i also don't want to mess with embedding an interpreter for controller bindings and configuration. the benefits to this focus are clear, but it also has downsides:
  - harder to distribute / collaborate with non-developers
  - danger of feature creep when there is no UI to guide the design
  - more components / potential time-sinks to manage (like the liblo/oscpack rabbit hole above)

so, i think it's important to keep a final UI in mind. for now assuming compatibility with Daisy Petal. i will also add a simple supercollider interface patch including MIDI responders, in lieu of adding rtmidi (or something) to the c++ project.

- "insert" mode. for some reason not something i've been drawn to in the past. but this is about trying things out, so i'd like to do it. my implem idea so far is something like:
    - each layer has an optional "pause frame." when the active phasor reaches this frame, the layer pauses itself and triggers a different layer to start (probably the one above it.) 
    - the other layer doesn't loop when reaching its loop endpoint, but rather triggers the first layer (likely below it) to resume.
    - when closing a loop in "insert" mode, the close position becomes the pause frame for the next-lowest layer.

- again, the key to the design is behavior around what happens when a layer wraps, and how this affects other layers. (reset / pause / resume, etc.) keeping on eye on how this behavior can best be encapsulated, maybe scripted.

- i do want to add varispeed writes. considering the resampling-window-collision problem and wondering if there is a more effective way to handle it than was done for _softcut_. viz., without adding a gap of N samples between the read and write heads. the main thing that comes to mind would be to keep a little extra buffer space for the interpolated writes, and "finalize" those changes to the main buffer once the read head has passed. (and the follow-on concern there is what happens when rates are modulated arbitrarily...)

### changes

added this stuff to the main kernel / API:

- added explicit layer selection
- cleaned up and clarified "loop start frame" versus "reset frame"
- added setters for loop start/end/reset
- added "loop enable" (1shot mode)
- add "sync last layer" flag (for disengaging "multiply" mode and engaging "async" mode)
- add explicit commands to reset current layer / any layer
- add fade time control
- tweak the way layer selection is advanced (now on loop open when appropriate)
- layer advance now wraps around

**TODO**: 

- also would like a mode flag where layer selection not advanced, only set explicitly, and loop open/close just re-sets the loop endpoints for the current layer 
- some OSC queries and callbacks would be nice. (query loop positions, callback for layer selection, callback for loop wrap? etc)

...added glue code for all the above. still haven't really tested, it needs a bit more of a proper interface for OSC tx.

... partially updated the test supercollider script with GUI. my refactoring seems to have introduced some bugs in loop set/reset logic... 

... fixed bug. now, layer selection wraps on advance or stop. 

main thing that feels weird now is that first layer doesn't trigger resets on last layer, but otherwise there is no differentiation between layers or feedback on which is selected, so it seems arbitrary. one remedy might be to say: when all layers are stopped, the next one to start will become the "innermost."

**time**: ~3.5hr

# 2024/8/17

my time has been sort of atomized lately, so i have added a few things but it has been scattered, mostly working in the abstract and rarely with a chance to test/play.

finally today i've taken at least enough time to play with the present OSC interface and supercollider GUI. it is shonky as heck, but essentially the "nested multiply" behavior is already interesting to use - i think it would be especially attractive for "beat-driven" sampling flows. i have not tested very systematically but also haven't noticed any glaring problems.

design- and architecture-wise, some things have been clarified by coding and conceptualizing. i'm now thinking in terms of layer _actions_ (**reset**, **pause**, **resume**,) and _conditions_ (**trigger**, **wrap**.) conditions are checked at the end of each frame. a condition on one layer can trigger a set of actions on itself and/or other layers. internal looping of a layer is a special default case that can be modulated arbitrarily. other layer parameters include frame positions for reset (which defines an action/input), trigger (a condition/output), and loop endpoint (condition/output with default self-action.) 

this feels like a good place to me. i think that with this small set of defined actions/conditions, all the EDP-derived behaviors can be acheived, as can many others. but it is also (hopefully) not overwhelmingly complex either from an implementation or usage standpoint. viz., regarding the latter, behaviors can be defined declaratively, without needing "scripting" or "programming", and without needing to interpret code during runtime.

oh! i almost forgot. conditions can have _counters_,(*) and setting a condition counter is also an action. nonzero counter values are decremented when a condition is met; the action list only "fires" on subsequently non-zero values, and negative counter values are ignored. a counter value of 1 is needed to implement some of the EDP modes with this system, and i think other values could be pretty interesting ("stutter" effects and so on.)

(*or should the counter be on an action instead? or a condition->action binding? or both/all of the above? hm...)

absurdly, i started a latex file (`mode-spec.tex`) describing the "theoretical" side of the design. pretty ridiculous, but something about writing in tex activates a formal and considered mindset, which feels useful sometimes.

**time spent**: 3-4hr? very spread out / multitasked over last few days.