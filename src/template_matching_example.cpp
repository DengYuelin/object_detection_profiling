#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <filesystem>
// There are 6 comparison methods to choose from:
// TM_CCOEFF, TM_CCOEFF_NORMED, TM_CCORR, TM_CCORR_NORMED, TM_SQDIFF,
// TM_SQDIFF_NORMED You can see the differences at a glance here:
// https://docs.opencv.org/master/d4/dc6/tutorial_py_template_matching.html
std::map<std::string, int> kMethods = {
    {"TM_CCOEFF", cv::TM_CCOEFF}, {"TM_CCOEFF_NORMED", cv::TM_CCOEFF_NORMED},
    {"TM_CCORR", cv::TM_CCORR},   {"TM_CCORR_NORMED", cv::TM_CCORR_NORMED},
    {"TM_SQDIFF", cv::TM_SQDIFF}, {"TM_SQDIFF_NORMED", cv::TM_SQDIFF_NORMED}};

int main(int argc, char** argv) {
  // Process input arguments
  if (argc < 3) {
    std::cout << "Not enough parameters" << std::endl;
    std::cout << "Usage:\n"
              << argv[0] << " <image_name> <template_name> [<mask_name>]"
              << std::endl;
    return -1;
  }

  std::string img_src(argv[1]);
  std::string tmp_src(argv[2]);
  std::string tm_method("TM_CCOEFF_NORMED");
  float threshold = 0.8;
  if (argc > 3) tm_method = argv[3];
  if (argc > 4) threshold = std::stof(argv[4]);

  cv::Mat img_rgb;
  cv::Mat img_gray;
  cv::Mat templ;  // in grayscale
  cv::Mat result;
  // Read source image
  img_rgb = cv::imread(argv[1], cv::IMREAD_COLOR);
  // Read template image
  templ = cv::imread(argv[2], cv::IMREAD_GRAYSCALE);
  // Check for error
  if (img_rgb.empty() || templ.empty()) {
    std::cout << "Can't read one of the images" << std::endl;
    return EXIT_FAILURE;
  }
  // Process source
  cv::cvtColor(img_rgb, img_gray, cv::COLOR_BGR2GRAY);
  // Apply template matching
  matchTemplate(img_gray, templ, result, kMethods[tm_method]);

  // Apply threshold to detect multiple objects
  if (kMethods[tm_method] == cv::TM_SQDIFF ||
      kMethods[tm_method] == cv::TM_SQDIFF_NORMED) {
    cv::threshold(result, result, threshold, 1., cv::THRESH_BINARY_INV);
  } else {
    cv::threshold(result, result, threshold, 1., cv::THRESH_BINARY);
  }
  std::vector<cv::Point>
      loc;  // location of all points respective to the threshold
  cv::findNonZero(result, loc);
  // Draw bounding box
  for (int i = 0; i < loc.size(); ++i) {
    cv::rectangle(img_rgb, loc[i],
                  cv::Point(loc[i].x + templ.cols, loc[i].y + templ.rows),
                  cv::Scalar(0, 0, 255, 255), 2);
  }

  std::cout << loc.size() << " objects detected using method " << tm_method
            << " with a threshold of " << threshold << std::endl;
  try {
    std::filesystem::create_directory("runs");
  } catch (std::exception& e) {
    std::cout << "Can't create directory" << std::endl;
    return EXIT_FAILURE;
  }
  cv::imwrite("runs/tm_result.png", img_rgb);
  std::cout << "Result saved to runs/tm_result.png" << std::endl;

  // cv::imshow("test", img_rgb);
  // cv::waitKey(0);
  return EXIT_SUCCESS;
}
