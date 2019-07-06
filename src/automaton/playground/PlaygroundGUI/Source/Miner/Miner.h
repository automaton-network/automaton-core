#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

#include "automaton/core/crypto/cryptopp/secure_random_cryptopp.h"
#include "automaton/core/crypto/cryptopp/SHA256_cryptopp.h"
#include "automaton/core/io/io.h"

class Miner:
  public Component,
  public Button::Listener,
  private Timer {
 public:
  //==============================================================================
  Miner();
  ~Miner();

  void paint(Graphics& g) override;
  void resized() override;

  void update();

  // Button::Listener overrides.
  void buttonClicked(Button* btn) override;

  // Mining
  struct slot {
    std::string difficulty;
    std::string owner;

    slot() {
      // Set min difficulty.
      char buf[32];
      memset(buf, 0, 32);
      buf[0] = 0xFF;
      buf[1] = 0xF0;
      difficulty = std::string(buf, 32);
    }
  };
  void initSlots();
  void processMinedKey(std::string _pk);
  size_t getSlotsNumber() { return slots.size(); }
  slot& getSlot(int _slot) { return slots[_slot]; }

  void addMinerThread();
  void stopMining();

 private:
  // Mining
  int totalSlots = 1024;
  OwnedArray<Thread> miners;
  std::vector<slot> slots;
  std::string minerAddress;

  // UI
  OwnedArray<Component> components;
  TextEditor* txtRpcServer;
  Button* btnContract;
  TextEditor* txtContract;
  TextEditor* txtMinerAddress;

  void addComponent(Component* c) {
    components.add(c);
    addAndMakeVisible(c);
  }

  TextButton* TB(String text, int x, int y, int w, int h) {
    TextButton* tb = new TextButton(text);
    tb->addListener(this);
    addComponent(tb);
    tb->setBounds(x, y, w, h);

    tb->setColour(TextButton::textColourOffId,  Colours::black);
    tb->setColour(TextButton::textColourOnId,   Colours::black);
    tb->setColour(TextButton::buttonColourId,   Colours::white);
    tb->setColour(TextButton::buttonOnColourId, Colours::cyan.brighter());

    return tb;
  }

  int GTB(int gid, int def, StringArray texts, int x, int y, int w, int h) {
    int firstButtonIndex = components.size();
    for (unsigned int i = 0; i < texts.size(); i++) {
      String text = texts[i];
      TextButton* tb = TB(text, x, y, w, h);
      tb->setClickingTogglesState(true);
      tb->setRadioGroupId(gid);
      if (i == def) {
        tb->setToggleState(true, dontSendNotification);
      }

      tb->setConnectedEdges(
          ((i == 0) ? 0 : Button::ConnectedOnLeft) |
          ((i == (texts.size() - 1)) ? 0 : Button::ConnectedOnRight));

      x += w;
    }
  }

  Label* LBL(String text, int x, int y, int w, int h) {
    Label* lbl = new Label("", text);
    addComponent(lbl);
    lbl->setBounds(x, y, w, h);
    lbl->setColour(Label::textColourId, Colours::white);
    return lbl;
  }

  TextEditor* TXT(String name, int x, int y, int w, int h) {
    TextEditor* txt = new TextEditor(name);
    addComponent(txt);
    txt->setBounds(x, y, w, h);
    // txt->setColour(TextEditor::backgroundColourId, Colours::black);
    // txt->setColour(TextEditor::textColourId, Colours::white);
    // txt->setJustificationType(Justification::centred);
    return txt;
  }

  void timerCallback() override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Miner)
};
