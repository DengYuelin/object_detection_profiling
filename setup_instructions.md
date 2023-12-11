# Setup instruction

## Initial installation of Raspberry Pi OS Lite

Follow instructions [here](https://www.raspberrypi.com/documentation/computers/getting-started.html#install-using-imager) to install 64-bit Raspberry Pi OS Lite (based on Debian/Ubuntu) onto your SD card. You may enter the Pi OS to inflate the file system (automatic upon first boot), and perform initialization steps (setting up user/password). It is recommended to setup username/password and enable SSH when flashing the image to make initial access easier.

## Apply PREEMPT_RT Patch to the Linux Kernel

The PREEMPT_RT patch enables preemptive task scheduling under real-time priority, hence reducing execution time jitter for high-priority processes. To apply the patch, we need to recompile the Linux kernel from source, which could be done on either the Pi, or your personal computer. Please follow one of the three options below.

### Cross-compilation with a PC using Docker (Recommended)

-   System requirements
    -   OS: Ubuntu (tested), Windows (needs minor tweaks to the `docker_env_enter.sh` if not working out-of-the-box), MacOS (theoretically it should work out-of-the-box)
    -   RAM: xx GB per thread running the build process.
    -   Storage: 4 GB free disk space.

Docker creates a containerized environemnt, much like a light-weight virtual machine. It can run on various OS and hardware combinations. Using Docker to create a standardized cross-compilation environment isolates the complexity of your native OS, hence significantly eliminating issues related to your personal computer setup. Before we begin, please install Docker Engine following the [instructions specific to your PC OS](https://docs.docker.com/engine/install/). If your PC OS is Linux, also follow the recommended [post-installation procedures](https://docs.docker.com/engine/install/linux-postinstall/).

1. Clone the build environment folder, build the Docker container and enter the environment:

    ```sh
    git clone https://gitlab.com/HaoguangYang/docker-cross-compile-aarch64.git
    cd docker-cross-compile-aarch64
    ./build_env_enter.sh
    ```

    In the build environment, any files you create under the home folder (`/home/ubuntu`) will be mapped to the `docker-cross-compile-aarch64` folder on your host machine.

2. Create the kernel build workspace:

    ```sh
    mkdir rpi-rt-linux-6.1
    cd rpi-rt-linux-6.1
    ```

    Continue to Step 3 in **Cross-compilation with a PC natively** section, all the way to Step 10.

3. [*Alternative to Step 10 below*] If you have a Windows machine and cannot mount an `Ext4` partition, Let's collect everything inside a compressed file:

    ```sh
    cd ../install
    tar czf ../rt-kernel.tgz *
    cd ..
    ```

    Now, copy the `rt-kernel.tgz` into the boot partition (it has FAT32 formatting) of your SD card (`bootfs`), and apply the kernel from your Raspberry Pi:

    (Insert the SD card to the Pi and power up. The following commands are run on the Pi)

    ```sh
    sudo mv /boot/firmware/rt-kernel.tgz /tmp
    cd /tmp
    tar xzf rt-kernel.tgz
    sudo cp -rd ./boot/* /boot/firmware/
    sudo cp -rd ./broadcom/bcm* /boot/firmware/
    sudo cp -rd ./overlays/* /boot/firmware/overlays
    sudo cp -rd ./lib/* /lib
    sudo rm -rf rt-kernel.tgz
    rm -rf boot broadcom overlays lib
    ```

    On the Raspberry Pi, edit `/boot/firmware/config.txt` file (`sudo nano /boot/firmware/config.txt`) and add the following line:

    ```
    kernel=rt-kernel8.img
    ```

    Reboot the Raspberry Pi.

4. Finally, press `Ctrl-D` to exit the build environment on your PC. The docker container destroys automatically upon exit.

### Cross-compilation with a PC natively

-   System requirements
    -   OS: Ubuntu 20.04 or newer
    -   RAM: xx GB per thread running the build process.
    -   Storage: 4 GB free disk space.

1. Install system software.

    ```sh
    sudo apt-get update
    sudo apt-get install git bc bison flex gcc g++ libssl-dev make libc6-dev \
        libncurses5-dev crossbuild-essential-arm64 ca-certificates wget
    ```

2. Pick a place in your hard drive (denoted `$YOUR_FOLDER_LOCATION`) to create a kernel build workspace:

    ```
    cd $YOUR_FOLDER_LOCATION
    mkdir rpi-rt-linux-6.1
    cd rpi-rt-linux-6.1
    ```

3. Download kernel source code into the `linux` folder

    ```sh
    git clone -b stable_20231123 --depth=1 https://github.com/raspberrypi/linux
    ```

    This kernel version is based on Linux 6.1.63.

4. Download `RT_PREEMPT` patch

    ```sh
    wget https://cdn.kernel.org/pub/linux/kernel/projects/rt/6.1/older/patch-6.1.59-rt16.patch.xz
    ```

    **NOTE:** The `RT_PREEMPT` patch is applicable to Linux 6.1.59 kernel. When changing the kernel version by yourself, make sure to get the patch for the closest possible kernel version (in this case, patch version `6.1.59` is applied to kernel version `6.1.63`). The kernel version should be _no smaller than_ the patch version.

5. Setup build workspace structure

    ```sh
    mkdir build
    mkdir -p install/boot
    ```

    Your workspace should now look like this:

    ```
    rpi-rt-linux-6.1/                   <-- You are here
      |__ build/
      |__ install/
      |     |__ boot/
      |__ linux/                        <-- Linux kernel source
      |     |__ arch/
      |     |__ ...
      |__ patch-6.1.59-rt16.patch.xz    <-- the PREEMPT_RT patch
    ```

6. Apply the PREEMPT_RT patch

    ```sh
    cd linux
    xzcat ../patch-6.1.59-rt16.patch.xz | patch -p1 --dry-run    #check the patch fits
    ```

    Inspect the output and fix any mismatches before applying the patch.

    Now actually apply the patch with:

    ```sh
    xzcat ../patch-6.1.59-rt16.patch.xz | patch -p1
    ```

7. All operations from this step on until Step 9 assume you are inside the `linux` folder. Configure the kernel

    ```sh
    cd arch/arm64/configs
    cp bcm2711_defconfig bcm2711_rt_defconfig
    ```

    Inspect the file `bcm2711_rt_defconfig` with a text editor (e.g. `nano bcm2711_rt_defconfig`). Modify lines 1, 10 and insert a line after line 10:

    ```diff
    @@ -1,4 +1,4 @@
    -CONFIG_LOCALVERSION="-v8"
    +CONFIG_LOCALVERSION="-v8-RT"
    # CONFIG_LOCALVERSION_AUTO is not set
    CONFIG_SYSVIPC=y
    CONFIG_POSIX_MQUEUE=y
    @@ -7,7 +7,8 @@ CONFIG_NO_HZ=y
    CONFIG_HIGH_RES_TIMERS=y
    CONFIG_BPF_SYSCALL=y
    CONFIG_BPF_JIT=y
    -CONFIG_PREEMPT=y
    +CONFIG_PREEMPT_RT=y
    +CONFIG_PREEMPT_RT_FULL=y
    CONFIG_BSD_PROCESS_ACCT=y
    CONFIG_BSD_PROCESS_ACCT_V3=y
    CONFIG_TASKSTATS=y
    ```

    In other words, in line 1, append `LOCALVERSION` option with a unique string of your own.

    In line 10,

    ```
    CONFIG_PREEMPT=y
    ```

    should become:

    ```
    CONFIG_PREEMPT_RT=y
    CONFIG_PREEMPT_RT_FULL=y
    ```

    Change directory back to the linux kernel base folder:

    ```sh
    cd ../../../
    ```

    Generate Makefile for this configuration:

    ```sh
    export KERNEL=kernel8
    make O=../build/ ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bcm2711_rt_defconfig
    ```

8. Build the kernel with 8 threads (`-j8`, you can change the number if you want to utilize more CPU cores to accelerate the build.)

    ```sh
    make O=../build/ ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KCFLAGS="-pipe" -j8 Image.gz modules dtbs
    ```

    Grab a cup of tea / coffee / take a nap. Your computer will usually take more than 10 minutes to complete this step.

9. Generate installation files

    ```sh
    export INSTALL_MOD_PATH=$(pwd)/../install
    export INSTALL_DTBS_PATH=${INSTALL_MOD_PATH}
    make O=../build/ ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules_install dtbs_install
    cp ../build/arch/arm64/boot/Image.gz ../install/boot/rt-kernel8.img
    cp arch/arm64/boot/dts/overlays/README ../install/overlays/
    cd ..
    ```

10. We are done building. The `install` folder essentially has everything you need to load the new kernel onto your SD card. If you have a Linux machine, please refer to [the official guide](https://www.raspberrypi.com/documentation/computers/linux_kernel.html#install-directly-onto-the-sd-card) about the differences between `bootfs` (the FAT32 partition) and `rootfs` (the Ext4 partition). Assume you have mounted your two partitions to `/media/$USER/bootfs` and `/media/$USER/rootfs` respectively (where $USER) is your user name, do the following copy commands:

    ```sh
    cd install
    sudo cp -rd ./boot/* /media/$USER/bootfs
    sudo cp -rd ./broadcom/bcm* /media/$USER/bootfs
    sudo cp -rd ./overlays/* /media/$USER/bootfs/overlays
    sudo cp -rd ./lib/* /media/$USER/rootfs/lib
    ```

    Finally, edit `/media/$USER/bootfs/config.txt` file (`sudo nano /media/$USER/bootfs/config.txt`) and add the following line:

    ```
    kernel=rt-kernel8.img
    ```

    Insert the SD card into your Pi and power up.

### Patch and compile locally on Pi

Alternatively, you can apply the PREEMPT_RT patch on the Pi, and compile it on the Pi. It may take up to two hours due to the limited processing power.

1. Get the sources

    ```sh
    sudo apt-get update
    sudo apt-get upgrade
    sudo apt install git bc bison flex libssl-dev make libncurses5-dev  #I think this is all the tools required
    cd ~
    mkdir rpi-rt-linux-6.1
    cd rpi-rt-linux-6.1
    git clone -b stable_20231123 --depth=1 https://github.com/raspberrypi/linux
    wget https://cdn.kernel.org/pub/linux/kernel/projects/rt/6.1/older/patch-6.1.59-rt16.patch.xz
    mkdir build
    ```

    Your workspace should now look like this:

    ```
    ~/rpi-rt-linux-6.1/                 <-- You are here
      |__ build/
      |__ linux/                        <-- Linux kernel source
      |     |__ arch/
      |     |__ ...
      |__ patch-6.1.59-rt16.patch.xz    <-- the PREEMPT_RT patch
    ```

2. Apply the patch

    ```sh
    cd linux
    xzcat ../patch-6.1.59-rt16.patch.xz | patch -p1 --dry-run    #check the patch fits
    ```

    Inspect the output and fix any mismatches before applying the patch.

    Now actually apply the patch with:

    ```sh
    xzcat ../patch-6.1.59-rt16.patch.xz | patch -p1
    ```

3. Build configuration

    ```sh
    export KERNEL=kernel8
    make O=../build/ bcm2711_defconfig
    make O=../build/ menuconfig
    ```

    In the prompt showed up, select `Real Time` option in `General -> Preemption Model`. You need to append a suffix (e.g. `-v8-RT`) to your custom-built kernel name under `General -> Local version - append to kernel release`. Please ensure your customized kernel does not receive the same version string as the upstream kernel.

    **Alternatively**, you can create a separate build config file derived from the default one. Refer to Step 7 in **Cross-compilation with a PC natively** section. For the derived config `bcm2711_rt_defconfig`, your command will be:

    ```sh
    export KERNEL=kernel8
    make O=../build/ bcm2711_rt_defconfig
    ```

4. Build the kernel with 4 threads (all that a Pi has)

    ```sh
    make O=../build/ -j4 Image.gz modules dtbs
    ```

    This step will take approx. 2 hours.

5. Install the new kernel
    ```sh
    sudo make O=../build/ modules_install
    sudo cp -rd ../build/arch/arm64/boot/dts/broadcom/bcm*.dtb /boot/firmware/
    sudo cp -rd ../build/arch/arm64/boot/dts/overlays/*.dtb* /boot/firmware/overlays/
    sudo cp ../build/arch/arm64/boot/Image.gz /boot/firmware/rt-kernel8.img
    ```
    Edit `/boot/firmware/config.txt` and add the following line:
    ```
    sudo vi /boot/firmware/config.txt
    ```
    then add
    ```
    kernel=rt-kernel8.img
    ```
    Finally, reboot the Pi:
    ```
    sudo reboot
    ```
