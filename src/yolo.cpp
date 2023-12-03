#include <fstream>
#include <iostream>
#include <vector>
#include <cstddef>
#include <opencv2/opencv.hpp>

std::vector<std::string> load_class_list() {
  std::vector<std::string> class_list;
  std::ifstream ifs("network/classes_mario.txt");
  std::string line;
  while (getline(ifs, line)) {
    class_list.push_back(line);
  }
  return class_list;
}

void load_net(cv::dnn::Net &net, bool is_cuda) {
  auto result = cv::dnn::readNet("network/yolov5s_mario.onnx");
  if (is_cuda) {
    std::cout << "Attempty to use CUDA\n";
    result.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
    result.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA_FP16);
  } else {
    std::cout << "Running on CPU\n";
    result.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    result.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
  }
  net = result;
}

const std::vector<cv::Scalar> colors = {
    cv::Scalar(255, 255, 0), cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 255),
    cv::Scalar(255, 0, 0)};

const int INPUT_WIDTH = 640;
const int INPUT_HEIGHT = 640;
const float SCORE_THRESHOLD = 0.; // to match with the python implementation
const float NMS_THRESHOLD = 0.4;
const float CONFIDENCE_THRESHOLD = 0.4;

struct Detection {
  int class_id;
  float confidence;
  cv::Rect box;
};

/**
 * @brief fit the input image to the smallest square, starting from the upper left
 */
cv::Mat make_square(const cv::Mat &source, int& out_size) {
  int col = source.cols;
  int row = source.rows;
  out_size = std::max(col, row);
  cv::Mat result = cv::Mat::zeros(out_size, out_size, CV_8UC3);
  source.copyTo(result(cv::Rect(0, 0, col, row)));
  return result;
}

void detect(const cv::Mat &image, cv::dnn::Net &net, std::vector<Detection> &output,
            std::vector<int> &date_record,
            const std::vector<std::string> &className) {
  static cv::Mat blob;

  /*--------------------Pre-process--------------------*/
  int pre_blob_image_size;
  // Preprocessing
  cv::dnn::blobFromImage(make_square(image, pre_blob_image_size), blob, 1. / 255.,
                         cv::Size(INPUT_WIDTH, INPUT_HEIGHT), cv::Scalar(),
                         true, false);
  net.setInput(blob);
  static std::vector<cv::Mat> outputs;

  /*--------------------Main Inference-----------------*/
  net.forward(outputs, net.getUnconnectedOutLayersNames());

  /*--------------------Post-process-------------------*/
  // Resizing factor
  float resizing_factor = pre_blob_image_size / static_cast<float>(INPUT_WIDTH);

  // max possible raw detections
  const int rows = 25200;
  // size of each raw detection
  const int dimensions = className.size() + 5;

  // Initialize vectors to hold respective outputs while unwrapping detections.
  std::vector<int> class_ids;
  std::vector<float> confidences;
  std::vector<cv::Rect> boxes;

  // start of the output data
  float *data = (float *)outputs[0].data;

  for (int i = 0; i < rows; ++i) {
    float confidence = data[4];
    // Discard bad detections and continue.
    if (confidence >= CONFIDENCE_THRESHOLD) {
      float *classes_scores = data + 5;
      cv::Mat scores(1, className.size(), CV_32FC1, classes_scores);
      // Perform minMaxLoc and acquire index of best class score.
      cv::Point class_id;
      double max_class_score;
      minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
      // Continue if the class score is above the threshold.
      if (max_class_score > SCORE_THRESHOLD) {
        // Store class ID and confidence in the pre-defined respective vectors.
        confidences.push_back(confidence);

        class_ids.push_back(class_id.x);

        float x = data[0];
        float y = data[1];
        float w = data[2];
        float h = data[3];
        int left = static_cast<int>((x - 0.5 * w) * resizing_factor);
        int top = static_cast<int>((y - 0.5 * h) * resizing_factor);
        int width = static_cast<int>(w * resizing_factor);
        int height = static_cast<int>(h * resizing_factor);
        // Store good detections in the boxes vector.
        boxes.push_back(cv::Rect(left, top, width, height));
      }
    }

    data += dimensions;
  }

  /*--------------------Non Maximum Suppression--------------------*/
  std::vector<int> nms_result;
  cv::dnn::NMSBoxes(boxes, confidences, SCORE_THRESHOLD, NMS_THRESHOLD,
                    nms_result);
  for (int i = 0; i < nms_result.size(); i++) {
    int idx = nms_result[i];
    Detection result;
    result.class_id = class_ids[idx];
    result.confidence = confidences[idx];
    result.box = boxes[idx];
    output.push_back(result);
  }

  // TODO: measure and store the execution time and object count of each frame.
  int inference_time = 0;
  date_record.push_back(inference_time);
}

/**
 * @brief This function reads the video into the RAM, placing it in a vector of frames.
 */
void loadImages(cv::VideoCapture& videoIn, std::vector<cv::Mat>& frames) {
  cv::Mat frame;
  while (true) {
    videoIn.read(frame);
    if (frame.empty()) break;
    frames.emplace_back(frame);
  }
}

int main(int argc, char **argv) {
  std::vector<std::string> class_list = load_class_list();
  std::ofstream rt_data_file("runs/data.txt",
                             std::ofstream::out | std::ofstream::trunc);
  std::cout << "This network can detect the following objects: " << std::endl;
  for (int i = 0; i < class_list.size(); ++i) {
    std::cout << class_list[i].c_str() << std::endl;
  }

  cv::Mat frame;
  cv::VideoCapture capture("data/mario.mp4");
  if (!capture.isOpened()) {
    std::cerr << "Error opening video file\n";
    return -1;
  }

  cv::dnn::Net net;
  load_net(net, false);

  std::vector<int> data_record;

  std::vector<cv::Mat> frames;
  loadImages(capture, frames);
  size_t total_frames = frames.size();
  std::cout << "Loaded " << total_frames << " frames from the video stream.\n";

  for (const auto& f: frames) {
    std::vector<Detection> output;
    detect(f, net, output, data_record, class_list);

    int empty_block_count = 0;
    for (int i = 0; i < output.size(); ++i) {
      if (output[i].class_id == 2) {
        empty_block_count++;
      }
    }
    // TODO: count and store the numbers of other other blocks

    std::cout << empty_block_count << "\tempty blocks detected" << std::endl;
    rt_data_file << empty_block_count << "\tempty blocks detected using\t"
                 << data_record.at(0) << "\tμs" << std::endl;
  }

  rt_data_file.close();
  std::cout << "Completed processing " << total_frames << " frames.\n";

  return 0;
}
