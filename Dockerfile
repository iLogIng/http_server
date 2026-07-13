# 构建

FROM ubuntu:24.04 AS builder

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
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

# 仅安装运行时库
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libboost-system1.83.0 \
        libboost-thread1.83.0 \
        libboost-filesystem1.83.0 \
        libboost-log1.83.0 \
        libboost-program-options1.83.0 \
        libboost-json1.83.0 \
    && rm -rf /var/lib/apt/lists/*

# 从构建阶段拷贝编译好的二进制
COPY --from=builder /src/build/http_server /usr/local/bin/http_server

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
# docker build -t my-http-server:v2 .
#   docker 在当前目录下 构建(build) 名为 my-http-server 的镜像 版本标签为 v2
# docker run --rm -p 8080:8080 my-http-server:v2
#   docker 镜像创建并启动名为 my-http-server:v2 的容器 映射端口为<host-port 8080>:<container-port 8080> 当容器停止/退出时，自动删除该容器实例