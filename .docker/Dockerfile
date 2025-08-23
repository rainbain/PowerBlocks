FROM ubuntu:24.04

ARG LLVM_EXTRAS=""
ARG EXTRA_INSTALLS=""

ENV SDK_HOME=/powerblocks
ENV TOOLCHAIN=${SDK_HOME}/toolchain

COPY --link . ${SDK_HOME}

RUN apt update && apt upgrade -y && \
    apt install -y --no-install-recommends clang cmake git lld make ninja-build python3-full python3-pip zlib1g-dev

RUN git clone --depth 1 https://github.com/llvm/llvm-project.git && \
    cd llvm-project && \
    mkdir build && \
    cd build && \
    cmake -G Ninja ../llvm \
        -DCMAKE_BUILD_TYPE=RELEASE \
        -DCMAKE_INSTALL_PREFIX=${TOOLCHAIN} \
        -DLLVM_TARGETS_TO_BUILD="PowerPC" \
        -DLLVM_ENABLE_PROJECTS="clang;lld${LLVM_EXTRAS}" \
        -DLLVM_ENABLE_LLD=ON \
        -DLLVM_ENABLE_ASSERTIONS=OFF \
        -DLLVM_INCLUDE_EXAMPLES=OFF \
        -DLLVM_INCLUDE_TESTS=OFF \
        -DLLVM_INCLUDE_DOCS=OFF \
        -DCLANG_INCLUDE_TESTS=OFF \
        -DCLANG_INCLUDE_DOCS=OFF \
        -DLLVM_ENABLE_BACKTRACES=OFF && \
    ninja && \
    ninja install-clang install-lld install-llvm-ar install-llvm-ranlib install-llvm-objcopy install-llvm-strip install-llvm-objdump install-clang-resource-headers ${EXTRA_INSTALLS} && \
    cp -a lib/* ${TOOLCHAIN}/lib/ && \
    cd ../.. && \
    rm -rf llvm-project

RUN apt remove -y clang lld && \
    apt autoremove -y && \
    rm -rf /var/lib/apt/lists/*

ENV EXTRA_C_FLAGS=""
ENV PATH="$TOOLCHAIN/bin:$PATH"
ENV CMAKE_TOOLCHAIN_FILE="$SDK_HOME/toolchain.cmake"
ENV CMAKE_GENERATOR="Ninja"
ENV CMAKE_BUILD_TYPE=Release
ENV CMAKE_PREFIX_PATH=$SDK_HOME

RUN git clone https://github.com/picolibc/picolibc.git && \
    cd picolibc && \
    mkdir build && \
    cd build && \
    cp ${SDK_HOME}/picolibc/setjmp.S ../newlib/libc/machine/powerpc/setjmp.S && \
    export CMAKE_GENERATOR="Unix Makefiles" && \
    cmake .. -DCMAKE_INSTALL_PREFIX=${SDK_HOME}/picolibc -DEXTRA_C_FLAGS="-I${SDK_HOME}/picolibc/" && \
    make -j$(nproc) && \
    make install && \
    cp ../COPYING.picolibc ${SDK_HOME}/picolibc/COPYING.picolibc && \
    cd ../.. && \
    rm -rf picolibc