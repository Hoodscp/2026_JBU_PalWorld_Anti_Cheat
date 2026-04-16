@echo off
echo Cloning third-party libraries into Vendor directory...
mkdir Vendor
cd Vendor

echo 1. Cloning ImGui...
git clone https://github.com/ocornut/imgui.git ImGui

echo 2. Cloning MinHook...
git clone https://github.com/TsudaKageyu/minhook.git MinHook

echo 3. Cloning Kiero (Universal D3D Hook)...
git clone https://github.com/Rebzzel/kiero.git kiero

echo Done! Open folder in Visual Studio (Open Folder as CMake Project).
pause
