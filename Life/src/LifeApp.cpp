#include <array>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include "cinder/Timer.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

template <int MAX, typename T> inline T wrap_map(const T &x) {
  if (x < 0)
    return (MAX + x);
  if (x >= MAX)
    return (x - MAX);
  return x;
}

using namespace std;
using namespace ci;
using namespace ci::app;

class LifeApp : public App {
public:
#ifdef NDEBUG
  static const size_t map_height = 6400;
  static const size_t map_width = 6400;
#else
  static const size_t map_height = 300;
  static const size_t map_width = 600;
#endif
private:
  array<vector<int>, 2> map_cells;
  size_t read_idx, write_idx;

  unsigned int generation_count;

  bool is_updating;
  bool is_moving;
  bool is_benchmarking;
  string update_mode_name;

  function<void(void)> update_func;

  ivec2 last_mouse_pos; // int (x, y)
  ivec2 view_origin;
  ivec2 view_size;
  const int header_height; // Pixels
  int cell_size;
  Font text_font;

  void mouseDown(MouseEvent event);
  void mouseDrag(MouseEvent event);
  void mouseUp(MouseEvent event);
  void mouseWheel(MouseEvent event);
  void keyDown(KeyEvent event);

  vector<ivec2> load_creature(fs::fstream &fin);
  void populate_map(const fs::path &app_path);
  void draw_header() const;
  void refresh_map();

  template <typename Func> void draw_map(Func draw_if_pred) const {
    const array<const Color, 2> color_mapper = {Color::black(), Color::white()};
    for (int y = view_origin.y; y < (view_origin.y + view_size.y); ++y) {
      const float screen_y =
          float(header_height + (y - view_origin.y) * cell_size);
      for (int x = view_origin.x; x < (view_origin.x + view_size.x); ++x) {
        if (draw_if_pred(map_cells[read_idx][y * map_width + x],
                         map_cells[write_idx][y * map_width + x])) {
          gl::color(color_mapper[map_cells[read_idx][y * map_width + x]]);
          const float screen_x = float(x - view_origin.x) * cell_size;
          const auto cell_rect = Rectf(screen_x, screen_y, screen_x + cell_size,
                                       screen_y + cell_size);
          gl::drawSolidRect(cell_rect);
        }
      }
    }
  }

  void zoom_view(int new_cell_size);
  void resize_window();

  void update_cpu();
  void update_amp();
  void update_amp_tiled();

public:
  void setup();
  void update();
  void draw();

  LifeApp()
      : read_idx(0), write_idx(1), generation_count(1), is_updating(false),
        is_moving(false), is_benchmarking(false),
        update_func(bind(&LifeApp::update_cpu, this)), update_mode_name("CPU"),
        view_origin(0, 0), view_size(300, 160), header_height(50),
        cell_size(4) {
    map_cells[read_idx] = vector<int>(map_height * map_width, 0);
    map_cells[write_idx] = vector<int>(map_height * map_width, 0);
  }

  int read_map(const int y, const int x) const {
    return map_cells[read_idx][wrap_map<map_height>(y) * map_width +
                               wrap_map<map_width>(x)];
  }
};

// Cinder: Setup application

void LifeApp::setup() {
  populate_map(cinder::app::getAppPath());
  setWindowSize(view_size.x * cell_size,
                header_height + view_size.y * cell_size);
  text_font = Font("Courier New", 24.0f);
  gl::clear(Color::black());
}

// Cinder I/O event handlers

void LifeApp::mouseDown(MouseEvent event) {
  is_moving = true;
  last_mouse_pos = event.getPos();
}

void LifeApp::mouseDrag(MouseEvent event) {
  auto pos = event.getPos();
  view_origin += (last_mouse_pos - pos) / cell_size;
  view_origin.x = clamp(view_origin.x, 0, int(map_width) - view_size.x);
  view_origin.y = clamp(view_origin.y, 0, int(map_height) - view_size.y);
  last_mouse_pos = pos;
}

void LifeApp::mouseUp(MouseEvent event) { is_moving = false; }

void LifeApp::mouseWheel(MouseEvent event) {
  if (event.getWheelIncrement() > 0.0f)
    zoom_view(cell_size >> 1);
  if (event.getWheelIncrement() < 0.0f)
    zoom_view(cell_size << 1);
  refresh_map();
}

void LifeApp::keyDown(KeyEvent event) {
  switch (event.getCode()) {
  case KeyEvent::KEY_b: // Toggle benchmark mode
    if (is_benchmarking)
      refresh_map();
    is_benchmarking = !is_benchmarking;
    break;
  case KeyEvent::KEY_q: // Quit.
    quit();
    break;
  case KeyEvent::KEY_r: // Reset.
    is_updating = false;
    populate_map(getAppPath());
    refresh_map();
    break;
  case KeyEvent::KEY_s: // Start/Stop.
    is_updating = !is_updating;
    break;
  case KeyEvent::KEY_1: // CPU mode.
    update_func = bind(&LifeApp::update_cpu, this);
    update_mode_name = "CPU      ";
    break;
  case KeyEvent::KEY_UP: // Zoom in.
    zoom_view(cell_size << 1);
    refresh_map();
    break;
  case KeyEvent::KEY_DOWN: // Zoom out.
    zoom_view(cell_size >> 1);
    refresh_map();
    break;
  }
}

// Cinder: Update state

void LifeApp::update() {
  if (!is_updating || is_moving)
    return;

  update_func();

  ++generation_count;
  swap(read_idx, write_idx);
}

// 1.Any live cell with fewer than two live neighbors dies, as if caused by
// under-population.
// 2 Any live cell with two or three live neighbors lives on to the next
// generation.
// 3.Any live cell with more than three live neighbors dies, as if by
// overcrowding.
// 4.Any dead cell with exactly three live neighbors becomes a live cell, as if
// by reproduction.

int update_cell(const int value, const int neighbors) {
  // const int cell_mapper[2][9] = { { 0, 0, 0, 1, 0, 0, 0, 0, 0 }, { 0, 0, 1,
  // 1, 0, 0, 0, 0, 0 } };
  // return cell_mapper[value][neighbors];

  if (value == 0)
    return (neighbors == 3) ? 1 : 0;
  return (neighbors == 2 || neighbors == 3) ? 1 : 0;
}

void LifeApp::update_cpu() {
  for (int y = 0; y < map_height; ++y) {
    const int top = y - 1;
    const int btm = y + 1;
    for (int x = 0; x < map_width; ++x) {
      const int left = x - 1;
      const int right = x + 1;
      const int neighbors = read_map(top, left) + read_map(top, x) +
                            read_map(top, right) + read_map(y, left) +
                            read_map(y, right) + read_map(btm, left) +
                            read_map(btm, x) + read_map(btm, right);
      map_cells[write_idx][y * map_width + x] =
          update_cell(read_map(y, x), neighbors);
    }
  }
}

// Cinder: Draw UI

void LifeApp::draw() {
  draw_header();

  if (is_updating && !is_moving && !is_benchmarking) {
    draw_map([=](const char new_value, const char old_value) {
      return (new_value != old_value);
    });
    return;
  }
  if (is_moving)
    refresh_map();
}

// Helper functions

vector<ivec2> LifeApp::load_creature(fs::fstream &fin) {
  vector<ivec2> result;
  char ch;
  int y = 0, x = 0;
  while (fin >> noskipws >> ch) {
    switch (ch) {
    case '#':
      result.push_back(ivec2(x, y));
    case ' ':
      ++x;
      break;
    case '\n':
      ++y;
      x = 0;
      break;
    default:
      break;
    }
  }
  return result;
}

void LifeApp::populate_map(const fs::path &app_path) {
  const int creature_count = map_height * map_width / 1600;

  vector<fs::path> paths;
  copy_if(fs::directory_iterator(app_path), fs::directory_iterator(),
          back_inserter(paths), [=](const fs::path &p) {
            return fs::is_regular_file(p) &&
                   (p.extension().compare(fs::path(".life")) == 0);
          });
  if (paths.size() == 0)
    return;

  vector<vector<ivec2>> creature_library(paths.size());
  transform(begin(paths), end(paths), begin(creature_library),
            [=](const fs::path &p) {
              fs::fstream fin(p.c_str(), fs::fstream::in);
              cout << "  " << p.filename() << "...\n";
              return load_creature(fin);
            });

  random_device rnd_dev;
  mt19937 rnd_gen(rnd_dev());
  uniform_int_distribution<size_t> creature_dist(0,
                                                 creature_library.size() - 1);
  uniform_int_distribution<int> width_dist(0, map_width - 1);
  uniform_int_distribution<int> height_dist(0, map_height - 1);

  for (auto &map : map_cells)
    fill(begin(map), end(map), 0);

  for (int i = 0; i < creature_count; ++i) {
    vector<ivec2> creature_definition =
        creature_library[creature_dist(rnd_gen)];
    ivec2 pos(width_dist(rnd_gen), height_dist(rnd_gen));
    for (auto &cell : creature_definition)
      map_cells[read_idx][wrap_map<map_height>(pos.y + cell.y) * map_width +
                          wrap_map<map_width>(pos.x + cell.x)] = 1;
  }
  generation_count = 0;
}

void LifeApp::draw_header() const {
  gl::color(Color::black());
  gl::drawSolidRect(
      Rectf(vec2(0.0f, 0.0f), vec2(getWindowSize().x, header_height)));
  stringstream buf;
  buf << "Framerate: " << fixed << setprecision(1) << setw(5) << getAverageFps()
      << "       Generation: " << generation_count << "       "
      << update_mode_name << " "
      << (is_benchmarking ? "Benchmark" : "         ");
  gl::drawString(buf.str(), vec2(10.0f, 5.0f), Color::white(), text_font);
  buf.str("");
  buf.clear();
  buf << "Area: [ " << map_width << " x " << map_height << " ]  View: [ "
      << setfill(' ') << setw(4) << view_origin.x << " - " << setw(4)
      << (view_origin.x + view_size.x) << " x " << setw(4) << view_origin.y
      << " - " << setw(4) << (view_origin.y + view_size.y) << " ] Zoom: x"
      << setw(2) << cell_size;
  gl::drawString(buf.str(), vec2(10.0f, 30.0f), Color::white(), text_font);
}

void LifeApp::refresh_map() {
  gl::color(Color::black());
  gl::drawSolidRect(
      Rectf(vec2(0.0f, float(header_height)), vec2(getWindowSize())));
  draw_map([=](const char new_value, const char old_value) {
    return (new_value != 0);
  });
}

void LifeApp::zoom_view(int new_cell_size) {
  new_cell_size = clamp(new_cell_size, 1, 32);
  ivec2 view_center = view_origin + (view_size / 2);
  ivec2 window_size = getWindowSize();
  window_size.y -= header_height;
  view_size = window_size / new_cell_size;
  view_size.x = clamp(view_size.x, 10, int(map_width));
  view_size.y = clamp(view_size.y, 10, int(map_height));
  view_origin = view_center - (view_size / 2);
  view_origin.x = clamp(view_origin.x, 0, int(map_width) - view_size.x);
  view_origin.y = clamp(view_origin.y, 0, int(map_height) - view_size.y);
  cell_size = new_cell_size;
}

void LifeApp::resize_window() {
  zoom_view(cell_size);
  setWindowSize(view_size.x * cell_size,
                header_height + view_size.y * cell_size);
  refresh_map();
}

CINDER_APP(LifeApp, RendererGl)
