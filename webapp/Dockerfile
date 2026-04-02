FROM ghcr.io/astral-sh/uv:python3.14-bookworm-slim

ARG LLM_PROMPT_PRESET
ARG PLATFORM

WORKDIR /app

# Copy project metadata first to take advantage of caching
COPY pyproject.toml uv.lock ./

# Install dependencies into the project-local virtual environment
RUN uv sync --frozen --no-dev

# Copy the application source
COPY src/app.py .
COPY src/templates/ templates/
COPY src/static/ static/

ENV PATH="/app/.venv/bin:${PATH}"
ENV LLM_PROMPT_PRESET="${LLM_PROMPT_PRESET}"
ENV PLATFORM="${PLATFORM}"

EXPOSE 3000

CMD ["uv", "run", "python", "app.py"]
