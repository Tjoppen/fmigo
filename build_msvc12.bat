@echo Easy-to-use Windows build script
@echo This mirrors what Jenkins is doing
@echo Prerequirements: cmake, Visual Studio 2013 (especially msbuild.exe)

set CMAKE_BUILD_TYPE=Debug
set BUILD_OPTIONS=/verbosity:m /p:Configuration=%CMAKE_BUILD_TYPE%
set CMAKE_GENERATOR="Visual Studio 12 2013"
set CMAKE_BUILD_TOOL=msbuild
set BUILD_DIR=build

mkdir %BUILD_DIR%
cd %BUILD_DIR%
cmake .. -G %CMAKE_GENERATOR% -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% && ^
cmake --build .                  -- /m %BUILD_OPTIONS% && ^
cmake --build . --target install -- %BUILD_OPTIONS%

cd ..