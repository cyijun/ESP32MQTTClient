# 开发与文档维护

[English](development.md) | **简体中文**

[README](../README_CN.md) 负责安装、API 行为和使用示例；根目录的 [Agent 指令](../AGENTS.md) 负责源码结构和实现约束。本指南负责本地验证和文档维护流程。

## 验证改动

在仓库根目录使用支持 C++11 的编译器运行主机行为测试：

```sh
sh tests/run_host_tests.sh
```

[测试脚本](../tests/run_host_tests.sh) 优先使用 `CXX`，未设置时使用 `c++`。编译时将警告视为错误，禁用 RTTI，并启用 pthread；产物写入 `${TMPDIR:-/tmp}/esp32mqttclient-host-tests`。同样的五项用例分别覆盖 ESP-IDF 4.4.6 和 5.3 配置分支：发布参数与返回码、主题匹配、订阅失败回滚与保留消息时序、取消订阅失败回滚、分片重组与大小限制。[主机替身](../tests/fakes/mqtt_client.h) 代替平台服务，因此测试通过不能证明设备、网络或 TLS 行为正确。

安装 Arduino CLI 和 ESP32 开发板核心后，可以编译 Arduino 示例：

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 --library . examples/HelloToMyself
```

这条命令使用本机安装的核心版本。[Arduino 工作流](../.github/workflows/ci4main.yml) 定义 CI 支持矩阵；一次本地编译不能覆盖所有矩阵项。[ESP-IDF 工作流](../.github/workflows/esp_idf_ci.yml) 使用其中配置的工具链构建 `examples/CppEspIdf`。ESP-IDF 的构建步骤见 README；源码改动所需的平台验证范围见根目录 Agent 指令。编译成功不能代替开发板与 MQTT broker 联调。

## 维护文档

API 契约和示例继续由现有中英文 README 负责，实现规则由根目录 Agent 指令负责。本指南的流程变化应同步到中英文版本。通过链接引用这些内容的所有者，避免在其他页面复制 API 表格或平台矩阵。`.agents/docs-config.json` 显式纳入 `README_CN.md`，因为这个文件名不在检查器默认识别的根目录 README 名称中。

撰写初始化内容前，分别运行以下命令，在当前会话完整阅读每份输出中的指令和引用材料：

```sh
python3 .agents/doc-tools/docs.py activate --root . --part docs
python3 .agents/doc-tools/docs.py activate --root . --part prose
python3 .agents/doc-tools/docs.py activate --root . --part trim
python3 .agents/doc-tools/docs.py activate --root . --part archive
```

新复制的技能可能尚未进入 Agent 技能列表。如果输出被截断，直接打开遗漏文件补读。投递回执只记录文件哈希，不能证明 Agent 已阅读或应用规则。修改指令后，需要重新阅读受影响的材料。

将完整文档与其来源文件对照审阅，再运行：

```sh
python3 .agents/doc-tools/docs.py check --root .
```

检查器覆盖本地文件链接目标、受管决策文档结构、配置的篇幅预算和归档封印；链接锚点、翻译含义、技术准确性与命令执行结果需单独检查。初始化时，将有来源依据的语义审阅写入 `.agents/doc-tools/.local/review.json`：`outputs` 条目包含文档 `path` 和已存在的 `sources` 路径；`reviews` 条目包含 `principle`、输出文档的 `path` 和具体 `finding`；`verification` 列出真实执行结果及未验证的范围。审阅需覆盖 `fact-ownership`、`complete-propositions`、`session-independent`、`existing-content-preserved` 四项原则。本地[校验器](../.agents/doc-tools/docs.py)负责报告格式。然后运行：

```sh
python3 .agents/doc-tools/docs.py finish --root . --report .agents/doc-tools/.local/review.json
```

被 Git 忽略的 `.local` 文件是工作证据。只有符合[决策记录政策](../.agents/skills/doc-maintenance/references/note-policy.md)的长期设计理由才进入决策文档；仅凭代码不能推断历史备选方案或日期。[决策文档入口](../.agents/notes/README.md) 负责生命周期导航。冻结归档属于历史证据，`.rgignore` 将其排除在普通搜索之外。
