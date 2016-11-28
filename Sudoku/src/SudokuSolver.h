
#ifndef SUDOKU_SUDOKUSOLVER_H
#define SUDOKU_SUDOKUSOLVER_H

#include <numeric>
#include <cctype>
#include <strings.h>

using namespace ::std;

struct cell {

public:
    static const unsigned short value_mask = 0b0000000111111111;

    unsigned short values;

    cell() : values(0b0000000111111111) {}

    cell(unsigned char v) : values(value_mask & (0b0000001000000000 >> (10 - v)) | locked_mask) {}

    inline bool is_locked() const { return values & locked_mask; }

    inline bool has_value() const { return  __builtin_popcount(values & value_mask) == 1; }

    inline int get_value() const { return has_value() ? ffs(values & value_mask) : 0; }

private:
    static const unsigned short locked_mask = 0b0000001000000000;
};

struct row_indexer {
    row_indexer(const int& row) : i(row * 9) {}

    int operator()() { return i++; }

private:
    int i;
};

struct column_indexer {
    column_indexer(const int& col) : i(col - 9) {}

    int operator()() { i += 9; return i; }

private:
    int i;
};

struct block_indexer {
    block_indexer(const int& block) : row((block / 3) * 3), col((block % 3) * 3), i(-1) {}

    int operator()() { ++i; return ((row + i / 3)* 9 + col + i % 3); }

private:
    int i;
    int row;
    int col;
};

struct read_sdm {
    inline cell operator()(char c) { return (isdigit(c)) ? cell(atoi(&c)) : cell(); }
};

class SudokuSolver {

private:
    array<cell, 81> board;
    array<array<char, 9>, 27> group_offsets;

public:
    SudokuSolver() : board()
    {
        for (auto i = 0; i < 9; ++i) {
            generate(begin(group_offsets[i]), end(group_offsets[i]), row_indexer(i));
            generate(begin(group_offsets[9 + i]), end(group_offsets[9 + i]), column_indexer(i));
            generate(begin(group_offsets[18 + i]), end(group_offsets[18 + i]), block_indexer(i));
        }
    }

    inline cell get_cell(const int row, const int col) const { return board[row * 9 + col]; }

    void load_sdm(string data)
    {
        if (data.length() != 81)
            fill(begin(board), end(board), 0);
        else
            transform(cbegin(data), cend(data), begin(board), read_sdm());
    }

    bool update()
    {
        bool changed = false;

        for (const auto& g: group_offsets)
        {
            unordered_map<unsigned short, int> values_counts(9);
            auto m = 0b0;
            for (const auto i: g)
            {
                if (board[i].has_value())                       // Add known values to masks
                    m |= board[i].values;
                else
                    values_counts[board[i].values]++;           // Count populations of masks
            }

            for (const auto i: g)                                     // Remove known values from group
                if (!board[i].has_value()) {
                    board[i].values &= ~m;
                    changed = true;
                }

            for (const auto& c: values_counts)
                if (__builtin_popcount(c.first) > 1 && __builtin_popcount(c.first) == c.second)
                    for (const auto i: g)
                        if (!board[i].has_value() && board[i].values != c.first) {
                            board[i].values &= ~c.first;
                            changed = true;
                        }
        }

        return changed;
    }

    bool is_complete() const
    {
        return all_of(cbegin(board), cend(board), [](const cell c) { return c.has_value(); }) && is_correct();
    }

    bool is_correct() const
    {
        return all_of(cbegin(group_offsets), cend(group_offsets), [this](const array<char, 9> g) { return is_correct(g); });
    }

    bool is_correct(const array<char, 9>& group) const
    {
        return accumulate(cbegin(group), cend(group), 0b0, [this](const unsigned short v, const char i) { return v | (board[i].has_value() ? board[i].values : 0); }) == cell::value_mask;
    }
};

#endif //SUDOKU_SUDOKUSOLVER_H
