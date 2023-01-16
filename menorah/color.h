class Color {
public:
  int r, g, b;
  Color(int red, int green, int blue) {
    r = min(max(red, 0), 255);;
    g = min(max(green, 0), 255);;
    b = min(max(blue, 0), 255);;
  }

  static Color HSV(int h, int s, int v) {
    h = (h < 0 ? -h : h) % 360;  // Clamp h to [0, 360]
    s = min(max(s, 0), 255);     // Clamp s to [0, 255]
    v = min(max(v, 0), 255);     // Clap v to [0, 255]
    int c = v * s / 255;
    int m = v - c;

    if (h < 60)
      return Color(c + m, c * h / 60 + m, m);
    else if (h < 120)
      return Color(2 * c - c * h / 60 + m, c + m, m);
    else if (h < 180)
      return Color(m, c + m, c * (h - 120) / 60 + m);
    else if (h < 240)
      return Color(m, 2 * c - c * (h - 120) / 60 + m, c + m);
    else if (h < 300)
      return Color(c * (h - 240) / 60 + m, m, c + m);
    else
      return Color(c + m, m, 2 * c - c * (h - 240) / 60 + m);
  }
};