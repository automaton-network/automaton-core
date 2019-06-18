#include "DemoMiner.h"

#include "DemosMainComponent.h"

class DemoBlank: public Component {
 public:
  DemoBlank() {}
  ~DemoBlank() {}

  void paint(Graphics& g) override {
  }

  void resized() override {
  }

 private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoBlank)
};


DemosMainComponent::DemosMainComponent() {
  // menuBar.reset(new MenuBarComponent(this));
  // addAndMakeVisible(menuBar.get());

  tabbedComponent.reset(new TabbedComponent(TabbedButtonBar::TabsAtTop));
  addAndMakeVisible(tabbedComponent.get());
  tabbedComponent->setTabBarDepth(37);
  tabbedComponent->addTab(TRANS("Miner"), Colour(0xff404040), new DemoMiner(), true);
  tabbedComponent->addTab(TRANS("Treasury"), Colour(0xff404040), new DemoBlank(), true);
  tabbedComponent->addTab(TRANS("Protocols"), Colour(0xff404040), new DemoBlank(), true);
  tabbedComponent->addTab(TRANS("DApps"), Colour(0xff404040), new DemoBlank(), true);
  tabbedComponent->addTab(TRANS("Network"), Colour(0xff404040), new DemoBlank(), true);
  tabbedComponent->setCurrentTabIndex(0);

  setSize(1024, 768);
}

DemosMainComponent::~DemosMainComponent() {
  tabbedComponent = nullptr;
}

//==============================================================================
void DemosMainComponent::paint(Graphics& g) {
  g.fillAll(Colour(0xff323e44));
}

void DemosMainComponent::resized() {
  auto b = getLocalBounds();

  // auto height = LookAndFeel::getDefaultLookAndFeel().getDefaultMenuBarHeight();
  // menuBar->setBounds(b.removeFromTop(height));

  tabbedComponent->setBounds(8, 8, getWidth() - 16, getHeight() - 16);
}
