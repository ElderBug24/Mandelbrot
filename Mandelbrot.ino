#include "M5Cardputer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "SD.h"

#include <cmath>
#include <cfloat>
#include <algorithm>
#include <vector>
#include <stdint.h>


LGFX_Sprite  buffer(&M5Cardputer.Display);
unsigned int WIDTH;
unsigned int HEIGHT;
const float  FRAME_TIME = 16.0f;
const double SCALE = 2.5 / 240.0;
const double STEP_SIZE = 16.0;

double                    frame_x = -0.75;
double                    frame_y =  0.0;
int                       frame_zoom = 0;
unsigned int              max_iterations = 100;
double                    prev_frame_x = frame_x;
double                    prev_frame_y = frame_y;
int                       prev_frame_zoom = frame_zoom;
unsigned int              prev_max_iterations = max_iterations;
double                    next_frame_x;
double                    next_frame_y;
unsigned int              color_mode = 0;
bool                      double_precision;
bool                      prev_double_precision;

unsigned int              show_coordinates;
bool                      show_axis;
bool                      show_zoom;
bool                      show_max_iterations;
bool                      show_battery;
bool                      show_verified_inside;
bool                      smoothing;
bool                      modular_coloring;
bool                      histogram_coloring;

bool                      block_render;
bool                      buffer_change;
bool                      color_change;
bool                      hud_change;
bool                      prev_buffer_change;
bool                      prev_color_change;
bool                      prev_hud_change;

bool                      julia = false;
double                    julia_x;
double                    julia_y;
double                    julia_frame_x;
double                    julia_frame_y;
int                       julia_frame_zoom;
unsigned int              julia_max_iterations = max_iterations;
bool                      julia_double_precision;

const uint16_t black = M5Cardputer.Display.color565(  0,   0,   0);
const uint16_t white = M5Cardputer.Display.color565(255, 255, 255);
const uint16_t red   = M5Cardputer.Display.color565(255,  80,  80);
const uint16_t green = M5Cardputer.Display.color565( 21, 245, 186);
const uint16_t blue  = M5Cardputer.Display.color565( 49, 111, 234);
const uint8_t PALETTES_COUNT = 5;
uint16_t palettes[PALETTES_COUNT][256];

float zoom_f, frac_1_zoom_f;
double zoom, frac_1_zoom, d_zoom, frac_1_d_zoom;
unsigned int total_iterations_counts;
unsigned int total_iterations_counts_b;
double f_x, f_y;
int f_z;
unsigned int max_it;
bool d;
std::vector<unsigned int> pixels_iteration_counts;
std::vector<unsigned int> num_iterations_per_pixel;
std::vector<unsigned int> num_iterations_per_pixel_b;
std::vector<uint8_t> smooth_color_index;
char   buf[64];
char line1[12];
char line2[12];

void drawTextWithOutline(const char* text, int x, int y, uint16_t fillColor, uint16_t outlineColor, lgfx::LovyanGFX& gfx) {
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      if (dx != 0 || dy != 0) {
        gfx.setCursor(x + dx, y + dy);
        gfx.setTextColor(outlineColor);
        gfx.print(text);
      }
    }
  }

  gfx.setCursor(x, y);
  gfx.setTextColor(fillColor);
  gfx.print(text);
}

uint16_t grayscale(float l) {
  uint8_t gray = (uint8_t)(l * 255.0f);

  return M5Cardputer.Display.color565(gray, gray, gray);
}

uint16_t grayscale_int(uint8_t l) {
  return M5Cardputer.Display.color565(l, l, l);
}

uint16_t palette_original(int i) {
  if (i < 0) i = 0;
  if (i > 255) i = 255;

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if (i < 32) {
    r = i * 8;
    g = i * 8;
    b = 127 - i * 4;
  } else if (i < 128) {
    int t = i - 32;
    r = 255;
    g = 255 - (t * 8) / 3;
    b = (t * 4) / 3;
  } else if (i < 192) {
    int t = i - 128;
    r = 255 - t * 4;
    g = 0 + t * 3;
    b = 127 - t;
  } else {
    int t = i - 192;
    r = 0;
    g = 192 - t * 3;
    b = 64 + t;
  }

  return M5Cardputer.Display.color565(r, g, b);
}

uint16_t palette_gold(int i) {
  if (i < 0) i = 0;
  if (i > 255) i = 255;

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if (i < 32) {
    r = 54 + ((i * (224 - 54)) / 32);
    g = 11 + ((i * (115 - 11)) / 32);
    b = 2 + ((i * (10 - 2)) / 32);
  } else if (i < 64) {
    int t = i - 32;
    r = 224 + ((t * (255 - 224)) / 32);
    g = 115 + ((t * (192 - 115)) / 32);
    b = 10 + ((t * (49 - 10)) / 32);
  } else if (i < 192) {
    int t = i - 64;
    r = 255;
    g = 192 + ((t * (255 - 192)) / 128);
    b = 49 + ((t * (166 - 49)) / 128);
  } else if (i < 224) {
    int t = i - 192;
    r = 255;
    g = 255 + ((t * (192 - 255)) / 32);
    b = 166 + ((t * (49 - 166)) / 32);
  } else {
    int t = i - 224;
    r = 255 + ((t * (54 - 255)) / 32);
    g = 192 + ((t * (11 - 192)) / 32);
    b = 49 + ((t * (2 - 49)) / 32);
  }

  return M5Cardputer.Display.color565(r, g, b);
}

uint16_t palette_neon(int i) {
  if (i < 0) i = 0;
  if (i > 255) i = 255;

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if (i < 32) {
    r = i * 4;
    g = 0;
    b = i * 8;
  } else if (i < 64) {
    int t = i - 32;
    r = 124 - t * 4;
    g = 0;
    b = 248 - t * 8;
  } else if (i < 96) {
    int t = i - 64;
    r = t * 8;
    g = t * 4;
    b = 0;
  } else if (i < 128) {
    int t = i - 96;
    r = 248 - t * 8;
    g = 124 - t * 4;
    b = 0;
  } else if (i < 160) {
    int t = i - 128;
    r = 0;
    g = t * 4;
    b = t * 8;
  } else if (i < 192) {
    int t = i - 160;
    r = 0;
    g = 124 - t * 4;
    b = 248 - t * 8;
  } else if (i < 224) {
    int t = i - 192;
    r = t * 4;
    g = t * 8;
    b = t * 4;
  } else {
    int t = i - 224;
    r = 124 - t * 4;
    g = 248 - t * 8;
    b = 124 - t * 4;
  }

  return M5Cardputer.Display.color565(r, g, b);
}

float cubicHermite(float p0, float p1, float m0, float m1, float t) {
  float t2 = t * t;
  float t3 = t2 * t;
  float h00 = 2*t3 - 3*t2 + 1;
  float h10 = t3 - 2*t2 + t;
  float h01 = -2*t3 + 3*t2;
  float h11 = t3 - t2;

  return h00*p0 + h10*m0 + h01*p1 + h11*m1;
}

void computeTangents(const float* positions, const uint8_t (*colors)[3], int count, float tangents[][3]) {
  for (unsigned int i = 0; i < count; i++) {
    for (unsigned int c = 0; c < 3; c++) {
      if (i == 0) {
        tangents[i][c] = (colors[i+1][c] - colors[i][c]) / (positions[i+1] - positions[i]);
      } else if (i == count-1) {
        tangents[i][c] = (colors[i][c] - colors[i-1][c]) / (positions[i] - positions[i-1]);
      } else {
        tangents[i][c] = (colors[i+1][c] - colors[i-1][c]) / (positions[i+1] - positions[i-1]);
      }
    }
  }
}

uint16_t sampleGradient(const float* positions, const uint8_t (*colors)[3], int count, float x) {
    x = std::clamp(x, 0.0f, 1.0f);

    int i = 0;
    while (i < count-1 && x > positions[i+1]) i++;

    if (i >= count-1) i = count-2;

    float t = (x - positions[i]) / (positions[i+1] - positions[i]);

    float tangents[count][3];
    computeTangents(positions, colors, count, tangents);

    uint8_t r = (uint8_t)std::clamp(cubicHermite(colors[i][0], colors[i+1][0], tangents[i][0], tangents[i+1][0], t), 0.0f, 255.0f);
    uint8_t g = (uint8_t)std::clamp(cubicHermite(colors[i][1], colors[i+1][1], tangents[i][1], tangents[i+1][1], t), 0.0f, 255.0f);
    uint8_t b = (uint8_t)std::clamp(cubicHermite(colors[i][2], colors[i+1][2], tangents[i][2], tangents[i+1][2], t), 0.0f, 255.0f);

    return M5.Display.color565(r, g, b);
}

bool almost_equal(double a, double b) {
  return fabs(a - b) < DBL_EPSILON;
}

uint16_t get_color(uint8_t l, unsigned int color_mode) {
  return palettes[color_mode][l];
}

void drawAxis(int dx, int dy, unsigned int detail, int detail_size, uint16_t color) {
  M5Cardputer.Display.drawFastVLine(dx+WIDTH/2, 0, HEIGHT, color);
  M5Cardputer.Display.drawFastHLine(0, dy+HEIGHT/2, WIDTH, color);
  if (detail >= 1) {
    int s = detail_size;
    M5Cardputer.Display.drawFastVLine(dx+WIDTH/2-s, dy+HEIGHT/2-4, 9, color);
    M5Cardputer.Display.drawFastVLine(dx+WIDTH/2+s, dy+HEIGHT/2-4, 9, color);
    M5Cardputer.Display.drawFastHLine(dx+WIDTH/2-4, dy+HEIGHT/2-s, 9, color);
    M5Cardputer.Display.drawFastHLine(dx+WIDTH/2-4, dy+HEIGHT/2+s, 9, color);
  }
  if (detail > 1) {
    int s2 = 2 * detail_size;
    M5Cardputer.Display.drawFastVLine(dx+WIDTH/2-s2, dy+HEIGHT/2-2, 5, color);
    M5Cardputer.Display.drawFastVLine(dx+WIDTH/2+s2, dy+HEIGHT/2-2, 5, color);
    M5Cardputer.Display.drawFastHLine(dx+WIDTH/2-2, dy+HEIGHT/2-s2, 5, color);
    M5Cardputer.Display.drawFastHLine(dx+WIDTH/2-2, dy+HEIGHT/2+s2, 5, color);
  }
}

void writeAxis(int dx, int dy, unsigned int detail, int detail_size, uint16_t color) {
  M5Cardputer.Display.writeFastVLine(dx+WIDTH/2, 0, HEIGHT, color);
  M5Cardputer.Display.writeFastHLine(0, dy+HEIGHT/2, WIDTH, color);
  if (detail >= 1) {
    int s = detail_size;
    M5Cardputer.Display.writeFastVLine(dx+WIDTH/2-s, dy+HEIGHT/2-4, 9, color);
    M5Cardputer.Display.writeFastVLine(dx+WIDTH/2+s, dy+HEIGHT/2-4, 9, color);
    M5Cardputer.Display.writeFastHLine(dx+WIDTH/2-4, dy+HEIGHT/2-s, 9, color);
    M5Cardputer.Display.writeFastHLine(dx+WIDTH/2-4, dy+HEIGHT/2+s, 9, color);
  }
  if (detail > 1) {
    int s2 = 2 * detail_size;
    M5Cardputer.Display.writeFastVLine(dx+WIDTH/2-s2, dy+HEIGHT/2-2, 5, color);
    M5Cardputer.Display.writeFastVLine(dx+WIDTH/2+s2, dy+HEIGHT/2-2, 5, color);
    M5Cardputer.Display.writeFastHLine(dx+WIDTH/2-2, dy+HEIGHT/2-s2, 5, color);
    M5Cardputer.Display.writeFastHLine(dx+WIDTH/2-2, dy+HEIGHT/2+s2, 5, color);
  }
}

int saveScreenshot(const char* filename) {
  int w = M5Cardputer.Display.width();
  int h = M5Cardputer.Display.height();

  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    return 1;
  }

  uint32_t fileSize = 54 + w * h * 3;

  uint8_t bmpHeader[54] = {
    'B','M',
    fileSize, fileSize>>8, fileSize>>16, fileSize>>24,
    0,0,0,0,
    54,0,0,0,
    40,0,0,0,
    w, w>>8, w>>16, w>>24,
    h, h>>8, h>>16, h>>24,
    1,0,
    24,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0
  };

  file.write(bmpHeader, 54);

  for (int y = h-1; y >= 0; y--) {
    for (int x = 0; x < w; x++) {
      RGBColor c = M5Cardputer.Display.readPixelRGB(x, y);

      uint8_t r = c.r;
      uint8_t g = c.g;
      uint8_t b = c.b;

      file.write(b);
      file.write(g);
      file.write(r);
    }
  }

  file.close();

  return 0;
}

uint16_t findHighestScreenshot() {
  uint16_t highest = 0;

  File root = SD.open("/");
  if (!root) return 0;

  File file = root.openNextFile();
  while (file) {
    String name = file.name();
    if (name.startsWith("screenshot") && name.endsWith(".bmp")) {
      uint16_t num = name.substring(10, name.length() - 4).toInt();
      if (num > highest && num <= 255) {
        highest = (uint16_t)num;
      }
    }
    file = root.openNextFile();
  }
  root.close();

  return highest;
}

TaskHandle_t workerHandle;
TaskHandle_t loopHandle;

void workerTask(void* param) {
  int x_, y_;
  unsigned int i, x, y;
  float a0f, b0f, af, bf, a2f, b2f, pxf, pyf;
  double a0, b0, a, b, a2, b2, px, py;

  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    for (x = WIDTH/2; x < WIDTH; x++) {
      for (y = 0; y < HEIGHT; y++) {
        x_ = x - WIDTH / 2;
        y_ = -y + HEIGHT / 2;
        i = 0;

        if (!d) {
          // pxf = (float)x_ * (float)SCALE * frac_1_zoom_f + (float)f_x;
          // pyf = (float)y_ * (float)SCALE * frac_1_zoom_f + (float)f_y;
          pxf = (float)((double)x_ * SCALE * frac_1_zoom + f_x);
          pyf = (float)((double)y_ * SCALE * frac_1_zoom + f_y);

          if (!julia) {
            a0f = pxf;
            b0f = pyf;

            af = 0.0f;
            bf = 0.0f;
            a2f = 0.0f;
            b2f = 0.0f;
          } else {
            a0f = (float)julia_x;
            b0f = (float)julia_y;

            af = pxf;
            bf = pyf;
            a2f = af*af;
            b2f = bf*bf;
          }

          while (a2f + b2f <= 4.0f and i < max_it) {
            a2f = af*af;
            b2f = bf*bf;
            bf = (af + af) * bf + b0f;
            af = a2f - b2f + a0f;

            i++;
          }
        } else {
          px = (double)x_ * SCALE * frac_1_zoom + f_x;
          py = (double)y_ * SCALE * frac_1_zoom + f_y;

          if (!julia) {
            a0 = px;
            b0 = py;

            a = 0.0;
            b = 0.0;
            a2 = 0.0;
            b2 = 0.0;
          } else {
            a0 = julia_x;
            b0 = julia_y;

            a = px;
            b = py;
            a2 = a*a;
            b2 = b*b;
          }

          while (a2 + b2 <= 4.0 and i < max_it) {
            a2 = a*a;
            b2 = b*b;
            b = (a + a) * b + b0;
            a = a2 - b2 + a0;

            i++;
          }

          a2f = (float)a2;
          b2f = (float)b2;
        }
        pixels_iteration_counts[y * WIDTH + x] = i;

        float l = ((float)(i + 2) - log2f(logf(a2f + b2f))) / (float)max_it;
        l = std::clamp(l, 0.0f, 1.0f);
        uint8_t color_index_smooth = (uint8_t)(l * 255.0f);
        smooth_color_index[y * WIDTH + x] = color_index_smooth;

        if (!histogram_coloring) {
          uint16_t c;
          if (i == max_it and show_verified_inside) {
            c = black;
          }
          if (i != max_it or !show_verified_inside) {
            uint8_t color_index;
            if (modular_coloring) {
              color_index = (uint8_t)(i % max_it);
            } else if (smoothing) {
              color_index = color_index_smooth;
            } else {
              color_index = (uint8_t)((float)i / (float)max_it * 255.0f);
            }

            c = get_color(color_index, color_mode);
          }
          buffer.writePixel(x, y, c);
        } else {
          num_iterations_per_pixel_b[i]++;
          total_iterations_counts_b++;
        }
      }
      M5Cardputer.Display.writeFastVLine(x, HEIGHT-2, 2, green);
    }

    xTaskNotifyGive(loopHandle);
  }
}

void setup() {
  esp_task_wdt_deinit();
  M5Cardputer.begin(M5.config());
  M5Cardputer.Power.begin();

  SD.begin;

  loopHandle = xTaskGetCurrentTaskHandle();
  xTaskCreate(workerTask, "Worker", 1024, nullptr, 1, &workerHandle);

  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Power.setBatteryCharge(false);

  WIDTH  = M5Cardputer.Display.width();
  HEIGHT = M5Cardputer.Display.height();

  buffer.createSprite(WIDTH, HEIGHT);
  buffer.setTextSize(3);
  buffer.setTextColor(red);

  buffer_change          = true;
  color_change           = true;
  hud_change             = true;
  prev_buffer_change     = buffer_change;
  prev_color_change      = color_change;
  prev_hud_change        = hud_change;

  show_axis              = false;
  show_coordinates       =     0;
  show_zoom              = false;
  show_max_iterations    = false;
  show_battery           = false;
  show_verified_inside   =  true;
  smoothing              = false;
  modular_coloring       = false;
  histogram_coloring     = false;
  double_precision       = false;
  block_render           = false;

  julia_double_precision = false;

  pixels_iteration_counts.resize(WIDTH * HEIGHT);
  num_iterations_per_pixel.resize(max_iterations+1);
  num_iterations_per_pixel_b.resize(max_iterations+1);
  std::fill(num_iterations_per_pixel.begin(), num_iterations_per_pixel.end(), 0);
  std::fill(num_iterations_per_pixel_b.begin(), num_iterations_per_pixel_b.end(), 0);

  total_iterations_counts = 0;
  total_iterations_counts_b = 0;
  smooth_color_index.resize(WIDTH * HEIGHT);

  float positions[] = {0.0f, 0.2f, 0.4f, 0.6f, 0.8f};
  int count = sizeof(positions) / sizeof(positions[0]);
  uint8_t colors[][3] = {
    {  0,   0,   0},
    { 16,  26,  88},
    {234, 181, 192},
    {255, 190, 105},
    {255,  68,  61}
  };

  for (unsigned int i = 0; i < 256; i++) {
    palettes[0][i] = grayscale_int(i);
    palettes[1][i] = sampleGradient(positions, colors, count, (float)i / 256.0f);
    palettes[2][i] = palette_original(i);
    palettes[3][i] = palette_neon(i);
    palettes[4][i] = palette_gold(i);
  }
}

void loop() {
  M5Cardputer.update();

  if (!julia) {
    f_x = frame_x;
    f_y = frame_y;
    f_z = frame_zoom;
    max_it = max_iterations;
    d = double_precision;
  } else {
    f_x = julia_frame_x;
    f_y = julia_frame_y;
    f_z = julia_frame_zoom;
    max_it = julia_max_iterations;
    d = julia_double_precision;
  }

  zoom_f = ldexpf(1.0f, f_z);
  frac_1_zoom_f = ldexpf(1.0f, -f_z);
  // zoom = ldexp(1.0, f_z);
  frac_1_zoom = ldexp(1.0, -f_z);

  if (block_render) {
    prev_buffer_change |= buffer_change;
    prev_color_change |= color_change;
    prev_hud_change |= hud_change;
  } else {
    prev_max_iterations = max_it;
    prev_double_precision = d;
  }

  M5Cardputer.Display.startWrite();
  buffer.startWrite();
  if (buffer_change) {
    if (max_it and !block_render) {
      M5Cardputer.Display.fillCircle(WIDTH-9, 9, 4, green);
      total_iterations_counts = 0;
      total_iterations_counts_b = 0;
      std::fill(num_iterations_per_pixel.begin(), num_iterations_per_pixel.end(), 0);
      std::fill(num_iterations_per_pixel_b.begin(), num_iterations_per_pixel_b.end(), 0);

      xTaskNotifyGive(workerHandle);

      for (unsigned int x = 0; x < WIDTH/2; x++) {
        for (unsigned int y = 0; y < HEIGHT; y++) {
          int x_ = x - WIDTH / 2;
          int y_ = -y + HEIGHT / 2;
          unsigned int i = 0;

          float a2f;
          float b2f;
          if (!d) {
            // float px = (float)x_ * (float)SCALE * frac_1_zoom_f + (float)f_x;
            // float py = (float)y_ * (float)SCALE * frac_1_zoom_f + (float)f_y;
            float px = (float)((double)x_ * SCALE * frac_1_zoom + f_x);
            float py = (float)((double)y_ * SCALE * frac_1_zoom + f_y);

            float a0, b0, a, b, a2, b2;
            if (!julia) {
              a0 = px;
              b0 = py;

              a = 0.0f;
              b = 0.0f;
              a2 = 0.0f;
              b2 = 0.0f;
            } else {
              a0 = (float)julia_x;
              b0 = (float)julia_y;

              a = px;
              b = py;
              a2 = a*a;
              b2 = b*b;
            }

            while (a2 + b2 <= 4.0f and i < max_it) {
              a2 = a*a;
              b2 = b*b;
              b = (a + a) * b + b0;
              a = a2 - b2 + a0;

              i++;
            }

            a2f = a2;
            b2f = b2;
          } else {
            double px = (double)x_ * SCALE * frac_1_zoom + f_x;
            double py = (double)y_ * SCALE * frac_1_zoom + f_y;

            double a0, b0, a, b, a2, b2;
            if (!julia) {
              a0 = px;
              b0 = py;

              a = 0.0;
              b = 0.0;
              a2 = 0.0;
              b2 = 0.0;
            } else {
              a0 = julia_x;
              b0 = julia_y;

              a = px;
              b = py;
              a2 = a*a;
              b2 = b*b;
            }

            while (a2 + b2 <= 4.0 and i < max_it) {
              a2 = a*a;
              b2 = b*b;
              b = (a + a) * b + b0;
              a = a2 - b2 + a0;

              i++;
            }

            a2f = (float)a2;
            b2f = (float)b2;
          }
          pixels_iteration_counts[y * WIDTH + x] = i;

          float l = ((float)(i + 2) - log2f(logf(a2f + b2f))) / (float)max_it; // log is expensive and smoothing not very useful
          l = std::clamp(l, 0.0f, 1.0f);
          uint8_t color_index_smooth = (uint8_t)(l * 255.0f);
          smooth_color_index[y * WIDTH + x] = color_index_smooth;

          if (!histogram_coloring) {
            uint16_t c;
            if (i == max_it and show_verified_inside) {
              c = black;
            }
            if (i != max_it or !show_verified_inside) {
              uint8_t color_index;
              if (modular_coloring) {
                color_index = (uint8_t)(i % max_it);
              } else if (smoothing) {
                color_index = color_index_smooth;
              } else {
                color_index = (uint8_t)((float)i / (float)max_it * 255.0f);
              }

              c = get_color(color_index, color_mode);
            }
            buffer.writePixel(x, y, c);
          } else {
            num_iterations_per_pixel[i]++;
            total_iterations_counts++;
          }
        }
        M5Cardputer.Display.writeFastVLine(WIDTH/2-x, HEIGHT-2, 2, green);
      }

      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      total_iterations_counts += total_iterations_counts_b;

      if (histogram_coloring) {
        M5Cardputer.Display.fillCircle(WIDTH-9, 9, 4, white);
        for (unsigned int x = 0; x < WIDTH; x++) {
          for (unsigned int y = 0; y < HEIGHT; y++) {
            unsigned int iterations = pixels_iteration_counts[y * WIDTH + x];
            uint16_t c;

            bool skip = false;
            if (iterations == max_it) {
              if (show_verified_inside) {
                c = black;
                skip = true;
              } else {
                iterations = max_it - 1;
              }
            }
            if (!skip) {
              float l = 0.0f;
              for (unsigned int i = 0; i <= iterations; i++) {
                l += (float)num_iterations_per_pixel[i];
                l += (float)num_iterations_per_pixel_b[i];
              }
              l = l / total_iterations_counts;

              uint8_t color_index = (uint8_t)(l * 255.0f);
              c = get_color(color_index, color_mode);
            }

            buffer.writePixel(x, y, c);
          }
        }
      }
    } else if (!block_render) {
      buffer.fillScreen(black);
      buffer.drawString("[No Render]", WIDTH / 2 - buffer.textWidth("[No Render]") / 2, HEIGHT / 2 - 10);
    }
  } else if (color_change) {
    M5Cardputer.Display.fillCircle(WIDTH-9, 9, 4, white);
    if (!histogram_coloring) {
      for (unsigned int x = 0; x < WIDTH; x++) {
        for (unsigned int y = 0; y < HEIGHT; y++) {
          unsigned int iterations = pixels_iteration_counts[y * WIDTH + x];
          uint16_t c;

          if (iterations == prev_max_iterations and show_verified_inside) {
            c = black;
          }
          if (iterations != max_it or !show_verified_inside) {
            uint8_t color_index;
            if (modular_coloring) {
              color_index = (uint8_t)(iterations % prev_max_iterations);
            } else if (smoothing) {
              color_index = smooth_color_index[y * WIDTH + x];
            } else {
              color_index = (uint8_t)((float)iterations / (float)prev_max_iterations * 255.0f);
            }

            c = get_color(color_index, color_mode);
          }
          buffer.writePixel(x, y, c);
        }
      }
    } else {
      total_iterations_counts = 0;
      std::fill(num_iterations_per_pixel.begin(), num_iterations_per_pixel.end(), 0);
      for (unsigned int x = 0; x < WIDTH; x++) {
        for (unsigned int y = 0; y < HEIGHT; y++) {
          unsigned int iterations = pixels_iteration_counts[y * WIDTH + x];
          num_iterations_per_pixel[iterations]++;
          total_iterations_counts++;
        }
      }

      for (unsigned int x = 0; x < WIDTH; x++) {
        for (unsigned int y = 0; y < HEIGHT; y++) {
          unsigned int iterations = pixels_iteration_counts[y * WIDTH + x];
          uint16_t c;

          bool skip = false;
          if (iterations == prev_max_iterations) {
            if (show_verified_inside) {
              c = black;
              skip = true;
            } else {
              iterations = prev_max_iterations - 1;
            }
          }
          if (!skip) {
            float l = 0.0f;
            for (unsigned int i = 0; i <= iterations; i++) {
              l += (float)num_iterations_per_pixel[i];
            }
            l = l / total_iterations_counts;

            uint8_t color_index = (uint8_t)(l * 255.0f);
            c = get_color(color_index, color_mode);
          }

          buffer.writePixel(x, y, c);
        }
      }
    }
  }

  double r = STEP_SIZE * frac_1_zoom;

  if (hud_change) {
    buffer.pushSprite(0, 0);

    if (block_render) {
      double d_frame_x = f_x - prev_frame_x;
      double d_frame_y = f_y - prev_frame_y;
      int d_frame_zoom = f_z - prev_frame_zoom;
      d_zoom = ldexp(1.0, d_frame_zoom);
      frac_1_d_zoom = ldexp(1.0, -d_frame_zoom);
      int dx = (int)(ldexp(d_frame_x / SCALE, prev_frame_zoom));
      int dy = (int)(ldexp(-d_frame_y / SCALE, prev_frame_zoom));

      if (show_axis) {
        writeAxis(0, 0, 0, 0, white);
      }
      writeAxis(dx, dy, 2, STEP_SIZE * frac_1_d_zoom, green);

      if (show_zoom) {
        if (d_frame_zoom >= 0) {
          snprintf(buf, sizeof(buf), "x%.0f", d_zoom);
        } else {
          snprintf(buf, sizeof(buf), "/%.0f", frac_1_d_zoom);
        }
        drawTextWithOutline(buf, 5, HEIGHT - 40, black, white, M5Cardputer.Display);
      }

      if (d_frame_zoom > 0) {
        int w = (int)((double)WIDTH * frac_1_d_zoom);
        int wh = (int)((double)WIDTH * frac_1_d_zoom / 2.0);
        int h = (int)((double)HEIGHT * frac_1_d_zoom);
        int hh = (int)((double)HEIGHT * frac_1_d_zoom / 2.0);
        M5Cardputer.Display.writeFastHLine(WIDTH/2 + dx - wh, HEIGHT/2 + dy - hh, w, green);
        M5Cardputer.Display.writeFastHLine(WIDTH/2 + dx - wh, HEIGHT/2 + dy + hh, w+(int)(d_frame_zoom == 3), green);
        M5Cardputer.Display.writeFastVLine(WIDTH/2 + dx - wh, HEIGHT/2 + dy - hh, h, green);
        M5Cardputer.Display.writeFastVLine(WIDTH/2 + dx + wh, HEIGHT/2 + dy - hh, h, green);
      }
    }

    if (show_axis and !block_render) {
      writeAxis(0, 0, 2, STEP_SIZE, white);
    }

    if (show_coordinates) {
      if (show_coordinates == 1) {
        snprintf(buf, sizeof(buf), "re: %.12f", f_x);
        drawTextWithOutline(buf, 5, 5, black, white, M5Cardputer.Display);

        snprintf(buf, sizeof(buf), "im: %.12f", f_y);
        drawTextWithOutline(buf, 5, 25, black, white, M5Cardputer.Display);
      } else if (show_coordinates == 2) {
        snprintf(buf, sizeof(buf), "%.20f", f_x);
        memcpy(line1, buf, 11);
        line1[11] = '\0';
        memcpy(line2, buf + 11, 11);
        line2[11] = '\0';
        snprintf(buf, sizeof(buf), "re: %s", line1);
        drawTextWithOutline(buf, 5, 5, black, white, M5Cardputer.Display);
        snprintf(buf, sizeof(buf), "%s", line2);
        drawTextWithOutline(buf, 5 + M5Cardputer.Display.textWidth("re: "), 25, black, white, M5Cardputer.Display);

        snprintf(buf, sizeof(buf), "%.20f", f_y);
        memcpy(line1, buf, 11);
        line1[11] = '\0';
        memcpy(line2, buf + 11, 11);
        line2[11] = '\0';
        snprintf(buf, sizeof(buf), "im: %s", line1);
        drawTextWithOutline(buf, 5, 45, black, white, M5Cardputer.Display);
        snprintf(buf, sizeof(buf), "%s", line2);
        drawTextWithOutline(buf, 5 + M5Cardputer.Display.textWidth("im: "), 65, black, white, M5Cardputer.Display);
      }
    }

    if (show_zoom) {
      if (f_z >= 0) {
        snprintf(buf, sizeof(buf), "x%.0f", zoom_f);
      } else {
        snprintf(buf, sizeof(buf), "/%.0f", frac_1_zoom_f);
      }
      drawTextWithOutline(buf, 5, HEIGHT - 20, black, white, M5Cardputer.Display);
    }

    if (show_max_iterations) {
      snprintf(buf, sizeof(buf), "%u%s%s%s%s", max_it, (smoothing and !histogram_coloring and !modular_coloring) ? "+S" : "", (modular_coloring and !histogram_coloring) ? "+M" : "", histogram_coloring ? "+H" : "", d ? "+D" : "");
      drawTextWithOutline(buf, WIDTH - M5Cardputer.Display.textWidth(buf) - 5, HEIGHT - 20, black, white, M5Cardputer.Display);
    }

    if (show_battery) {
      int battery = M5Cardputer.Power.getBatteryLevel();
      int y = HEIGHT - 15;
      if (show_max_iterations) {
        y -= 25;
      }

      M5Cardputer.Display.fillRect(WIDTH-34, y, 25, 12, white);
      M5Cardputer.Display.fillRect(WIDTH-9, y+4, 4, 4, white);
      M5Cardputer.Display.fillRect(WIDTH-33, y+1, 23, 10, black);
      M5Cardputer.Display.fillRect(WIDTH-10, y+5, 4, 2, black);

      int n = (int)((float)battery / 100.0f * 27.0f);
      M5Cardputer.Display.fillRect(WIDTH-33, y+1, min(n, 23), 10, green);
      M5Cardputer.Display.fillRect(WIDTH-10, y+5, max(n - 23, 0), 2, green);

      // if (M5Cardputer.Power.isCharging()) {
      //   M5Cardputer.Display.setTextColor(green);
      //   M5Cardputer.Display.setTextSize(1);
      //   M5Cardputer.Display.drawChar(43, WIDTH-8, y-3);
      //   M5Cardputer.Display.setTextSize(2);
      // }
    }

    if (block_render) {
      M5Cardputer.Display.fillCircle(WIDTH-9, 9, 4, red);
    }
  }

  buffer.endWrite();
  M5Cardputer.Display.endWrite();

  hud_change = false;
  color_change = false;
  buffer_change = false;
  next_frame_x = f_x;
  next_frame_y = f_y;

  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isKeyPressed('a')) { // show axis
      show_axis = !show_axis;

      hud_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('c')) { // show coordinates
      show_coordinates = (show_coordinates + 1) % 3;

      hud_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('z')) { // show zoom
      show_zoom = !show_zoom;

      hud_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('i')) { // show max iterations
      show_max_iterations = !show_max_iterations;

      hud_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('b')) { // show battery
      show_battery = !show_battery;

      hud_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('v')) { // show verified inside
      show_verified_inside = !show_verified_inside;

      color_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('[')) { // decr max iterations
      if (!julia) {
        if (max_iterations >= 50) {
          max_iterations -= 50;
          num_iterations_per_pixel.resize(max_iterations+1);
          num_iterations_per_pixel_b.resize(max_iterations+1);

          buffer_change = true;
        }
      } else {
        if (julia_max_iterations >= 50) {
          julia_max_iterations -= 50;
          num_iterations_per_pixel.resize(julia_max_iterations+1);
          num_iterations_per_pixel_b.resize(julia_max_iterations+1);

          buffer_change = true;
        }
      }
    } else if (M5Cardputer.Keyboard.isKeyPressed(']')) { // incr max iterations
      if (!julia) {
        max_iterations += 50;
        num_iterations_per_pixel.resize(max_iterations+1);
        num_iterations_per_pixel_b.resize(max_iterations+1);
      } else {
        julia_max_iterations += 50;
        num_iterations_per_pixel.resize(julia_max_iterations+1);
        num_iterations_per_pixel_b.resize(julia_max_iterations+1);
      }

      buffer_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('s')) { // smoothing
      smoothing = !smoothing;

      color_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('m')) { // modular coloring
      modular_coloring = !modular_coloring;

      color_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('h')) { // histogram coloring
      histogram_coloring = !histogram_coloring;

      color_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('d')) { // double precision
      if (!julia) {
        double_precision = !double_precision;
      } else {
        julia_double_precision = !julia_double_precision;
      }

      buffer_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('p')) { // next color mode / palette
      color_mode = (color_mode + 1) % PALETTES_COUNT;

      color_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('o')) { // prev color mode / palette
      color_mode = (color_mode - 1 + PALETTES_COUNT) % PALETTES_COUNT;

      color_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed(' ')) { // block render
      block_render = !block_render;
      if (block_render) {
        prev_frame_x = f_x;
        prev_frame_y = f_y;
        prev_frame_zoom = f_z;

        prev_buffer_change = false;
        prev_color_change = false;
        hud_change = true;
      } else {
        bool same = almost_equal(prev_frame_x, f_x) and almost_equal(prev_frame_y, f_y) and prev_frame_zoom == f_z and prev_double_precision == d and prev_max_iterations == max_it;
        if (same) {
          prev_buffer_change = false;
          prev_color_change = true;
          prev_hud_change = true;
        }
        buffer_change = prev_buffer_change;
        color_change = prev_color_change;
        hud_change = prev_hud_change;
      }
    } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) { // cancel block render
      if (block_render) {
        if (!julia) {
          frame_x = prev_frame_x;
          frame_y = prev_frame_y;
          frame_zoom = prev_frame_zoom;
          double_precision = prev_double_precision;
          max_iterations = prev_max_iterations;
        } else {
          julia_frame_x = prev_frame_x;
          julia_frame_y = prev_frame_y;
          julia_frame_zoom = prev_frame_zoom;
          julia_double_precision = prev_double_precision;
          julia_max_iterations = prev_max_iterations;
        }

        buffer_change = false;
        color_change = false;
        hud_change = true;

        block_render = false;
      }
    } else if (M5Cardputer.Keyboard.isKeyPressed('q')) { // screenshot
      uint16_t n = findHighestScreenshot();
      char filename[32];
      snprintf(filename, sizeof(filename), "/screenshot%u.bmp", n+1);

      int r = saveScreenshot(filename);
      M5Cardputer.Display.fillCircle(WIDTH-9, 9, 4, blue);
    } else if (M5Cardputer.Keyboard.isKeyPressed('j')) { // Julia mode
      julia = !julia;
      if (julia) {
        julia_x = frame_x;
        julia_y = frame_y;
        julia_frame_x = 0.0;
        julia_frame_y = 0.0;
        julia_frame_zoom = 0;
        num_iterations_per_pixel.resize(julia_max_iterations+1);
        num_iterations_per_pixel_b.resize(julia_max_iterations+1);
      } else {
        num_iterations_per_pixel.resize(max_iterations+1);
        num_iterations_per_pixel_b.resize(max_iterations+1);
      }

      buffer_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('r')) { // reset zoom
      if (!julia) {
        frame_zoom = 0;
      } else {
        julia_frame_zoom = 0;
      }

      buffer_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('=')) { // + zoom in
      if (!julia) {
        frame_zoom += 1;
      } else {
        julia_frame_zoom += 1;
      }

      buffer_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('-')) { // - zoom out
      if (!julia) {
        frame_zoom -= 1;
      } else {
        julia_frame_zoom -= 1;
      }

      buffer_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('/')) { // right
      if (!julia) {
        frame_x += r * SCALE;
      } else {
        julia_frame_x += r * SCALE;
      }

      buffer_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed(',')) { // left
      if (!julia) {
        frame_x -= r * SCALE;
      } else {
        julia_frame_x -= r * SCALE;
      }

      buffer_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed(';')) { // up
      if (!julia) {
        frame_y += r * SCALE;
      } else {
        julia_frame_y += r * SCALE;
      }

      buffer_change = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('.')) { // down
      if (!julia) {
        frame_y -= r * SCALE;
      } else {
        julia_frame_y -= r * SCALE;
      }

      buffer_change = true;
    } else {
      if (M5Cardputer.Keyboard.isKeyPressed('`')) {
        next_frame_x = 0.0;
        next_frame_y = 0.0;
      }
      if (!julia) {
        if (M5Cardputer.Keyboard.isKeyPressed('1')) { // points
          next_frame_x = -0.75;
          next_frame_y = 0.0;
        } else if (M5Cardputer.Keyboard.isKeyPressed('2')) {
          next_frame_x = -0.1528459638;
          next_frame_y = 1.0396947861;
        } else if (M5Cardputer.Keyboard.isKeyPressed('3')) {
          next_frame_x = -0.5937632322;
          next_frame_y = -0.4961385429;
        } else if (M5Cardputer.Keyboard.isKeyPressed('4')) {
          next_frame_x = -0.9943835735;
          next_frame_y = 0.2997867465;
        } else if (M5Cardputer.Keyboard.isKeyPressed('5')) {
          next_frame_x = -0.7981594801;
          next_frame_y = 0.1794066280;
        } else if (M5Cardputer.Keyboard.isKeyPressed('6')) {
          // next_frame_x = -0.6367543340;
          // next_frame_y = -0.6850312948;
          next_frame_x = 0.26711215318732706159;
          next_frame_y = -0.0043170512292552603;
        } else if (M5Cardputer.Keyboard.isKeyPressed('7')) {
          next_frame_x = -0.743517833;
          next_frame_y = 0.127094578;
        } else if (M5Cardputer.Keyboard.isKeyPressed('8')) {
          next_frame_x = -0.3438543964986979;
          next_frame_y = -0.6112538802506510;
        } else if (M5Cardputer.Keyboard.isKeyPressed('9')) {
          next_frame_x = -1.7687788000474986560;
          next_frame_y = 0.00173890994736915347;
        } else if (M5Cardputer.Keyboard.isKeyPressed('0')) {
          next_frame_x = -1.0069455230908423981;
          next_frame_y = 0.31240071852809647712;
        }

        if (!almost_equal(frame_x, next_frame_x)) {
          frame_x = next_frame_x;

          buffer_change = true;
        }
        if (!almost_equal(frame_y, next_frame_y)) {
          frame_y = next_frame_y;

          buffer_change = true;
        }
      } else {
        if (!almost_equal(julia_frame_x, next_frame_x)) {
          julia_frame_x = next_frame_x;

          buffer_change = true;
        }
        if (!almost_equal(julia_frame_y, next_frame_y)) {
          julia_frame_y = next_frame_y;
          buffer_change = true;
        }
      }
    }
  }

  color_change |= buffer_change;
  hud_change |= color_change;

  delay(FRAME_TIME);
}

