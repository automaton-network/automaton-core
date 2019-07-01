#include "DemoSimNet.h"

#include <memory>  // NOLINT
#include <set>
#include <thread>
#include <unordered_map>

#include "automaton/examples/node/blockchain_cpp_node/blockchain_cpp_node.h"

using automaton::core::io::bin2hex;
using automaton::core::node::node;
using automaton::core::node::node_updater_tests;
using automaton::core::smartproto::smart_protocol;
using automaton::core::testnet::testnet;

const uint32_t WORKER_SLEEP_TIME_MS = 1800;
const uint32_t NODES = 100000;
const uint32_t PEERS = 2;
const uint32_t SIMULATION_SLEEP_TIME_MS = 30;

const Colour DemoSimNet::bg_clr = Colour::fromHSV(0.91, 0.04, 0.16, 1.0);
const Colour DemoSimNet::border_clr = Colour::fromHSV(0.0, 0.0, 0.69, 0.7);
const Colour DemoSimNet::text_clr = Colour::fromHSV(0.83, 0.16, 0.82, 1.0);

const Colour ButtonFeelAndLook::button_clr = Colour::fromHSV(0.58, 0.28, 0.62, 1.0);
const Colour ButtonFeelAndLook::button_border_clr = Colour::fromHSV(0.58, 0.28, 0.72, 1.0);
const Colour ButtonFeelAndLook::button_over_clr = Colour::fromHSV(0.58, 0.28, 0.62, 0.8);
const Colour ButtonFeelAndLook::button_over_border_clr = Colour::fromHSV(0.58, 0.28, 0.72, 0.8);
const Colour ButtonFeelAndLook::button_on_clr = Colour::fromHSV(0.58, 0.28, 0.62, 0.5);
const Colour ButtonFeelAndLook::button_on_border_clr = Colour::fromHSV(0.58, 0.28, 0.72, 0.5);
const Colour ButtonFeelAndLook::button_inactive_clr = Colour::fromHSV(0.58, 0.15, 0.62, 0.5);
const Colour ButtonFeelAndLook::button_inactive_border_clr = Colour::fromHSV(0.58, 0.15, 0.72, 0.5);

void ButtonFeelAndLook::drawButtonBackground(Graphics& g, Button& btn, const Colour& backgroundColour,  // NOLINT
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
  if (btn.getToggleState() == false) {
    g.setColour(button_inactive_clr);
    g.fillRoundedRectangle(btn.getLocalBounds().toFloat(), 13.0f);
    g.setColour(button_inactive_border_clr);
    g.drawRoundedRectangle(btn.getLocalBounds().toFloat(), 13.0f, 1.8f);
  } else {
    if (shouldDrawButtonAsHighlighted) {
      g.setColour(button_over_clr);
      g.fillRoundedRectangle(btn.getLocalBounds().toFloat(), 13.0f);
      g.setColour(button_over_border_clr);
      g.drawRoundedRectangle(btn.getLocalBounds().toFloat(), 13.0f, 1.8f);
    } else if (shouldDrawButtonAsDown) {
      g.setColour(button_on_clr);
      g.fillRoundedRectangle(btn.getLocalBounds().toFloat(), 13.0f);
      g.setColour(button_on_border_clr);
      g.drawRoundedRectangle(btn.getLocalBounds().toFloat(), 13.0f, 1.8f);
    } else {
      g.setColour(button_clr);
      g.fillRoundedRectangle(btn.getLocalBounds().toFloat(), 13.0f);
      g.setColour(button_border_clr);
      g.drawRoundedRectangle(btn.getLocalBounds().toFloat(), 13.0f, 1.8f);
    }
  }
}

const Rectangle<float> DemoSimNet::pie_chart(30, 200, 600, 600);

DemoSimNet::DemoSimNet() {
  hue = 0.1;
  sat = 0.22;

  Rectangle<int> first_button_rect(start_button_pos_x, start_button_pos_y, button_width, button_height);
  Rectangle<int> second_button_rect(start_button_pos_x + button_width + button_space, start_button_pos_y, button_width,
      button_height);
  start_button.setToggleState(true, NotificationType::dontSendNotification);
  pause_button.setToggleState(false, NotificationType::dontSendNotification);

  start_button.setButtonText("start");
  pause_button.setButtonText("pause");

  start_button.setBounds(first_button_rect);
  pause_button.setBounds(second_button_rect);

  start_button.onClick = std::bind(&DemoSimNet::start_btn_handle, this);
  pause_button.onClick = std::bind(&DemoSimNet::pause_btn_handle, this);

  start_button.setLookAndFeel(&lf);
  pause_button.setLookAndFeel(&lf);

  addAndMakeVisible(start_button);
  addAndMakeVisible(pause_button);

  setFramesPerSecond(12);

  sim_threads = std::vector<std::thread>(2);

  logworker = std::unique_ptr<g3::LogWorker>(g3::LogWorker::createLogWorker());
  auto l_handler = logworker->addDefaultLogger("demo", "./");
  g3::initializeLogging(logworker.get());

  node::register_node_type("blockchain", [](const std::string& id, const std::string& proto_id)->
      std::shared_ptr<automaton::core::node::node> {
      return std::shared_ptr<automaton::core::node::node>(new blockchain_cpp_node(id, proto_id));
    });
  if (smart_protocol::load("blockchain", "/home/carrie/Desktop/automaton/src/automaton/examples/smartproto/blockchain/")
      == false) {
    LOG(ERROR) << "Blockchain protocol was NOT loaded!!!";
    throw std::runtime_error("Protocol was not loaded!");
  }

  sim = automaton::core::network::simulation::get_simulator();
}

DemoSimNet::~DemoSimNet() {
  stop();
}

void DemoSimNet::update() {
  counter.clear();
  blocks.clear();
  percentages.clear();

  for (auto n : ids) {
    std::shared_ptr<blockchain_cpp_node> node = std::dynamic_pointer_cast<blockchain_cpp_node>(node::get_node(n));
    block b = node->get_blockchain_top();
    auto hash = b.block_hash();
    counter[hash]++;
    if (blocks.find(hash) == blocks.end()) {
      blocks[hash] = b;
    }
    if (clrs.find(hash) == clrs.end()) {
      clrs[hash] = new_color();
    }
  }
  fix_percentages();
}

void DemoSimNet::paint(Graphics& g) {
  g.fillAll(bg_clr);

  // pie chart

  if (percentages.size() == 0) {
    g.setColour(border_clr);
    g.drawEllipse(pie_chart, 0.6f);
  } else {
    float full_circle = MathConstants<float>::pi * 2.;
    PathStrokeType stroke_tp(0.6f);
    float cpos = 0.0;
    float cend = 0.0;
    for (auto it = percentages.begin(); it != percentages.end(); ++it) {
      Path p;
      cend += (it->second * full_circle);
      g.setColour(clrs[it->first]);
      p.addPieSegment(pie_chart, cpos, cend, 0);
      g.fillPath(p);
      g.setColour(border_clr);
      g.strokePath(p, stroke_tp);
      cpos = cend;
    }
  }

  // data

  g.setColour(Colour::fromHSV(0.75, 0.13, 0.71, 1.0));
  int height = 30;
  int width = 200;
  int y = data_pos_y;
  for (auto it = counter.begin(); it != counter.end(); ++it) {
    g.setColour(clrs[it->first]);
    g.fillRect(data_pos_x, y, width, height);
    g.setColour(Colours::black);
    g.drawFittedText(hashstr(it->first) + " / height " + std::to_string(blocks.at(it->first).height),
        data_pos_x, y, width, height, Justification::Flags::centred, 1, 1);
    g.setColour(text_clr);
    g.drawFittedText(std::to_string(it->second),
        data_pos_x + width + 20, y, width, height, Justification::Flags::centred, 1, 1);
    y += 5 + height;
  }
}

void DemoSimNet::resized() {}

Colour DemoSimNet::new_color() {
  hue += 0.18;
  if (hue > 1.0) {
    hue -= 1.0;
    sat += 0.04;
  }
  return Colour::fromHSV(hue, sat, 0.62, 1.0);
}

void DemoSimNet::fix_percentages() {
  float diff = 0.0;
  for (auto it = counter.begin(); it != counter.end(); ++it) {
    float sol = it->second / static_cast<float>(NODES);
    if (sol < 0.015) {
      diff = 0.015 - sol;
      sol = 0.015;
    } else if (sol > 0.4) {
      sol -= diff;
      diff = 0.0;
    }
    percentages[it->first] = sol;
  }

  if (diff > 0.01) {
    auto biggest = percentages.begin();
    for (auto it = percentages.begin(); it != percentages.end(); ++it) {
      if (it->second > biggest->second) {
        biggest = it;
      }
    }
    biggest->second -= diff;
  }
}

void DemoSimNet::simulation() {
  current_time = 0;

  sim_threads[0] = std::thread([&](){
    while (true) {
      if (sim_running) {
        sim->process(current_time);
        std::this_thread::sleep_for(std::chrono::milliseconds(SIMULATION_SLEEP_TIME_MS));
        current_time += SIMULATION_SLEEP_TIME_MS;
      }
      if (!sim_created) {
        return;
      }
    }
  });

  sim_threads[1] = std::thread([&](){
    while (true) {
      if (sim_running) {
        sim->process_handlers();
        std::this_thread::sleep_for(std::chrono::milliseconds(SIMULATION_SLEEP_TIME_MS));
      }
      if (!sim_created) {
        return;
      }
    }
  });
}

void DemoSimNet::start_btn_handle() {
  if (!sim_created) {
    pause_button.setToggleState(true, NotificationType::dontSendNotification);
    start_button.setButtonText("stop");
    start();
  } else {
    pause_button.setToggleState(false, NotificationType::dontSendNotification);
    pause_button.setButtonText("pause");
    start_button.setButtonText("start");
    stop();
  }
}

void DemoSimNet::pause_btn_handle() {
  if (!sim_created) {
    return;
  }
  if (sim_running) {
    pause_button.setButtonText("resume");
    pause();
  } else {
    pause_button.setButtonText("pause");
    resume();
  }
}

void DemoSimNet::start() {
  if (sim_created) {
    return;
  }
  sim_created = true;
  sim_running = true;
  simulation();
  testnet::create_testnet("blockchain", "testnet", "doesntmatter", testnet::network_protocol_type::simulation, NODES,
      automaton::core::testnet::create_rnd_connections_vector(NODES, PEERS));
  ids = testnet::get_testnet("testnet")->list_nodes();
  updater = new node_updater_tests(WORKER_SLEEP_TIME_MS, std::set<std::string>(ids.begin(), ids.end()));
  updater->start();
}

void DemoSimNet::pause() {
  updater->stop();
  sim_running = false;
}

void DemoSimNet::resume() {
  updater->start();
  sim_running = true;
}

void DemoSimNet::stop() {
  if (!sim_created) {
    return;
  }
  updater->stop();
  delete updater;
  ids.clear();
  sim_running = false;
  sim_created = false;
  sim_threads[0].join();
  sim_threads[1].join();
  testnet::destroy_testnet("testnet");
}
