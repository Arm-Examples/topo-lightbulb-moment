FROM alpine/ollama:0.12.10

# Pull TinyLlama model during build
RUN ollama serve & \
    sleep 5 && \
    ollama pull smollm:135m && \
    pkill ollama

EXPOSE 11434
