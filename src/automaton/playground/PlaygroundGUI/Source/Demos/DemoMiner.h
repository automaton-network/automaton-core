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

  slot slots[256][256];
  unsigned int total_balance = 0;
  unsigned int total_supply = 0;

  unsigned int max_leading_bits = 1;

  unsigned int t;

  uint32 sz = 5;
  uint32 gap = 1;
  uint32 mining_power = 25;

  void timerCallback() override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoMiner)
};
