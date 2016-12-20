#ifndef SUDOKU_SUDOKUSOLVER_H
#define SUDOKU_SUDOKUSOLVER_H

#include <algorithm>
#include <array>
#include <cctype>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stack>
#include <string>
#include <sstream>
#include <unordered_map>

using namespace ::std;

namespace Sudoku
{
    typedef unsigned short Cell;
    typedef array<Cell, 81> Board;
    typedef array<char, 9> Group;

    static const unsigned short value_mask =  0b0000000111111111;
    static const unsigned short locked_mask = 0b0000001000000000;
    static const unsigned short guess_mask =  0b0000010000000000;

    int cell_value(Cell c);
    bool cell_guess(Cell c);
    bool cell_locked(Cell c);
    bool cell_certainty(const Cell &i, const Cell &j);

    class SudokuSolver
    {
    private:
        stack<Board> boards;
        int move_count;
        static const array<Group, 27> group_offsets;

    public:
        SudokuSolver() : boards(), move_count(0) {}

        inline Cell get_cell(const int row, const int col) const { return boards.top()[row * 9 + col]; }

        bool load_sdm(const string &data)
        {
            if (data.length() != 81)
                return false;

            Board board;
            transform(cbegin(data), cend(data), begin(board), [this](const char c) {
                auto v = atoi(&c);
                return v == 0 ? value_mask : (0b1 << (v - 1) | locked_mask);
            });
            boards = stack<Board>({board});
            move_count = 0;
            return true;
        }

        inline bool is_correct() const
        {
            return all_of(cbegin(group_offsets), cend(group_offsets), [this](const Group &g) { return is_group_correct(g); });
        }

        bool solve()
        {
            cout << "Move " << setw(2) << ++move_count <<
                    " on board " << setw(2) << boards.size() <<
                    " =================================================" << endl;

            bool changed = false;
            for (const auto &g: SudokuSolver::group_offsets)
                changed |= solve_group(g);

            if (changed)
                return true;

            if (is_correct()) {
                move_count--;
                cout << "FINISHED !!" << endl;
                return false;
            }
            // TODO: If the board is incorrect and this is a guess we can unwind here, rather than waiting for a complete board.

            if (!is_complete()) {
                make_guesses();
                return true;
            }
            cout << "BAD GUESS. Unrolling." << endl;
            boards.pop();
            return true;
        }

    private:
        bool make_guesses()
        {
            auto i = find_guess_candidate();
            auto current_board = boards.top();
            boards.pop();
            for (auto m = 0b1; m < 512; m <<= 1)
                if (current_board[i] & m) {
                    boards.push(current_board);
                    boards.top()[i] = m | guess_mask;
                    cout << "[" << (i / 9 + 1) << ", " << (i % 9 + 1) << "]: " << to_string(current_board[i]) << " => " << to_string(m) << " <------------ GUESS" << endl;
                }
        }

        int find_guess_candidate()
        {
            // TODO: Look at finding a better approach to making a guess
            return distance(cbegin(boards.top()), min_element(cbegin(boards.top()), cend(boards.top()), cell_certainty));
        }

        bool solve_group(const Group &g)
        {
            // Count the number of occurences of each possibility set values within a group.

            unordered_map<unsigned short, int> values_counts(9);
            for (const auto i: g)
                values_counts[boards.top()[i] & value_mask]++;

            // If the number of occurences of a possibility set is the same as the set size then remove those
            // possible values from all other cells in the group.

            bool changed = false;
            for (const auto &c: values_counts)
                if (__builtin_popcount(c.first) == c.second)
                    for (const auto i: g)
                        if (__builtin_popcount(boards.top()[i] & value_mask) != 1 && boards.top()[i] != c.first && boards.top()[i] & c.first) {
                            cout << "[" << (i / 9 + 1) << ", " << (i % 9 + 1) << "]: " << to_string(boards.top()[i]);
                            boards.top()[i] &= ~c.first;
                            cout << " => " << to_string(boards.top()[i]) << endl;
                            changed = true;
                        }
            return changed;
        }

        bool is_complete() const { return all_of(cbegin(boards.top()), cend(boards.top()), [this](const Cell &c) { return cell_value(c); }); }

        bool is_group_correct(const Group &group) const
        {
            // A group is correct if all the cells have a unique value and they are all themselves unique.

            auto r = accumulate(cbegin(group), cend(group), 0b0, [this](const unsigned short v, const char i) {
                auto c = boards.top()[i] & value_mask;
                return v | ((__builtin_popcount(c) == 1) ? c : 0);
            });
            return r == value_mask;
        }

        string to_string(Cell c) const
        {
            stringstream buf;
            c &= value_mask;
            if (__builtin_popcount(c) == 1) {
                buf << __builtin_ffs(c);
                return buf.str();
            }
            buf << "[ ";
            for (auto m = 0b1; m < 512; m <<= 1)
                if (c & m)
                    buf << __builtin_ffs(m) << ", ";
            buf << " ]";
            auto s = buf.str();
            s.erase(s.end() - 3);
            return s;
        }
    };
};

#endif //SUDOKU_SUDOKUSOLVER_H
