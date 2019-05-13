#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

class DemoNetwork  : public Component {
 public:
  DemoNetwork();
  ~DemoNetwork();

  void paint(Graphics& g) override;
  void resized() override;

 private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoNetwork)
};
