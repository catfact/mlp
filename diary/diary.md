
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

## TODO

- i'm tempted to switch focus to a plugin, but not sure:
  - the next functional piece of implementation is to try this refactor / redesign of the layer logic
  - but regardless of the UI, it really needs feedback from the state of the system,
  - i don't really relish doing that in SC/OSC, but i suppose it's probably less time than doing it in plugin UI,
  - and this is all about the most minimal path to a good design, so i guess that's the answer
- and tangentially, varispeed is on the menu, including reversal, so:
  - elements of the layer logic will need to be rebuilt to accomodate this
  - but at present, i'm ok with continuing using simple integer frame count while the logic is being worked out,
  - just bearing in mind that e.g. a condition will end up being a test that's aware of history and direction, rather than just a simple equality check

**time spent**: maybe ~4hr? very spread out / multitasked over last few days.

# 2024/8/19

finished rewrite of layer logic. this was one of those little spirals where things were overcomplicated, then reduced. ah well.

  now it is reasonably clean, and the mode system essentially works. this is an interim level of generality, where layer actions and conditions are somewhat well-defined lists of opcodes, but mode behaviors are still hardcoded as lambda functions in c++. at some point, we want runtime-definable opcode lists here, but hardcoding is a good way of exploring the space of what is needed.

however there are at least some bugs around overdub behavior right now, so keeping it on WIP branch.

need to test insert mode, and very much need to have UI feedback soon on the state of the system.

**time**: ~3.5hr

# 2024/8/20

added a lot of per-layer control glue and made things generally more granular. probably addressed overdub issues in the process. still needs testing, more effective GUI, state feedback. 

**time** ~1hr

# 2024/8/29

spent a couple days focused on a quick paying gig, which is rather crucial. things are totally busy and chaotic, but it feels important to dip back into this project to maintain momentum every few days.

a more complete GUI from supercollider feels like low hanging fruit right now, so i'll give it a go...

[several days pass..]

... well! that did not really go as predicted. every time i sat down to make a SC gui it felt like a silly prospect. the project has entered a stage of complexity that i find familiar: it needs a fairly complex interface to test everything, all the next steps are fiddly and complex, and it represents a real hurdle in terms of building enough momentum to get through it.

anyways, i ended up talking myself into going directly to JUCE to build a GUI. this is after considering several other directions, but the bottom line is that for a plugin release i will end up doing a JUCE UI anyways, and i really don't to do this part twice.

so i've set things up with a top-level graphics component class that can be used in an audio plugin project, or in a graphical application that serves only as an OSC interface with a running `mlp` audio process.

that component now has most things exposed, it's not very pretty but the layout is at least comprehensible. i need to add a couple more widgets, for layer and mode selection. 

and, in another plot twist, i am encountering sudden failure loading the coreaudio framework on macOS, when running the rtaudio-based `mlp-cli` program:

```
dyld[7341]: dyld cache '(null)' not loaded: syscall to map cache into shared region failed
dyld[7341]: Library not loaded: /System/Library/Frameworks/CoreAudio.framework/Versions/A/CoreAudio
  Referenced from: <6B074DFD-CC05-329D-8E2D-BEAD048B1DFE> /Users/emb/code/mlp/cmake-build-debug/mlp-cli
```

this happens as soon as the program launches and loads frameworks, before entry to `main()`.

the issue presents on the `wip` branch, but not on the `main` branch. i can't explain this; there don't seem to be any relevant changes between them:
- i suspected that pulling in JUCE cmake stuff might have been causing the conflict (loading the coreaudio framework twice.) but leaving it out doesn't help!
- i have to assume it is something about building including rtaudio as a static library. but again i can't see any relevant difference. the issue seemed to simply appear out of nowhere, and i'll need to bisect history to start trying to isolate it.

so! having burned an hour or two being stuck on this, i am considering shelving the rtaudio client for now and focusing fully on a JUCE client. (which, if needed, could also forgo the GUI in favor of an OSC interface as a runtime option.) i've encountered a lot of little issues with rtaudio over the years, most are never explained or resolved really, so maybe it would be smart to just stop wrangling multiple platform audio wrappers in one project.

i think the total time i've spent since last diary update is getting up to the 8 or 10 hour range, spread out over more than a week (i have a lot going on at home these days!), which feels like the maximum time to wait between diary updates. so here we are, despite being in a not terribly satisfying state.

on the other hand, last time i _did_ have things working was pretty satisfying! the layer condition/behavior system seems to work and make sense (more thoughts on this later,) and a number of bugs have been ironed out regarding auto-layer-selection logic, and the special treatment of "inner" and "outer" layers regarding how they interact with others. working with the system in the "unquantized multiply" mode, with round-robin layer selection, feels predictable yet with clear potential for complexity. so overall i'm feeling pretty good about the direction.   

**time**: ~10hr


# 2024/8/29

ok managed to put together a JUCE project pretty efficiently and with no real issues. the GUI is ugly as heck but it does most of the things that it needs to.

----

final touches that the basic GUI still needs to be "complete" :

- mode buttons
- current-layer selection buttons
- labels for slider parameters

oh, and for sure:
- reflect the initial state of the system

and maybe:
- live position indicators? (but, a rabbit hole: we do not actually report loop length, so hard to know how to scale.)

- architecturally, at some point there should be a clean way of specifying float parameter rangs / curves / mappings.

----

i still haven't added any facility to change modes. i think i'm basically just scared to test Insert and any other new modes, because i suspect some deep flaw in my logic around imagining that they are [pssible with this limited set of "sync opcodes" i've come up with. bah! how silly. i will get past this little obstacle very soon.

the other thing that this version needs pretty badly is some kind of MIDI mapping. not sure of the best way to do this. the simplest is with just a configuration file. i think a simple YAML for now, which could be serialized and managed by the plugin framework.

---

had some more thoughts / realizations about planned features:

- filters. each layer needs some for sure. i think two per layer should do it; the main questions are:
  - how many parameters to expose? the minimal version would be HP and LP in series, only cutoff controls. maximally, each could offer arbitrary mode-blending, resonance controls, maybe saturation, and the routing could be series or parallel.
  - how many models / which models to expose? i have a nice 2p/4p ladder model that might suffice, maybe with different saturators for flavor.

- inter-layer audio routing matrix. with levels of course, but i would _really_ like to have (optional) stereo processing in that matrix as well. (and on layer->main outputs.) i have a nice band-split stereoizer module that could be adapted to stereo->stereo operation.

- mode specification. i have been thinking of this in terms of a DSL or arbitrarily complex data structure, but in reality it could almost just be... a patch matrix? with per-layer conditions on one axis, and per-layer actions on the other?? (a pretty big and unwieldy matrix to be sure.) of course a sparse text-based representation would still be useful.

**time**: ~4hr

---

# 2024/9/4

hm, has been a few days again. lots of stresses right now, working on the looper is a welcome and nearly compulsive distraction, but there are few times in which to indulge it.

let's see, i suppose i mostly added GUI. there are more individual controls, a "lookandfeel" is at least existing, there is an ugly but working per-layer position indicator. GUI is always a slog for me, and i tend to go straight for physical controls and simple visual feedback, skipping the design iteration that is needed to make a really good interface. so this part is a little challenging.

which is to say, "doldrums" seems a bit apropos, as i start to feel that the loop-layer behavior logic, and the audio->control layer communication spec, are becoming more spaghettified than i'd like. and of course the GUI, which is random and busy, and also effects an architecture that seemed clear once but becomes ever more tortured. on some level i know the "right" move is to redo it now, with a better-considered plan for state flow... but. this isn't an exercise in doing things the "right way," rather it's to beeline for MVP and see how well it works. (maybe quite buggily.)

anyways! i'm going to forge ahead. it's fun to play with already. somehow there is yet more to do for basic playability, starting with some controls to add (doesn't need to be all of these):
 - individual layer: (bang) reset, restart, pause, resume, stop, store reset, store trigger
 - global: (bang) stop all, reset all, (toggle)  auto select, clear on set, read on set, write on set

and, ech, gui stuff:
- need to display more arbitrary scalar values per layer, such as loop endpoint, reset position etc. unfortunately specifying such things is a point of weakness in the current GUI architecture (such as it is) 

and i think really the most effective way to add powerful midi mapping, at least for my purposes, is to just embed a scripting interpreter. either wren or lua. i personally don't mind hardcoding controller numbers, but a simple alternative could be to expose a set of generic integer controls (7- or 14-bit) that can be MIDI-learned in a host DAW.

the main reason that a scripting layer makes sense to me, and is worth skipping ahead towards, is that the exposed controls are pretty low-level. common musical functions require manipulating several controls synchronously in different ways: for example "overdub" engaging `Write` while disengaging `Clear` (and possibly setting `Preserve(1)`.) it just seems most pragmatic to script this one way or another.

so:
 - add scripting interpreter, glue code, supporting GUI (ech)

that is probably enough for a 0.0.2 alpha release. 

i think other features can wait, but at the moment (and in order of challenge) those might look omething like this for a 1.0.0 milestone:
- varispeed (the biggest lift and scariest refactor)
- settable loop and condition counters
- arbitrary sync behavior specification
- filters
- routing matrix

**time**: ~6hr