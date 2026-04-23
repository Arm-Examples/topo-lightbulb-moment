ARG BUILDPLATFORM
ARG TARGETPLATFORM
FROM --platform=${BUILDPLATFORM} zephyrprojectrtos/zephyr-build:v0.28.0 AS build

ARG PLATFORM=stm32mp257

USER root
RUN pip install jsonschema
USER user

RUN for i in 1 2 3; do \
      west init && break; \
      echo "west init failed (attempt $i), cleaning up before retry..."; \
      rm -rf /workdir/.west; \
      sleep 15; \
    done

RUN git -C /workdir/zephyr fetch --all && \
    git -C /workdir/zephyr checkout tags/v4.3.0

RUN for i in 1 2 3; do \
      west update && break; \
      echo "west update failed (attempt $i), retrying..."; \
      sleep 15; \
    done

COPY workdir/ /workdir/

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

FROM --platform=${TARGETPLATFORM:-linux/arm64} scratch
COPY --from=build /tmp/zephyr.elf /zephyr.elf

ENTRYPOINT [ "/zephyr.elf" ]
