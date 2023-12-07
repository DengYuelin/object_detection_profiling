# Project: Profiling Object Detection Algorithms

In this project we will try to automate [Super Mario Brothers](https://en.wikipedia.org/wiki/Super_Mario_Bros.) game play by detecting blocks in the environment.

<p align="center">
  <img src="examples/example.png" />
</p>

Your task is to test two types of objection algorithms, namely [YOLOv5](https://github.com/ultralytics/yolov5) and [template matching](https://www.sciencedirect.com/topics/engineering/template-matching), and preform [WECT](https://en.wikipedia.org/wiki/Worst-case_execution_time) analysis on them. Most of the code will be provided to you, follow the steps in this document and complete all **TASKS**.

## Preparations

### Install RTOS on Raspberry Pi

Follow the instructions [here](/setup_instructions.md) to setup your Pi.

### Editing files in Pi OS Lite

For those of you who are not familiar with text-based operating system, here are some quick tutorials to edit files:

1. [SSH](https://www.ssh.com/academy/ssh/protocol)
   [A beginner's tutorial](https://itsfoss.com/ssh-into-raspberry/)
2. [VIM](https://www.vim.org/) allows you to edit files in command line.
   See [how to get started with vim](https://opensource.com/article/19/3/getting-started-vim).
3. [SSH + VSCode](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-ssh)
   Use this extension with VSCode to browse and edit files.

### Verify the PREEMPT_RT kernel on your Pi

> **TASKS:**
> Follow the instructions in this section and answer the following questions:
>
> 1. What is the difference between a PREEMPT kernel and a PREEMPT_RT kernel? What scheduling algorithms are available? Refer to [this](https://wiki.linuxfoundation.org/realtime/start) page for more information.
> 2. How does the latency and execution time jitter of the same program compare, between a real-time kernel and a non-real-time one?
> 3. How does the latency and execution time jitter change with priority and niceness?

1.  Verify that your custom-built kernel is loaded properly:

    ```bash
    uname -a
    ```

    You should see both your customized string and the `SMP PREEMPT_RT` string. For example,

    ```
    Linux raspberrypi 6.1.63-rt16-v8-RT+ #1 SMP PREEMPT_RT Tue Nov 28 21:23:27 GMT 2023 aarch64 GNU/Linux
    ```

    If you see:

    ```
    Linux raspberrypi 6.1.0-rpi4-rpi-v8 #1 SMP PREEMPT Debian 1:6.1.54-1+rpt2 (2023-10-05) aarch64 GNU/Linux
    ```

    it means the real-time kernel wasn't successfully patched.

2.  Assessment and comparison of latency and jitter under different real-time scheduling parameters. You need to install and run real-time testing utilities to test system latency:

    ```sh
    sudo apt install rt-tests
    ```

    Refer to [the official document](https://wiki.linuxfoundation.org/realtime/documentation/howto/tools/rt-tests) on what kind of tests you can perform and try them with different parameters.
    **HINT:** One option is to use [cyclictest](https://wiki.linuxfoundation.org/realtime/documentation/howto/tools/cyclictest/start):

    ```sh
    sudo cyclictest --help
    ```

    You can also change the priority of a process being run using:

    ```sh
    sudo chrt -r $YOUR_PRIORITY $YOUR_COMMAND
    ```

    and

    ```sh
    sudo nice -n $YOUR_NICENESS $YOUR_COMMAND
    ```

    The two commands can be cascaded. Refer to [the manual page of `chrt`](https://man7.org/linux/man-pages/man1/chrt.1.html), [and `nice`](https://man7.org/linux/man-pages/man1/nice.1.html) to see how that works.

    -   More on niceness vs. priority can refer to [this forum post](https://askubuntu.com/questions/656771/process-niceness-vs-priority).

    **NOTE:** When running the task over a `ssh` connection, a good practice is to use a terminal emulation tool (such as `screen`) to put your tasks to the local `stdout` buffer of the RTOS. This can prevent halts caused by output buffer filling up with an unstable connection [[Reference]](https://unix.stackexchange.com/questions/282973/do-programs-run-from-an-ssh-session-depend-on-the-connection).

3.  Compare execution time characteristics between a real-time kernel and a non-real-time kernel. You can specify which kernel to load on boot by editing `/boot/firmware/config.txt`. If you have followed the kernel patching procedures, the stock (non-real-time) kernel should be `kernel8.img`, and the real-time kernel is `rt-kernel8.img`. Repeat Step 2 on the non-real-time kernel and compare your results.

## Running a computer vision task on Raspberry Pi (Python)

If you prefer to use C++, refer to [this](cpp_instructions.md) document.

Install OpenCV library:

```bash
sudo apt-get install python3-opencv
# other used packages
sudo apt install python3-matplotlib
# TODO: add other necessary packages
```

**NOTE:** Python version can also be installed with `pip`.

With OpenCV installed, we can now preform all kinds of computer vision tasks. Clone this repo to your Pi.

```sh
mkdir ~/Project && cd ~/Project
git clone https://github.com/DengYuelin/object_detection_profiling.git
cd object_detection_profiling
```

### Template matching

#### Understanding the basics

In this section, we will learn the basics of template matching, and explore its time consumption when detecting multiple objects.

The base code is provided to you, try:

```sh
python3 scripts/template_matching_example.py examples/Source.jpg examples/templates/empty_block.png TM_CCOEFF_NORMED 0.9
```

You should see an output like:

```sh
2 objects detected using method TM_CCOEFF_NORMED with a threshold of 0.9
Result saved to runs/tm_result.png
```

If you open the figure (recommend coping it to your local system using [ssh](https://stackoverflow.com/questions/30553428/copying-files-from-server-to-local-computer-using-ssh)), you should see something like

<p align="center">
  <img src="examples/tm_result.png" />
</p>

where detected objects are highlighted with red bounding boxes.

> **TASKS:**
>
> 1. Read through the code, explore using different template matching [methods](https://docs.opencv.org/3.4/df/dfb/group__imgproc__object.html#ga3a7850640f1fe1f58fe91a2d7583695d) and thresholds, also try detecting different [objects](examples/templates). Identify a [method + threshold] combination for each "block". (brick, empty, hard, and usdestructible block)
> 2. Based on your observations thus far, forecast how the execution time is affected by the number of objects depicted in the source figure. Additionally, predict on how the variety of object types to be identified might influence the execution time.

#### WCET analysis of template matching

In this section, we will write code to preform template matching on [this](data/mario.mp4) video, and record the execution time of each frame.

The code framework if provide to you, try:

```sh
python3 scripts/template_matching.py
```

This script iterate through each frame in the source video, and detect empty blocks. Your job is to modify the given script, and complete the following tasks in order.

> **TASKS:**
>
> 1. Implement code to measure the execution time of each frame and store as a file. Plot the execution time in both statistically (histogram), temporally (frame_num v.s. time), and relative to object count (time v.s. objects detected).
> 2. Implement detection of all four types of blocks. Record and plot the execution time again, how did it change?
> 3. Report what are the impact factors to the WCET of template matching of multiple objects.

**NOTE:** Tweak the real-time scheduling parameters as discussed earlier, using `chrt` and `nice` to reduce system latency and jitter:

```sh
# Execute as a **normal** program with niceness settings
sudo nice -n $N python scripts/template_matching.py # N is niceness ranging from 19 to -20 where -20 is the highest priority
# Execute as a real time program with priority settings
sudo chrt -r $N python scripts/template_matching.py # N is priority ranging from 1 to 99 where 99 is the highest priority
```

### Deploy a pre-trained YOLOv5 object recognition model

In this section we move on to object detection using DNN method. For your convenience, we pre-trained YOLOv5 using [this](https://universe.roboflow.com/baptiste-hustaix-znm0u/mario-ia-b8iuw/dataset/1) open source dataset. You can refer to [this](https://docs.ultralytics.com/yolov5/tutorials/train_custom_data) website to learn more.

Refer to `scripts/yolo.py` for the executable source code. The code can be run with:

```sh
python3 ./scripts/yolo.py
```

**NOTE:** The implementation uses the same onnx model as the C++ example for main inference, but adopts the official post-processing implementation for the raw network output. The post-processing implementation is included in `scripts/utils/general.py`.

#### Understanding the basics

Research and answer the following question to make sure you understand how YOLO inference work.

> **TASKS:**
> Answer the following questions:
>
> 1. How many types of features can [this](network/yolov5s_mario.onnx) custom pre-trained network detect?
> 2. What is the difference between a ["small"](network/yolov5s_mario.onnx) model and a ["nano"](network/yolov5n_mario.onnx) model?
> 3. Read through the given code, what is the input and output of "Pre-processing"? What about "Main inference" and "Post-processing"?
> 4. How do you interpret the output `detection_list`? What can you do if you are only interested in the number and location of one type (class) of object?

#### WCET analysis of YOLO inference

Now that we understand how YOLO works, let's evaluate the WCET. Complete the following tasks in order and report your findings.

> **TASKS:**
>
> 1. Implement code to measure the **total** execution time of each frame and store as a file. Plot the execution time in both statistically (histogram), temporally (frame_num v.s. time), and relative to object count (time v.s. total objects detected).
> 2. Try to extract the numbers of each block detected, does the variation in interested types (classes) numbers affect the overall processing time? Compare this to the methodology of template matching.
> 3. Insert more timers inside the source code to identify the source of jitter during each step YOLO inference.
> 4. Try running the same task using a smaller model (network/yolov5n_mario.onnx), observe and report the differences.
> 5. Modify the code and run another video with time-varying scene complexity through the detection pipeline, remember to change the pre-trained network to standard YOLO `network/yolov5s.onnx`. An example is provided [here](data/party.mp4). Examine how the processing time of each frame changes over time. What is the overall distribution of these processing times?
> 6. Based on your experiment thus far, summarize the difference in WCET between template matching and YOLO, what are the advantages and disadvantages of each method?

## Other RTOS analysis on Raspberry Pi

Now let's explore the impact of running a real-time task on other processes.

> **TASKS:**
>
> 1. Try running the detection code with the highest priority and least niceness. Put your process into the background with an `&` (e.g. `python3 ./yolo.py &`). Now you free up the foreground terminal to execute other tasks. Then `cyclictest` in the foreground as you have done earlier, but _use a default priority (20) and niceness (0)_. Does your `cyclictest` execute as expected? Change the priority and niceness settings of `cyclictest` and report what you find.
> 2. Report any _physical anomalies_ you observe on the Raspberry Pi when running the detection code with the highest priority and least niceness. Try to interpret why based on [the electrical schematics](https://datasheets.raspberrypi.com/rpi4/raspberry-pi-4-reduced-schematics.pdf) and forum discussions (Hint: it is related to how the OS services the GPIO pins).
