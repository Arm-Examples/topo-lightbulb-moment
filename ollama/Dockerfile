ARG BUILDPLATFORM
FROM --platform=${BUILDPLATFORM} alpine/ollama:0.12.10 AS model-pull

RUN ollama serve & \
    pid="$!" && \
    until ollama list >/dev/null 2>&1; do sleep 1; done && \
    ollama pull smollm:135m && \
    kill "$pid"

FROM alpine/ollama:0.12.10

COPY --from=model-pull /root/.ollama /root/.ollama

EXPOSE 11434
