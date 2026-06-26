# Build image should be run on host architecture; but built for target architecture.
FROM --platform=${BUILDPLATFORM} ghcr.io/astral-sh/uv:python3.14-alpine AS builder

WORKDIR /app

COPY pyproject.toml uv.lock ./

RUN uv sync --frozen --no-dev --python /usr/local/bin/python3 --python-platform aarch64-unknown-linux-musl

FROM public.ecr.aws/docker/library/python:3.14.6-alpine3.24

ARG LLM_PROMPT_PRESET
ARG PLATFORM

WORKDIR /app

COPY --from=builder /app/.venv/lib/python3.14/site-packages /app/site-packages
COPY src/app.py .
COPY src/templates/ templates/
COPY src/static/ static/

ENV PYTHONPATH="/app/site-packages"
ENV LLM_PROMPT_PRESET="${LLM_PROMPT_PRESET}"
ENV PLATFORM="${PLATFORM}"

EXPOSE 3000

CMD ["python3", "app.py"]
