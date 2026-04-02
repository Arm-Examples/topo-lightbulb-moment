import os
import threading
import time
from typing import Any, Dict, Literal
from flask import Flask, render_template
from flask_socketio import SocketIO, emit
import requests

app = Flask(__name__)
app.config["SECRET_KEY"] = os.environ.get("SECRET_KEY", "dev-secret")
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")

current_state = "off"
current_message = "Initializing..."
state_lock = threading.Lock()

MESSAGE_INTERVAL = int(os.environ.get("MESSAGE_INTERVAL", "10"))
OLLAMA_URL = os.environ.get("OLLAMA_URL", "http://ollama:11434")
LLM_PROMPT_PRESET = os.environ.get("LLM_PROMPT_PRESET", "modern english")
MODEL_NAME = "smollm:135m"
DEFAULT_PLATFORM = "stm32mp257"
PLATFORM = os.environ.get("PLATFORM", DEFAULT_PLATFORM).lower()
DEVICE_PATH = "/dev/ttyRPMSG0" if PLATFORM == DEFAULT_PLATFORM else "/dev/ttyRPMSG1"

PINOUT_CONFIGS: Dict[str, Dict[str, Any]] = {
    "stm32mp257": {
        "platform_name": "STM32MP257",
        "target_gpio": "GPIOA_1",
        "ground_gpio": "GND",
        "highlight_row": 3,
        "row_description": "4th row from the top",
        "labels_left": ["3V3", "5V", "GPIO_2", "GPIOA_1", "GPIO_4", "GPIO_5", "GND", "GPIO_7", "GPIO_8", "3V3"],
        "labels_right": ["5V", "GND", "GPIO_3", "GND", "GPIO_6", "GPIO_9", "GPIO_10", "GPIO_11", "GND", "GND"],
    },
    "imx93": {
        "platform_name": "i.MX93",
        "target_gpio": "GPIO_3",
        "ground_gpio": "GND",
        "highlight_row": 2,
        "row_description": "3rd row from the top",
        "labels_left": ["3V3", "GPIO_2", "GPIO_3", "GPIO_4", "GND", "GPIO_5", "GND", "GPIO_17", "GPIO_27", "GPIO_22"],
        "labels_right": ["5V", "5V", "GND", "GPIO_14", "GPIO_15", "GPIO_18", "GND", "GPIO_23", "GPIO_24", "GND"],
    },
}


def get_pinout_config() -> Dict[str, Any]:
    return PINOUT_CONFIGS[PLATFORM]


def generate_llm_message(state: Literal["on", "off"]) -> str:
    user_message = f"In the style of {LLM_PROMPT_PRESET}, write one short sentence about a lightbulb turning {state}."
    message = None
    try:
        response = requests.post(
            f"{OLLAMA_URL}/api/chat",
            json={
                "model": MODEL_NAME,
                "messages": [
                    {
                        "role": "system",
                        "content": "You are a creative writer. Respond with only one short, creative sentence. Do not include any preamble, explanation, or extra text."
                    },
                    {
                        "role": "user",
                        "content": user_message
                    }
                ],
                "stream": False,
                "options": {
                    "temperature": 0.9,
                    "top_p": 0.95,
                    "num_predict": 30
                }
            },
            timeout=30
        )

        if response.status_code == 200:
            result = response.json()
            chat_message = result.get("message", {})
            message = chat_message.get("content", "").strip()

            if message and not message.endswith(('.', '!', '?')):
                message += '.'
        else: 
            print(f"Ollama API error: {response.status_code}")

    except Exception as e:
        print(f"Error generating LLM message: {e}", flush=True)
        import traceback
        traceback.print_exc()
    return message if message else f"The switch is {state}."

# This is a hack to start the ttyrpmsg device.
def activate_rpmsgtty():
    """Best-effort activation by writing a char to the device."""
    import time

    # Wait for device to appear (up to 30 seconds)
    for _ in range(30):
        if os.path.exists(DEVICE_PATH):
            break
        print(f"HACK: {DEVICE_PATH} not present yet; waiting...")
        time.sleep(1)

    try:
        if not os.path.exists(DEVICE_PATH):
            print(f"HACK: {DEVICE_PATH} not present after 30s; skipping activation")
            return

        with open(DEVICE_PATH, "w") as device:
            device.write("0")
            device.flush()

        print(f"HACK: Wrote activation byte to {DEVICE_PATH}")
    except Exception as err:
        print(f"HACK: Failed to activate {DEVICE_PATH}: {err}")


def generate_messages():
    """
    Background thread that generates LLM messages based on current state.
    Runs every MESSAGE_INTERVAL seconds and broadcasts to all clients.
    """
    global current_message

    print("Starting LLM message generation thread")

    print("Waiting for Ollama to be ready...", flush=True)
    for i in range(30):
        try:
            response = requests.get(f"{OLLAMA_URL}/api/tags", timeout=2)
            if response.status_code == 200:
                print("Ollama is ready!", flush=True)
                break
        except Exception as e:
            print(f"Ollama not ready yet (attempt {i+1}/30): {e}", flush=True)
            time.sleep(2)

    while True:
        try:
            print(f"Starting message generation", flush=True)
            with state_lock:
                state = current_state
            print(f"Current state: {state}", flush=True)

            message = generate_llm_message(state)
            with state_lock:
                current_message = message

                print(f"Generated message: {message}", flush=True)

                # Broadcast to all connected clients
                socketio.emit('message_update', {'message': message}, namespace='/')

                # And.. wait for the next interval before sending an update
                socketio.sleep(MESSAGE_INTERVAL)

        except Exception as e:
            print(f"Error in LLM message generation loop: {e}")
            socketio.sleep(MESSAGE_INTERVAL)


def monitor_rpmsgtty():
    """
    Background thread monitoring /dev/rpmsgtty for changes.
    Reads newline deliminated messages from tty stream
    """
    global current_state

    print("Starting rpmsgtty monitor thread")

    while True:
        try:
            if not os.path.exists(DEVICE_PATH):
                print(f"Waiting for {DEVICE_PATH} to appear...")
                socketio.sleep(2)
                continue

            print(f"Opening {DEVICE_PATH}...")
            with open(DEVICE_PATH, "r") as device:
                print(
                    f"Successfully opened {DEVICE_PATH}, monitoring for state changes..."
                )

                while True:
                    line = device.readline()

                    if not line:
                        # Device disconnected (EOF)
                        print(f"Device {DEVICE_PATH} disconnected")
                        break

                    state = line.strip().lower()

                    if state in ["on", "off"]:
                        with state_lock:
                            if state != current_state:
                                current_state = state
                                print(f"State changed to: {state}")

                                # Broadcast state change to all connected clients
                                socketio.emit(
                                    "state_update", {"state": state}, namespace="/"
                                )
                    else:
                        print(f"Received unexpected message: {repr(line)}")

        except Exception as e:
            print(f"Error reading from {DEVICE_PATH}: {e}")
            socketio.sleep(2)


@app.route("/")
def index():
    """Serve the main page."""
    return render_template(
        "index.html",
        initial_state=current_state,
        initial_message=current_message,
        pinout=get_pinout_config(),
    )



@socketio.on("connect")
def handle_connect():
    """Handle client connection - send current state."""
    print("Client Connected")
    with state_lock:
        emit("state_update", {"state": current_state})
        emit("message_update", {"message": current_message})


@socketio.on("disconnect")
def handle_disconnect():
    print("Client Disconnected")


if __name__ == "__main__":
    activate_rpmsgtty()

    # Start the rpmsgtty monitoring thread
    monitor_thread = threading.Thread(target=monitor_rpmsgtty, daemon=True)
    monitor_thread.start()

    # Start the LLM message generation thread
    llm_thread = threading.Thread(target=generate_messages, daemon=True)
    llm_thread.start()

    # Start the Flask-SocketIO server
    print("Starting web server on http://0.0.0.0:3000")
    socketio.run(
        app, host="0.0.0.0", port=3000, debug=False, allow_unsafe_werkzeug=True
    )
