@echo Easy-to-use Windows build script
@echo This mirrors what Jenkins is doing
@echo Prerequirements: cmake, Visual Studio 2013 (especially msbuild.exe)

pause

set CMAKE_BUILD_TYPE=Release
set BUILD_OPTIONS=/verbosity:d /p:Configuration=%CMAKE_BUILD_TYPE%
set CMAKE_GENERATOR="Visual Studio 12 2013"
set CMAKE_BUILD_TOOL=msbuild

mkdir build
cd build
cmake .. -G %CMAKE_GENERATOR% -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE%
cmake --build .                  -- %BUILD_OPTIONS%
cmake --build . --target install -- %BUILD_OPTIONS%
