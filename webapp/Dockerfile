ARG BUILDPLATFORM
ARG TARGETPLATFORM

FROM --platform=${BUILDPLATFORM} ghcr.io/astral-sh/uv:python3.14-bookworm-slim AS dependencies

ARG TARGETPLATFORM

WORKDIR /app

COPY pyproject.toml uv.lock ./

RUN set -eux; \
    case "$TARGETPLATFORM" in \
    "linux/arm64") python_platform="aarch64-manylinux_2_36" ;; \
    "linux/amd64") python_platform="x86_64-manylinux_2_36" ;; \
    *) echo "Unsupported TARGETPLATFORM: $TARGETPLATFORM" >&2; exit 1 ;; \
    esac; \
    uv export --frozen --no-dev --no-emit-project --format requirements.txt > /tmp/requirements.txt; \
    uv pip install --target /app/vendor --python-version 3.14 --python-platform "$python_platform" --only-binary :all: -r /tmp/requirements.txt

FROM ghcr.io/astral-sh/uv:python3.14-bookworm-slim

ARG LLM_PROMPT_PRESET
ARG PLATFORM

WORKDIR /app

COPY --from=dependencies /app/vendor /app/vendor

COPY src/app.py .
COPY src/templates/ templates/
COPY src/static/ static/

ENV PYTHONPATH="/app/vendor"
ENV LLM_PROMPT_PRESET="${LLM_PROMPT_PRESET}"
ENV PLATFORM="${PLATFORM}"

EXPOSE 3000

CMD ["python", "app.py"]
