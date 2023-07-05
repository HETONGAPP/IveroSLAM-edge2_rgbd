cd
# update apt get
sudo apt-get update -y
apt-get upgrade -y
apt-get dist-upgrade -y
 
sudo apt-get install -y build-essential cmake git pkg-config wget software-properties-common libeigen3-dev libboost-all-dev libsuitesparse-dev libssl-dev freeglut3-dev libusb-1.0-0-dev pkg-config libgtk-3-dev unzip -y

mkdir software
cd software

#Locally suppress stderr to avoid raising not relevant messages
exec 3>&2
exec 2> /dev/null
con_dev=$(ls /dev/video* | wc -l)
exec 2>&3

if [ $con_dev -ne 0 ];
then
	echo -e "\e[32m"
	read -p "Remove all RealSense cameras attached. Hit any key when ready"
	echo -e "\e[0m"
fi

lsb_release -a
echo "Kernel version $(uname -r)"
sudo apt-get update
cd ~/
sudo rm -rf ./librealsense_build
mkdir librealsense_build && cd librealsense_build

if [ $(sudo swapon --show | wc -l) -eq 0 ];
then
	echo "No swapon - setting up 1Gb swap file"
	sudo fallocate -l 2G /swapfile
	sudo chmod 600 /swapfile
	sudo mkswap /swapfile
	sudo swapon /swapfile
	sudo swapon --show
fi

echo Installing Librealsense-required dev packages
rm -f ./master.zip

wget https://github.com/IntelRealSense/librealsense/archive/master.zip
unzip ./master.zip -d .
cd ./librealsense-master

echo Install udev-rules
sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/ 
sudo cp config/99-realsense-d4xx-mipi-dfu.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger 
mkdir build && cd build
cmake ../ -DFORCE_RSUSB_BACKEND=true -DCMAKE_BUILD_TYPE=release
make -j2
sudo make install
echo -e "\e[92m\n\e[1mLibrealsense script completed.\n\e[0m"

## install ros foxy
sudo apt install software-properties-common
sudo add-apt-repository universe
sudo apt update && sudo apt install curl -y
sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null
sudo apt update
sudo apt upgrade
sudo apt install ros-humble-desktop python3-argcomplete ros-dev-tools
echo 'source /opt/ros/humble/setup.bash' >> ~/.bashrc 

cd ~/software
## install yaml cpp
git clone https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp && mkdir build && cd build
cmake .. -DYAML_BUILD_SHARED_LIBS=ON
make -j$(nproc) && sudo make install
cd ~/software

## install pangolin
git clone --recursive https://github.com/stevenlovegrove/Pangolin.git
cd Pangolin
./scripts/install_prerequisites.sh recommended
cmake -B build
cmake --build build
cd build && sudo make install
cd ~/software

## install zipper
git clone --recursive https://github.com/sebastiandev/zipper.git
cd zipper && mkdir build && cd build
cmake ../
make -j$(nproc) && sudo make install

sudo ldconfig

cd ~projects/IveroSLAM
chmod u+x build.sh
./build.sh

# cd ~
# mkdir -p ros2_ws/src
# git clone https://github.com/Luxolis/IveroSLAM.git ~/ros2_ws/src
# git clone https://github.com/Luxolis/IveroSLAM_interfaces.git ~/ros2_ws/src
# cd ros2_ws
# colcon build
