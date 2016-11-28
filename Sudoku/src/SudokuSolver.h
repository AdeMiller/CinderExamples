
#ifndef SUDOKU_SUDOKUSOLVER_H
#define SUDOKU_SUDOKUSOLVER_H

#include <numeric>
#include <cctype>
#include <strings.h>

using namespace ::std;

struct cell {

private:
    static const unsigned short locked_mask = 0b0000001000000000;
    static const unsigned short value_mask =  0b0000000111111111;

public:
    unsigned char val;
    unsigned short mask;

    cell() : val(0), mask(0b0000000111111111) {}

    cell(unsigned char v) : val(v), mask(0b0000000111111111 & (v != 0 ? locked_mask : 0b0 ))
    {
        if (v)
            mask = 0b0000000111111111 & (0b0000001000000000 >> (10 - v)) | locked_mask;
    }

    inline bool is_locked() { return mask & locked_mask; }
    inline int get_value() { return __builtin_popcount(mask & 0b0000000111111111) == 1 ? ffs(mask & 0b0000000111111111) : 0; }
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
    cell operator()(char c)
    {
        return (isdigit(c)) ? cell(atoi(&c)) : cell();
    }
};

class SudokuSolver {

private:
    array<cell, 81> board;
    array<array<char, 9>, 27> groups;

public:
    SudokuSolver() : board()
    {
        for (auto i = 0; i < 9; ++i) {
            generate(begin(groups[i]), end(groups[i]), row_indexer(i));
            generate(begin(groups[9 + i]), end(groups[9 + i]), column_indexer(i));
            generate(begin(groups[18 + i]), end(groups[18 + i]), block_indexer(i));
        }
    }

    cell get_cell(int row, int col) const { return board[row * 9 + col]; }

    void load_sdm(string data) {
        if (data.length() != 81) return;
        transform(cbegin(data), cend(data), begin(board), read_sdm());
    }

    bool update()
    {
        bool changed = false;

        for (auto g: groups)
        {
            unordered_map<unsigned short, int> count(9);
            auto m = 0b0;
            for (auto i: g)
            {
                if (board[i].val != 0)          // Add known values to masks
                    m |= board[i].mask;
                else
                    count[board[i].mask]++;     // Count populations of masks
            }

            for (auto i: g)                     // Remove known values from group
                if (board[i].val == 0)
                    board[i].mask &= ~m;

            for (auto c: count)
                if (__builtin_popcount(c.first) > 1 && __builtin_popcount(c.first) == c.second)
                    for (auto i: g)
                        if (board[i].val == 0 && board[i].mask != c.first)
                            board[i].mask &= ~c.first;
        }

        for (auto& c: board)
        {
            if ((c.val == 0) && (__builtin_popcount(c.mask) == 1))
            {
                c.val = ffs(c.mask);
                changed = true;
            }
        }

        return changed;
    }

    bool is_complete() {
        return all_of(begin(board), end(board), [](const cell c) { return c.val != 0; }) && is_correct();
    }

    bool is_correct() {
        for (auto g: groups) {
            auto count = 0b0;
            for (auto i: g)
                count += 0b1 << board[i].val;
            if (count != 0b0000001111111110)
                return false;
        }
        return true;
    }
};

#endif //SUDOKU_SUDOKUSOLVER_H
