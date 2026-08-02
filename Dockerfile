# 构建

# apt 源镜像
ARG APT_MIRROR=archive.ubuntu.com

FROM ubuntu:24.04 AS builder
ARG APT_MIRROR

# 安装编译工具和依赖库 进行超时检测
RUN sed -i "s|//archive.ubuntu.com|//${APT_MIRROR}|" /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get -o Acquire::Retries=5 -o Acquire::http::Timeout=60 -o Acquire::https::Timeout=60 update \
    && apt-get install -o Acquire::Retries=5 -o Acquire::http::Timeout=60 -o Acquire::https::Timeout=60 \
        -y --no-install-recommends \
        g++ cmake make \
        libboost-system-dev libboost-thread-dev \
        libboost-filesystem-dev libboost-log-dev \
        libboost-program-options-dev libboost-json-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . /src
WORKDIR /src/build

RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
    && cmake --build . --parallel $(nproc) \
    && ls -la /src/build/

# 运行

FROM ubuntu:24.04
ARG APT_MIRROR

# 仅安装运行时库
RUN sed -i "s|//archive.ubuntu.com|//${APT_MIRROR}|" /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get -o Acquire::Retries=5 -o Acquire::http::Timeout=60 -o Acquire::https::Timeout=60 update \
    && apt-get install -o Acquire::Retries=5 -o Acquire::http::Timeout=60 -o Acquire::https::Timeout=60 \
        -y --no-install-recommends \
        libboost-system1.83.0 \
        libboost-thread1.83.0 \
        libboost-filesystem1.83.0 \
        libboost-log1.83.0 \
        libboost-program-options1.83.0 \
        libboost-json1.83.0 \
    && rm -rf /var/lib/apt/lists/*

# 从构建阶段拷贝编译好的二进制 根据CMakeLists.txt构建规则
COPY --from=builder /src/build/src/http_server /usr/local/bin/http_server

# 创建工作目录
WORKDIR /srv

# 仅拷贝静态资源目录
COPY ./app ./app

# 创建非 root 用户 授权工作目录用于写日志
RUN useradd -m -s /bin/bash http_server_user \
    && chown -R http_server_user:http_server_user /srv
USER http_server_user

EXPOSE 8080

ENTRYPOINT ["http_server"]
CMD ["--address", "0.0.0.0", "--port", "8080", "--doc_root", "./app", "--threads", "2"]

# ----------------------------------------------------
# 构建（官方源）:
#   docker build -t my-http-server:v2 .
# 构建（国内源，绕过不稳定代理）:
#   docker build --build-arg APT_MIRROR=mirrors.tuna.tsinghua.edu.cn -t my-http-server:v2 .
# 运行:
#   docker run --rm -p 8080:8080 my-http-server:v2
