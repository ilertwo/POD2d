# POD2d

**Pixel animation drawing software for displays.**

POD2d is a desktop application written in C++ with Qt 6. Draw pixel-art animations frame by frame — with layers, a custom palette and a minimap — and **export them as ready-to-paste code arrays** for microcontroller displays (Arduino-style `PROGMEM` bitmaps).

![POD2d screenshot](https://raw.githubusercontent.com/ilertwo/POD2d/main/docs/screenshot.png)

## Features

- **Export to code** — turn a frame or the whole animation into a C++ or MicroPython array you can paste straight into your firmware (see [Code export](#code-export))
- **Frame-by-frame animation** — frame list with per-frame duration, playback, duplicate / hide / reorder / delete frames
- **Layers** — add, delete, duplicate, hide, merge down, reorder (drag & drop or context menu), layer thumbnails
- **Drawing tools** — pen, eraser, text, fill, dithering, pipette
- **Shapes** — line, broken line, circle, rectangle
- **Selection tools** — rectangle, lasso, shape selection, lighten
- **View tools** — pan, rotate, mirror, center view, minimap
- **Custom palette** — add, edit, remove and reorder colors; primary / secondary color
- **Undo / redo**
- **Three themes** — Dark, Light and 1bit (pure black-and-white, monospace "pixel" look)
- **Dockable workspace** — every panel (Minimap, Tools, Layers, Palette, Frames) can be floated, tabbed, moved to any side and resized; the layout is restored on the next launch (**View → Reset Layout** brings back the default)
- **Adaptive panels** — Layers and Frames switch between row and column layout depending on the panel shape; the Tools panel reflows its buttons into 1–6 columns
- Drag & drop of project files, recent projects on the start page
- Export to `.png`, animated `.gif` and the native `.pod2d` project format
- Settings dialog
- Logging to the `logs/` folder

## Code export

The main purpose of POD2d: get your artwork into a display's memory with no extra tools.
Open the **Export** dialog, choose the output and press **Generate** (then **Copy Code**).

| Option | What it does |
|---|---|
| Language | Output language of the generated code: C++ (default) or MicroPython |
| Optimize memory | Packs the data more compactly to save flash / RAM on the device |
| Invert colors | Inverts the output colors (for displays with inverted polarity) |
| Data arrays only | Outputs only the data arrays, without the surrounding boilerplate |
| Export entire animation | Generates an array for every frame instead of only the current one |

Example output (a 32×32 frame in RGB565, 2048 bytes):

```cpp
// Method: RGB565 RAW (16-bit) (Frame 0)
// Size: 1024 pixels (2048 bytes)
const uint16_t bitmap_frame_0[] PROGMEM = {
  0X0000, 0X0000, 0X0000, 0X0000, 0X0000, 0X0000, 0X0000, 0X0000,
  ...
};
```

You can also export to a file: **Save Project (.pod2d)**, **Export Image (.png)** and **Export Animation (.gif)**.

## Usage tips

| Where | Action |
|---|---|
| Palette | Left click — primary color, right click — secondary color |
| Palette | Double click — edit color, middle click — remove color |
| Palette | Shift + click on two swatches — swap them |
| Layers | Right click — hide / duplicate / merge / move / delete |
| Frames | Right click — hide / duplicate / move / delete |
| Frames | Double click on empty space — duplicate the current frame |
| Layers, Frames | Drag items to reorder them |
| Dock panel | Double click on a tab — detach the panel; drop it on another panel's edge to place side by side, on its title to make tabs |

## Tech stack

| | |
|---|---|
| Language | C++17 |
| Framework | Qt 6.8.3 (Widgets) |
| Build system | CMake |
| Compiler | MinGW 64-bit |
| Platform | Windows |

## Download

Ready-to-run Windows build: see the [Releases](https://github.com/ilertwo/POD2d/releases) page.

1. Download the `.zip` file.
2. Unzip it.
3. Run `POD2d.exe`.

## Build from source

### Requirements

- Qt 6.8.3 with the MinGW 64-bit kit
- CMake 3.16 or newer
- Qt Creator (optional, recommended)

### Steps

```bash
git clone https://github.com/ilertwo/POD2d.git
cd POD2d/POD2d

cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Or open `POD2d/CMakeLists.txt` in Qt Creator, select the **Desktop Qt 6.8.3 MinGW 64-bit** kit, and press **Run**.

To collect all needed Qt DLLs next to the executable:

```bash
windeployqt --release path\to\POD2d.exe
```

> **Note:** themes (`themes/*.qss`) and icons (`image/<theme>/`) are loaded from the source folder, so keep these directories next to the executable (or next to the sources when running from Qt Creator).

## Themes

Themes are plain Qt style sheets in `themes/`:

| File | Look |
|---|---|
| `dark.qss` | Graphite surfaces, blue accent |
| `light.qss` | Soft paper-gray surfaces, blue accent |
| `1bit.qss` | Black and white only, monospace font, inverted hover / selection |

Each theme has its own icon set in `image/<theme>/`. The 1bit theme uses letters and symbols instead of icons. The selected theme is stored in `QSettings` (`POD2d` / `EditorSettings`).

## Project structure

```
POD2d/
├── core/          # application logic
├── dialogs/       # dialog windows
├── models/        # data models
├── widgets/       # custom Qt widgets (frame list, layer list, palette, ...)
├── themes/        # dark / light / 1bit themes (.qss)
├── image/         # icons and images, one folder per theme
├── main.cpp       # entry point
├── mainwindow.*   # main window (code + .ui form)
└── CMakeLists.txt
```

## Roadmap

- [ ] More export formats and output languages
- [ ] Linux build

## License

This project is licensed under the [MIT License](LICENSE).

## Author

**Artem Navroshchuk**
[GitHub](https://github.com/ilertwo) · [LinkedIn](https://www.linkedin.com/in/artem-navroshchuk-3a7849282/)
