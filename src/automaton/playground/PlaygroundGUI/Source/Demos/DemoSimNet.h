#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

#include "automaton/core/io/io.h"
#include "automaton/core/network/simulated_connection.h"
#include "automaton/core/node/node_updater.h"
#include "automaton/core/testnet/testnet.h"
#include "automaton/examples/node/blockchain_cpp_node/blockchain_cpp_node.h"

#include "../../JuceLibraryCode/JuceHeader.h"

class ButtonFeelAndLook: public LookAndFeel_V4 {
  static const Colour button_clr;
  static const Colour button_border_clr;
  static const Colour button_over_clr;
  static const Colour button_over_border_clr;
  static const Colour button_on_clr;
  static const Colour button_on_border_clr;
  static const Colour button_inactive_clr;
  static const Colour button_inactive_border_clr;

 public:
  ButtonFeelAndLook() {}

  void drawButtonBackground(Graphics& g, Button& btn, const Colour& backgroundColour,  // NOLINT
      bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown);
};

class DemoSimNet: public AnimatedAppComponent {
 public:
  DemoSimNet();
  ~DemoSimNet();

  void update() override;
  void paint(Graphics& g) override;
  void resized() override;

 private:
  Colour new_color();
  void fix_percentages();

  void simulation();

  void start_btn_handle();
  void pause_btn_handle();

  void start();
  void pause();
  void resume();
  void stop();

  std::unique_ptr<g3::LogWorker> logworker;
  std::shared_ptr<automaton::core::network::simulation> sim;
  automaton::core::node::node_updater_tests* updater;
  std::vector<std::string> ids;
  bool sim_created = false;
  bool sim_running = false;
  uint64_t current_time;
  std::vector<std::thread> sim_threads;

  std::unordered_map<std::string, Colour> clrs;
  std::unordered_map<std::string, uint32_t> counter;
  std::unordered_map<std::string, block> blocks;
  std::unordered_map<std::string, float> percentages;

  // UI

  static const Colour bg_clr;
  static const Colour border_clr;
  static const Colour text_clr;

  static const uint32_t start_button_pos_x = 102;
  static const uint32_t start_button_pos_y = 50;
  static const uint32_t button_width = 180;
  static const uint32_t button_height = 55;
  static const uint32_t button_space = 65;

  static const Rectangle<float> pie_chart;

  static const uint32_t data_pos_x = 750;
  static const uint32_t data_pos_y = 200;

  float hue;
  float sat;
  TextButton start_button;
  TextButton pause_button;
  ButtonFeelAndLook lf;
};
