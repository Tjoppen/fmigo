@echo Easy-to-use Windows build script
@echo This mirrors what Jenkins is doing
@echo Prerequirements: cmake, Visual Studio 2015 (especially msbuild.exe)


set CMAKE_BUILD_TYPE=Release
set BUILD_OPTIONS=/verbosity:d /p:Configuration=%CMAKE_BUILD_TYPE%
set CMAKE_GENERATOR="Visual Studio 14 2015"
set CMAKE_BUILD_TOOL=msbuild

mkdir build
cd build
cmake .. -G %CMAKE_GENERATOR% -DBUILD_FMUS=0 -DUSE_GPL=0 -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% && ^
cmake --build .                  -- /m %BUILD_OPTIONS% && ^
cmake --build . --target install -- %BUILD_OPTIONS%

cd ..