FROM zephyrprojectrtos/zephyr-build:v0.28.0 AS base
RUN west init
RUN git -C /workdir/zephyr fetch --all \
    && git -C /workdir/zephyr checkout 402adc4a3709c4be8f2f9f3cd10b1a2d838060a9

RUN west update

FROM base AS build
COPY workdir/ipm_stm32_ipcc.patch /tmp/
# Patch IPM driver to skip clock init in remoteproc mode (Linux manages clocks on this board)
RUN cd /workdir/zephyr && patch -p1 < /tmp/ipm_stm32_ipcc.patch
COPY workdir/ /workdir/

RUN west build -b stm32mp257f_dk/stm32mp257fxx/m33 && \
    cp build/zephyr/zephyr.elf /tmp/zephyr.elf

FROM scratch AS deploy
COPY --from=build /tmp/zephyr.elf /zephyr.elf

ENTRYPOINT [ "/zephyr.elf" ]

FROM base AS dev
