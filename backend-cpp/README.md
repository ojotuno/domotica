# Backend en C++ (Crow)

## Requisitos
- CMake (≥3.16)
- Compilador C++17: MSVC (Visual Studio Build Tools) en Windows, GCC/Clang en Linux
- vcpkg

## 1. Instalar vcpkg (una sola vez, por máquina)

**Windows (PowerShell):**
```powershell
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
```

**Linux:**
```bash
sudo apt install build-essential curl zip unzip tar pkg-config   # deps de vcpkg
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.sh
```

Anota la ruta donde lo clonaste, la necesitas en el paso 3 (la ruta cambia
según la máquina/SO, pero el comando de configuración es el mismo).

## 2. Instalar dependencias del proyecto
Desde la carpeta `backend-cpp` (donde está `vcpkg.json`), vcpkg instala
automáticamente `crow` y `cpr` en modo manifest al configurar CMake — no
hace falta instalarlas a mano.

## 3. Compilar
Mismo comando en Windows y Linux, solo cambia la ruta al toolchain:

```bash
cd backend-cpp
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<ruta-a-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```
- Windows ej: `-DCMAKE_TOOLCHAIN_FILE=C:/Users/adria/vcpkg/scripts/buildsystems/vcpkg.cmake`
- Linux ej: `-DCMAKE_TOOLCHAIN_FILE=/home/adria/vcpkg/scripts/buildsystems/vcpkg.cmake`

La primera vez tardará varios minutos por máquina (vcpkg compila Crow/cpr
para ese SO en concreto — los binarios de Windows y Linux no se comparten,
tendrás una carpeta `build/` distinta en cada uno, no la subas a git).

## 4. Ejecutar
- Windows: `.\build\Release\server.exe`
- Linux: `./build/server`

Escucha en `http://localhost:3001`, mismos endpoints que la versión Node:
`/api/login`, `/api/logout`, `/api/session`, `/api/widgets`, `/api/toggle/:id`.

## En VS Code
Instala la extensión **CMake Tools**. Al abrir la carpeta detectará el
`CMakeLists.txt`. En la barra inferior:
- Selecciona el *kit* (compilador, ej. "Visual Studio Community 2022 - x64")
- Click en **Build** para compilar
- Click en **Run** o `F5` para ejecutar/depurar con breakpoints

Puede que tengas que indicarle el toolchain de vcpkg en
`.vscode/settings.json`:
```json
{
  "cmake.configureArgs": [
    "-DCMAKE_TOOLCHAIN_FILE=<ruta-a-vcpkg>/scripts/buildsystems/vcpkg.cmake"
  ]
}
```

## Notas
- Config y comportamiento son idénticos a la versión Node: `config.json` sigue
  siendo la fuente de verdad para id → IP/endpoint del ESP32.
- El frontend (`../frontend/index.html`) no cambia nada — sigue hablando por
  REST con `http://localhost:3001`.
