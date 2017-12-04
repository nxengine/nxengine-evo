# sdl2
sudo add-apt-repository ppa:zoogie/sdl2-snapshots -y
# gcc
[[ -z "${INSTALLGCC}" ]] || sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y

sudo apt-get update -qq
sudo apt-get install -qq libsdl2-dev
sudo apt-get install -qq libsdl2-mixer-dev
sudo apt-get install -qq libpng-dev
[[ -z "${INSTALLGCC}" ]] || sudo apt-get install -qq g++-6
