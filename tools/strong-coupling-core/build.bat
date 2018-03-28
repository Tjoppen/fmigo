mkdir build
cd build
@REM Change UMFPACK_INCLUDE_DIR to where you have umfpack installed
cmake -DUMFPACK_INCLUDE_DIR="C:\Users\thardin\Downloads\suitesparse-metis-for-windows-master\build\install\include\suitesparse" ..
msbuild Project.sln
pause
