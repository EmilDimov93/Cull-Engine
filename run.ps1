Write-Host "Building..." -ForegroundColor Blue
g++ src/main.cpp src/renderer.cpp src/rasterization.cpp src/raytracing.cpp src/modelLoader.cpp -o build/main.exe -std=c++20

Write-Host "Running..." -ForegroundColor Green
./build/main.exe

Write-Host "Done" -ForegroundColor Red

./build/img.ppm