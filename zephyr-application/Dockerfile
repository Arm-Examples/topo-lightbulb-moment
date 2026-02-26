FROM zephyrprojectrtos/zephyr-build:v0.28.0 AS build

ARG PLATFORM=stm32mp257

RUN west init

# Checkout Zephyr version
RUN git -C /workdir/zephyr fetch --all && \
    git -C /workdir/zephyr checkout tags/v4.3.0

RUN west update

# Install missing Python dependencies required by Zephyr
USER root
RUN pip install jsonschema
USER user

COPY workdir/ /workdir/

WORKDIR /workdir

# Copy platform-specific configs to workdir root and apply patches (if needed)
RUN if [ "$PLATFORM" = "stm32mp257" ]; then \
    cp boards/stm32mp257f/prj.conf . && \
    cp boards/stm32mp257f/stm32mp257f_dk_stm32mp257fxx_m33.overlay . && \
    cp boards/stm32mp257f/resource_table.ld . && \
    cd /workdir/zephyr && patch -p1 < /workdir/boards/stm32mp257f/ipm_stm32_ipcc.patch; \
    elif [ "$PLATFORM" = "imx93" ]; then \
    cp boards/imx93/prj.conf . && \
    cp boards/imx93/imx93_m33.conf . && \
    cp boards/imx93/nxp-frdm-imx93.overlay . ; \
    fi

# Build
RUN if [ "$PLATFORM" = "stm32mp257" ]; then \
    west build -b stm32mp257f_dk/stm32mp257fxx/m33 \
    -- -DDTC_OVERLAY_FILE=stm32mp257f_dk_stm32mp257fxx_m33.overlay && \
    cp build/zephyr/zephyr.elf /tmp/zephyr.elf; \
    elif [ "$PLATFORM" = "imx93" ]; then \
    west build -p always -b imx93_evk/mimx9352/m33 -s . -d build \
    -- -DOVERLAY_CONFIG=imx93_m33.conf -DDTC_OVERLAY_FILE=nxp-frdm-imx93.overlay && \
    cp build/zephyr/zephyr_openamp_rsc_table.elf /tmp/zephyr.elf; \
    else \
    echo "Unknown PLATFORM: $PLATFORM. Use stm32mp257 or imx93" && exit 1; \
    fi

FROM scratch
COPY --from=build /tmp/zephyr.elf /zephyr.elf

ENTRYPOINT [ "/zephyr.elf" ]
