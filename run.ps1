$SourceFile = "example/example.cpp"

Write-Host "Building..." -ForegroundColor Blue

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

g++ $SourceFile src/renderer/renderer.cpp src/renderer/editor.cpp src/renderer/raytracer.cpp src/renderer/modelLoader.cpp -o build/main.exe -I"C:/Program Files/glfw-3.4.bin.WIN64/include" -L"C:/Program Files/glfw-3.4.bin.WIN64/lib-mingw-w64" -lglfw3 -lopengl32 -lgdi32 -std=c++20

Write-Host "Running..." -ForegroundColor Green
./build/main.exe

Write-Host "Done" -ForegroundColor Red