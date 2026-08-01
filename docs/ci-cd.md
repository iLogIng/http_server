# 构建发布 CI/CD

> ***.github/workflows/http-server-ci.yml***
> ***.github/workflows/http-server-cd.yml***
> ***持续集成与持续部署流水线***
>

> 依赖:
> ***GitHub Actions***
> ***Docker（CD）***
>

基于 GitHub Actions 提供自动化构建、测试与发布能力。

- **CI**：合并前的质量门禁，编译并运行全部测试
- **CD**：打 semver 标签后自动构建 Docker 镜像并推送镜像仓库

## 流水线一览

| 流水线 | 文件 | 触发条件 | 作用 |
|:--|:--|:--|:--|
| CI | `http-server-ci.yml` | push / PR → main、手动 | 双构建类型编译 + 全部测试 |
| CD | `http-server-cd.yml` | semver 标签（待启用） | 构建镜像并推送 GHCR |

## 持续集成 CI

在 push / Pull Request 到 `main` 时触发，亦可通过 `workflow_dispatch` 手动运行。

### 构建矩阵

- **构建类型**：Debug / Release
- **运行环境**：ubuntu-24.04

### 主要步骤

- **安装系统依赖**
  > 安装 g++、cmake、make、ccache 与各 Boost 组件；Ubuntu 的 GTest 仅提供源码，需 `cmake --install` 构建后安装，供 `find_package(GTest CONFIG)` 使用
- **缓存 ccache**
  > 以 `ccache-构建类型-提交哈希` 为键缓存编译产物，加速重复构建
- **配置与编译**
  > 注入构建类型，启用 ccache 编译启动器与 `BUILD_TESTING=ON`
- **运行测试**
  > `ctest` 自动发现 `gtest_discover_tests` 注册的用例（59 用例）

### 注意点

- `CMAKE_BUILD_PARALLEL_LEVEL=2` 限制并行编译数，避免 Actions 环境内存不足 OOM

## 持续部署 CD（待启用）

当前 `http-server-cd.yml` 为注释模板，未参与实际运行。

### 启用步骤

1. 取消注释并提交
2. 推送 `v1.2.3` 形式的 semver 标签触发
3. 授权镜像仓库推送
   > 模板已配置 `packages: write` 权限与 `GITHUB_TOKEN` 登录
4. 确认发布产物
   > 当前仅 `linux/amd64` 单架构；多架构需启用 QEMU 步骤并实测

### 镜像标签规则

- `v1.2.3` -> `1.2.3`、`1.2`、`1`
- 稳定版本（不含 `-`）额外打 `latest`

## 本地复现 CI

安装对应依赖后，在项目根目录执行：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build
cd build && ctest --output-on-failure
```

-----

> **注意**：`http-server-cd.yml` 尚未纳入版本控制。
