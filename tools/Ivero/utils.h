#pragma once

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include <Eigen/Core>
#include <boost/filesystem.hpp>
#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>
#include <sophus/se3.hpp>
#include <yaml-cpp/yaml.h>

#include <opencv2/highgui/highgui_c.h>
#include <opencv2/opencv.hpp>

rs2_option get_sensor_option(const rs2::sensor& sensor) {
  std::cout << "Sensor supports the following options:\n" << std::endl;
  for (int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++) {
    rs2_option option_type = static_cast<rs2_option>(i);
    std::cout << "  " << i << ": " << option_type;
    if (sensor.supports(option_type)) {
      std::cout << std::endl;
      const char* description = sensor.get_option_description(option_type);
      std::cout << "       Description   : " << description << std::endl;
      float current_value = sensor.get_option(option_type);
      std::cout << "       Current Value : " << current_value << std::endl;
    } else {
      std::cout << " is not supported" << std::endl;
    }
  }

  uint32_t selected_sensor_option = 0;
  return static_cast<rs2_option>(selected_sensor_option);
}

std::string get_date_string() {
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");
  return oss.str();
}

std::string get_vocab_path() {
  boost::filesystem::path cwd = boost::filesystem::current_path();
  std::string vocab_file = std::string(getenv("HOME")) + "/projects/IveroSLAM/Vocabulary/ORBvoc.txt";
  return vocab_file;
}

std::string get_config_path(const std::string& serial_number) {
  std::string config_file =
      std::string(getenv("HOME")) + "/projects/IveroSLAM/tools/Ivero/config/" + serial_number + ".yaml";
  return config_file;
}

bool config_exists(const std::string& serial_number) {
  std::string config_file = get_config_path(serial_number);
  return boost::filesystem::exists(config_file);
}

Eigen::Matrix4f get_extrinsics(const rs2::stream_profile& to_stream, const rs2::stream_profile& from_stream) {
  float* Rbc = from_stream.get_extrinsics_to(to_stream).rotation;
  float* tbc = from_stream.get_extrinsics_to(to_stream).translation;
  Eigen::Matrix<float, 3, 3, Eigen::RowMajor> R_to_from(Rbc);
  Eigen::Vector3f t_to_from(tbc);
  Eigen::Matrix4f T_to_from = Eigen::Matrix4f::Identity();
  T_to_from.block<3, 3>(0, 0) = R_to_from;
  T_to_from.block<3, 1>(0, 3) = t_to_from;
  return T_to_from;
}

bool create_config_file(const rs2::stream_profile& cam_left, const rs2::stream_profile& cam_right,
                        const std::string& config_file) {
  rs2_intrinsics intrinsics_left = cam_left.as<rs2::video_stream_profile>().get_intrinsics();
  rs2_intrinsics intrinsics_right = cam_left.as<rs2::video_stream_profile>().get_intrinsics();

  YAML::Node config;

  YAML::Emitter out;
  out << YAML::BeginMap;

  out << YAML::Key << "File.version" << YAML::Value << YAML::DoubleQuoted << "1.0";
  out << YAML::Key << "Camera.type" << YAML::Value << YAML::DoubleQuoted << "Rectified";

  out << YAML::Key << "Camera1.fx" << YAML::Value << intrinsics_left.fx;
  out << YAML::Key << "Camera1.fy" << YAML::Value << intrinsics_left.fy;
  out << YAML::Key << "Camera1.cx" << YAML::Value << intrinsics_left.ppx;
  out << YAML::Key << "Camera1.cy" << YAML::Value << intrinsics_left.ppy;

  out << YAML::Key << "Camera2.fx" << YAML::Value << intrinsics_right.fx;
  out << YAML::Key << "Camera2.fy" << YAML::Value << intrinsics_right.fy;
  out << YAML::Key << "Camera2.cx" << YAML::Value << intrinsics_right.ppx;
  out << YAML::Key << "Camera2.cy" << YAML::Value << intrinsics_right.ppy;

  out << YAML::Key << "Camera.fps" << YAML::Value << 15;
  out << YAML::Key << "Camera.RGB" << YAML::Value << 1;

  out << YAML::Key << "Stereo.b" << YAML::Value << cam_right.get_extrinsics_to(cam_left).translation[0];
  out << YAML::Key << "Stereo.ThDepth" << YAML::Value << std::to_string(40.0);

  out << YAML::Key << "Camera.width" << YAML::Value << intrinsics_left.width;
  out << YAML::Key << "Camera.height" << YAML::Value << intrinsics_left.height;

  out << YAML::Key << "ORBextractor.nFeatures" << YAML::Value << 1250;
  out << YAML::Key << "ORBextractor.scaleFactor" << YAML::Value << std::to_string(1.2);
  out << YAML::Key << "ORBextractor.nLevels" << YAML::Value << 8;
  out << YAML::Key << "ORBextractor.iniThFAST" << YAML::Value << 20;
  out << YAML::Key << "ORBextractor.minThFAST" << YAML::Value << 7;

  out << YAML::Key << "Viewer.KeyFrameSize" << YAML::Value << std::to_string(0.05);
  out << YAML::Key << "Viewer.KeyFrameLineWidth" << YAML::Value << std::to_string(1.0);
  out << YAML::Key << "Viewer.GraphLineWidth" << YAML::Value << std::to_string(0.9);
  out << YAML::Key << "Viewer.PointSize" << YAML::Value << std::to_string(2.0);
  out << YAML::Key << "Viewer.CameraSize" << YAML::Value << std::to_string(0.08);
  out << YAML::Key << "Viewer.CameraLineWidth" << YAML::Value << std::to_string(3.0);
  out << YAML::Key << "Viewer.ViewpointX" << YAML::Value << std::to_string(0.0);
  out << YAML::Key << "Viewer.ViewpointY" << YAML::Value << std::to_string(-0.7);
  out << YAML::Key << "Viewer.ViewpointZ" << YAML::Value << std::to_string(-3.5);
  out << YAML::Key << "Viewer.ViewpointF" << YAML::Value << std::to_string(500.0);
  out << YAML::Key << "RGBD.DepthMapFactor" << YAML::Value << std::to_string(1000.0);
  out << YAML::EndMap;

  std::ofstream fout(config_file);
  fout << "%YAML 1.0\n---\n";
  fout << out.c_str();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void set_sensor_options(const std::vector<rs2::sensor>& sensors, std::string& error) {
  int index = 0;
  for (rs2::sensor sensor : sensors) {
    if (sensor.supports(RS2_CAMERA_INFO_NAME)) {
      ++index;
      if (index == 1) {
        try {
          sensor.set_option(RS2_OPTION_VISUAL_PRESET, rs2_rs400_visual_preset::RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY);
        } catch (const rs2::error& e) { error = "Failed to set RS2_OPTION_VISUAL_PRESET.\n" + std::string(e.what()); }

        try {
          sensor.set_option(RS2_OPTION_EXPOSURE, 15000);
        } catch (const rs2::error& e) {
          error = "Failed to set RS2_OPTION_EXPOSURE for IR camera.\n" + std::string(e.what());
        }

        // try {
        //   sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1);
        // } catch (const rs2::error& e) { error = "Failed to set RS2_OPTION_ENABLE_AUTO_EXPOSURE for IR camera."; }

        // try {
        //   sensor.set_option(RS2_OPTION_LASER_POWER, 360);
        // } catch (const rs2::error& e) { error = "Failed to set RS2_OPTION_LASER_POWER for IR camera."; }

        try {
          sensor.set_option(RS2_OPTION_EMITTER_ALWAYS_ON, 0.0);
        } catch (const rs2::error& e) {
          error = "Failed to set RS2_OPTION_EMITTER_ALWAYS_ON.\n" + std::string(e.what());
        }

        try {
          sensor.set_option(RS2_OPTION_EMITTER_ON_OFF, 1.0);
        } catch (const rs2::error& e) { error = "Failed to set RS2_OPTION_EMITTER_ON_OFF.\n" + std::string(e.what()); }
      }
      if (index == 2) {
        try {
          sensor.set_option(RS2_OPTION_EXPOSURE, 100.f);
        } catch (const rs2::error& e) {
          error = "Failed to set RS2_OPTION_EXPOSURE for color camera.\n" + std::string(e.what());
        }
      }
    }
  }
}

bool copy_file(const std::string& _src, const std::string& _dst) {
  std::ifstream src(_src.c_str(), std::ios::binary);
  std::ofstream dst(_dst.c_str(), std::ios::binary);
  dst << src.rdbuf();
  return src && dst;
}

double nsec_to_sec(const uint64_t nsec) {
  double sec = (double)nsec * 1e-9;
  return sec;
}

std::string setup_output_folders(const std::string& date, const std::string& config_file) {
  auto root_path = std::string(getenv("HOME")) + "/ivero_results/";
  auto output_path = root_path + date;

  boost::filesystem::path root_folder = root_path;
  boost::filesystem::path output_folder = output_path;
  boost::filesystem::path depth_folder = output_path + "/depth/";
  boost::filesystem::path rgb_folder = output_path + "/rgb/";

  boost::filesystem::path left_ir_folder = output_path + "/left_ir/";
  boost::filesystem::path right_ir_folder = output_path + "/right_ir/";

  if (!boost::filesystem::is_directory(root_folder)) { boost::filesystem::create_directory(root_folder); }
  if (!boost::filesystem::is_directory(output_folder)) { boost::filesystem::create_directory(output_folder); }
  if (!boost::filesystem::is_directory(depth_folder)) { boost::filesystem::create_directory(depth_folder); }
  if (!boost::filesystem::is_directory(rgb_folder)) { boost::filesystem::create_directory(rgb_folder); }
  if (!boost::filesystem::is_directory(left_ir_folder)) { boost::filesystem::create_directory(left_ir_folder); }
  if (!boost::filesystem::is_directory(right_ir_folder)) { boost::filesystem::create_directory(right_ir_folder); }
  std::string config_file_copy = output_path + "/config.yaml";
  copy_file(config_file, config_file_copy);
  return output_path;
}

void output_timestamps_to_file(const std::set<double>& timestamps, const std::string& filename) {
  std::ofstream f;
  f.open(filename.c_str());
  f << std::fixed;
  for (const auto& t : timestamps) {
    uint64_t nsec = t * 1e9;
    f << nsec << std::endl;
  }
  f.close();
}

void output_transform_to_file(const std::string& filename, const Eigen::Matrix4f& T) {
  std::ofstream f;
  f.open(filename.c_str());
  f << std::fixed;
  f << T;
  f.close();
}

Eigen::Matrix4f read_transform_from_file(const std::string& filename) {
  std::ifstream infile(filename.c_str());
  float a, b, c, d;
  int i = 0;
  Eigen::Matrix4f T = Eigen::Matrix4f::Identity();
  while (infile >> a >> b >> c >> d) {
    T(i, 0) = a;
    T(i, 1) = b;
    T(i, 2) = c;
    T(i, 3) = d;
    i++;
  }
  return T;
}
cv::Mat clahe(const cv::Mat& input) {
  cv::Mat lab_image;
  cv::cvtColor(input, lab_image, cv::COLOR_BGR2Lab);
  std::vector<cv::Mat> lab_planes(6);
  cv::split(lab_image, lab_planes);
  cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
  clahe->setClipLimit(3);
  cv::Mat dst;
  clahe->apply(lab_planes[0], dst);
  dst.copyTo(lab_planes[0]);
  cv::merge(lab_planes, lab_image);
  cv::Mat new_image;
  cv::cvtColor(lab_image, new_image, cv::COLOR_Lab2BGR);
  return new_image;
}