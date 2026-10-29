# Real-Time Audio Filtering & Lossless Compression (DSP)

## Overview
This repository contains the C source code, experimental analysis, and technical report for real-time digital signal processing and audio compression developed at Politecnico di Torino. The project focuses on fixed-point optimization, hardware acceleration, and real-time execution constraints on DSP platforms.

---

### 🔑 Key Engineering Highlights
- **Real-Time Audio Filtering (`src/audio_loop`):** Implemented 16-bit fixed-point FIR (Linear & Circular delay buffers) and IIR Biquad filters (Direct Form I & II) using $Q0.15$ and $Q3.12$ arithmetic.
- **Hardware Acceleration & Profiling:** Achieved up to **$70\times$ execution speedup** transitioning from floating-point routines to hardware-optimized fixed-point assembly and circular buffer addressing.
- **DMA Double-Buffering:** Integrated real-time audio I/O using double-buffered DMA interrupts ($F_s = 24\text{ kHz}$ and $48\text{ kHz}$), ensuring continuous acquisition without buffer underrun.
- **Lossless Audio Codec (`src/audio_shorten`):** Designed a predictor-based encoder (Polynomial orders $P_0$ to $P_2$ and $5^{\text{th}}$-order FIR) paired with **Adaptive Rice-Golomb entropy coding**, reducing silence bitrate down to 4 bits/sample.

---

### 📂 Repository Structure
```text
embedded-dsp-realtime/
├── docs/
│   └── DSP_Report.pdf          # Full experimental report (Theoretical derivations, cycles, spectral plots)
├── src/
│   ├── audio_loop/             # Real-time audio filtering module
│   │   ├── fir.c               # FIR filter implementations (Linear & Circular buffers)
│   │   ├── iir.c               # IIR Biquad implementations (Direct Form I & II)
│   │   └── main_bypass1.c      # Real-time processing loop and DMA interrupt handler
│   └── audio_shorten/          # Lossless audio compression module
│       ├── main.c              # Codec testbench and adaptive parameter logic
│       ├── prediction.c        # Linear predictor algorithms (P0, P1, P2, P_FIR)
│       └── rice_codec.c        # Rice-Golomb encoding/decoding routines
└── README.md                   # Project overview
