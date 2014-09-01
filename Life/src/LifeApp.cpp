#include "stdafx.h"

using namespace std;
using namespace boost::filesystem;
using namespace ci;
using namespace ci::app;

class LifeApp : public AppNative 
{
public:
#if _DEBUG
    static const size_t map_height = 300;
    static const size_t map_width = 600;
#else
    static const size_t map_height = 1600;
    static const size_t map_width = 6400;
#endif
private:
    array<vector<int>, 2> map_cells;
    array<unique_ptr<concurrency::array_view<int, 2>>, 2> map_cells_vw;
    size_t read_idx, write_idx;

    unsigned int generation_count;

    bool is_updating;
    bool is_moving;
    bool is_benchmarking;
    string update_mode_name;
    
    function<void(void)> update_func;

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
                if (draw_if_pred(map_cells[read_idx][y * map_width + x], map_cells[write_idx][y * map_width + x]))
                {
                    gl::color(color_mapper[map_cells[read_idx][y * map_width + x]]);
                    const float screen_x = float(x - view_origin.x) * cell_size;
                    const auto cell_rect = Rectf(screen_x, screen_y, screen_x + cell_size, screen_y + cell_size);
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

    LifeApp() : 
        read_idx(0), 
        write_idx(1),
        generation_count(1),
        is_updating(false),
        is_moving(false),
        is_benchmarking(false),
        update_func(bind(&LifeApp::update_cpu, this)),
        update_mode_name("CPU"),
        view_origin(0, 0),
        view_size(400, 200),
        header_height(50),
        cell_size(4)
    {
        map_cells[read_idx] =  vector<int>(map_height * map_width, 0);
        map_cells[write_idx] = vector<int>(map_height * map_width, 0);
        map_cells_vw[read_idx] = unique_ptr<concurrency::array_view<int, 2>>(new concurrency::array_view<int, 2>(map_height, map_width, map_cells[0].data()));
        map_cells_vw[write_idx] = unique_ptr<concurrency::array_view<int, 2>>(new concurrency::array_view<int, 2>(map_height, map_width, map_cells[1].data()));
    }

    int read_map(const int y, const int x) const
    {
        return map_cells[read_idx][wrap_map<map_height>(y) * map_width + wrap_map<map_width>(x)];
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
    case KeyEvent::KEY_b:                                       // Toggle benchmark mode
        if (is_benchmarking)
            refresh_map();
        is_benchmarking = !is_benchmarking;
        break;
    case KeyEvent::KEY_q:                                       // Quit.
        quit();
        break;
    case KeyEvent::KEY_r:                                       // Reset.
        is_updating = false;
        populate_map(getAppPath());
        refresh_map();
        break;
    case KeyEvent::KEY_s:                                       // Start/Stop.
        is_updating = !is_updating;
        break;
    case KeyEvent::KEY_1:                                       // CPU mode.
        update_func = bind(&LifeApp::update_cpu, this);
        update_mode_name = "CPU";
        break;
    case KeyEvent::KEY_2:                                       // AMP mode.
        update_func = bind(&LifeApp::update_amp, this);
        update_mode_name = "AMP";
        break;
    case KeyEvent::KEY_3:                                       // AMP Tiled mode.
        update_func = bind(&LifeApp::update_amp_tiled, this);
        update_mode_name = "AMP Tiled";
        break;
    case KeyEvent::KEY_UP:                                      // Zoom in.
        zoom_view(cell_size << 1);
        refresh_map();
        break;
    case KeyEvent::KEY_DOWN:                                    // Zoom out.
        zoom_view(cell_size >> 1);
        refresh_map();
        break;
    }
}

// Cinder: Update state

void LifeApp::update()
{
    if (!is_updating || is_moving)
        return;

    update_func();

    ++generation_count;
    swap(read_idx, write_idx);
}

// 1.Any live cell with fewer than two live neighbors dies, as if caused by under-population.
// 2 Any live cell with two or three live neighbors lives on to the next generation.
// 3.Any live cell with more than three live neighbors dies, as if by overcrowding.
// 4.Any dead cell with exactly three live neighbors becomes a live cell, as if by reproduction.

void LifeApp::update_cpu()
{
    const array<array<const int, 9>, 2> cell_mapper = { {
            { { 0, 0, 0, 1, 0, 0, 0, 0, 0 } },
            { { 0, 0, 1, 1, 0, 0, 0, 0, 0 } }
            } };

    concurrency::parallel_for(0, int(map_height), [=](const int& y)
    {
        const int top = y - 1;
        const int btm = y + 1;
        for (int x = 0; x < map_width; ++x)
        {
            const int left = x - 1;
            const int right = x + 1;
            const int neighbors = read_map(top, left) + read_map(top, x) + read_map(top, right)
                                + read_map(y  , left)                    + read_map(y  , right)
                                + read_map(btm, left) + read_map(btm, x) + read_map(btm, right);
            map_cells[write_idx][y * map_width + x] = cell_mapper[read_map(y, x)][neighbors];
        }
    }, concurrency::affinity_partitioner());
}

void LifeApp::update_amp()
{
    auto read_map_vw = *map_cells_vw[read_idx];
    auto write_map_vw = *map_cells_vw[write_idx];
    write_map_vw.discard_data();
    auto compute_domain = concurrency::extent<2>(map_height, map_width);

    concurrency::parallel_for_each(compute_domain, [=](concurrency::index<2> idx) restrict(amp)
    {
        const int y = idx[0];
        const int x = idx[1];
        int cell_mapper[2][8] = { { 0, 0, 0, 1, 0, 0, 0, 0 }, { 0, 0, 1, 1, 0, 0, 0, 0 } };
        const int top = wrap_map<map_height>(y - 1);
        const int btm = wrap_map<map_height>(y + 1);
        const int left = wrap_map<map_width>(x - 1);
        const int right = wrap_map<map_width>(x + 1);
        const int neighbors = read_map_vw[top][left] + read_map_vw[top][x] + read_map_vw[top][right]
                            + read_map_vw[y][left]                         + read_map_vw[y][right]
                            + read_map_vw[btm][left] + read_map_vw[btm][x] + read_map_vw[btm][right];
        write_map_vw[y][x] = cell_mapper[read_map_vw[y][x]][neighbors];
    });
    write_map_vw.synchronize();
}

int read_map_vw(const concurrency::array_view<int, 2>& map_vw, int y, int x) restrict(amp)
{
    return map_vw[wrap_map<LifeApp::map_height>(y)][wrap_map<LifeApp::map_width>(x)];
}

void LifeApp::update_amp_tiled()
{
    //auto read_map_vw = *map_cells_vw[read_idx];
    //auto write_map_vw = *map_cells_vw[write_idx];
    //write_map_vw.discard_data();
    //auto compute_domain = concurrency::tiled_extent<16, 16>(concurrency::extent<2>(map_height, map_width)).pad();

    //concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<16, 16> tidx) restrict(amp)
    //{
    //    tile_static int tile_data[18][18];
    //    const int gy = wrap_map<map_height>(tidx.global[0]);
    //    const int gx = wrap_map<map_height>(tidx.global[1]);
    //    const int y = tidx.local[0];
    //    const int x = tidx.local[1];

    //    tile_data[y+1][x+1] = read_map_vw[gy][gx];
    //    //if (x == 0)
    //    //    tile_data[y + 1][0] = 
    //    tidx.barrier.wait();
    //    int cell_mapper[2][8] = { { 0, 0, 0, 1, 0, 0, 0, 0 }, { 0, 0, 1, 1, 0, 0, 0, 0 } };
    //    const int top = y - 1;
    //    const int btm = y + 1;
    //    const int left = x - 1;
    //    const int right = x + 1;
    //    const int neighbors = tile_data[top][left] + tile_data[top][x] + tile_data[top][right]
    //        + tile_data[y][left] + tile_data[y][right]
    //        + tile_data[btm][left] + tile_data[btm][x] + tile_data[btm][right];
    //    write_map_vw[gy][gx] = cell_mapper[read_map_vw[y][x]][neighbors];
    //});
    //write_map_vw.synchronize();
}

// Cinder: Draw UI

void LifeApp::draw()
{
    draw_header();

    if (is_updating && !is_moving && !is_benchmarking)
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
        fill(begin(map), end(map), 0);

    for (int i = 0; i < creature_count; ++i)
    {
        vector<Vec2i> creature_definition = creature_library[creature_dist(rnd_gen)];
        Vec2i pos(width_dist(rnd_gen), height_dist(rnd_gen));
        for (auto& cell : creature_definition)
            map_cells[read_idx][wrap_map<map_height>(pos.y + cell.y) * map_width + wrap_map<map_width>(pos.x + cell.x)] = 1;
    }
    generation_count = 0;
}

void LifeApp::draw_header() const
{
    stringstream buf;
    buf << "Framerate: " << fixed << setprecision(1) << setw(5) << getAverageFps() << 
           "       Generation: " << generation_count << 
           "       " << update_mode_name << " " << (is_benchmarking ? "Benchmark" : "         ");
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
