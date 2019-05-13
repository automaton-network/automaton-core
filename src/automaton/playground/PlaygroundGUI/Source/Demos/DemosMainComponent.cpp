#include "DemoAccount.h"
#include "DemoMiner.h"
#include "DemoNetwork.h"

#include "DemosMainComponent.h"

DemosMainComponent::DemosMainComponent() {
  menuBar.reset(new MenuBarComponent(this));
  addAndMakeVisible (menuBar.get());

  tabbedComponent.reset(new TabbedComponent(TabbedButtonBar::TabsAtTop));
  addAndMakeVisible(tabbedComponent.get());
  tabbedComponent->setTabBarDepth(37);
  tabbedComponent->addTab(TRANS("Account"), Colour(0xff245230), new DemoAccount(), true);
  tabbedComponent->addTab(TRANS("Miner"), Colour(0xff522424), new DemoMiner(), true);
  tabbedComponent->addTab(TRANS("Network"), Colour(0xff243352), new DemoNetwork(), true);
  tabbedComponent->setCurrentTabIndex(0);

  setSize(800, 600);
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
  
  auto height = LookAndFeel::getDefaultLookAndFeel().getDefaultMenuBarHeight();
  menuBar->setBounds(b.removeFromTop(height));

  tabbedComponent->setBounds(8, 8 + height, getWidth() - 16, getHeight() - 16);
}
