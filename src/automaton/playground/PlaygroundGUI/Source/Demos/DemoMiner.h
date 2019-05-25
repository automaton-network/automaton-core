#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

class DemoMiner:
  public Component,
  private Timer {
 public:
  //==============================================================================
  DemoMiner();
  ~DemoMiner();

  void paint(Graphics& g) override;
  void resized() override;

  void update();

 private:
  struct slot {
    Colour bg;
    Colour fg;
    uint64 diff;
    int owner;
    int tm;
  };

  const uint64 supply_cap = 1UL << 40;
  const uint32 m = 256;
  const uint32 n = 256;
  const uint64 reward_per_period = 20000;
  const double periods_per_day = 1.0;
  const uint32 iters = 1;

  slot slots[256][256];
  uint64 total_balance = 0;
  uint64 total_supply = 0;

  unsigned int max_leading_bits = 1;
  unsigned int min_leading_bits = 0;

  unsigned int t;

  uint32 sz = 1;
  uint32 gap = 0;
  uint32 mining_power = 10;

  void timerCallback() override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoMiner)
};
