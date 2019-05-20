#include "DemoMiner.h"

unsigned int leading_bits(unsigned int x) {
  unsigned int c = 0;
  while (x & (1 << 31)) {
    c++;
    x <<= 1;
  }
  return c;
}

Colour HSV(double h, double s, double v) {
  double hh, p, q, t, ff;
  int64 i;
  double r, g, b;

  if (s <= 0.0) {
    r = v;
    g = v;
    b = v;
    return Colour(uint8(r*255), uint8(g*255), uint8(b*255));
  }

  hh = h;
  if (hh >= 360.0) hh = 0.0;
  hh /= 60.0;
  i = static_cast<int64>(hh);
  ff = hh - i;
  p = v * (1.0 - s);
  q = v * (1.0 - (s * ff));
  t = v * (1.0 - (s * (1.0 - ff)));

  switch (i) {
  case 0:
    r = v;
    g = t;
    b = p;
    break;
  case 1:
    r = q;
    g = v;
    b = p;
    break;
  case 2:
    r = p;
    g = v;
    b = t;
    break;
  case 3:
    r = p;
    g = q;
    b = v;
    break;
  case 4:
    r = t;
    g = p;
    b = v;
    break;
  case 5:
  default:
    r = v;
    g = p;
    b = q;
    break;
  }
  return Colour(uint8(r * 255), uint8(g * 255), uint8(b * 255));
}

//==============================================================================
DemoMiner::DemoMiner() {
  setSize(800, 600);
  startTimer(10);
//  AlertWindow::showMessageBoxAsync(
//      AlertWindow::InfoIcon,
//      "leading bits",
//      "Total " + String(leading_bits(0xffffff00)));
}

DemoMiner::~DemoMiner() {
}

void DemoMiner::paint(Graphics& g) {
  int k = sz;
  int k2 = gap;
  int m = 64;
  int n = 64;
  unsigned int slots_owned = 0;
  static unsigned int my_seed;

  g.setColour(Colours::white);
  g.drawRect(19 - k2, 99 - k2, m * k + k2 + 2, n * k + k2 + 2);
  for (int x = 0; x < m; x++) {
    for (int y = 0; y < n; y++) {
      int i = y * 256 + x;
      if (slots[x][y].diff == 0) {
        slots[x][y].bg = Colours::black;
      }
      unsigned int r = rand_r(&my_seed);
      r |= 0xff000000;
      // float rr = float(r) / RAND_MAX;
      // float cc = (r >> 16) / 255.0;
      if (r > slots[x][y].diff) {
        unsigned int reward = (t - slots[x][y].tm);
        if (slots[x][y].tm != 0) {
          total_supply += reward;
        }
        if (slots[x][y].owner == 1) {
          total_balance += reward;
        }
        slots[x][y].diff = r;
        slots[x][y].owner = (rand_r(&my_seed) % 10000 < mining_power) ? 1 : 0;
        slots[x][y].tm = t;
      }
      if (leading_bits(r) > max_leading_bits) {
        max_leading_bits = leading_bits(r);
      }

      double lb = 1.0 * leading_bits(slots[x][y].diff) / max_leading_bits;
      slots[x][y].bg = HSV(200, 1.0 - lb, lb);
      g.setColour(slots[x][y].bg);
      g.fillRect(20 + x * k, 100 + y * k, k - k2, k - k2);
      if (slots[x][y].owner == 1) {
        slots_owned++;
        g.setColour(Colours::yellow);
        g.drawRect(19 + x * k, 99 + y * k, k + 1, k + 1);
      }
    }
  }

  g.setColour(Colours::white);
  g.drawMultiLineText(
      " Slots owned: " + String(slots_owned) + "\n" +
      " My Balance: " + String(total_balance) + " AUTO\n" +
      " Total Supply: " + String(total_supply) + " AUTO",
      20, 40, 500, Justification::left);
}

void DemoMiner::resized() {
}

void DemoMiner::update() {
  t++;
}

void DemoMiner::timerCallback() {
  update();
  repaint();
}
