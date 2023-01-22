int randomRange(int a, int b);

class Color {
public:
  int r, g, b;
  Color(int red, int green, int blue) {
    r = min(max(red, 0), 255);
    g = min(max(green, 0), 255);
    b = min(max(blue, 0), 255);
  }

  Color() {
    // create black
    r = g = b = 0;
  }

  // Define a few nice color constants;
  static Color* BLACK() {
    return new Color(0, 0, 0);
  }
  static Color* WHITE() {
    return new Color(255, 255, 255);
  }
  static Color* RED() {
    return new Color(255, 0, 0);
  }
  static Color* GREEN() {
    return new Color(0, 255, 0);
  }
  static Color* BLUE() {
    return new Color(0, 0, 255);
  }
  static Color* YELLOW() {
    return new Color(255, 255, 0);
  }
  static Color* MAGENTA() {
    return new Color(255, 0, 255);
  }
  static Color* CYAN() {
    return new Color(0, 255, 255);
  }

  static Color* HSV(int h, int s, int v) {
    Color* c = new Color();
    c->setHSV(h, s, v);
    return c;
  }

  void set(int red, int green, int blue) {
    r = red;
    g = green;
    b = blue;
  }

  virtual int red() {
    return r;
  }

  virtual int green() {
    return g;
  }

  virtual int blue() {
    return b;
  }

  void setHSV(int hue, int sat, int value) {
    int h = hue % 360 + (hue < 0 ? 360 : 0);  // Clamp h to [0, 360]
    int s = min(max(sat, 0), 255);     // Clamp s to [0, 255]
    int v = min(max(value, 0), 255);     // Clap v to [0, 255]
    int c = v * s / 255;
    int m = v - c;

    if (h < 60) {
      r = c + m;
      g = c * h / 60 + m;
      b = m;
    } else if (h < 120) {
      r = 2 * c - c * h / 60 + m;
      g = c + m;
      b = m;
    } else if (h < 180) {
      r = m;
      g = c + m;
      b = c * (h - 120) / 60 + m;
    } else if (h < 240) {
      r = m;
      g = 2 * c - c * (h - 120) / 60 + m;
      b = c + m;
    } else if (h < 300) {
      r = c * (h - 240) / 60 + m;
      g = m;
      b = c + m;
    } else {
      r = c + m;
      g = m;
      b = 2 * c - c * (h - 240) / 60 + m;
    }
  }
};

class Flicker : public Color {
public:
  Flicker() {
    setHSV(30, 245, 75);
  }

  virtual void flicker() {
    int h = randomRange(0, 60);  // Red is 0 and yellow is 60
    int s = randomRange(220, 240);
    int v = randomRange(220, 255);
    setHSV(h, s, v);
  }

  int red() override {
    flicker();
    return r;
  }
};

class FlickerShamash : public Flicker {
public:
  FlickerShamash() {
    setHSV(30, 245, 75);
  }

  void flicker() override {
    int h = randomRange(0, 60);  // Red is 0 and yellow is 60
    int s = randomRange(220, 240);
    int v = randomRange(60, 90);
    setHSV(h, s, v);
  }
};

int randomRange(int a, int b) {
  return random(b - a) + a;
}

class Rainbow : public Color {
public:
  int h;
  Rainbow(int phase = 0) {
    h = phase;
    setHSV(phase, 255, 255);
  }

  void rotate() {
    setHSV(h++, 255, 255);
  }

  int red() override {
    rotate();
    return r;
  }
};