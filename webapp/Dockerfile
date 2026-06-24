FROM --platform=${BUILDPLATFORM} ghcr.io/astral-sh/uv:python3.14-alpine AS builder

ARG LLM_PROMPT_PRESET
ARG PLATFORM

WORKDIR /app

COPY pyproject.toml uv.lock ./

RUN uv sync --frozen --no-dev --python /usr/local/bin/python3

COPY src/app.py .
COPY src/templates/ templates/
COPY src/static/ static/

FROM dhi.io/python:3.14.6-alpine3.22

ARG LLM_PROMPT_PRESET
ARG PLATFORM

WORKDIR /app

COPY --from=builder /app/.venv/lib/python3.14/site-packages /app/site-packages
COPY --from=builder /app/app.py .
COPY --from=builder /app/templates/ templates/
COPY --from=builder /app/static/ static/

ENV PYTHONPATH="/app/site-packages"
ENV LLM_PROMPT_PRESET="${LLM_PROMPT_PRESET}"
ENV PLATFORM="${PLATFORM}"

EXPOSE 3000

CMD ["python3", "app.py"]
