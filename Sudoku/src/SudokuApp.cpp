#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "SudokuSolver.h"

using namespace ci;
using namespace ci::app;

using namespace Sudoku;

class SudokuApp : public App {

public:
    SudokuApp(): board_offset(50.0f, 50.0f),
                 sqr_size(90.0f),
                 num_size(sqr_size / 3.0f),
                 num_mid(num_size / 2.0f),
                 blk_size(num_size * 3.0f),
                 blk_mid(blk_size / 2.0f),
                 big_font("Courier New", 42.0f),
                 sml_font("Courier New", 16.0f),

                 solver(),
                 puzzles(),
                 puzzle(0),
                 is_dirty(false)
    {}

    void setup() override;
    void keyDown(KeyEvent event) override;
    void resize() override;
    void draw() override;

private:
    const ivec2 board_offset;
    const float sqr_size;
    const Font big_font;
    const Font sml_font;
    const float num_size;
    const float num_mid;
    const float blk_size;
    const float blk_mid;

    SudokuSolver solver;
    vector<string> puzzles;
    int puzzle;
    bool is_dirty;

    void draw_board();
    void draw_cell(const int row, const int col) const;
    void draw_value(const int row, const int col, const Cell &cell) const;
    void draw_values(const int row, const int col, const Cell &cell) const;
};

void prepareSettings(SudokuApp::Settings* settings)
{
    settings->setMultiTouchEnabled(false);
}

void SudokuApp::keyDown(KeyEvent event)
{
    switch(event.getCode())
    {
        case KeyEvent::KEY_SPACE:                                   // Next move.
            is_dirty = solver.solve() | solver.is_finished();
            break;
        case KeyEvent::KEY_n:                                       // Next puzzle.
            if (puzzle < puzzles.size())
                is_dirty = solver.load_sdm(puzzles[++puzzle]);
            break;
        case KeyEvent::KEY_q:                                       // Quit.
            quit();
            break;
        case KeyEvent::KEY_r:                                       // Reset current puzzle.
            is_dirty = solver.load_sdm(puzzles[puzzle]);
            break;
    }
}

void SudokuApp::resize()
{
    gl::clear(ColorA::black());
    is_dirty = true;
}

void SudokuApp::setup()
{
    setWindowSize(board_offset.x * 2 + sqr_size * 9, board_offset.y * 2 + sqr_size * 9);

    gl::clear(ColorA::black());

    puzzles = {
            "97...6.5...67..21.....5...668......7..5...9..7......414...7.....37..26...2.5...73",
            ".164.....2....9...4......62.7.23.1..1.......3..3.87.4.96......5...8....7.....682.",
            "........74.6..7.....71285.6..3.71.5.8.......3.1.84.2..6.89327.....4..9.51........",
            "964.........6..1......7.5...8.9.3...25......63...4...7.....4.....25...4.6..8....3",
            "9672415832.......64.......98.......57958321641.......26.......75.......1321756498",
            "97.....5.6..5.8.3....6..748...3...2..6.....9..1...9...187..5....3.2.7..4.2.....73",
            "97....4.8...1....5....54.....98....414.....763....19.....67....4....9...7.6....91",
            "97...483..5..8......631...........2.7..925..8.4...........672......4..9..351...76",
            "97...6.....4..8...1.2...6..3...9...5428...31.7...3...6..6...7.1...2..4.....1...58",
            "97..581.....9...2..2...6.5.7...82.6.....7.....1.39...2.4.8...1..5...3.....156..79",
            "97.1..4.2...7...9.......761....6.1...85...94...7.9....893.......1...5...7.4..9.36"
    };
    solver.load_sdm(puzzles[puzzle]);
    is_dirty = true;
}

void SudokuApp::draw()
{
    if (!is_dirty)
        return;

    gl::clear(ColorA::black());
    draw_board();

    for (auto r = 0; r < 9; ++r) {
        for (auto c = 0; c < 9; ++c)
            draw_cell(r, c);
    }
    is_dirty = false;
}

void SudokuApp::draw_board()
{
    for (auto x = 0; x <= 9; ++x) {
        gl::lineWidth(x % 3 == 0 ? 3.0f : 1.0f);
        gl::color(ColorA::gray(x % 3 == 0 ? 1.0f : 0.5f));
        gl::drawLine(board_offset + ivec2(x * sqr_size, 0.0f), board_offset + ivec2(x * sqr_size, sqr_size * 9.0f));
        gl::drawLine(board_offset + ivec2(0.0f, x * sqr_size), board_offset + ivec2(sqr_size * 9.0f, x * sqr_size));
    }
}

void SudokuApp::draw_cell(const int row, const int col) const
{
    auto cell = solver.get_cell(row, col);
    cell_value(cell) ? draw_value(row, col, cell) : draw_values(row, col, cell);
}

void SudokuApp::draw_value(const int row, const int col, const Cell &cell) const
{
    ColorA color(1, 1, 1);          // Default - White

    switch(cell & 0b0000111000000000)
    {
        case 0b0000001000000000:    // Cell is locked, set by original puzzle - Green
        case 0b0000011000000000:
            color = ColorA(0, 1, 0);
            break;
        case 0b0000010000000000:    // Cell is a guess - Yellow
            color = ColorA(1, 1, 0);
            break;
        case 0b0000100000000000:    // Cell is part of incorrect group - Red
        case 0b0000101000000000:
        case 0b0000111000000000:
            color = ColorA(1, 0, 0);
            break;
    }

    gl::drawStringCentered(to_string(cell_value(cell)),
                           board_offset + ivec2(blk_mid + sqr_size * col, blk_mid + sqr_size * row),
                           color, big_font);
}

void SudokuApp::draw_values(const int row, const int col, const Cell& cell) const
{
    auto offset = board_offset + ivec2(num_mid + sqr_size * col, num_mid + sqr_size * row);
    for (auto i = 0; i < 9; ++i) {
        if (cell & (0b1 << i))
            gl::drawStringCentered(to_string(i + 1),
                                   offset + ivec2(num_size * (i % 3), num_size * (i / 3)),
                                   ColorA::white(), sml_font);
    }
}

CINDER_APP(SudokuApp, RendererGl, prepareSettings)
