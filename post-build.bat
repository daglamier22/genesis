@echo off

REM Run from root directory!
if not exist "%cd%\bin\assets\shaders\" mkdir "%cd%\bin\assets\shaders"

echo "Compiling shaders..."

echo "assets/shaders/shader.vert.glsl -> bin/assets/shaders/shader.vert.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=vert assets/shaders/shader.vert.glsl -o bin/assets/shaders/shader.vert.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "assets/shaders/shader.frag.glsl -> bin/assets/shaders/shader.frag.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=frag assets/shaders/shader.frag.glsl -o bin/assets/shaders/shader.frag.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "Copying assets..."
echo xcopy "assets" "bin\assets" /h /i /c /k /e /r /y
xcopy "assets" "bin\assets" /h /i /c /k /e /r /y

echo "Done."
