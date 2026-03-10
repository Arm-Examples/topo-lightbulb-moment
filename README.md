# Lightbulb Moment

> This project is a [Topo](https://github.com/arm/topo) template and follows the [Topo Template Format Specification](https://github.com/arm/Topo-Template-Format).

Reads a switch over GPIO pins on an M class cpu, reports switch state over Remoteproc Message, then a web application on the A class reads this and displays a lightbulb in either the on or off state. The lightbulb state is described by an LLM in any user-specified style.

## Requirements

- Either an stm32mp257 or imx93 board running Linux, with SSH root access and docker installed.
- ~15GB free disk space on the build machine (the Zephyr build image alone is over 5GB).

Before deploying to your board, you must ensure the board is set up.

### IMX93 setup

The imx93 may crash when you start the m-class core using remoteproc. You need to `run prepare_mcore` in the u-boot menu, [guide here](https://github.com/arm/remoteproc-runtime/blob/main/docs/IMX93_WORKAROUNDS.md).

### STM32MP257 setup

The stm32 may need you to stop the pre-existing demo program running on the m-core.
You can ensure both cores are stopped using this ssh command:
```bash
ssh root@<target hostname> 'echo stop > /sys/class/remoteproc/remoteproc1/state && echo stop > /sys/class/remoteproc/remoteproc0/state'
```
Note: if nothing is currently running on the cores, you will see "Invalid argument" errors — this is safe to ignore.


## Usage

The easiest way to deploy is using `topo`. Install it by following the instructions at [github.com/arm/topo](https://github.com/arm/topo).

### Clone the project:

The clone step will ask you for the following build arguments:
- `PLATFORM`: either `stm32mp257` or `imx93`
- `REMOTEPROC`: either `m33` (for stm32mp257) or `imx-rproc` (for imx93)
- `LLM_PROMPT_PRESET`: any style description (e.g., "pirate", "shakespearean english", "haiku", "detective noir")

Clone and set arguments:
```bash
topo clone ./lightbulb https://github.com/Arm-Examples/topo-lightbulb-moment.git
```

Topo uses [remoteproc-runtime](https://github.com/arm/remoteproc-runtime) to deploy containers to remote processors.
If it is not already installed, you can install it using topo:
```bash
topo install remoteproc-runtime --target <ip-address-of-target>
```

### Build and Deploy the project:

Note: the first build will download several large Docker images and may take 10+ minutes.

```bash
cd lightbulb
topo deploy --target <ip-address-of-target>
```
