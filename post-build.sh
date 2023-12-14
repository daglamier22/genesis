#!/bin/bash

# Run from root directory!
mkdir -p bin/assets
mkdir -p bin/assets/shaders

echo "Compiling shaders..."

echo "assets/shaders/shader.vert.glsl -> bin/assets/shader/shader.vert.spv"
$VULKAN_SDK/bin/glslc -fshader-stage=vert assets/shaders/shader.vert.glsl -o bin/assets/shaders/shader.vert.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "assets/shaders/shader.frag.glsl -> bin/assets/shader/shader.frag.spv"
$VULKAN_SDK/bin/glslc -fshader-stage=frag assets/shaders/shader.frag.glsl -o bin/assets/shaders/shader.frag.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "Copying assets..."
echo cp -R "assets" "bin"
cp -R "assets" "bin"

echo "Done."
