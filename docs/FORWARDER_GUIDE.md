# NXMP 主页独立图标（Forwarder NSP）制作与安装指南

本指南用于解决**“无法在 Switch 系统主屏点击图标直接启动 NXMP”**的问题。
通过安装 Forwarder NSP，可以在 Switch Horizon 官方桌面生成专属图标，并且自动开启 **Title 内存全开模式（Title Override）**，可用内存从相册 Applet 的 400MB 解锁至 **3GB+**，播放 1080P/4K 高清视频杜绝内存不足闪退。

---

## 推荐生成参数

| 配置项 | 推荐值 | 说明 |
| :--- | :--- | :--- |
| **Title Name (应用名称)** | `NXMP 视频播放器` | 主屏上显示的软件名称 |
| **Publisher (发布者)** | `NXMP Team` | 开发者信息 |
| **Version (版本号)** | `0.7.2` | 版本号 |
| **Title ID** | `054E584D50000000` | 专属于 NXMP 的未占用 Title ID（16位16进制） |
| **NRO 目标路径** | `sdmc:/switch/nxmp/nxmp.nro` | 存储卡上实际执行的 nro 路径 |
| **Icon (图标文件)** | 项目根目录的 `icon.jpg` (256x256) | 主屏展示的 Logo 贴图 |

---

## 制作方式（二选一）

### 方式一：使用 PC 端工具「Switch Army Knife (SAK)」一键生成（最简单）
1. 在 PC 上打开 **SAK (Switch Army Knife)**。
2. 选择 **NRO to NSP** 功能。
3. 填入上述参数：
   - 目标 NRO 文件选择编译出来的 `nxmp.nro`。
   - 图标选择项目根目录的 `icon.jpg`。
   - NRO 路径填写：`sdmc:/switch/nxmp/nxmp.nro`。
   - Title ID 填写：`054E584D50000000`。
4. 点击 **Build NSP**，即可在输出目录生成 `NXMP_forwarder.nsp`。

### 方式二：使用 devkitPro 自制工具链编译生成
如果你已经安装了 devkitPro，可直接使用官方 `hactool` / `linkle` / `nacptool`：
```bash
# 1. 生成 nacp 元数据
nacptool --create "NXMP" "NXMP Team" "0.7.2" nxmp.nacp --titleid=054E584D50000000

# 2. 生成带图标的 forwarder control
# 将 forwarder 载荷与 icon.jpg、nxmp.nacp 打包为 control.nca 与 program.nca，最后封装为 .nsp
```

---

## 安装到 Switch
1. 将编译好的 `nxmp.nro` 放入 Switch SD 卡：`sdmc:/switch/nxmp/nxmp.nro`。
2. 将生成的 `NXMP_forwarder.nsp` 拷贝到 SD 卡（如 `sdmc:/install/`）。
3. 在 Switch 上打开任一安装工具（**DBI** / **Awoo-Installer** / **Tinfoil**）。
4. 选择并安装该 NSP。
5. 返回 Switch 官方桌面，即可看到漂亮的 NXMP 播放器图标，点击即可秒开进入！
