FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# 1. Install essential dependencies
RUN apt-get update && apt-get install -y \
    # Basic utilities
    wget \
    curl \
    ca-certificates \
    git \
    sudo \
    # Development tools
    build-essential \
    cmake \
    g++ \
    gcc \
    gfortran \
    # Python
    python3 \
    python3-dev \
    python3-pip \
    python3-venv \
    # SCIP dependencies
    libgmp-dev \
    libreadline-dev \
    zlib1g-dev \
    libboost-program-options-dev \
    libblas-dev \
    liblapack-dev \
    libtbb-dev \
    # Other useful
    pkg-config \
    autoconf \
    automake \
    libtool \
    && rm -rf /var/lib/apt/lists/*

# 2. Install SCIP 10.0.0
RUN wget -q https://scipopt.org/download/release/scipoptsuite_10.0.0-1+jammy_amd64.deb -O scip.deb && \
    apt-get update && \
    apt-get install -y ./scip.deb && \
    rm scip.deb && \
    rm -rf /var/lib/apt/lists/*

# 3. Set up Python packages
RUN python3 -m pip install --upgrade pip && \
    python3 -m pip install numpy scipy

WORKDIR /src

CMD ["bash"]