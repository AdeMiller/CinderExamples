#include "stdafx.h"

using namespace std;
using namespace concurrency;
using namespace boost::filesystem;
using namespace ci;
using namespace ci::app;

class LifeApp : public AppNative 
{
private:
#if _DEBUG
    static const size_t map_height = 300;
    static const size_t map_width = 600;
#else
    static const size_t map_height = 1600;
    static const size_t map_width = 6400;
#endif
    array<vector<vector<char>>, 2> map_cells;
    size_t read_idx, write_idx;
    unsigned int generation_count;

    bool is_updating;
    bool is_moving;
    Vec2i last_mouse_pos;       // int (x, y) 
    Vec2i view_origin;
    Vec2i view_size;
    const int header_height;    // Pixels
    int cell_size;
    Font text_font;

    void mouseDown(MouseEvent event);
    void mouseDrag(MouseEvent event);
    void mouseUp(MouseEvent event);
    void mouseWheel(MouseEvent event);
    void keyDown(KeyEvent event);

    vector<Vec2i> load_creature(fstream& fin);
    void populate_map(const path& app_path);
    void draw_header() const;
    void refresh_map();

    template<typename Func>
    void draw_map(Func draw_if_pred) const
    {
        const array<const Color, 2> color_mapper = { Color::black(), Color::white() };
        for (int y = view_origin.y; y < (view_origin.y + view_size.y); ++y)
        {
            const float screen_y = float(header_height + (y - view_origin.y) * cell_size);
            for (int x = view_origin.x; x < (view_origin.x + view_size.x); ++x)
            {
                if (draw_if_pred(map_cells[read_idx][y][x], map_cells[write_idx][y][x]))
                {
                    gl::color(color_mapper[map_cells[read_idx][y][x]]);
                    const float screen_x = float(x - view_origin.x) * cell_size;
                    const auto cell_rect = Rectf(screen_x, screen_y, screen_x + cell_size, screen_y + cell_size);
                    gl::drawSolidRect(cell_rect);
                }
            }
        }
    }

    void zoom_view(int new_cell_size);
    void resize_window();

public:
    void setup();
    void update();
    void draw();

    LifeApp() : 
        read_idx(0), 
        write_idx(1),
        generation_count(1),
        is_updating(false),
        is_moving(false),
        view_origin(0, 0),
        view_size(400, 200),
        header_height(50),
        cell_size(4)
    {
        map_cells[read_idx] = vector<vector<char>>(map_height, vector<char>(map_width, 0));
        map_cells[write_idx] = vector<vector<char>>(map_height, vector<char>(map_width, 0));
    }
};

// Cinder: Setup application

void LifeApp::setup()
{
    populate_map(getAppPath());
    setWindowSize(view_size.x * cell_size, header_height + view_size.y * cell_size);
    text_font = Font( "Courier New", 24.0f );
    gl::clear(Color::black());
}

// Cinder I/O event handlers

void LifeApp::mouseDown(MouseEvent event)
{
    is_moving = true;
    last_mouse_pos = event.getPos();
}

void LifeApp::mouseDrag(MouseEvent event)
{
    auto pos = event.getPos();
    view_origin += (last_mouse_pos - pos) / cell_size;
    view_origin.x = clamp(view_origin.x, 0, int(map_width) - view_size.x);
    view_origin.y = clamp(view_origin.y, 0, int(map_height) - view_size.y);
    last_mouse_pos = pos;
}

void LifeApp::mouseUp(MouseEvent event)
{
    is_moving = false;
}

void LifeApp::mouseWheel(MouseEvent event)
{
    if (event.getWheelIncrement() > 0.0f)
        zoom_view(cell_size >> 1);
    if (event.getWheelIncrement() < 0.0f)
        zoom_view(cell_size << 1);
    refresh_map();
}

void LifeApp::keyDown(KeyEvent event)
{
    switch(event.getCode())
    {
    case KeyEvent::KEY_s:           // Start/Stop.
        is_updating = !is_updating;
        break;
    case KeyEvent::KEY_r:           // Reset.
        is_updating = false;
        populate_map(getAppPath());
        refresh_map();
        break;
    case KeyEvent::KEY_UP:          // Zoom in.
        zoom_view(cell_size << 1);
        refresh_map();
        break;
    case KeyEvent::KEY_DOWN:        // Zoom out.
        zoom_view(cell_size >> 1);
        refresh_map();
        break;
    case KeyEvent::KEY_q:           // Quit.
        quit();
        break;
    }
}

// Cinder: Update state

void LifeApp::update()
{
    if (!is_updating || is_moving)
        return;

    // 1.Any live cell with fewer than two live neighbours dies, as if caused by under-population.
    // 2 Any live cell with two or three live neighbours lives on to the next generation.
    // 3.Any live cell with more than three live neighbours dies, as if by overcrowding.
    // 4.Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.

    const array<array<const char, 9>, 2> cell_mapper = { { 
        { { 0, 0, 0, 1, 0, 0, 0, 0, 0 } },
        { { 0, 0, 1, 1, 0, 0, 0, 0, 0 } }
    } };

    const auto& read_map = map_cells[read_idx];
    parallel_for(0, int(map_height), [=](const int& y)
    {
        const int top = wrap_map<map_height>(y - 1);
        const int btm = wrap_map<map_height>(y + 1);
        for (int x = 0; x < map_width; ++x)
        {
            const size_t left = wrap_map<map_height>(x - 1);
            const size_t right = wrap_map<map_height>(x + 1);
            const int neighbors = read_map[top][left] + read_map[top][x] + read_map[top][right]
                                + read_map[y  ][left]                    + read_map[y  ][right]
                                + read_map[btm][left] + read_map[btm][x] + read_map[btm][right];
            map_cells[write_idx][y][x] = cell_mapper[read_map[y][x]][neighbors];
        }
    }, affinity_partitioner());

    ++generation_count;
    swap(read_idx, write_idx);
}

// Cinder: Draw UI

void LifeApp::draw()
{
    draw_header();

    if (is_updating && !is_moving)
    {
        draw_map([=](const char new_value, const char old_value) { return (new_value != old_value); });
        return;
    }
    if (is_moving) 
        refresh_map();
}

// Helper functions

vector<Vec2i> LifeApp::load_creature(fstream& fin)
{
    vector<Vec2i> result;
    char ch;
    int y = 0, x = 0;
    while (fin >> noskipws >> ch) 
    {
        switch (ch)
        {
        case '#':
            result.push_back(Vec2i(x, y));
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

void LifeApp::populate_map(const path& app_path)
{
    const int creature_count = map_height * map_width / 1600;

    vector<path> paths;
    copy_if(directory_iterator(app_path), 
        directory_iterator(), back_inserter(paths), 
        [=](const path& p)
    {
        return is_regular_file(p) && 
            (p.extension().compare(L".life") == 0);
    });
    if (paths.size() == 0)
        return;

    vector<vector<Vec2i>> creature_library(paths.size());
    transform(begin(paths), end(paths), 
        begin(creature_library), 
        [=](const path& p)
    {
        fstream fin(p.c_str(), fstream::in);
        cout << "  " << p.filename() << "...\n";
        return load_creature(fin);
    });

    random_device rnd_dev;
    mt19937 rnd_gen(rnd_dev());
    uniform_int_distribution<size_t> creature_dist(0, creature_library.size() - 1);
    uniform_int_distribution<int> width_dist(0, map_width - 1);
    uniform_int_distribution<int> height_dist(0, map_height - 1);

    for (auto& map : map_cells)
        for (auto& row : map)
            fill(begin(row), end(row), 0);

    for (int i = 0; i < creature_count; ++i)
    {
        vector<Vec2i> creature_definition = creature_library[creature_dist(rnd_gen)];
        Vec2i pos(width_dist(rnd_gen), height_dist(rnd_gen));
        for (auto& cell : creature_definition)
            map_cells[read_idx][wrap_map<map_height>(pos.y + cell.y)][wrap_map<map_width>(pos.x + cell.x)] = 1;
    }
    generation_count = 0;
}

void LifeApp::draw_header() const
{
    stringstream buf;
    buf << "Framerate: " << fixed << setprecision(1) << setw(5) << getAverageFps() << 
           "       Generation: " << generation_count << " ";
    gl::drawString(buf.str(), Vec2f( 10.0f, 5.0f ), Color::white(), text_font );
    buf.str("");
    buf.clear(); 
    buf << "Area: [ " << map_width << " x " << map_height <<
           " ]  View: [ " << setfill(' ') << setw(4) << view_origin.x << " - " << setw(4) << (view_origin.x + view_size.x) << 
           " x " << setw(4) << view_origin.y << " - " << setw(4) << (view_origin.y + view_size.y) <<
           " ] Zoom: x" << setw(2) << cell_size;
    gl::drawString(buf.str(), Vec2f( 10.0f, 30.0f ), Color::white(), text_font);
}

void LifeApp::refresh_map()
{
    gl::color(Color::black());  
    gl::drawSolidRect(Rectf(Vec2f(0.0f, float(header_height)), Vec2f(getWindowSize())));
    draw_map([=](const char new_value, const char old_value) { return (new_value != 0); });
}

void LifeApp::zoom_view(int new_cell_size)
{
    new_cell_size = clamp(new_cell_size, 1, 32);
    Vec2i view_center = view_origin + (view_size / 2);
    Vec2i window_size = getWindowSize();
    window_size.y -= header_height;
    view_size = window_size / new_cell_size;
    view_size.x = clamp(view_size.x, 10, int(map_width));
    view_size.y = clamp(view_size.y, 10, int(map_height));
    view_origin = view_center - (view_size / 2);
    view_origin.x = clamp(view_origin.x, 0, int(map_width) - view_size.x);
    view_origin.y = clamp(view_origin.y, 0, int(map_height) - view_size.y);
    cell_size = new_cell_size;
}

void LifeApp::resize_window()
{
    zoom_view(cell_size);
    setWindowSize(view_size.x * cell_size, header_height + view_size.y * cell_size);
    refresh_map();
}

CINDER_APP_NATIVE( LifeApp, RendererGl )
