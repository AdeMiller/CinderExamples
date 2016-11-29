#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "SudokuSolver.h"

using namespace ci;
using namespace ci::app;

class SudokuApp : public App {

public:
    void setup() override;

    void keyDown(KeyEvent event) override;
    void resize() override;
    void draw() override;
    void update() override;

private:
    SudokuSolver m_solver;
    Font big_font;
    Font sml_font;
    bool is_dirty;
    bool is_finished;
};

void prepareSettings(SudokuApp::Settings* settings)
{
    settings->setMultiTouchEnabled( false );
}

void SudokuApp::keyDown(KeyEvent event)
{
    is_dirty = m_solver.update();
    if (!is_dirty) {
        is_finished = m_solver.is_correct();
        if (is_finished)
            is_dirty = true;
    }
}

void SudokuApp::resize()
{
    gl::clear(ColorA::black());
    is_dirty = true;
}

void SudokuApp::setup()
{
    big_font = Font( "Courier New", 42.0f );
    sml_font = Font( "Courier New", 16.0f );
    setWindowSize(100 + 90 * 9, 100 + 90 * 9);

    gl::clear(ColorA::black());

    // NO m_solver.load_sdm("016400000200009000400000062070230100100000003003087040960000005000800007000006820");
    m_solver.load_sdm("........74.6..7.....71285.6..3.71.5.8.......3.1.84.2..6.89327.....4..9.51........");
    //m_solver.load_sdm("964.........6..1......7.5...8.9.3...25......63...4...7.....4.....25...4.6..8....3");
    //m_solver.load_sdm("9672415832.......64.......98.......57958321641.......26.......75.......1321756498");
    //m_solver.load_sdm("97.....5.6..5.8.3....6..748...3...2..6.....9..1...9...187..5....3.2.7..4.2.....73");
    //m_solver.load_sdm("97....4.8...1....5....54.....98....414.....763....19.....67....4....9...7.6....91");
    //m_solver.load_sdm("97...483..5..8......631...........2.7..925..8.4...........672......4..9..351...76");
    //m_solver.load_sdm("97...6.....4..8...1.2...6..3...9...5428...31.7...3...6..6...7.1...2..4.....1...58");
    //m_solver.load_sdm("97...6.5...67..21.....5...668......7..5...9..7......414...7.....37..26...2.5...73");
    //m_solver.load_sdm("97..581.....9...2..2...6.5.7...82.6.....7.....1.39...2.4.8...1..5...3.....156..79");
    //m_solver.load_sdm("97.1..4.2...7...9.......761....6.1...85...94...7.9....893.......1...5...7.4..9.36");

    is_dirty = true;
}

void SudokuApp::update()
{
    //is_dirty = m_solver.update();
}

void SudokuApp::draw()
{
    if (!is_dirty)
        return;

    auto board_offset = ivec2(50.0f, 50.0f);
    auto sqr_size = 90.0f;

    auto num_size = sqr_size / 3.0f;
    auto num_mid = num_size / 2.0f;

    auto blk_size = num_size * 3.0f;
    auto blk_mid = blk_size / 2.0f;

    for (auto x = 0; x <= 9; ++x) {
        gl::lineWidth(x % 3 == 0 ? 3.0f : 1.0f);
        gl::color(ColorA::gray(x % 3 == 0 ? 1.0f : 0.5f));
        gl::drawLine(board_offset + ivec2(x * sqr_size, 0.0f), board_offset + ivec2(x * sqr_size, sqr_size * 9.0f));
        gl::drawLine(board_offset + ivec2(0.0f, x * sqr_size), board_offset + ivec2(sqr_size * 9.0f, x * sqr_size));
    }

    for (auto r = 0; r < 9; ++r) {
        for (auto y = 0; y < 9; ++y) {
            gl::color(ColorA::black());

            auto co = board_offset + ivec2(2 + r * sqr_size, 2 + y * sqr_size);
            gl::drawSolidRect(Rectf(co, co + ivec2(sqr_size - 4, sqr_size - 4)));
            gl::color(ColorA::white());

            auto c = m_solver.get_cell(y, r);
            stringstream buf;
            if (m_solver.get_cell(y, r).get_value() == 0) {
                auto cell_offset = board_offset + ivec2(sqr_size * r, sqr_size * y);
                unsigned short m = 0b1;
                for (auto i = 0; i < 9; ++i) {
                    if (c.values & m) {
                        buf.str("");
                        buf.clear();
                        buf << i + 1;
                        gl::drawStringCentered(buf.str(),
                                               cell_offset +
                                               ivec2(num_mid + num_size * (i % 3), num_mid + num_size * (i / 3)),
                                               ColorA::white(), sml_font);
                    }
                    m <<= 1;
                }
                continue;
            }
            buf.str("");
            buf.clear();
            auto v = m_solver.get_cell(y, r).get_value();
            buf << v;

            gl::drawStringCentered(buf.str(), board_offset + ivec2(blk_mid + sqr_size * r, blk_mid + sqr_size * y),
                                   c.is_locked() ? ColorA( 1, 1, 1 ) : (is_finished ? ColorA( 0, 1, 0 ) : ColorA( 1, 0, 0 )),
                                   big_font);
        }
        is_dirty = false;
    }
}

CINDER_APP( SudokuApp, RendererGl, prepareSettings )
