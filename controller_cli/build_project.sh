# script to build and compile project

echo "Building Motor/Trigger Controller Project"
echo

# Clean build directories
rm -rf build

# create the build directory
mkdir build
cd build

# build the project
cmake ..

# compile the project
cmake --build . --config Release -- -j$(nproc)

