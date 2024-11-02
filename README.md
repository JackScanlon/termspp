# termspp

<!-- badges: start -->
[![Project Status: Concept – Minimal or no implementation has been done yet, or the repository is only intended to be a limited example, demo, or proof-of-concept.](https://www.repostatus.org/badges/latest/concept.svg)](https://www.repostatus.org/#concept)
<!-- badges: end -->

## 1. Overview

> [!NOTE]
> TODO


## 2. Development

### 2.1. Tooling

> [!NOTE]
> TODO


### 2.2. Debugging

#### 2.2.1. Compiling [![clangd](https://img.shields.io/badge/clangd-3399dd.svg)](https://clangd.llvm.org/design/compile-commands) Virtual Commands
> [!TIP]
> - See hedronvision's compile commands extractor [here](https://github.com/hedronvision/bazel-compile-commands-extractor) for more information.

To build clangd compile commands:
- Enter: `bazel run :refresh_compile_commands -- -c dbg`

#### 2.2.2. Debug Builds via CLI
1. Building:
    - To build & run a binary built with debug symbols enter: `bazel run //src:termspp -c dbg`
    - To build the debug binary and to resolve the clangd flags enter: `bazel build //src:termspp -c dbg --verbose_failures && bazel run :refresh_compile_commands -- -c dbg`

2. Debugging:
    - See [![valgrind](https://img.shields.io/badge/Valgrind-AC441D.svg)](https://valgrind.org/docs/manual/quick-start.html) documentation

#### 2.2.3. Debug Builds using [![vscode](https://img.shields.io/badge/vscode-0078d7.svg)](https://code.visualstudio.com/)
> [!TIP]
> - See `./.vscode/launch.json` & `./.vscode/tasks.json` for more information

1. Add breakpoints where required
2. When ready, press `F5` or launch via '_Debug: Select and Start Debugging_' context menu, accessed by `CTRL + SHIFT + P`
3. Wait for program to compile
4. Step through the program from within vscode
