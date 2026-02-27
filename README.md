# Lightbulb Moment

Reads a switch over GPIO pins on an M class cpu, reports switch state over Remoteproc Message, then a web application on the A class reads this and displays a lightbulb in either the on or off state. The lightbulb state is described by an LLM in any user-specified style.

Features: SVE, NEON

## Usage

The easiest way to deploy is using `topo`. Download and install `topo` from [here](https://github.com/arm/topo)

### Clone the project:
```bash
topo clone ./lightbulb https://github.com/Arm-Examples/topo-lightbulb-moment.git
```

Or to use a custom prompt non-interactively:
```bash
topo clone ./lightbulb https://github.com/Arm-Examples/topo-lightbulb-moment.git -- LLM_PROMPT_PRESET="pirate"
```

You can use any style description (e.g., "pirate", "shakespearean english", "haiku", "detective noir").

Topo uses [remoteproc-runtime](https://github.com/arm/remoteproc-runtime) to deploy containers to remote processors.
If it is not already installed, you can install it using topo:
```bash
topo install remoteproc-runtime --target <ip-address-of-target>
```

### Build and Deploy the project:
```bash
cd lightbulb
topo deploy --target <ip-address-of-target>
```
