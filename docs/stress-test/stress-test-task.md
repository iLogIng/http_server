# 压力测试任务

使用 `wrk` 压测工具进行压力测试

> 仅获取 `wrk` 压测工具产生的输出，并进行分析，不需要捕获 `http_server` 程序产生的任何输出

---

## http_server 程序参数

根据测试平台实际性能进行合理设置

---

## wrk 测试参数

```bash
./wrk <args -t -c -d> <url>

./wrk -t8 -c400 -d30s http://0.0.0.0:8080/index.html
```

**`wrk` 压测程序可变参数选择**

- `wrk` 测试线程数 `-t` threads
    - `-t4`
    - `-t6`
    - `-t8`

- `wrk` 每线程测试连接数 `-c` connection
    - `-c100`
    - `-c400`
    - `-c500`
    - `-c600`
    - `-c800`
    - `-c1000`

- `wrk` 测试时长 `-d` delay
    - `-d30s`

---

## 数据收集

不需要 `http_server` 程序的任何输出

收集 `wrk` 对 `http_server` 程序的压测输出

---

## 输出格式

**输出路径：`项目根目录/docs/stress-test/stress-test-record`**

**文件名称：`stress_test_yyyy_mm_dd.md`，其中 `yyy_mm_dd` 为压测实际开始的时间点，若存在相同日期的压测任务，则添加 `_tnn` 后缀，其中 `nn` 为该文件标号（00~99）**

**文件格式见 `./stress-test-format.md` 规范**

### 文件参考

- **[./stress-test-i5-8250u.md](./stress-test-i5-8250u.md)**
- **[./stress-test-i5-1235u.md](./stress-test-i5-1235u.md)**

---

## END

