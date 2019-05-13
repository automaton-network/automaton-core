#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

class DemosMainComponent:
  public Component,
  public ApplicationCommandTarget,
  public MenuBarModel {
 public:
  enum CommandIDs
  {
  };

  DemosMainComponent();
  ~DemosMainComponent();

  void paint(Graphics& g) override;
  void resized() override;

  StringArray getMenuBarNames() override
  {
      return { "Menu Position", "Outer Colour", "Inner Colour" };
  }

  void getAllCommands (Array<CommandID>& c) override
  {
    /*
    Array<CommandID> commands { CommandIDs::innerColourRed,
                                CommandIDs::innerColourGreen,
                                CommandIDs::innerColourBlue };

    c.addArray (commands);
    */
  }

  PopupMenu getMenuForIndex (int topLevelMenuIndex, const String& menuName) override {
    PopupMenu menu;
    return menu;
  }

  bool perform (const InvocationInfo& info) override {
    return false;
  }

  void menuItemSelected (int /*menuItemID*/, int /*topLevelMenuIndex*/) override {}

  ApplicationCommandTarget* getNextCommandTarget() {
    return nullptr;
  }

  void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override
  {
    /*
    switch (commandID)
    {
      case CommandIDs::menuPositionInsideWindow:
          result.setInfo ("Inside Window", "Places the menu bar inside the application window", "Menu", 0);
          result.setTicked (menuBarPosition == MenuBarPosition::window);
          result.addDefaultKeypress ('w', ModifierKeys::shiftModifier);
          break;
      case CommandIDs::menuPositionGlobalMenuBar:
          result.setInfo ("Global", "Uses a global menu bar", "Menu", 0);
          result.setTicked (menuBarPosition == MenuBarPosition::global);
          result.addDefaultKeypress ('g', ModifierKeys::shiftModifier);
          break;
      case CommandIDs::menuPositionBurgerMenu:
          result.setInfo ("Burger Menu", "Uses a burger menu", "Menu", 0);
          result.setTicked (menuBarPosition == MenuBarPosition::burger);
          result.addDefaultKeypress ('b', ModifierKeys::shiftModifier);
          break;
      default:
          break;
    }
    */
 }

 private:
  std::unique_ptr<MenuBarComponent> menuBar;
  std::unique_ptr<TabbedComponent> tabbedComponent;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemosMainComponent)
};
