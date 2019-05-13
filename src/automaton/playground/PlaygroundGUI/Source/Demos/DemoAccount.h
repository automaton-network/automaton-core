#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

class DemoAccount  : public Component {
 public:
  DemoAccount();
  ~DemoAccount();

  void paint(Graphics& g) override;
  void resized() override;

 private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoAccount)
};
