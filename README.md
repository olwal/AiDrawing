# AIDrawing

An interactive C++ drawing application that combines AI-powered visual completion through real-time canvas interpretation and image generation.  
Built with the Cinder framework, this application allows users to draw naturally without entering prompts — an AI model analyzes the artwork in real time and collaborates with an image diffusion pipeline (**StreamDiffusion**) to *augment and auto-complete* the drawing.

## Features

- **Interactive Vector Drawing**: Cursor-based drawing  
- **AI Canvas Analysis**: Real-time interpretation of drawings using Ollama vision models  
- **AI Image Generation**: Real-time augmentation of drawings through diffusion models  
- **Real-time Communication**:  
  - Spout integration for texture sharing between applications  
  - OSC messaging for communication with TouchDesigner and other tools  
- **Advanced Drawing System**:  
  - Undo/redo via command pattern  
  - Stroke smoothing and dynamic width  
  - Customizable colors and brushes  

---

## Examples

*(All GIFs below show AI-assisted drawing auto-completion in real time.)*

| Final result | Animation (4X speed) |
|------------|------------|
| <img src="assets/car_perspective_thumb.png" width="480"/> | <img src="assets/car_perspective_4x_0.25scale.gif" width="480"/> |
| <img src="assets/earth_thumb.png" width="480"/> | <img src="assets/earth_4x_0.25scale.gif" width="480"/> |
| <img src="assets/guitar_cello_thumb.png" width="480"/> | <img src="assets/guitar_cello_4x_0.25scale.gif" width="480"/> |
| <img src="assets/happy_sun_thumb.png" width="480"/> | <img src="assets/happy_sun_4x_0.25scale.gif" width="480"/> |
| <img src="assets/house_thumb.png" width="480"/> | <img src="assets/house_4x_0.25scale.gif" width="480"/> |
| <img src="assets/ocean_sailboat_thumb.png" width="480"/> | <img src="assets/ocean_sailboat_4x_0.25scale.gif" width="480"/> |
| <img src="assets/rainy_cloud_thumb.png" width="480"/> | <img src="assets/rainy_cloud_4x_0.25scale.gif" width="480"/> |
| <img src="assets/surfer_thumb.png" width="480"/> | <img src="assets/surfer_4x_0.25scale.gif" width="480"/> |
| <img src="assets/train_thumb.png" width="480"/> | <img src="assets/train_4x_0.25scale.gif" width="480"/> |

---

## Application Architecture

### Class Hierarchy

```
App (Cinder) → CinderApp → DrawingApp → AIDrawingApp
```


### Core Components

#### Drawing System (`vdraw` namespace)
- **Vec2** — 2D vector math  
- **Color** — RGBA color representation  
- **StrokePoint** — Points with position, pressure, timestamp  
- **Stroke** — Collection of stroke points with styling  
- **Drawing** — Main canvas for stroke management  
- **DrawingCommand** — Command pattern for undo/redo  

#### AI Integration
- **OllamaClient** — Interface to Ollama local AI models for vision analysis  
- **ThreadSafeList** — Thread-safe container for AI results  

#### Communication
- **Spout** — Real-time texture sharing (Windows)  
- **OSC** — Open Sound Control messaging  

---

### System Overview

```
      ┌────────────────────────────────────────────────────────────┐
      │                        User                                │
      │                  (Interactive Drawing)                     │
      └───────────────┬────────────────────────────────────────────┘
                      │
                      ▼
           ┌────────────────────┐
           │     AIDrawing      │
           │  (Cinder App)      │
           │────────────────────│
           │ - Captures strokes │
           │ - Renders drawing  │
           │ - Manages AI I/O   │
           └─────────┬──────────┘
                     │
     Drawing image   │   Vision prompt
            ┌────────┴────────┐
            │                 │
            ▼                 ▼
  ┌───────────────────┐   ┌───────────────────────────┐
  │ OllamaClientCinder│   │ StreamDiffusionSpoutServer│
  │ (AI Interpretation│   │ (AI Image Generation)     │
  │  via Ollama)      │   │  via Diffusion Models)    │
  │───────────────────│   │───────────────────────────│
  │ - Sends drawing   │   │ - Receives image+prompt   │
  │   to Ollama model │   │ - Generates enhanced img  │
  │ - Gets description│   │ - Outputs via Spout       │
  └─────────┬─────────┘   └────────────┬──────────────┘
            │                          │
            └───────────┬──────────────┘
                        ▼
           ┌──────────────────────────────┐
           │     Real-time Compositing     │
           │ (Cinder + Spout Integration)  │
           │ - Combines AI output + drawing│
           │ - Displays augmented canvas   │
           └──────────────────────────────┘
```


---

## Dependencies & Libraries

### Core Framework
- **[Cinder](https://libcinder.org/)** — Creative coding framework for C++  
  - OpenGL rendering  
  - Cross-platform windowing and input handling  
  - Image processing and file I/O  

---

### AI Integration
- **[Ollama](https://ollama.com/)** — Local AI model inference  
  - Supports vision models like LLaVA for image analysis  
  - REST API for local communication  
  - Runs locally, no cloud dependencies  

- **[OllamaClientCinder](https://github.com/olwal/OllamaClient)** — Cross-framework C++ client for Ollama API  
  - Works with Cinder and OpenFrameworks  
  - Supports text and vision models  
  - Async/sync APIs for flexible integration  
  - Windows support via WinHTTP  
  - No external dependencies (includes Base64 + JSON)  

- **[StreamDiffusionSpoutServer](https://github.com/olwal/StreamDiffusionSpoutService)** — Real-time image generation server  
  - Receives **drawing image + prompt**  
  - Performs **real-time image-to-image diffusion**  
  - GPU-accelerated output shared via Spout  
  - Controlled via OSC  
  - Windows-only (requires Spout)  

---

### Real-time Communication
- **[Spout](https://spout.zeal.co/)** — Real-time texture sharing framework  
  - GPU texture sharing between applications  
  - DirectX/OpenGL interoperability  
  - Popular in VJ and creative coding communities  

- **OSC (Open Sound Control)** — Network protocol  
  - UDP-based messaging  
  - Used for communication with TouchDesigner, Max/MSP, etc.  

---

## Project Structure

```
AiDrawing/
├── src/                                 # Source code
│   ├── main.cpp                         # Application entry point
│   ├── AIDrawingApp.cpp/.h              # Main application class with AI integration
│   ├── DrawingApp.cpp/.h                # Base drawing functionality
│   ├── CinderApp.cpp/.h                 # Cinder framework wrapper
│   ├── VectorDrawing.cpp/.h             # Vector drawing system
│   ├── ThreadSafeList.*                 # Thread-safe data structures
│   └── CinderConsole.cpp/.h             # Console utilities
├── external/                            # External dependencies (git submodules)
│   └── OllamaClient/                    # C++ client for Ollama API
├── vc2022/                              # Visual Studio 2022 project files
│   ├── AIDrawing.sln                    # Solution file
│   ├── AIDrawing.vcxproj                # Project file
│   └── AIDrawing.vcxproj.filters        # Project filters
├── include/                             # Header files
├── blocks/                              # Third-party Cinder blocks (from Cinder install)
├── assets/                              # Demo images and animations
├── resources/                           # Application resources (icons, etc.)
└── README.md                            # This file
```



---

## Getting Started

### Prerequisites

1. **Visual Studio 2022 or newer** with C++ support
2. **Cinder Framework** — Download from [libcinder.org](https://libcinder.org/) and include the Spout block
3. **Ollama** — Install from [ollama.com](https://ollama.com/)
4. **LLaVA model** — Run `ollama pull llava:7b` to download vision model
5. [**StreamDiffusionSpoutServer**](https://github.com/olwal/StreamDiffusionSpoutService) — Real-time image generation server (separate application, clone and run separately)
6. [**Spout**](https://spout.zeal.co/) — GPU texture sharing (included via Cinder Spout block)

**Note:** [OllamaClientCinder](https://github.com/olwal/OllamaClient) is included automatically as a git submodule.

### Building

1. **Clone this repository with submodules:**
   ```bash
   git clone --recursive https://github.com/olwal/AiDrawing.git
   ```

   *If you already cloned without `--recursive`, initialize submodules:*
   ```bash
   git submodule update --init --recursive
   ```

2. Open `vc2022/AIDrawing.sln` in Visual Studio
3. Configure Cinder paths in project settings if needed
4. Build and run (F5)

### Setup AI Models

```bash
# Install Ollama
# Download from https://ollama.com/

# Pull vision model
ollama pull llava:7b

# Verify installation
ollama list
```

### Running the AI Services

Before launching **AIDrawing**, make sure both AI backends are running:

1. **Start Ollama** — This runs automatically after installation.
   - Handles vision analysis of your drawing.
   - Provides semantic descriptions that guide the diffusion model.

2. **Run StreamDiffusionSpoutServer** — Must be active for image generation.
   - Receives both the user’s drawing (as an image) and the AI-generated description (as a text prompt).
   - Performs real-time **image-to-image diffusion** to enhance and complete your artwork.
   - Sends the generated result back to **AIDrawing** via **Spout** (GPU texture sharing).

   When both services are active, **AIDrawing** forms a real-time creative loop:

  User draws → AIDrawing captures strokes → OllamaClientCinder analyzes drawing
  → StreamDiffusionSpoutServer generates enhanced imagery → Spout sends result
  → AIDrawing composites drawing + AI output in real time

## Usage

### Controls
- **Mouse**: Draw on canvas
- **T**: Toggle text overlay
- **Escape**: Exit application

## AI Models

The application supports any Ollama vision model, for example:
- **llava:7b** (default) - Good balance of speed and quality
- **llava:13b** - Higher quality, slower inference  
- **granite3.2-vision** - Alternative vision model

Change models by modifying the `model` variable in `AIDrawingApp::setup()`.

### Performance Notes

- **Closed-Loop AI Augmentation**  
  AIDrawing forms a continuous feedback cycle where each user stroke is interpreted by the vision model and enhanced by diffusion — no text prompting required.

- **Real-Time Vision Analysis**  
  **OllamaClientCinder** communicates with **Ollama** for multimodal inference, asynchronously generating semantic descriptions of the drawing.

- **AI Image Generation**  
  **StreamDiffusionSpoutServer** takes the current drawing and its interpreted prompt to perform real-time **image-to-image diffusion**, producing AI-augmented content streamed back via **Spout**.

- **Concurrent Pipeline**  
  Drawing, inference, and generation operate in separate threads, coordinated by a **ThreadSafeList** for synchronized data sharing.

- **GPU Optimization**  
  Spout enables zero-copy GPU texture sharing for seamless real-time compositing between the Cinder app and the diffusion server.

- **Recommended Hardware**  
  - GPU: ≥ 8 GB VRAM (for stable diffusion models)  
  - CPU: ≥ 6 cores for smooth concurrency and real-time updates


## Contributing

The codebase follows standard C++ practices with:
- Command pattern for undo/redo
- Namespace organization (`vdraw`)
- Clear separation of concerns between drawing, AI, and communication

## Author

[Alex Olwal](https://github.com/olwal)

## Related works: Closed-Loop AI Drawing Tools with VLM Interpretation
- **[Graffiti-X](https://arxiv.org/abs/2508.19254)** - Real-time generative drawing system that interprets both formal intent (line quality, proportion) and contextual intent (semantic meaning) using VLMs for multi-user collaborative creation.
- **[SketchAgent](https://sketch-agent.csail.mit.edu/)** - Language-driven sequential sketch generation using multimodal LLMs that enables stroke-by-stroke collaboration and chat-based editing of sketches.
- **[AutoSketch](https://arxiv.org/abs/2502.06860)** - VLM-assisted style-aware vector sketch completion that iteratively analyzes and adjusts partial sketches to match the original drawing style.
- **[CadVLM](https://www.research.autodesk.com/publications/cad-vlm/)** - End-to-end vision language model for parametric CAD sketch generation that performs autocompletion by interpreting partial sketches as both image and text inputs.
