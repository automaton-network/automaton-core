#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

class EmbeddedFonts {
 private:
  Font play;
  Font playBold;

 public:
  EmbeddedFonts() {
    play = Font(Typeface::createSystemTypefaceFor(BinaryData::PlayRegular_ttf, BinaryData::PlayRegular_ttfSize));
    playBold = Font(Typeface::createSystemTypefaceFor(BinaryData::PlayBold_ttf, BinaryData::PlayBold_ttfSize));
  }

  Font& getPlay() { return play; }
  Font& getPlayBold() { return playBold; }
};


class DemosMainComponent:
  public Component,
  public ApplicationCommandTarget,
  public MenuBarModel {
 public:
  EmbeddedFonts fonts;

  enum CommandIDs {
  };

  DemosMainComponent();
  ~DemosMainComponent();

  void paint(Graphics& g) override;
  void resized() override;

  StringArray getMenuBarNames() override {
    return { "Menu Position", "Outer Colour", "Inner Colour" };
  }

  void getAllCommands(Array<CommandID>& c) override {
  }

  PopupMenu getMenuForIndex(int topLevelMenuIndex, const String& menuName) override {
    PopupMenu menu;
    return menu;
  }

  bool perform(const InvocationInfo& info) override {
    return false;
  }

  void menuItemSelected(int /*menuItemID*/, int /*topLevelMenuIndex*/) override {}

  ApplicationCommandTarget* getNextCommandTarget() {
    return nullptr;
  }

  void getCommandInfo(
     CommandID commandID,
     ApplicationCommandInfo& result) override {
  }

 private:
  std::unique_ptr<MenuBarComponent> menuBar;
  std::unique_ptr<TabbedComponent> tabbedComponent;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemosMainComponent)
};
