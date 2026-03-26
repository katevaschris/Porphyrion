FROM alpine:3.18 AS builder
RUN --mount=type=cache,target=/var/cache/apk \
    apk add --no-cache \
    brotli-static \
    curl-dev \
    curl-static \
    gcc \
    libidn2-static \
    libpsl-static \
    libunistring-static \
    make \
    musl-dev \
    nghttp2-static \
    openssl-libs-static \
    upx \
    zlib-static
WORKDIR /app
COPY Makefile ./
COPY src/ src/
COPY include/ include/
RUN --mount=type=cache,target=/app/obj \
    make -j$(nproc) LDFLAGS="-static -lcurl -lssl -lcrypto -lnghttp2 \
                             -lbrotlidec -lbrotlicommon -lz -lidn2 \
                             -lunistring -lpsl -lm -s" && \
    strip --strip-all porter && \
    upx --best --lzma porter

FROM scratch
COPY --from=builder /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/ca-certificates.crt
COPY --from=builder /app/porter /porter
COPY resources/ resources/
COPY web/ web/
EXPOSE 8099
CMD ["/porter"]
