ARG BUILDPLATFORM
ARG TARGETPLATFORM=linux/arm64
ARG PREBUILT_FIRMWARE_IMAGE=ghcr.io/arm-examples/test-topo-lightbulb/zephyr-application:latest-stm32mp257

FROM --platform=${BUILDPLATFORM} alpine:3.22 AS source-check

RUN apk add --no-cache coreutils findutils

COPY workdir/ /source/workdir/

RUN cd /source && \
    find workdir -type f -print0 | sort -z | xargs -0 sha256sum | sha256sum | cut -d ' ' -f1 > /source.sha256

FROM --platform=${BUILDPLATFORM} zephyrprojectrtos/zephyr-build:v0.28.0 AS build

ARG PLATFORM=stm32mp257

USER root
RUN --mount=type=cache,target=/root/.cache/pip \
    pip install jsonschema
USER user

RUN west init

RUN git -C /workdir/zephyr fetch --depth=1 origin refs/tags/v4.3.0 && \
    git -C /workdir/zephyr checkout FETCH_HEAD

RUN --mount=type=cache,target=/git-cache \
    west update --path-cache /git-cache --narrow -o=--depth=1

COPY --chown=user:user workdir/ /workdir/

WORKDIR /workdir

RUN if [ "$PLATFORM" = "stm32mp257" ]; then \
    cp boards/stm32mp257f/resource_table.ld . && \
    cd /workdir/zephyr && patch -p1 < /workdir/boards/stm32mp257f/ipm_stm32_ipcc.patch; \
    fi

RUN if [ "$PLATFORM" = "stm32mp257" ]; then \
    west build -b stm32mp257f_dk/stm32mp257fxx/m33 \
    -- -DCONF_FILE=boards/stm32mp257f/prj.conf \
    -DDTC_OVERLAY_FILE=boards/stm32mp257f/stm32mp257f_dk_stm32mp257fxx_m33.overlay && \
    cp build/zephyr/zephyr.elf /tmp/zephyr.elf; \
    elif [ "$PLATFORM" = "imx93" ]; then \
    west build -b imx93_evk/mimx9352/m33 \
    -- -DCONF_FILE=boards/imx93/prj.conf \
    -DOVERLAY_CONFIG=boards/imx93/imx93_m33.conf \
    -DDTC_OVERLAY_FILE=boards/imx93/nxp-frdm-imx93.overlay && \
    cp build/zephyr/zephyr_openamp_rsc_table.elf /tmp/zephyr.elf; \
    else \
    echo "Unknown PLATFORM: $PLATFORM. Use stm32mp257 or imx93" && exit 1; \
    fi

COPY --from=source-check /source.sha256 /tmp/source.sha256

FROM scratch AS source
COPY --from=build /tmp/zephyr.elf /zephyr.elf
COPY --from=build /tmp/source.sha256 /source.sha256

ENTRYPOINT [ "/zephyr.elf" ]

FROM ${PREBUILT_FIRMWARE_IMAGE} AS prebuilt

FROM --platform=${BUILDPLATFORM} alpine:3.22 AS verify-cache

COPY --from=prebuilt /source.sha256 /prebuilt-source.sha256
COPY --from=source-check /source.sha256 /local-source.sha256

RUN local_source_sha256="$(cat /local-source.sha256)" && \
    prebuilt_source_sha256="$(cat /prebuilt-source.sha256)" && \
    if [ "$local_source_sha256" != "$prebuilt_source_sha256" ]; then \
    printf '%s\n' \
    "Zephyr source checksum mismatch." \
    "Local workdir checksum:    $local_source_sha256" \
    "Prebuilt image checksum:   $prebuilt_source_sha256" \
    "Rebuild from source with:  docker build --target source"; \
    exit 1; \
    fi

FROM ${PREBUILT_FIRMWARE_IMAGE} AS cached

COPY --from=verify-cache /local-source.sha256 /source.sha256

ENTRYPOINT [ "/zephyr.elf" ]
