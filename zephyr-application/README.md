# topo-ambient-zephyr

A minimal Zephyr firmware with an OpenAMP remoteproc application for STM32MP257 and i.MX93 boards.
The application reads a board-specific GPIO input to determine whether it is connected to ground, and reports over RPMsg TTY either `on` or `off`.

## Features
  - Uses Zephyr RTOS
  - Builds for the `stm32mp257f_dk/stm32mp257fxx/m33` board by default
  - Supports `imx93_evk/mimx9352/m33` when `PLATFORM=imx93`
  - Dockerized multistage image for reproducible builds
  - Extensive debug logs and comments to assist with configuring this on other boards.

## Board-specific configuration

The GPIO input and remoteproc annotation depend on the target board:

| Board | `PLATFORM` | Zephyr board target | `REMOTEPROC` | GPIO monitored |
| --- | --- | --- | --- | --- |
| STM32MP257 | `stm32mp257` | `stm32mp257f_dk/stm32mp257fxx/m33` | `m33` | `GPIOA_1` via `&gpioa 1` |
| i.MX93 | `imx93` | `imx93_evk/mimx9352/m33` | `imx-rproc` | `GPIO2_IO3` via `&gpio2 3` |

The GPIO definitions live in the board overlays:

- [`workdir/boards/stm32mp257f/stm32mp257f_dk_stm32mp257fxx_m33.overlay`](./workdir/boards/stm32mp257f/stm32mp257f_dk_stm32mp257fxx_m33.overlay)
- [`workdir/boards/imx93/nxp-frdm-imx93.overlay`](./workdir/boards/imx93/nxp-frdm-imx93.overlay)

Use the SoC GPIO name from the table above and then map it to the physical board header using the board schematic or user manual.

## Dependencies
The build is container-based; the host machine only needs the tools that
bootstrap the containerised build:

- Docker – to run the build and deploy container

## Usage

### Using Docker Compose (Recommended)

The project includes a `compose.yaml` file that simplifies building and deploying the application with the correct runtime and remoteproc settings.

**For STM32MP257 (default):**
```bash
docker compose build
docker compose up -d
```

**For IMX93:**
```bash
PLATFORM=imx93 REMOTEPROC=imx-rproc docker compose build
PLATFORM=imx93 REMOTEPROC=imx-rproc docker compose up -d
```

### Manual Build and Deploy

Alternatively, you can build and deploy manually:

```bash
# Pre-fetch the base image
# this will save a lot of time when building as docker wont otherwise cache layers this large
docker pull zephyrprojectrtos/zephyr-build:v0.28.0

# Build container (for STM32MP257)
docker build -t amcomms .

# Build container (for IMX93)
docker build --build-arg PLATFORM=imx93 -t amcomms .

# Copy to remote
docker save amcomms | ssh root@remote 'docker load'

# Launch (for STM32MP257)
ssh root@remote 'docker run -d --runtime io.containerd.remoteproc.v1 --annotation remoteproc.name=m33 amcomms'

# Launch (for IMX93)
ssh root@remote 'docker run -d --runtime io.containerd.remoteproc.v1 --annotation remoteproc.name=imx-rproc amcomms'
```

From the Linux side, you can check rprocmsg is working by running
```bash
cat /dev/ttyRPMSG0
```
on one terminal, and on the other terminal echo arbitrary messages to the same device
```bash
echo "arbitrary" > /dev/ttyRPMSG0
```
You will now see the on/off responses in the cat terminal when the switch's state changes.
