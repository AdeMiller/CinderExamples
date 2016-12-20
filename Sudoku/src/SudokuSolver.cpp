
#include "SudokuSolver.h"

namespace Sudoku
{
    int cell_value(Cell c) { return (__builtin_popcount(c & value_mask) == 1) ? __builtin_ffs(c & value_mask) : 0; }

    bool cell_guess(Cell c) { return c & guess_mask; }

    bool cell_locked(Cell c) { return c & locked_mask; }

    bool cell_certainty(const Cell &i, const Cell &j)
    {
        auto vi = __builtin_popcount(i & value_mask);
        auto vj = __builtin_popcount(j & value_mask);
        return (vi > 1 ? vi : 10) < (vj > 1 ? vj : 10);
    }

    const array<Group, 27> SudokuSolver::group_offsets = array<Group, 27>({
        Group({  0,  1,  2,  3,  4,  5,  6,  7,  8 }), Group({  9, 10, 11, 12, 13, 14, 15, 16, 17 }),
        Group({ 18, 19, 20, 21, 22, 23, 24, 25, 26 }), Group({ 27, 28, 29, 30, 31, 32, 33, 34, 35 }),
        Group({ 36, 37, 38, 39, 40, 41, 42, 43, 44 }), Group({ 45, 46, 47, 48, 49, 50, 51, 52, 53 }),
        Group({ 54, 55, 56, 57, 58, 59, 60, 61, 62 }), Group({ 63, 64, 65, 66, 67, 68, 69, 70, 71 }),
        Group({ 72, 73, 74, 75, 76, 77, 78, 79, 80 }),
        Group({  0,  9, 18, 27, 36, 45, 54, 63, 72 }), Group({  1, 10, 19, 28, 37, 46, 55, 64, 73 }),
        Group({  2, 11, 20, 29, 38, 47, 56, 65, 74 }), Group({  3, 12, 21, 30, 39, 48, 57, 66, 75 }),
        Group({  4, 13, 22, 31, 40, 49, 58, 67, 76 }), Group({  5, 14, 23, 32, 41, 50, 59, 68, 77 }),
        Group({  6, 15, 24, 33, 42, 51, 60, 69, 78 }), Group({  7, 16, 25, 34, 43, 52, 61, 70, 79 }),
        Group({  8, 17, 26, 35, 44, 53, 62, 71, 80 }),
        Group({  0,  1,  2,  9, 10, 11, 18, 19, 20 }), Group({  3,  4,  5, 12, 13, 14, 21, 22, 23 }),
        Group({  6,  7,  8, 15, 16, 17, 24, 25, 26 }), Group({ 27, 28, 29, 36, 37, 38, 45, 46, 47 }),
        Group({ 30, 31, 32, 39, 40, 41, 48, 49, 50 }), Group({ 33, 34, 35, 42, 43, 44, 51, 52, 53 }),
        Group({ 54, 55, 56, 63, 64, 65, 72, 73, 74 }), Group({ 57, 58, 59, 66, 67, 68, 75, 76, 77 }),
        Group({ 60, 61, 62, 69, 70, 71, 78, 79, 80 })
    });
};
