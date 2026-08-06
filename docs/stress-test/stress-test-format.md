# stress_test

> 压测时间：yyyy-mm-dd hh:mm:ss

> 该压力测试基于 **wrk** 工具
> ***[wrk tool](https://github.com/wg/wrk)***
>

使用示例

```bash
:$ wrk -t8 -c400 -d30s http://<静态文件>

:$ wrk -t8 -c400 -d30s http://localhost:8080/index.html
```

## 测试平台

> 填写实际的测试平台配置

```text
Operating System: Fedora Linux 43
KDE Plasma Version: 6.6.4
KDE Frameworks Version: 6.25.0
Qt Version: 6.10.3
Kernel Version: 6.19.11-200.fc43.x86_64 (64-bit)
Graphics Platform: Wayland

Processors: 8 × Intel® Core™ i5-8250U CPU @ 1.60GHz
Memory: 8 GiB of RAM (7.6 GiB usable)
Graphics Processor: Intel® UHD Graphics 620

Manufacturer: Acer
Product Name: Swift SF514-52T
System Version: V1.07
```

## 测试参数

### http_server
> 在此处写明测试时 `http_server` 的参数选择

### wrk
> 在此处写明测试时 `wrk` 的参数选择

## 测试
> 记录每次 `wrk` 的压测输出

```bash
$ ./wrk -t8 -c100 -d30s http://0.0.0.0:8080/index.html

Running 30s test @ http://0.0.0.0:8080/index.html
  8 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    41.71ms    1.91ms 132.20ms   96.21%
    Req/Sec   288.64     26.01   363.00     65.94%
  69084 requests in 30.09s, 608.87MB read
Requests/sec:   2295.91
Transfer/sec:     20.24MB
```

... ...

> 汇总上述 `wrk` 压测输出
> 综合为 `mermaid` 表示的折线图

## END

