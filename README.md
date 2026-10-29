# Real-Time Audio Filtering & Lossless Compression (DSP)

## Overview
This repository contains the code, experimental analysis, and technical report for real-time digital signal processing and audio compression developed at Politecnico di Torino. The project focuses on fixed-point optimization and real-time execution constraints on DSP hardware.

---

### 🔑 Key Engineering Highlights
- **Fixed-Point Real-Time Filtering:** Implemented 16-bit fixed-point FIR (Linear & Circular delay buffers) and IIR Biquads (Direct Form I & II) using Q0.15 and Q3.12 arithmetic.
- **Hardware Acceleration:** Achieved up to **$70\times$ execution speedup** transitioning from floating-point routines to optimized assembly and hardware circular buffers.
- **DMA Double-Buffering:** Integrated real-time audio I/O using DMA interrupts ($F_s = 24\text{ kHz}$ and $48\text{ kHz}$) ensuring zero buffer-underrun processing.
- **Lossless Audio Codec:** Designed a predictor-based encoder (Orders $P_0$ to $P_5$) coupled with **Adaptive Rice-Golomb entropy coding**, reducing silence bitrate down to 4 bits/sample.

---

### 📂 Repository Structure
```text
embedded-dsp-realtime/
├── docs/                 # Full technical report (11_group_DSP.pdf)
└── README.md             # Project overview
