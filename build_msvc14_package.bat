@echo Easy-to-use Windows build script
@echo This mirrors what Jenkins is doing
@echo Prerequirements: cmake, Visual Studio 2015 (especially msbuild.exe)

set CMAKE_BUILD_TYPE=Release
set BUILD_OPTIONS=/verbosity:m /p:Configuration=%CMAKE_BUILD_TYPE%
set CMAKE_GENERATOR="Visual Studio 14 2015"
set CMAKE_BUILD_TOOL=msbuild
set BUILD_DIR=build_msvc14_win32

mkdir %BUILD_DIR%
cd %BUILD_DIR%

@rem Building FMUs is required for Jenkins. If you change the value of BUILD_FMUS in here you will receive a stern talking to
@rem Create your own copy of this file if you need to do that!
@rem To reiterate: ~vv  ABSOLUTELY NO GEFINGERPOKEN BUILD_FMUS vv~
cmake .. -G %CMAKE_GENERATOR% -DBUILD_FMUS=OFF -DUSE_GPL=0 -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% && ^
cmake --build .                  -- /m %BUILD_OPTIONS% && ^
cmake --build . --target package -- %BUILD_OPTIONS%

cd ..
