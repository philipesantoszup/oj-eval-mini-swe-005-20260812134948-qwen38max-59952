# QOI Format Codec

**ACMOJ Problem IDs**: 1730, 1734

## Table of Contents

- [QOI Format Codec](#qoi-format-codec)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
    - [Background](#background)
  - [Assignment Description](#assignment-description)
    - [Grade Composition](#grade-composition)
  - [Assignment Requirements](#assignment-requirements)
    - [QOI Specification Overview](#qoi-specification-overview)
      - [qoi-header](#qoi-header)
      - [qoi-data](#qoi-data)
      - [qoi-padding](#qoi-padding)
    - [Implementation Requirements](#implementation-requirements)
    - [Interface Requirements](#interface-requirements)
  - [Test Data](#test-data)
  - [Per-Testcase Resource Limits](#per-testcase-resource-limits)
    - [1730](#1730)
    - [1734](#1734)
  - [Submission Requirements](#submission-requirements)
    - [Submission Guidelines](#submission-guidelines)
    - [Evaluation Notes](#evaluation-notes)
    - [Academic Integrity](#academic-integrity)

## Introduction

### Background

QOI (Quite OK Image) is a lightweight, simple, and efficient lossless image format introduced by Dominic Szablewski. In practice, QOI often compresses RGB/RGBA images several times faster than PNG while achieving comparable compression ratios.

In this assignment, you will implement both the encoder and decoder for the QOI format.

## Assignment Description

### Grade Composition

| Component | Percentage |
| :--: | :--: |
| Pass **1730** | 40% |
| Pass **1734** | 40% |
| Code Review | 20% |

- Local sample data are provided to help with development and debugging. Passing local samples does not guarantee passing OJ tests.

- In the Code Review, we will **strictly examine your code style and repository organization structure, etc.**. 

## Assignment Requirements

### QOI Specification Overview

It is strongly recommended to read the official QOI documents: the [QOI specification](https://qoiformat.org/qoi-specification.pdf) and the [original blog post](https://phoboslab.org/log/2021/11/qoi-fast-lossless-image-compression). Local copies in both PDF and image formats are included under `qoi-specification/` and `PhobosLab/`.

QOI files consist of three parts: `qoi_header`, `qoi_data`, and `qoi_padding`.

#### qoi-header

Stores image width, height, channel count, and colorspace.

```cpp
struct qoi_header {
    char     magic[4] = {'q','o','i','f'};
    uint32_t width;
    uint32_t height;
    uint8_t  channels;   // 3 = RGB, 4 = RGBA
    uint8_t  colorspace; // 0 = sRGB with linear alpha, 1 = all channels linear
};
```

See `docs/assets/qoi-header.svg` for the layout.

#### qoi-data

A stream of encoded pixels using one of six operations per pixel:

1. QOI_OP_RUN
2. QOI_OP_INDEX
3. QOI_OP_DIFF
4. QOI_OP_LUMA
5. QOI_OP_RGB
6. QOI_OP_RGBA

When multiple encodings are possible for a pixel, use the following priority to ensure determinism:

QOI_OP_RUN > QOI_OP_INDEX > QOI_OP_DIFF > QOI_OP_LUMA > QOI_OP_RGB/QOI_OP_RGBA

#### qoi-padding

Eight bytes `0,0,0,0,0,0,0,1` marking the end of the image stream. See `docs/assets/qoi-padding.svg`.

### Implementation Requirements

- Implement only the two functions in `qoi.h`. Do not modify other files.
- Use the provided I/O helpers in `utils.h` (e.g., `QoiReadU8`, `QoiWriteU8`, `QoiReadU32`, `QoiWriteU32`, `QoiReadChar`, `QoiWriteChar`).
- Follow the operation tags and constants defined in `qoi.h`.

### Interface Requirements

Function signatures to implement (already declared in `qoi.h`):

```cpp
bool QoiEncode(uint32_t width, uint32_t height, uint8_t channels, uint8_t colorspace = 0);

bool QoiDecode(uint32_t &width, uint32_t &height, uint8_t &channels, uint8_t &colorspace);
```

Return `true` on success, `false` if the input is not a valid QOI stream.

## Test Data

Sample images are provided at `/workspace/data/005/sample/` with both RGB (`rgb/`) and RGBA (`rgba/`) sets. These are for local testing and reference only.

## Per-Testcase Resource Limits

### 1730

- **Time Limit (per test case)**: 2000 ms (min), 10000 ms (max)
- **Memory Limit (per test case)**: 512 MiB
- **Disk Usage**: Disk access is not permitted

### 1734

- **Time Limit (per test case)**: 2000 ms (min), 6000 ms (max)
- **Memory Limit (per test case)**: 512 MiB
- **Disk Usage**: Disk access is not permitted

## Submission Requirements

### Submission Guidelines

- For ACMOJ problems 1730 and 1734, submit only the `qoi.h` file.
- Programs must read from standard input and write to standard output.
- Ensure your implementation meets time and memory limits.
- Use C++.

### Evaluation Notes

- The evaluator will test your program with private OJ data in addition to local samples.
- Output must match the required format exactly.
- Exceeding time or memory limits will result in the corresponding verdict.
- Consider algorithmic complexity and implement efficient I/O and data handling.

### Academic Integrity

If any violations are found during evaluation or code review (including but not limited to using unconventional methods to pass test cases), your final score may be significantly reduced or become **0 points**.