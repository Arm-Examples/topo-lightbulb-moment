# Lightbulb Moment

> This project is a [Topo](https://github.com/arm/topo) template and follows the [Topo Template Format Specification](https://github.com/arm/Topo-Template-Format).

An Arm Cortex-M core reads a physical switch over GPIO and reports its state to a Cortex-A core via RPMsg. A web application on the Cortex-A core displays a lightbulb - on or off - described by an LLM in any user-specified style.

## Requirements

- Either an STM32MP257 or i.MX93 board running Linux, with SSH root access and Docker installed.
- ~15GB free disk space on the build machine (the Zephyr build image alone is over 5GB).
- Before deploying to your board, you must ensure the board is set up (see below).

### STM32MP257 setup

Depending on the Linux image your board is running, you may need to stop a pre-existing demo program running on the Cortex-M core.
You can ensure both cores are stopped using this SSH command:

```bash
ssh root@<target hostname> 'echo stop > /sys/class/remoteproc/remoteproc1/state; echo stop > /sys/class/remoteproc/remoteproc0/state'
```

Note: if nothing is currently running on the cores, you will see "Invalid argument" errors — this is safe to ignore.

### i.MX93 setup

The i.MX93 board requires a one-time U-Boot configuration to prepare the Cortex-M core for remoteproc. Without this, the board's remoteproc drivers may not start correctly. Run `prepare_mcore` in the U-Boot menu before first use — see the [setup guide](https://github.com/arm/remoteproc-runtime/blob/main/docs/IMX93_WORKAROUNDS.md) for details.

## Usage

The easiest way to deploy is using `topo`. Install it by following the instructions at [github.com/arm/topo](https://github.com/arm/topo).

In the commands below, `<user@hostname>` refers to the SSH address of your STM32MP257 or i.MX93 board.

### Clone the project using `topo`

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
topo install remoteproc-runtime --target <user@hostname>
```

### Build and deploy the project

Note: the first build will download the Zephyr toolchain Docker image and other dependencies (~15GB total), so it may take a while.

```bash
cd lightbulb
topo deploy --target <user@hostname>
```
