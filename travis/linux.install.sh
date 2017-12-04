# sdl2
sudo add-apt-repository ppa:zoogie/sdl2-snapshots -y
# gcc
sudo add-apt-repository ppa:ubuntu-toolchain-r/test

sudo apt-get update -qq
sudo apt-get install -qq libsdl2-dev
sudo apt-get install -qq libsdl2-mixer-dev
sudo apt-get install -qq libpng-dev
sudo apt-get install g++-6
export CC=g++-6
export CXX=g++-6
