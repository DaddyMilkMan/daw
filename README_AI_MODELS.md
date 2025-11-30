# AI Model Installation Instructions

To enable "True AI" features (Stem Separation and Voice Cloning) in Zenith DAW, you must download the pre-trained neural network models.

## 1. Stem Separation (Demucs)

**EASIEST METHOD:**
1.  Run the `install_ai_models.bat` file located in the `c:\zenith\daw\` folder.
2.  Follow the on-screen prompts.
3.  This will automatically download, convert, and install the `htdemucs.onnx` model for you.

**MANUAL METHOD (If script fails):**
1.  Clone https://github.com/sevagh/demucs.onnx
2.  Run `python scripts/convert-pth-to-onnx.py .`
3.  Copy the resulting `onnx-models/htdemucs.onnx` to:
    *   `c:\zenith\daw\zenith-core\Resources\models\htdemucs.onnx`

## 2. Voice Cloning (RVC)

We use RVC (Retrieval-based Voice Conversion) models.

1.  **Download:** Get the base RVC ONNX model (e.g., `hubert_base.onnx`).
2.  **Install:** Place the file at:
    *   `c:\zenith\daw\zenith-core\Resources\models\hubert_base.onnx`

## 3. Enable in Build

Once files are present, the `GrokDAWController` will automatically detect them.
To fully enable the C++ inference engine, you must link **ONNX Runtime** in your `CMakeLists.txt`:

```cmake
find_package(OnnxRuntime REQUIRED)
target_link_libraries(ZenithCore PRIVATE OnnxRuntime::OnnxRuntime)
add_compile_definitions(ZENITH_ENABLE_ONNX=1)
```
