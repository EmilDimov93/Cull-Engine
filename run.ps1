Write-Host "Building..." -ForegroundColor Blue
g++ src/main.cpp src/renderer.cpp src/rasterization.cpp src/raytracing.cpp src/modelLoader.cpp -o build/main.exe -I"C:/Program Files/glfw-3.4.bin.WIN64/include" -L"C:/Program Files/glfw-3.4.bin.WIN64/lib-mingw-w64" -lglfw3 -lopengl32 -lgdi32 -std=c++20

Write-Host "Running..." -ForegroundColor Green
./build/main.exe

Write-Host "Done" -ForegroundColor Red