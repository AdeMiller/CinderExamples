
#ifndef SUDOKU_SUDOKUSOLVER_H
#define SUDOKU_SUDOKUSOLVER_H

#include <numeric>
#include <cctype>
#include <strings.h>

using namespace ::std;

struct cell {

    static const unsigned short value_mask = 0b0000000111111111;

    unsigned short values;

    cell(const unsigned int v = 0) : values( v == 0 ? value_mask : (0b1 << (v - 1) | locked_mask) ) {}

    inline bool is_locked() const { return values & locked_mask; }

    inline bool has_value() const { return __builtin_popcount(values & value_mask) == 1; }

    inline int get_value() const { return has_value() ? ffs(values & value_mask) : 0; }

private:

    static const unsigned short locked_mask = 0b0000001000000000;
};

struct read_sdm {
    inline cell operator()(char c) { return cell(atoi(&c)); }
};

class SudokuSolver {

private:
    array<cell, 81> board;
    array<array<char, 9>, 27> group_offsets;

public:
    SudokuSolver() : board()
    {
        group_offsets = array<array<char, 9>, 27>({
            array<char, 9>({  0,  1,  2,  3,  4,  5,  6,  7,  8 }), array<char, 9>({  9, 10, 11, 12, 13, 14, 15, 16, 17 }),
            array<char, 9>({ 18, 19, 20, 21, 22, 23, 24, 25, 26 }), array<char, 9>({ 27, 28, 29, 30, 31, 32, 33, 34, 35 }),
            array<char, 9>({ 36, 37, 38, 39, 40, 41, 42, 43, 44 }), array<char, 9>({ 45, 46, 47, 48, 49, 50, 51, 52, 53 }),
            array<char, 9>({ 54, 55, 56, 57, 58, 59, 60, 61, 62 }), array<char, 9>({ 63, 64, 65, 66, 67, 68, 69, 70, 71 }),
            array<char, 9>({ 72, 73, 74, 75, 76, 77, 78, 79, 80 }),
            array<char, 9>({  0,  9, 18, 27, 36, 45, 54, 63, 72 }), array<char, 9>({  1, 10, 19, 28, 37, 46, 55, 64, 73 }),
            array<char, 9>({  2, 11, 20, 29, 38, 47, 56, 65, 74 }), array<char, 9>({  3, 12, 21, 30, 39, 48, 57, 66, 75 }),
            array<char, 9>({  4, 13, 22, 31, 40, 49, 58, 67, 76 }), array<char, 9>({  5, 14, 23, 32, 41, 50, 59, 68, 77 }),
            array<char, 9>({  6, 15, 24, 33, 42, 51, 60, 69, 78 }), array<char, 9>({  7, 16, 25, 34, 43, 52, 61, 70, 79 }),
            array<char, 9>({  8, 17, 26, 35, 44, 53, 62, 71, 80 }),
            array<char, 9>({  0,  1,  2,  9, 10, 11, 18, 19, 20 }), array<char, 9>({  3,  4,  5, 12, 13, 14, 21, 22, 23 }),
            array<char, 9>({  6,  7,  8, 15, 16, 17, 24, 25, 26 }), array<char, 9>({ 27, 28, 29, 36, 37, 38, 45, 46, 47 }),
            array<char, 9>({ 30, 31, 32, 39, 40, 41, 48, 49, 50 }), array<char, 9>({ 33, 34, 35, 42, 43, 44, 51, 52, 53 }),
            array<char, 9>({ 54, 55, 56, 63, 64, 65, 72, 73, 74 }), array<char, 9>({ 57, 58, 59, 66, 67, 68, 75, 76, 77 }),
            array<char, 9>({ 60, 61, 62, 69, 70, 71, 78, 79, 80 })
        });
    }

    inline cell get_cell(const int row, const int col) const { return board[row * 9 + col]; }

    void load_sdm(string data)
    {
        if (data.length() != 81)
            fill(begin(board), end(board), 0);
        else
            transform(cbegin(data), cend(data), begin(board), read_sdm());
    }

    bool solve()
    {
        bool changed = false;

        for (const auto& g: SudokuSolver::group_offsets)
        {
            unordered_map<unsigned short, int> values_counts(9);
            for (const auto i: g) {
                values_counts[board[i].values & cell::value_mask]++;
            }

            for (const auto& c: values_counts)
                if (__builtin_popcount(c.first) == c.second)
                    for (const auto i: g)
                        if (__builtin_popcount(board[i].values & cell::value_mask) != 1 && board[i].values != c.first) {
                            board[i].values &= ~c.first;
                            changed = true;
                        }
        }

        return changed;
    }

    bool is_correct() const
    {
        return all_of(cbegin(group_offsets), cend(SudokuSolver::group_offsets), [this](const array<char, 9> g) { return is_correct(g); });
    }

private:
    bool is_correct(const array<char, 9>& group) const
    {
        auto r = accumulate(cbegin(group), cend(group), 0b0, [this](const unsigned short v, const char i) { return v | (board[i].has_value() ? board[i].values : 0); });
        return r & cell::value_mask == cell::value_mask;
    }
};

#endif //SUDOKU_SUDOKUSOLVER_H
