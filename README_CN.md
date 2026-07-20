<div align="center">

<img src="docs/speechmind.png" alt="SpeechMind 架构图" width="900">

# SpeechMind

**基于 AIVoice 实时音频流、面向 Realtek Ameba 系列芯片的常开(always-on)AI 语音应用。**

[![SDK](https://badgen.net/badge/SDK/ameba-rtos/blue)](https://github.com/Ameba-AIoT/ameba-rtos)
[![Language](https://badgen.net/badge/language/C/blue)](https://github.com/Ameba-AIoT/ameba-rtos/search?l=c)
[![Component](https://badgen.net/badge/component/application/purple)](https://aiot.realmcu.com/zh-CN/latest/rtos/index.html)
[![Docs](https://badgen.net/badge/docs/online/green)](https://aiot.realmcu.com/zh-CN/latest/rtos/index.html)

[English](README.md) · [ameba-rtos](../../../README.md) · [在线文档](https://aiot.realmcu.com/zh-CN/latest/rtos/index.html) · [产品](https://aiot.realmcu.com/zh-CN/product/index.html)

</div>

SpeechMind 是一个语音开发套件(Speech Development Kit),演示如何以常开(always-on)方式在实时音频流上运行 AIVoice 流程。

录音、播放等音频功能集成在 MCU 中,因此该示例需要与运行在 MCU 上的 SpeechMind 配合使用。录音器会发送 3 路音频:通道 1 和通道 2 是 2mic50mm 阵列的麦克风信号,通道 3 是 AEC 参考信号。你可以尝试说 "你好小强"、"打开空调"、"你好小强"、"播放音乐" 等。

> **注意:** AFE res、KWS lib 和 FST lib 必须与 audio 的内容匹配,否则 AIVoice 无法识别。

## 🔌 支持的芯片

| 芯片                  |         master         |      release/v1.2      |      release/v1.1      |
|:--------------------- |:----------------------:|:----------------------:|:----------------------:|
| AmebaSmart (RTL8730E) | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] |
| AmebaLite (RTL8720E)  | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] |

[supported]: https://img.shields.io/badge/-supported-green "supported"

> 各芯片的具体功能可用性以对应目标的 Kconfig 为准。完整的按芯片功能矩阵请参考在线 SDK 文档。

## 🏗️ 目录结构

```text
speechmind/
├── src/
│   ├── speech_mind.c/.h        # 常开语音主流程
│   ├── aivoice_manager.c/.h    # AIVoice 引擎(AFE / VAD / KWS / ASR)管理
│   ├── audio_capture.c/.h      # 3 通道录音(2 麦 + AEC 参考)
│   ├── speech_tts.c/.h         # TTS 提示音播放
│   ├── music_player.c/.h       # 媒体播放器封装(MP3 / WAV)
│   ├── playlist*.c             # 播放列表管理与解析
│   ├── audio_dump.c/.h         # dump 实时音频流用于调试
│   ├── pc_recorder.c/.h        # 将采集到的音频推流到 PC
│   ├── speech_config.c/.h      # 运行时配置
│   ├── test_cmd.c/.h           # 控制台测试命令
│   ├── amebasmart/             # AmebaSmart 专用 manager / config
│   └── amebalite/              # AmebaLite 构建(DSP RPC 位于 aidl/)
├── res/                        # tts / eng_tts MP3 提示音、音乐、angle 参数
├── Kconfig                     # Menuconfig 选项
└── CMakeLists.txt              # 组件构建入口
```

## ✨ 主要特性

- **常开唤醒词** — 在连续音频流上进行低功耗 KWS 唤醒检测("你好小强")
- **命令识别** — 端侧 ASR 命令集("打开空调"、"播放音乐" 等)
- **双麦前端** — 基于 2mic50mm 阵列的 AFE,含 AEC 与 VAD(2 麦 + AEC 参考)
- **TTS 与媒体播放** — 通过音频媒体播放器播放 MP3 / WAV 提示音与音乐
- **调试工具** — audio dump 与 PC recorder,可抓取实时的麦克风 / AEC 音频流
- **控制台命令** — 从串口控制台快速验证与测试

## 🔧 硬件配置

所需元件:**扬声器(speaker)**。

将扬声器连接到开发板。

## 🟢 AmebaSmart

### 软件配置

**1. 将 SpeechMind 加入镜像构建**

在如下文件的\
**###############################  ADD COMPONENT ###################################**\
段落下,添加以下这行:\
**amebasmart_gcc_project/project_ap/asdk/make/image2/CMakeLists.txt:**

```cmake
ameba_add_subdirectory_if_exist(${c_CMPT_APP_DIR}/speechmind)
```

**2. Menuconfig**

在工程目录下执行 `./menuconfig.py`,并按如下配置:

```
CONFIG Link Option  --->
    IMG2(Application) running on PSRAM or FLASH? (FLASH)  --->
CONFIG APPLICATION  --->
    Audio Config  --->
        -*- Enable Audio Framework
            Select Audio Interfaces (Mixer)  --->
            Audio Devices  --->
        [*] Enable Media Player
            Media Formats  --->
                [*] WAV
                [*] MP3
    AI Config  --->
        [*] Enable TFLITE MICRO
        [*] Enable AIVoice
            Select AFE Resource (afe_res_2mic50mm)  --->
            Select VAD Resource (vad_v7_200K)  --->
            Select KWS Resource (kws_xiaoqiangxiaoqiang_nihaoxiaoqiang_v4_300K)  --->
            Select ASR Resource (asr_cn_v8_2M)  --->
```

### 构建资源 bin

**1.** 通过 `./menuconfig.py` 配置 VFS1 区域:

```
CONFIG User Config  --->
    Flash Layout Configuration  --->
        (0x8623000) VFS1 Offset
        (0x1DD000)  VFS1 Size
```

**2.** `cp tools/littlefs/linux/mklittlefs component/application/speechmind/res/`

**3.** 设置文件权限:`chmod 777 mklittlefs`

**4.** 使用以下命令构建:`./mklittlefs -b 4096 -p 256 -s 0x1DD000 -c tts tts.bin`(`-s` 与上面的 VFS1 Size 一致)

### 构建与下载

* 参考在线文档的 SDK Examples 章节生成镜像。
* 使用 Ameba Image Tool,在最后一行添加镜像名 **test.bin**,起始地址 **0x08623000**,结束地址 **0x08800000**。
* 使用 Ameba Image Tool 将镜像 `Download` 到开发板。

## 🔵 AmebaLite

### 构建 DSP bin

参考 AIVoice 仓库中的 **examples/speechmind_demo/README.md**。

### 软件配置

**1. 将 SpeechMind 加入镜像构建**

在如下文件的\
**###############################  ADD COMPONENT ###################################**\
段落下,添加以下这行:\
**amebalite_gcc_project/project_km4/asdk/make/image2/CMakeLists.txt:**

```cmake
ameba_add_subdirectory_if_exist(${c_CMPT_APP_DIR}/speechmind)
```

**2. Menuconfig**

在工程目录下执行 `./menuconfig.py`,并按如下配置:

```
CONFIG DSP Enable  --->
    [*] Enable DSP
CONFIG Link Option  --->
    IMG2(Application) running on FLASH or PSRAM? (CodeInXip_DataHeapInPsram)  --->
CONFIG APPLICATION  --->
    Audio Config  --->
        -*- Enable Audio Framework
            Select Audio Interfaces (Mixer)  --->
            Audio Devices  --->
        [*] Enable Media Player
            Media Formats  --->
                [*] WAV
                [*] MP3
    IPC Message Queue Config  --->
        [*] Enable IPC Message Queue
        [*]     Enable RPC
```

### 构建资源 bin

**1.** 通过 `./menuconfig.py` 配置 VFS1 区域:

```
CONFIG User Config  --->
    Flash Layout Configuration  --->
        (0x83E0000) VFS1 Offset
        (0x1DD000)  VFS1 Size
```

**2.** `cp tools/littlefs/linux/mklittlefs component/application/speechmind/res/`

**3.** 设置文件权限:`chmod 777 mklittlefs`

**4.** 使用以下命令构建:`./mklittlefs -b 4096 -p 256 -s 0x1DD000 -c tts tts.bin`(`-s` 与上面的 VFS1 Size 一致)

### 构建与下载

* 参考在线文档的 SDK Examples 章节生成镜像。
* 使用 Ameba Image Tool,在最后一行添加镜像名 **test.bin**,起始地址 **0x083E0000**,结束地址 **0x085BD000**。
* 使用 Ameba Image Tool 将镜像 `Download` 到开发板。

## 📚 文档

最新版本文档:[FreeRTOS SDK 与用户指南](https://aiot.realmcu.com/zh-CN/latest/rtos/index.html)。

更多 Ameba 系列芯片信息,请访问[产品页面](https://aiot.realmcu.com/zh-CN/product/index.html)。

## 📥 SDK 下载

SpeechMind 依赖 audio 框架与 AIVoice 引擎,二者以子模块形式随 [ameba-rtos](https://github.com/Ameba-AIoT/ameba-rtos) 提供,且仅在 **XDK(Extended)** 检出方式下拉取:

```bash
git clone --recurse-submodules https://github.com/Ameba-AIoT/ameba-rtos.git
```

若你已经克隆了 ameba-rtos 但未包含子模块:

```bash
git submodule update --init --recursive
```

> Basic SDK 检出方式不包含 audio / AIVoice 子模块。要构建 SpeechMind,必须使用上面的 XDK 检出方式。

## 🌐 使用 Gitee 加速

对于可访问 [Gitee](https://gitee.com) 的用户,当 GitHub 较慢时,推荐下载 [ameba-rtos](https://gitee.com/ameba-aiot/ameba-rtos) 的 Gitee 镜像以提升下载速度。子模块的拉取方式相同。

## 💬 反馈

* 开发过程中的问题或建议,请访问 [Real-AIOT 论坛](https://forum.real-aiot.com/)。
* Bug 或功能需求,请[查看 GitHub Issues](https://github.com/Ameba-AIoT/ameba-rtos/issues)。提交前请先检索已有 issue。
