# Codex 学习会话存档

从本机 Codex 的相关主会话中导出，保留用户提问及助手可见的进度和答复。
范围截止于本次 GitHub 上传请求之前；时间显示为北京时间（UTC+08:00）。

- [RK3588 PCIe Host 驱动与设备树](2026-10-22-rk3588-pcie-host.md)
- [NVMe PCI 驱动框架学习](2026-11-02-nvme-pcie-learning.md)

每份 Markdown 都有同名 JSONL，记录 timestamp、role、phase、text。
[manifest.json](manifest.json) 记录会话 ID、消息数量和导出文件 SHA-256。

导出仅包含上述相关对话，不包含系统及开发者消息、内部推理、工具执行记录、
子代理日志、账户凭据、Codex 配置或数据库。
原始本地会话文件保持不变。导出时对常见密钥和带凭据 URL 做了脱敏检查。

对话正文中的本地绝对路径和行号保留用于还原学习上下文，
它们不一定能在 GitHub 中直接跳转；历史答复反映当时的源码和构建状态。

OpenAI 关于本地历史记录的配置说明：
[history.persistence](https://learn.chatgpt.com/docs/config-file/config-reference)。
