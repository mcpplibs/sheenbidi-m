# sheenbidi

> 以 C++23 模块使用 SheenBidi——Unicode 双向文本算法。

[English](README.md) · **简体中文** · [Upstream](https://github.com/Tehreer/SheenBidi) · [Releases](https://github.com/mcpplibs/sheenbidi-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![SheenBidi](https://img.shields.io/badge/SheenBidi-Apache--2.0-blue.svg)](upstream/sheenbidi/LICENSE)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![CI](https://github.com/mcpplibs/sheenbidi-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/sheenbidi-m/actions/workflows/ci.yml)

SheenBidi 把按逻辑顺序存储的文本转换成显示所需的视觉顺序，用于阿拉伯文、
希伯来文和混合方向文本（UAX #9，Unicode 17.0.0）。它是 Apache-2.0 许可、
没有依赖的 C 库，静态链接不带来著佐权（copyleft）条款。上游 C API 保持不变：
`SBAlgorithmCreate`、`SBParagraphGetLevelsPtr`、`SBLineGetRunsPtr`、
`SBMirrorLocatorMoveNext` 都是 `<SheenBidi/SheenBidi.h>` 的上游全局名字。
库本身是 `compat.sheenbidi` 3.0.0。本包把 SheenBidi 的宏和无名枚举的枚举值
映射为常量和 constexpr 函数，并逐一与头文件对照检查。Unicode 的一致性测试在
每次 CI 中全部通过：BidiTest.txt 的全部 770,241 个用例和 BidiCharacterTest.txt
的全部 91,707 个用例，运行在基于模块编写的程序上。

## 模块

- `import mcpplibs.mwrap.sheenbidi;`：`<SheenBidi/SheenBidi.h>` 的完整 API（SBAlgorithm、SBParagraph、SBLine、SBRun、SBMirrorLocator、SBScriptLocator、SBCodepoint、SBCodepointSequence、SBAllocator、SBVersion），并重新导出下面的模块
- `import mcpplibs.mwrap.sheenbidi.macros;`：`SBBidiTypeR`、`SBScriptARAB`、`SBStringEncodingUTF8`、`SBLevelDefaultLTR`、`SBBidiTypeIsStrong` 等，位于 `mcpplibs::mwrap::sheenbidi::macros`

> 更多模块 API -> [用法与 API](docs/usage.md)

## 快速开始

```sh
mcpp new bidi --template mcpplibs.mwrap.sheenbidi@3.0.0
cd bidi
mcpp run
```

默认模板像渲染器那样排布 UTF-8 文本行：先求出段落的嵌入层级，再按视觉顺序
绘制该行的各个 run，并镜像括号。运行 `mcpp run -- "你的文本"` 处理自己的文本。

```toml
[dependencies]
"mcpplibs.mwrap.sheenbidi" = "3.0.0"
```

```cpp
import std;
import mcpplibs.mwrap.sheenbidi;
using namespace mcpplibs::mwrap::sheenbidi::macros;  // SBStringEncodingUTF8、SBLevelDefaultLTR …

int main() {
    std::string_view text = "abc אבג (123)";
    SBCodepointSequence seq{SBStringEncodingUTF8, text.data(), text.size()};
    SBAlgorithmRef algorithm = SBAlgorithmCreate(&seq);
    SBParagraphRef paragraph = SBAlgorithmCreateParagraph(algorithm, 0, text.size(), SBLevelDefaultLTR);
    SBLineRef line = SBParagraphCreateLine(paragraph, 0, SBParagraphGetLength(paragraph));
    const SBRun* runs = SBLineGetRunsPtr(line);  // 视觉顺序，偏移以 UTF-8 字节计
    for (SBUInteger i = 0; i < SBLineGetRunCount(line); ++i)
        std::println("[{}, {}) level {}", runs[i].offset, runs[i].offset + runs[i].length, runs[i].level);
    SBLineRelease(line);
    SBParagraphRelease(paragraph);
    SBAlgorithmRelease(algorithm);
}
```

## 宏与枚举值

`SBLevelMax` 这样的常量保持其宏的类型（`int`）。`SBBidiTypeIsStrong`、
`SBCodepointIsValid` 这样的计算是函数体即宏本身的 constexpr 函数，对每种参数
类型都与宏的值和类型一致。SheenBidi 的枚举值（`SBBidiTypeL`、`SBScriptARAB`
等）属于无名枚举，模块无法导出，因此它们是头文件为其给出的 typedef
（`SBBidiType`、`SBScript` 等）类型的常量。`SB_PUBLIC`、`SB_DEPRECATED` 和
`extern "C"` 括号仍是宏，位于 `<mwrap/sheenbidi/macros.hpp>`，在 import 之前
包含。详见 [docs/usage.md](docs/usage.md)。

## 示例

- [examples/sheenbidi](examples/sheenbidi)：一个排版工具（每个段落的逻辑到视觉顺序、层级、run 和文字
  run）和 Unicode 的两个一致性测试运行器，都基于模块编写。
  [tools/run_conformance.py](tools/run_conformance.py) 在它们上运行 BidiTest.txt 和 BidiCharacterTest.txt。

## 构建与测试

Linux（x86_64 的 GCC 和 LLVM、arm64 的 GCC）、macOS 和 Windows 的 CI 构建模块，
然后运行测试和导出审计（[tools/check_exports.py](tools/check_exports.py)）。
审计检查生成的源文件是最新的、每个声明都已导出、每个宏都已映射或注明保留原因。
CI 还从固定的上游版本运行 Unicode 17.0.0 的一致性测试，并核对每个文件与
Unicode 官方发布的一致：BidiTest.txt（770,241 个用例）和
BidiCharacterTest.txt（91,707 个用例，含镜像）。另有一个作业把默认模板作为
已安装的包生成并运行。

## 许可

封装部分为 [Apache-2.0](LICENSE)，由 mcpp-community 提供（[NOTICE](NOTICE)）。
SheenBidi 同样是 Apache-2.0（[upstream/sheenbidi/LICENSE](upstream/sheenbidi/LICENSE)）。
其数据表来自 Unicode 字符数据库（[Unicode License v3](upstream/unicode/license.txt)）。
两份声明都会部署到每个依赖本包的程序旁边。
