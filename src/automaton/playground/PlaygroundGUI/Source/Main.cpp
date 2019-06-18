/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
// #include "MainComponent.h"
#include "Demos/DemosMainComponent.h"


class EmbeddedFonts {
 private:
  Font play;
  Font playBold;

 public:
  EmbeddedFonts() {
    play = Font(Typeface::createSystemTypefaceFor(BinaryData::PlayRegular_ttf, BinaryData::PlayRegular_ttfSize));
    playBold = Font(Typeface::createSystemTypefaceFor(BinaryData::PlayBold_ttf, BinaryData::PlayBold_ttfSize));
  }

  const Font& getPlay() { return play; }
  const Font& getPlayBold() { return playBold; }
};


//==============================================================================
class PlaygroundGUIApplication: public JUCEApplication {
 public:
  //==============================================================================
  PlaygroundGUIApplication() {}

  const String getApplicationName() override       { return ProjectInfo::projectName; }
  const String getApplicationVersion() override    { return ProjectInfo::versionString; }
  bool moreThanOneInstanceAllowed() override       { return true; }

  //==============================================================================
  void initialise(const String& commandLine) override {
/*
    const Font& fontPlay = fonts.getPlay();
    typefacePlay = LookAndFeel::getDefaultLookAndFeel().getTypefaceForFont(fontPlay);
    LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypeface(typefacePlay);
*/
    mainWindow.reset(new MainWindow(getApplicationName()));
  }

  void shutdown() override {
    // Add your application's shutdown code here..
    mainWindow = nullptr;  // (deletes our window)
  }

  //==============================================================================
  void systemRequestedQuit() override {
    // This is called when the app is being asked to quit: you can ignore this
    // request and let the app carry on running, or call quit() to allow the app to close.
    quit();
  }

  void anotherInstanceStarted(const String& commandLine) override {
      // When another instance of the app is launched while this one is running,
      // this method is invoked, and the commandLine parameter tells you what
      // the other instance's command-line arguments were.
  }

  //==============================================================================
  /*
      This class implements the desktop window that contains an instance of
      our DemoMainComponent class.
  */
  class MainWindow    : public DocumentWindow {
   public:
    LookAndFeel_V2 lnf;

    MainWindow(String name):
      DocumentWindow(name,
                     Desktop::getInstance()
                       .getDefaultLookAndFeel()
                       .findColour(ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons) {
      LookAndFeel::setDefaultLookAndFeel(&lnf);

      setUsingNativeTitleBar(true);
      setContentOwned(new DemosMainComponent(), true);

      setFullScreen(true);
      setResizable(true, true);

      setVisible(true);
    }

    void closeButtonPressed() override {
        // This is called when the user tries to close this window. Here, we'll just
        // ask the app to quit when this happens, but you can change this to do
        // whatever you need.
        JUCEApplication::getInstance()->systemRequestedQuit();
    }

    /* Note: Be careful if you override any DocumentWindow methods - the base
       class uses a lot of them, so by overriding you might break its functionality.
       It's best to do all your work in your content component instead, but if
       you really have to override any DocumentWindow methods, make sure your
       subclass also calls the superclass's method.
    */

   private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
  };

 private:
  std::unique_ptr<MainWindow> mainWindow;
  EmbeddedFonts fonts;
  Typeface::Ptr typefacePlay;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(PlaygroundGUIApplication)
