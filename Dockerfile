# ---------- Stage 1: Build ----------
    FROM debian:bookworm-slim AS builder

    ENV DEBIAN_FRONTEND=noninteractive
    ENV TZ=Etc/UTC
    
    RUN apt-get update && apt-get install -y \
        build-essential \
        gcc \
        g++ \
        cmake \
        libssl-dev \
        libcurl4-openssl-dev \
     && rm -rf /var/lib/apt/lists/*
    
    WORKDIR /build
    
    COPY . .
    
    RUN mkdir -p build && cd build && cmake .. && make && strip server
    
    # ---------- Stage 2: Runtime ----------
    FROM debian:bookworm-slim
    
    ENV DEBIAN_FRONTEND=noninteractive
    
    # install only runtime dependencies (much lighter!)
    RUN apt-get update && apt-get install -y \
        libcurl4 \
        libssl3 \
     && rm -rf /var/lib/apt/lists/*
    
    WORKDIR /app
    
    COPY --from=builder /build/build/server .
    
    EXPOSE 61243
    
    CMD ["./server"]
    