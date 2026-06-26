ARG BUILDPLATFORM
ARG ZEPHYR_DEPS_IMAGE=ghcr.io/arm-examples/topo-lightbulb-moment/zephyr-application:deps

FROM --platform=${BUILDPLATFORM} ${ZEPHYR_DEPS_IMAGE} AS build

ARG PLATFORM=stm32mp257

COPY workdir/ /workdir/

WORKDIR /workdir

RUN if [ "$PLATFORM" = "stm32mp257" ]; then \
    cp boards/stm32mp257f/resource_table.ld . && \
    cd /workdir/zephyr && patch -p1 < /workdir/boards/stm32mp257f/ipm_stm32_ipcc.patch; \
    fi

RUN --mount=type=cache,target=/workdir/build,sharing=locked \
    if [ "$PLATFORM" = "stm32mp257" ]; then \
    west build -b stm32mp257f_dk/stm32mp257fxx/m33 \
    -d build/stm32mp257 \
    -- -DCONF_FILE=boards/stm32mp257f/prj.conf \
    -DDTC_OVERLAY_FILE=boards/stm32mp257f/stm32mp257f_dk_stm32mp257fxx_m33.overlay && \
    cp build/stm32mp257/zephyr/zephyr.elf /tmp/zephyr.elf; \
    elif [ "$PLATFORM" = "imx93" ]; then \
    west build -b imx93_evk/mimx9352/m33 \
    -d build/imx93 \
    -- -DCONF_FILE=boards/imx93/prj.conf \
    -DOVERLAY_CONFIG=boards/imx93/imx93_m33.conf \
    -DDTC_OVERLAY_FILE=boards/imx93/nxp-frdm-imx93.overlay && \
    cp build/imx93/zephyr/zephyr_openamp_rsc_table.elf /tmp/zephyr.elf; \
    else \
    echo "Unknown PLATFORM: $PLATFORM. Use stm32mp257 or imx93" && exit 1; \
    fi

FROM scratch AS source

COPY --from=build /tmp/zephyr.elf /zephyr.elf
COPY --from=build /workdir/NOTICE /licenses/NOTICE
COPY --from=build /workdir/zephyr/LICENSE /licenses/zephyr/LICENSE
COPY --from=build /workdir/zephyr/NOTICE /licenses/zephyr/NOTICE

LABEL org.opencontainers.image.title="Zephyr Lightbulb Moment Firmware"
LABEL org.opencontainers.image.description="Zephyr RTOS firmware for the Topo Lightbulb Moment reference application"
LABEL org.opencontainers.image.source="https://github.com/Arm-Examples/topo-lightbulb-moment"
LABEL org.opencontainers.image.vendor="Arm"
LABEL org.opencontainers.image.licenses="Apache-2.0 AND BSD-3-Clause"

ENTRYPOINT [ "/zephyr.elf" ]
