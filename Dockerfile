FROM alpine:3.18 AS builder
RUN apk add --no-cache \
    gcc musl-dev make \
    curl-dev curl-static \
    openssl-libs-static \
    nghttp2-static \
    zlib-static \
    brotli-static \
    libunistring-static \
    libidn2-static \
    libpsl-static \
    upx
WORKDIR /app
COPY src/ src/
COPY include/ include/
COPY Makefile ./
RUN --mount=type=cache,target=/app/obj \
    make -j4 LDFLAGS="-static -lcurl -lssl -lcrypto -lnghttp2 \
    -lbrotlidec -lbrotlicommon -lz -lidn2 -lunistring -lpsl -lm -s" && \
    upx --best --lzma porter

FROM scratch
COPY --from=builder /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/ca-certificates.crt
COPY --from=builder /app/porter /porter
COPY index.html hat.png /
EXPOSE 8099
CMD ["/porter"]
