#include "Miner.h"

#include "secp256k1/include/secp256k1_recovery.h"
#include "secp256k1/include/secp256k1.h"
#include "secp256k1/src/hash_impl.h"
#include "secp256k1/src/hash.h"
#include "automaton/tools/miner/miner.h"
#include "automaton/core/io/io.h"

using automaton::tools::miner::mine_key;
using automaton::tools::miner::sign;
using automaton::tools::miner::gen_pub_key;

using automaton::core::io::bin2hex;
using automaton::core::io::hex2bin;

static std::string get_pub_key_x(const unsigned char* priv_key) {
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();

  if (!secp256k1_ec_pubkey_create(context, pubkey, priv_key)) {
    LOG(WARNING) << "Invalid priv_key " << bin2hex(std::string(reinterpret_cast<const char*>(priv_key), 32));
    return "";
  }

  unsigned char pub_key_serialized[65];
  size_t outLen = 65;
  secp256k1_ec_pubkey_serialize(context, pub_key_serialized, &outLen, pubkey, SECP256K1_EC_UNCOMPRESSED);
  std::string pub_key_uncompressed(reinterpret_cast<char*>(pub_key_serialized), 65);
  return std::string(reinterpret_cast<char*>(pub_key_serialized+1), 32);
}

class MinerThread: public Thread {
 public:
  uint64 keysMined;
  int64 startTime;

  Miner* owner;

  MinerThread(Miner* _owner) : Thread("Miner Thread"), keysMined(0), owner(_owner) {
  }

  void run() override {
    keysMined = 0;
    startTime = Time::getCurrentTime().toMilliseconds();

    unsigned char mask[32];
    unsigned char difficulty[32];
    unsigned char pk[32];

    memcpy(mask, owner->getMask(), 32);
    memcpy(difficulty, owner->getDifficulty(), 32);

    while (!threadShouldExit()) {
      unsigned int keys_generated = mine_key(mask, difficulty, pk);
      if (owner) {
        owner->processMinedKey(std::string(reinterpret_cast<const char*>(pk), 32), keys_generated);
      }
      // wait(20);
    }
  }
};

void Miner::addMinerThread() {
  Thread * miner = new MinerThread(this);
  miner->startThread();
  miners.add(miner);
}

void Miner::stopMining() {
  for (int i = 0; i < miners.size(); i++) {
    miners[i]->stopThread(3000);
  }
  miners.clear(true);
}

void Miner::processMinedKey(std::string _pk, int keys_generated) {
  total_keys_generated += abs(keys_generated);
  if (keys_generated <= 0) {
    return;
  }
  std::string x = get_pub_key_x(reinterpret_cast<const unsigned char *>(_pk.c_str()));
  CryptoPP::Integer bn_x((bin2hex(x) + "h").c_str());
  uint32 slot = bn_x % totalSlots;
  for (int i = 0; i < 32; i++) {
    x[i] ^= mask[i];
  }
  if (x <= std::string(reinterpret_cast<char*>(difficulty), 32)) {
    return;
  }
  if (x > slots[slot].difficulty) {
    slots_claimed++;
    slots[slot].owner = std::string(reinterpret_cast<char*>(minerAddress), 32);
    slots[slot].difficulty = x;
    slots[slot].private_key = _pk;
  }
}

class TableSlots: public TableListBox, TableListBoxModel {
 public:
  Miner* owner;

  TableSlots(Miner * _owner) : owner(_owner) {
    // Create our table component and add it to this component..
    // addAndMakeVisible(table);
    setModel(this);

    // give it a border
    setColour(ListBox::outlineColourId, Colours::grey);
    setOutlineThickness(1);
    setRowHeight(16);

    struct column {
      String name;
      int ID;
      int width;
    };

    column columns[] = {
      {"Slot", 1, 40},
      {"Difficulty", 2, 100},
      {"Owner", 3, 50},
      {"Private Key", 4, 400},
      {"", 0, 0},
    };

    // Add some columns to the table header, based on the column list in our database..
    for (int i = 0; columns[i].ID != 0; i++) {
      getHeader().addColumn(columns[i].name,
                                  columns[i].ID,
                                  columns[i].width,
                                  50, 400,
                                  TableHeaderComponent::defaultFlags);
    }

    // we could now change some initial settings..
    getHeader().setSortColumnId(1, true);  // sort forwards by the ID column
    getHeader().setColumnVisible(7, false);  // hide the "length" column until the user shows it

    // un-comment this line to have a go of stretch-to-fit mode
    // getHeader().setStretchToFitActive(true);

    setMultipleSelectionEnabled(false);
  }

  // This is overloaded from TableListBoxModel, and must return the total number of rows in our table
  int getNumRows() override {
    return owner->getSlotsNumber();
  }

  void selectedRowsChanged(int) override {
    owner->createSignature();
  }

  // This is overloaded from TableListBoxModel, and should fill in the background of the whole row
  void paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override {
    auto alternateColour = getLookAndFeel().findColour(ListBox::backgroundColourId)
                                           .interpolatedWith(getLookAndFeel().findColour(ListBox::textColourId), 0.03f);
    if (rowIsSelected)
        g.fillAll(alternateColour.brighter(0.2f));
    else if (rowNumber % 2)
        g.fillAll(alternateColour);
  }

  // This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
  // components.
  void paintCell(Graphics& g, int rowNumber, int columnId,
                int width, int height, bool /*rowIsSelected*/) override {
    g.setColour(getLookAndFeel().findColour(ListBox::textColourId));
    g.setFont(font);

    String text = "";
    switch (columnId) {
      case 1: {
        char b[2];
        b[0] = (rowNumber >> 8) & 0xFF;
        b[1] = rowNumber & 0xFF;
        std::string x(b, 2);
        text = bin2hex(x);
        break;
      }
      case 2: {
        text = bin2hex(owner->getSlot(rowNumber).difficulty);
        break;
      }
      case 3: {
        text = "0x" + bin2hex(owner->getSlot(rowNumber).owner);
        break;
      }
      case 4: {
        text = bin2hex(owner->getSlot(rowNumber).private_key);
      }
    }
    g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);

    g.setColour(getLookAndFeel().findColour(ListBox::backgroundColourId));
    g.fillRect(width - 1, 0, 1, height);
  }

 private:
  // TableListBox table;     // the table component itself
  Font font  { 12.0f };

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TableSlots)
};

class ReadContractThread: public ThreadWithProgressWindow {
 public:
  ReadContractThread() : ThreadWithProgressWindow("Reading Contract...", true, true) {
    setStatusMessage("Getting ready...");
  }

  struct task {
    double progress;
    std::string msg;
    int tm;
  };

  task tasks[16] = {
      {-1.0, "Connecting to Ethereum Network...", 20},
      {-1.0, "Reading Mask...", 10},
      {-1.0, "Reading Number of Slots...", 10},
      { 0.0, "Reading Slots Difficulty...", 5},
      { 0.1, "Reading Slots Difficulty...", 5},
      { 0.2, "Reading Slots Difficulty...", 5},
      { 0.3, "Reading Slots Difficulty...", 5},
      { 0.4, "Reading Slots Difficulty...", 5},
      { 0.5, "Reading Slots Difficulty...", 5},
      { 0.6, "Reading Slots Difficulty...", 5},
      { 0.7, "Reading Slots Difficulty...", 5},
      { 0.8, "Reading Slots Difficulty...", 5},
      { 0.9, "Reading Slots Difficulty...", 5},
      { 1.0, "Reading Slots Difficulty...", 5},
      {-1.0, "Finishing off the last few bits and pieces!", 10},
      {10.0, "Done", 0},
  };

  void run() override {
    for (int i = 0; tasks[i].progress <= 1.0; i++) {
      setProgress(tasks[i].progress);
      setStatusMessage(tasks[i].msg);
      for (int t = 0; t < tasks[i].tm; t++) {
        if (threadShouldExit()) {
          return;
        }
        wait(100);
      }
    }
  }

  // This method gets called on the message thread once our thread has finished..
  void threadComplete(bool userPressedCancel) override {
    if (userPressedCancel) {
      AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                       "Progress window",
                                       "You pressed cancel!");
    } else {
      // thread finished normally..
      AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                       "Progress window",
                                       "Thread finished ok!");
    }

    // ..and clean up by deleting our thread object..
    delete this;
  }
};

static unsigned int leading_bits_char(char x) {
  unsigned int c = 0;
  while (x & 0x80) {
    c++;
    x <<= 1;
  }
  return c;
}

using automaton::core::io::bin2hex;

unsigned int leading_bits(const std::string& s) {
  unsigned int result = 0;
  for (unsigned int i = 0; i < s.size(); i++) {
    unsigned int lb = leading_bits_char(s[i]);
    result += lb;
    if (lb != 8) {
      break;
    }
  }
  return result;
}

static String sepitoa(uint64 n, bool lz = false) {
  if (n < 1000) {
    if (!lz) {
      return String(n);
    } else {
      return
          ((n < 100) ? String("0") : String("")) +
          ((n < 10) ? String("0") : String("")) +
          String(n);
    }
  } else {
    return sepitoa(n / 1000, lz) + "," + sepitoa(n % 1000, true);
  }
}

//==============================================================================
Miner::Miner() {
  startTimer(1000);

  int y = 0;

/*
  y += 50;
  LBL("RPC Server:", 20, y, 100, 24);
  txtRpcServer = TXT("RPC", 120, y, 500, 24);
  txtRpcServer->setText("HTTP://127.0.0.1:7545");

  y += 30;
  LBL("Contract:" , 20, y, 100, 24);
  txtContract = TXT("CONTR", 120, y, 500, 24);
  txtContract->setText("0xdb95dbb1be9e82f8caaa90f0feacabb5e7fa36ab");

  y += 30;
  btnContract = TB("Read Contract", 120, y, 120, 24);
*/

  y += 50;
  LBL("Slots: ", 20, y, 100, 24);
  txtSlotsNum = TXT("SLOTS", 120, y, 100, 24);
  txtSlotsNum->setInputRestrictions(5, "0123456789");

  y += 30;
  LBL("Mask: ", 20, y, 100, 24);
  txtMask = TXT("MASK", 120, y, 600, 24);
  txtMask->setInputRestrictions(78, "0123456789");

  y += 30;
  LBL("Mask (Hex):", 20, y, 100, 24);
  txtMaskHex = TXT("MASKHEX", 120, y, 600, 24);
  txtMaskHex->setReadOnly(true);

  y += 30;
  LBL("Min Difficulty:", 20, y, 100, 24);
  txtMinDifficulty = TXT("MINDIFF", 120, y, 25, 24);
  txtMinDifficulty->setInputRestrictions(2, "0123456789");

  txtMinDifficultyHex = TXT("MINDIFFHEX", 150, y, 500, 24);
  txtMinDifficultyHex->setReadOnly(true);

  y += 30;
  LBL("Miner Address: ", 20, y, 100, 24);
  txtMinerAddress = TXT("ADDR", 120, y, 600, 24);
  txtMinerAddress->setInputRestrictions(42, "0123456789abcdefABCDEFx");

  y += 50;
  TB("Add Miner", 120, y, 80, 24);
  TB("Stop Miners", 220, y, 80, 24);
  TB("Claim", 120, y + 30, 80, 24);
  txtMinerInfo = TXT("MINFO", 320, y, 400, 80);
  txtMinerInfo->setText("Not running.");
  txtMinerInfo->setReadOnly(true);

  y += 100;
  LBL("Validator Slots: ", 20, y, 300, 24);
  LBL("Claim Slot Parameters:", 650, y, 300, 24);

  y += 30;
  tblSlots = new TableSlots(this);
  tblSlots->setBounds(20, y, 600, 300);
  addComponent(tblSlots);
  txtClaim = TXT("CLAIM", 650, y, 600, 300);
  txtClaim->setReadOnly(true);

  setSlotsNumber(16);
  setMask("53272589901149737477561849970166322707710816978043543010898806474236585144509");
  setMinDifficulty(16);
  setMinerAddress("0x137dA20bb584469D880c0361E9A3Dd58b674F512");

  /*
  CryptoPP::byte buf[1024] = {0};
  CryptoPP::Integer n("53272589901149737477561849970166322707710816978043543010898806474236585144509");
  n = -n - 1;
  size_t min_size = n.MinEncodedSize(CryptoPP::Integer::SIGNED);
  std::cout << min_size << std::endl;
  n.Encode(buf, min_size, CryptoPP::Integer::SIGNED);
  std::string bn(reinterpret_cast<char*>(buf), min_size);
  // std::cout << bin2hex(bn) << std::endl;

  y += 30;
  LBL("Mined Keys: ", 20, y, 80, 24);
  auto keys = TXT("KEYS", 100, y, 550, 400);
  keys->setMultiLine(true);
  keys->setReturnKeyStartsNewLine(true);
  addr->setInputRestrictions(0, "0123456789abcdefABCDEF");

  y += 410;
  TB("Claim", 100, y, 80, 30);
  TB("Start Miner", 200, y, 80, 30);
  */
}

Miner::~Miner() {
}

void Miner::setMask(std::string _mask) {
  CryptoPP::Integer m(_mask.c_str());
  m.Encode(mask, 32);
  txtMask->setText(_mask);
  const unsigned int UPPER = (1 << 31);
  // txtMaskHex->setText(CryptoPP::IntToString(m, UPPER | 16), false);
  txtMaskHex->setText(bin2hex(std::string(reinterpret_cast<char*>(mask), 32)));
}

void Miner::setMinDifficulty(unsigned int _minDifficulty) {
  minDifficulty = _minDifficulty;
  txtMinDifficulty->setText(String(minDifficulty), false);
  CryptoPP::Integer d(1);
  d <<= minDifficulty;
  d--;
  d <<= (256 - minDifficulty);
  d.Encode(difficulty, 32);
  // const unsigned int UPPER = (1 << 31);
  // txtMinDifficultyHex->setText(IntToString(d, UPPER | 16), false);
  txtMinDifficultyHex->setText(bin2hex(std::string(reinterpret_cast<char*>(difficulty), 32)));
}

void Miner::setMinerAddress(std::string _address) {
  if (_address.substr(0, 2) == "0x") {
    _address = _address.substr(2);
  }
  _address += "h";
  CryptoPP::Integer a(_address.c_str());

  a.Encode(minerAddress, 32);
  const unsigned int UPPER = (1 << 31);
  txtMinerAddress->setText("0x" + IntToString(a, UPPER | 16), false);
}

void Miner::setSlotsNumber(int _slotsNum) {
  _slotsNum = jmax(0, jmin(65536, _slotsNum));
  totalSlots = _slotsNum;
  txtSlotsNum->setText(String(totalSlots), false);
  initSlots();
  repaint();
  tblSlots->updateContent();
}

void Miner::initSlots() {
  unsigned char difficulty[32];
  unsigned char pk[32];
  slots.clear();

  memset(mask, 0, 32);
  memset(difficulty, 0, 32);
  difficulty[0] = 0xff;

  for (int i = 0; i < totalSlots; i++) {
    slot s;
    char b[32];
    memset(b, 0, 32);
    b[0] = 0xff;
    b[1] = 0x00;
    s.difficulty = std::string(b, 32);
    s.owner = "";
    slots.push_back(s);
  }
}

void Miner::textEditorTextChanged(TextEditor & txt) {
  if (txtMask == &txt) {
    setMask(txtMask->getText().toStdString());
  }
  if (txtMinDifficulty == &txt) {
    auto minDiff = jmax(0, jmin(48, txtMinDifficulty->getText().getIntValue()));
    setMinDifficulty(minDiff);
  }
  if (txtSlotsNum == &txt) {
    setSlotsNumber(txtSlotsNum->getText().getIntValue());
  }
  if (txtMinerAddress == & txt) {
    setMinerAddress(txtMinerAddress->getText().toStdString());
  }
}

void Miner::buttonClicked(Button* btn) {
  auto txt = btn->getButtonText();
  if (txt == "Read Contract") {
    (new ReadContractThread())->launchThread();
  }
  if (txt == "Add Miner") {
    addMinerThread();
  }
  if (txt == "Stop Miners") {
    stopMining();
  }
  if (txt == "Claim") {
    createSignature();
  }
  repaint();
}

void Miner::paint(Graphics& g) {
  g.setColour(Colours::white);
  g.setFont(32.0f);
  g.drawText("King of the Hill Miner", 0, 0, getWidth(), 40, Justification::centred);
}

void Miner::resized() {
}

void Miner::update() {
  auto cur_time = Time::getCurrentTime().toMilliseconds();
  auto delta = cur_time - last_time;
  auto delta_keys = total_keys_generated - last_keys_generated;
  if (total_keys_generated > 0) {
    txtMinerInfo->setText(
      "Total keys generated: " + String(total_keys_generated) + "\n" +
      "Mining power: " + String(delta_keys * 1000 / delta) + " keys/s\n" +
      "Claimed slots: " + String(slots_claimed) + "\n" +
      "Active miners: " + String(miners.size()));
  }
  last_time = cur_time;
  last_keys_generated = total_keys_generated;
}

void Miner::createSignature() {
  txtClaim->setText("");
  int s = tblSlots->getSelectedRow();
  if (s < 0 || s >= slots.size()) {
    return;
  }
  auto& slot = slots[s];
  std::string priv_key = slot.private_key;
  if (priv_key.size() != 32) {
    return;
  }

  std::string pub_key = gen_pub_key((unsigned char*)priv_key.c_str());
  std::string sig =
      sign(reinterpret_cast<const unsigned char*>(priv_key.c_str()), minerAddress);
  txtClaim->setText(
    "Signature: \n"
    "PubKeyX = 0x" + bin2hex(pub_key.substr(0, 32)) + " \n"
    "PubKeyY = 0x" + bin2hex(pub_key.substr(32, 32)) + " \n"
    "R = 0x" + bin2hex(sig.substr(0, 32)) + " \n"
    "S = 0x" + bin2hex(sig.substr(32, 32)) + " \n"
    "V = 0x" + bin2hex(sig.substr(64, 1)) + " \n");
}

void Miner::timerCallback() {
  update();
  repaint();
  for (auto c : components) {
    c->repaint();
  }
}
