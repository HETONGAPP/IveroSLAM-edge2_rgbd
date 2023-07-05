
#include <assert.h>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

#include <boost/filesystem.hpp>
#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>
#include <yaml-cpp/yaml.h>

bool create_config_file(const rs2::stream_profile& cam_color, const std::string& config_file) {
  rs2_intrinsics intrinsics = cam_color.as<rs2::video_stream_profile>().get_intrinsics();

  YAML::Node config;

  YAML::Emitter out;
  out << YAML::BeginMap;

  out << YAML::Key << "File.version" << YAML::Value << YAML::DoubleQuoted << "1.0";
  out << YAML::Key << "Camera.type" << YAML::Value << YAML::DoubleQuoted << "Rectified";

  out << YAML::Key << "Camera1.fx" << YAML::Value << intrinsics.fx;
  out << YAML::Key << "Camera1.fy" << YAML::Value << intrinsics.fy;
  out << YAML::Key << "Camera1.cx" << YAML::Value << intrinsics.ppx;
  out << YAML::Key << "Camera1.cy" << YAML::Value << intrinsics.ppy;

  out << YAML::Key << "Camera.fps" << YAML::Value << 15;
  out << YAML::Key << "Camera.RGB" << YAML::Value << 1;
  out << YAML::Key << "RGBD.DepthMapFactor" << YAML::Value << std::to_string(1000.0);

  out << YAML::Key << "Stereo.b" << YAML::Value << std::to_string(0.1);
  out << YAML::Key << "Stereo.ThDepth" << YAML::Value << std::to_string(40.0);

  out << YAML::Key << "Camera.width" << YAML::Value << intrinsics.width;
  out << YAML::Key << "Camera.height" << YAML::Value << intrinsics.height;

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
  out << YAML::EndMap;

  std::ofstream fout(config_file);
  fout << "%YAML 1.0\n---\n";
  fout << out.c_str();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  return true;
}

std::string get_default_config_path() {
  std::string config_file = std::string(getenv("HOME")) + "/projects/IveroSLAM/tools/Ivero/config/config.yaml";
  return config_file;
}

int main(int argc, char** argv) {
  // look for realsense device
  rs2::device selected_device;
  try {
    rs2::context ctx;
    rs2::device_list devices = ctx.query_devices();
    while (devices.size() == 0) {
      std::cout << "No device connected, please connect a RealSense device." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      devices = ctx.query_devices();
    }
    selected_device = devices[0];
  } catch (const rs2::error& e) {
    std::cout << e.what() << "\n\nIs another process using the realsense device?" << std::endl;
    return 1;
  }

  // assert camera type
  const auto camera_name = std::string(selected_device.get_info(RS2_CAMERA_INFO_NAME));
  assert(camera_name == "Intel RealSense D455" && "Device must be an Intel RealSense D455.");
  std::cout << "Connected to realsense device." << std::endl;

  rs2::config cfg;
  cfg.enable_stream(RS2_STREAM_COLOR, 640, 360, RS2_FORMAT_RGB8, 30);
  rs2::pipeline pipe;
  rs2::pipeline_profile pipe_profile = pipe.start(cfg, [&](auto frame) { return; });
  std::cout << "Retrieving device streams." << std::endl;

  // Get respective streams for automatic config file creation
  rs2::stream_profile cam_color = pipe_profile.get_stream(RS2_STREAM_COLOR);

  // get config file, create if it doesnt exist
  const auto serial_number = std::string(selected_device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
  const auto config_file = get_default_config_path();
  create_config_file(cam_color, config_file);
  std::cout << "Saving config file to: " << config_file << std::endl;
}
