$SourceFile = "example/example.cpp"
$UseOptimizations = $true

Write-Host "Building..." -ForegroundColor Blue

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

if ($UseOptimizations) {
    $OptimizationFlags = @(
        "-O3", "-DNDEBUG", "-flto=auto", "-march=native", "-mtune=native",
        "-fno-math-errno", "-fno-trapping-math", "-fno-signed-zeros",
        "-freciprocal-math", "-fassociative-math", "-ffp-contract=fast",
        "-funroll-loops", "-mprefer-vector-width=128"
    )
}
else {
    $OptimizationFlags = @("-O0", "-g")
}

g++ $SourceFile src/editor/client.cpp src/editor/rasterization.cpp src/raytracer/raytracer.cpp src/raytracer/bvh.cpp src/scene/modelLoader.cpp -o build/main.exe `
    -std=c++20 `
    @OptimizationFlags `
    -pthread `
    -I"C:/Program Files/glfw-3.4.bin.WIN64/include" -L"C:/Program Files/glfw-3.4.bin.WIN64/lib-mingw-w64" `
    -lglfw3 -lopengl32 -lgdi32

Write-Host "Running..." -ForegroundColor Green
./build/main.exe

Write-Host "Done" -ForegroundColor Red