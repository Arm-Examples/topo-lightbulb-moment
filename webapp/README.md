# Switch Monitor Web Server

A user-space Linux Python web server that monitors `/dev/ttyRPMSG0` for switch state changes from a remoteprocessor and displays the state in real-time on a web page.

## Features

- Monitors `/dev/ttyRPMSG0` for newline-delimited state messages (`on\n` or `off\n`)
- Real-time updates via WebSocket (no page refresh needed)
- Displays custom images for ON and OFF states
- Dockerized for easy deployment
- Automatic reconnection if device disconnects

## Prerequisites

- Docker, Podman or compatible engine installed on your system
- Access to `/dev/ttyRPMSG0` device

## Setup

### 1. Build and transfer the Docker Image

```bash
podman build -t webapp .
podman save webapp | ssh root@remotedevice 'podman load'
```

### 2. Ensure the remote processor application is running, and /dev/ttyRPMSG0 exists

In order to bind the device, `/dev/ttyRPMSG0` must exist.

### 3. Run the Container

Run the container with access to the `/dev/ttyRPMSG0` device:

```bash
podman run --device=/dev/ttyRPMSG0:/dev/ttyRPMSG0 -p 3000:3000 webapp:latest
```
The `--device=/dev/ttyRPMSG0:/dev/ttyRPMSG0` flag gives the container access to the device, but podman must be run as root.

### 4. Access the Web Interface

Open your browser and navigate to:

```
http://localhost:3000
```

Or from another machine on the network:

```
http://<your-host-ip>:3000
```

## Running Without Docker (Development)

If you want to run it directly without Podman:

### 1. Install `uv`

Follow the [uv installation guide](https://docs.astral.sh/uv/getting-started/installation/) for your platform.

### 2. Create the virtual environment and install dependencies

```bash
uv sync
```

### 3. Run the Application

```bash
uv run python src/app.py
```

### 4. Access the Web Interface

Open your browser to `http://localhost:3000`
