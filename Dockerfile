FROM gcc:13 AS build

WORKDIR /build

COPY makefile .
COPY include/ include/
COPY lib/ lib/
COPY src/ src/
COPY redis_server.cpp .
COPY redis_client.cpp .

RUN make -j$(nproc)

FROM debian:bookworm-slim

WORKDIR /app

COPY --from=build /build/redis_server .
COPY --from=build /build/redis_client .

CMD ["./redis_server"]