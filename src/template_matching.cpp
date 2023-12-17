#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <filesystem>
#include <fstream>
// There are 6 comparison methods to choose from:
// TM_CCOEFF, TM_CCOEFF_NORMED, TM_CCORR, TM_CCORR_NORMED, TM_SQDIFF,
// TM_SQDIFF_NORMED You can see the differences at a glance here:
// https://docs.opencv.org/master/d4/dc6/tutorial_py_template_matching.html
std::map<std::string, int> kMethods = {
    {"TM_CCOEFF", cv::TM_CCOEFF}, {"TM_CCOEFF_NORMED", cv::TM_CCOEFF_NORMED},
    {"TM_CCORR", cv::TM_CCORR},   {"TM_CCORR_NORMED", cv::TM_CCORR_NORMED},
    {"TM_SQDIFF", cv::TM_SQDIFF}, {"TM_SQDIFF_NORMED", cv::TM_SQDIFF_NORMED}};

std::vector<cv::Point> detect(cv::Mat image, cv::Mat templ, int method,
                              float threshold) {
  cv::Mat result;
  cv::Mat img_gray;
  // Process source
  cv::cvtColor(image, img_gray, cv::COLOR_BGR2GRAY);
  // Apply template matching
  matchTemplate(img_gray, templ, result, method);

  // Apply threshold to detect multiple objects
  if (method == cv::TM_SQDIFF || method == cv::TM_SQDIFF_NORMED) {
    cv::threshold(result, result, threshold, 1., cv::THRESH_BINARY_INV);
  } else {
    cv::threshold(result, result, threshold, 1., cv::THRESH_BINARY);
  }
  std::vector<cv::Point>
      loc;  // location of all points respective to the threshold
  cv::findNonZero(result, loc);
  return loc;
}

/**
 * @brief This function reads the video into the RAM, placing it in a vector of
 * frames.
 */
void loadImages(cv::VideoCapture& videoIn, std::vector<cv::Mat>& frames) {
  while (true) {
    cv::Mat frame;
    videoIn.read(frame);
    if (frame.empty()) break;
    frames.emplace_back(frame);
  }
}

int main(int argc, char** argv) {
  // Load source video
  cv::VideoCapture capture("data/mario.mp4");
  if (!capture.isOpened()) {
    std::cerr << "Error opening video file\n";
    return -1;
  }
  std::vector<cv::Mat> frames;
  loadImages(capture, frames);
  size_t total_frames = frames.size();
  std::cout << "Loaded " << total_frames << " frames from the video stream.\n";
  // Load templates
  cv::Mat empty_block;  // template in grayscale
  empty_block =
      cv::imread("examples/templates/empty_block.png", cv::IMREAD_GRAYSCALE);
  if (empty_block.empty()) {
    std::cout << "Can't read one of the templates" << std::endl;
    return EXIT_FAILURE;
  }
  // Create data recorder
  std::ofstream rt_data_file("runs/tm_data.txt",
                             std::ofstream::out | std::ofstream::trunc);
  std::vector<int> data_record;
  std::vector<cv::Point> empty_block_loc;
  for (size_t n = 0; n < total_frames; n++) {
    empty_block_loc = detect(frames[n], empty_block, cv::TM_CCOEFF_NORMED, 0.9);
    // TODO: detect brick_block, hard_block, and undestructible_block
    // Insert your code here

    // TODO: measure and store the execution time and object count
    unsigned int time_taken_us = 0;
    std::cout << empty_block_loc.size()
              << " \tempty_block identified in frame\t" << n << std::endl;
    rt_data_file << empty_block_loc.size() << "\tidentified objects in frame\t"
                 << n << "\tusing\t" << time_taken_us << "\tμs" << std::endl;
  }

  rt_data_file.close();
  std::cout << "Completed processing " << total_frames << " frames.\n";

  return 0;
}
