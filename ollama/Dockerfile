FROM alpine/ollama:0.12.10

# Pull model during build
RUN ollama serve & \
    pid="$!" && \
    sleep 5 && \
    ollama pull smollm:135m && \
    kill "$pid" || true

EXPOSE 11434
