# topo-ambient-zephyr

A minimal STM32MP25x Ambient zephyr firmware with an openamp remoteproc application.
The application reads GPIO pin 'gpioa 1' to determine whether it is connected to ground, and reports over rpmsg tty either 'on' or 'off'

## Features
  - Uses Zephyr RTOS
  - Builds for the `stm23mp257f_dk/stmp257fxx/m33` board by default
  - Dockerized multistage image for reproducible builds
  - Extensive debug logs and comments to assist with configuring this on other boards.

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

### Configuration

The compose.yaml supports the following environment variables:

- `PLATFORM` - Target platform (`stm32mp257` or `imx93`, default: `stm32mp257`)
- `REMOTEPROC` - Remoteproc name (`m33` for STM32MP257, `imx-rproc` for IMX93, default: `m33`)

These can be set in a `.env` file or passed directly:
```bash
# Create a .env file
echo "PLATFORM=imx93" > .env
echo "REMOTEPROC=imx-rproc" >> .env

# Then run
docker compose build
docker compose up -d
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
