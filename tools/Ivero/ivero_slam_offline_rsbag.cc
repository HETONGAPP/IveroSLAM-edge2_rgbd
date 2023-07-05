
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include <boost/filesystem.hpp>
#include <boost/progress.hpp>
#include <boost/range/iterator_range.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/opencv.hpp>
#include <yaml-cpp/yaml.h>

#include <System.h>

using namespace boost::filesystem;

int main(int argc, char** argv) {
  if (argc < 2) { std::cout << "Invalid usage! \n ./ivero_slam_offline /path/to/iveroslam/output/folder" << std::endl; }
  // get config file, create if it doesnt exist
  const auto data_folder = std::string(argv[1]);
  const auto config_file = data_folder + "/config.yaml";

  // get all file names in either rgb or left ir folder
  std::string im1_folder;
  std::string im2_folder;
  im1_folder = data_folder + "/color/";
  im2_folder = data_folder + "/depth/";

  path p(im1_folder);
  std::set<int> stamps;

  if (is_directory(p)) {
    for (auto& entry : boost::make_iterator_range(directory_iterator(p), {})) {
      int nsec = std::stoi(entry.path().stem().string());
      stamps.insert(nsec);
      std::cout << std::setw(5) << std::setfill('0') << nsec << std::endl;
    }
  }

  // Create SLAM system. It initializes all system threads and gets ready to process frames.
  ORB_SLAM3::System SLAM(get_vocab_path(), config_file, ORB_SLAM3::System::RGBD, false, 0, get_date_string());

  boost::progress_display progress(stamps.size());
  for (const auto& nsec : stamps) {
    double timestamp = nsec_to_sec(nsec);

    std::stringstream ss;
    ss << std::setw(5) << std::setfill('0') << nsec;
    std::string file_name = ss.str();

    cv::Mat im1 = cv::imread(im1_folder + file_name + ".jpg", cv::IMREAD_COLOR);
    cv::Mat im2 = cv::imread(im2_folder + file_name + ".png", cv::IMREAD_UNCHANGED);
    const auto Tcw = SLAM.TrackRGBD(im1, im2, timestamp);
    ++progress;
  }
  SLAM.Shutdown();
  while (!SLAM.isShutDown()) { std::cout << "waiting to shutdown" << std::endl; }
  SLAM.SaveTrajectory(data_folder + "/trajectory.txt");
  SLAM.SaveKeyFrameTrajectory(data_folder + "/kf_trajectory.txt");
  return 0;
}
