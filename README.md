Realtime Lift and Drag
---
#### Christian Eckhart, William Newey, Austin Quick, Sebastian Seibert

### General Code Info

- **Uses the `c++17` standard for `constexpr if`, structured bindings, `string_view`, and more**

- **Non-local variables are given a prefix**

Prefix | Usage | Example
---|---|---
`m_` | non-public member variables | `class Foo { int m_bar; };`
`s_` | static variables | `static float s_lastTime;`
`k_` | constants | `static const int k_maxSize = 400;`
`g_` | global variables | `extern string g_usedInfrequently;`
`u_` | shader uniforms | `uniform vec3 u_color;`
`i_` | shader invocation variables | `vec3 i_totalLift;`

- **`#defines` should only be used when absolutely necessary.
Use constant variables to preserve type and code sense**

- **Source code is organized by project into `src` and `include` directories.
`src` should contain any code that is not to be used by other projects.
`include` should contain a directory with the same name as the project, which contains any headers to be used by other projects**

- **Resources, such as shaders, meshes, images, etc. are kept in the `resources` directory and are organized by project**

## Project Overviews

### Common

Common is a static library and serves as a catch-all project for any more "general purpose" code that is used accross multiple projects. All other projects use it. This project contains the `Global.hpp` header, included nearly everywhere, which contains some absolutely fundamental typedefs and includes.

### RLD

RLD (Realtime Lift and Drag) is a static library and contains the actual meat and potatoes of the repository, even if it currently has only one header and one source file. It contains the functions for simulating lift and drag on a model given a bunch of variables. It is designed like a public interface, such that it can be easily be used by other projects and depends only on the Common project.

### UI

UI (User Interface) is a static library designed to meet the GUI and user input needs of the repo. It follows a typical object-oriented Component class heirarchy, and contains classes for text, text fields, graphs, texture viewers, and groups to organize all these. To be honest, it's a little half-baked. It provides only the functionality that was necessary at the time - it has no buttons or sliders - and it's probably not the best designed. However, it works, and is infinitely better than a million hard-defined constants and pages of reused rendering code.

### Visualizer

Visualizer is an executable for visualizing the realtime lift and drag simulation at work. It features a front and side view of the model in its current simulation state. The simulation can be stepped through one slice at a time, or whole sweeps at once. Additionally, it shows curves of the lift, drag, and torq values per slice and per angle of attack. Visualizer uses the Common, RLD, and UI projects.

**Controls**
- `space` to step to the next slice
- `shift-space` to do a whole sweep, or finish the current one
- `ctrl-space` to toggle angle of attack auto progression. When sweeping, will automatically progress to the next angle of attack
- `up` or `down` to increment or decrement the angle of attack and perform a sweep
- `right` or `left` to increase or decrease the angle of attack and perform a sweep once released
- `o` and `i` to increase or decrease the rudder angle
- `k` and `j` to increase or decrase the elevator angle
- `m` and `n` to increase or decrease the aileron angle
- The graphs can be manipulated
  - `drag` to move the curves
  - `scroll` to zoom in or out
  - `shift-scroll` to zoom the range only
  - `ctrl-scroll` to zoom the domain only
  - `=` to reset
- The texture viewers can be manipulated
  - `drag` to move the texture
  - `scroll` to zoom in or out
  - `=` to reset
- The model viewer can be manipulated
  - `drag` to rotate the camera around the origin
  - `scroll` to zoom in or out
- Many of the variable texts can be edited by clicking the value, typing whatever, and then pressing `enter`

### FlightSim

FlightSim is an executable and a basic flight simulator using the RLD technology. Uses the Common, RLD, and UI projects.

**Controls**
- `w` or `s` to pitch up or down
- `a` or `d` to yaw left or right
- `q` or `e` to roll left or right
- `space` to thrust
- `k` to toggle wind-view
- `shift` when in wind-view to view orthographically
- `r` to reset the simulation
- `p` to pause the simulation
- `drag` to rotate the camera
- `esc` to exit

### ClothSim

ClothSim is an executable and cloth simulator using the RLD technology. Primarily set up for simulating a jib sail, but can be set up to simulate a flag or any other cloth object. Features option to see forces on each cloth vertex, or visualize the cloth vertex constraints. Additionally shows the front RLD texture. Uses the Common, RLD, and UI projects.

**Controls**
- `drag` to rotate the camera
- `scroll` to zoom in or out
- `left` or `right` to rotate the sail CCW or CW
- `up` or `down` to extend or retract the clew
- `right-click` to "blow" on sail
- `z` to disable rld simulation
- `x` to do one sweep per frame
- `c` to do one slice per frame
- `space` to step one slice
- The texture viewer can be manipulated
  - `drag` to move the texture
  - `scroll` to zoom in or out
  
The cloth simulation is done via a basic varlet integration approach in a similar manner as described in [this](https://viscomp.alexandra.dk/?p=147) article. There are two key differences however. First, the data is stored on the GPU and the algorithm is performed in parallel using OpenGL. Second, while the approach in the article is tailored specifically for rectangular cloth, we support cloth of any shape, so long as appropriate constraints can be provided. Although lacking self-collision functionality, the result is performant and otherwise realistic.
