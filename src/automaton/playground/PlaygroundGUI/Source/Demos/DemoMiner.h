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

 private:
  void timerCallback() override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoMiner)
};
