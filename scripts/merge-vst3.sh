#!/bin/bash
set -e

echo "================================================"
echo " Merging platform binaries into universal VST3"
echo "================================================"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

# Create universal bundle structure
echo "Creating universal bundle structure..."
mkdir -p build/WetReverb.vst3/Contents/Resources
mkdir -p build/WetReverb.vst3/Contents/x86_64-win
mkdir -p build/WetReverb.vst3/Contents/x86_64-linux
mkdir -p build/WetReverb.vst3/Contents/arm64-macos

# Copy resources from Linux build (could be any platform - they're identical)
echo "Copying shared resources..."
if [ -d "artifacts/linux/Contents/Resources" ]; then
    cp -r artifacts/linux/Contents/Resources/* build/WetReverb.vst3/Contents/Resources/
elif [ -d "artifacts/windows/Contents/Resources" ]; then
    cp -r artifacts/windows/Contents/Resources/* build/WetReverb.vst3/Contents/Resources/
elif [ -d "artifacts/macos/Contents/Resources" ]; then
    cp -r artifacts/macos/Contents/Resources/* build/WetReverb.vst3/Contents/Resources/
fi

# Copy Windows binary
echo "Copying Windows binary..."
WINDOWS_FOUND=false
if [ -f "artifacts/windows/Contents/x86_64-win/WetReverb.vst3" ]; then
    cp artifacts/windows/Contents/x86_64-win/WetReverb.vst3 build/WetReverb.vst3/Contents/x86_64-win/
    echo "  Found at artifacts/windows/Contents/x86_64-win/WetReverb.vst3"
    WINDOWS_FOUND=true
elif [ -f "artifacts/windows/VST3/Release/WetReverb.vst3/Contents/x86_64-win/WetReverb.vst3" ]; then
    cp artifacts/windows/VST3/Release/WetReverb.vst3/Contents/x86_64-win/WetReverb.vst3 build/WetReverb.vst3/Contents/x86_64-win/
    echo "  Found at artifacts/windows/VST3/Release/WetReverb.vst3/Contents/x86_64-win/WetReverb.vst3"
    WINDOWS_FOUND=true
fi
if [ "$WINDOWS_FOUND" = false ]; then
    echo "WARNING: Windows binary not found!"
    find artifacts/windows -name "*.dll" -o -name "WetReverb.vst3" 2>/dev/null | head -5 || echo "No Windows binary found"
fi

# Copy Linux binary
echo "Copying Linux binary..."
LINUX_FOUND=false
if [ -f "artifacts/linux/Contents/x86_64-linux/WetReverb.so" ]; then
    cp artifacts/linux/Contents/x86_64-linux/WetReverb.so build/WetReverb.vst3/Contents/x86_64-linux/
    echo "  Found at artifacts/linux/Contents/x86_64-linux/WetReverb.so"
    LINUX_FOUND=true
elif [ -f "artifacts/linux/VST3/Release/WetReverb.vst3/Contents/x86_64-linux/WetReverb.so" ]; then
    cp artifacts/linux/VST3/Release/WetReverb.vst3/Contents/x86_64-linux/WetReverb.so build/WetReverb.vst3/Contents/x86_64-linux/
    echo "  Found at artifacts/linux/VST3/Release/WetReverb.vst3/Contents/x86_64-linux/WetReverb.so"
    LINUX_FOUND=true
fi
if [ "$LINUX_FOUND" = false ]; then
    echo "WARNING: Linux binary not found!"
    find artifacts/linux -name "*.so" 2>/dev/null | head -5 || echo "No SO files found"
fi

# Copy macOS binary
echo "Copying macOS binary..."
MACOS_FOUND=false
if [ -f "artifacts/macos/Contents/arm64-macos/WetReverb" ]; then
    cp artifacts/macos/Contents/arm64-macos/WetReverb build/WetReverb.vst3/Contents/arm64-macos/
    echo "  Found at artifacts/macos/Contents/arm64-macos/WetReverb"
    MACOS_FOUND=true
elif [ -f "artifacts/macos/VST3/Release/WetReverb.vst3/Contents/MacOS/WetReverb" ]; then
    cp artifacts/macos/VST3/Release/WetReverb.vst3/Contents/MacOS/WetReverb build/WetReverb.vst3/Contents/arm64-macos/
    echo "  Found at artifacts/macos/VST3/Release/WetReverb.vst3/Contents/MacOS/WetReverb"
    MACOS_FOUND=true
elif [ -f "artifacts/macos/Contents/MacOS/WetReverb" ]; then
    cp artifacts/macos/Contents/MacOS/WetReverb build/WetReverb.vst3/Contents/arm64-macos/
    echo "  Found at artifacts/macos/Contents/MacOS/WetReverb"
    MACOS_FOUND=true
fi
if [ "$MACOS_FOUND" = false ]; then
    echo "WARNING: macOS binary not found!"
    find artifacts/macos -type f -name "WetReverb" 2>/dev/null | head -5 || echo "No WetReverb executable found"
fi

# Copy macOS bundle metadata
echo "Copying macOS bundle metadata..."
MACOS_PLIST_FOUND=false
if [ -f "artifacts/macos/VST3/Release/WetReverb.vst3/Contents/Info.plist" ]; then
    cp artifacts/macos/VST3/Release/WetReverb.vst3/Contents/Info.plist build/WetReverb.vst3/Contents/
    [ -f "artifacts/macos/VST3/Release/WetReverb.vst3/Contents/PkgInfo" ] && cp artifacts/macos/VST3/Release/WetReverb.vst3/Contents/PkgInfo build/WetReverb.vst3/Contents/
    echo "  Found at artifacts/macos/VST3/Release/WetReverb.vst3/Contents/"
    MACOS_PLIST_FOUND=true
elif [ -f "artifacts/macos/Contents/Info.plist" ]; then
    cp artifacts/macos/Contents/Info.plist build/WetReverb.vst3/Contents/
    [ -f "artifacts/macos/Contents/PkgInfo" ] && cp artifacts/macos/Contents/PkgInfo build/WetReverb.vst3/Contents/
    echo "  Found at artifacts/macos/Contents/"
    MACOS_PLIST_FOUND=true
fi
if [ "$MACOS_PLIST_FOUND" = false ]; then
    echo "WARNING: macOS Info.plist not found - codesign may fail!"
fi

echo ""
echo "================================================"
echo " Universal VST3 bundle created!"
echo "================================================"
echo ""
echo "Bundle location: build/WetReverb.vst3"
echo ""
echo "Bundle structure:"
find build/WetReverb.vst3 -type f | head -20
echo ""
echo "Bundle size: $(du -sh build/WetReverb.vst3 | cut -f1)"
echo ""