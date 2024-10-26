# termspp

<!-- badges: start -->
[![Project Status: WIP – Initial development is in progress, but there has not yet been a stable, usable release suitable for the public.](https://www.repostatus.org/badges/latest/wip.svg)](https://www.repostatus.org/#wip)
<!-- badges: end -->

## 1. Overview

> [!NOTE]
> TODO


## 2. Development

### 2.1. Tooling

> [!NOTE]
> TODO


### 2.2. Debugging

> [!TIP]
> - See hedronvision's compile commands extractor [here](https://github.com/hedronvision/bazel-compile-commands-extractor) for more information.

Debugging using the CLI:
1. Run `bazel run :refresh_compile_commands`
2. When debugging builds:
  - Build  `bazel build //src:termspp -c dbg --verbose_failures`
  - Run  `bazel run //src:termspp -c dbg`

Debugging using `launch.json`:
1. Add breakpoints where required
2. When ready, press `F5` or launch via '_Debug: Select and Start Debugging_' context menu, accessed by `CTRL + SHIFT + P`
3. Wait for program to compile
4. Step through the program
