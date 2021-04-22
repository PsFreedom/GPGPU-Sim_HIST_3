https://developer.nvidia.com/cuda-toolkit-archive

sudo apt-get -y install build-essential xutils-dev bison zlib1g-dev flex libglu1-mesa-dev
sudo apt-get -y install doxygen graphviz
sudo apt-get -y install python-pmw python-ply python-numpy libpng12-dev python-matplotlib
sudo apt-get -y install libxi-dev libxmu-dev libglut3-dev
sudo apt-get -y upgrade


mkdir -p ~/CUDA && cd ~/CUDA
wget https://developer.download.nvidia.com/compute/cuda/11.3.0/local_installers/cuda_11.3.0_465.19.01_linux.run
sudo sh cuda_11.3.0_465.19.01_linux.run


cd
git clone https://github.com/gpgpu-sim/gpgpu-sim_distribution.git
git clone https://github.com/gpgpu-sim/gpgpu-sim_simulations.git

export CUDA_INSTALL_PATH=/usr/local/cuda-11.3/
export PATH=$CUDA_INSTALL_PATH/bin:$PATH

vim ~/gpgpu-sim_simulations/benchmarks/src/cuda/common/common.mk
vim ~/gpgpu-sim_simulations/benchmarks/src/cuda/rodinia/3.1/common/make.config
cd ~/gpgpu-sim_simulations/benchmarks/src/cuda/rodinia/3.1/cuda/backprop
make clean && make 


cd ~/gpgpu-sim_distribution/
source setup_environment release
make clean && make -j 8

rm -rf ~/test_run && mkdir -p ~/test_run
cp -r ~/gpgpu-sim_distribution/configs/tested-cfgs/SM7_TITANV/* ~/test_run
cd ~/test_run/. 
source ~/gpgpu-sim_distribution/setup_environment release
~/gpgpu-sim_simulations/benchmarks/src/cuda/rodinia/3.1/cuda/backprop/backprop 1024 | grep HIST


