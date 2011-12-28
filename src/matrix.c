/*  matrix.c
 *
 *  Matrix operations.
 *
 *  (c) 2009-2011 Anton Olkhovik <ant007h@gmail.com>
 *
 *  This file is part of Mokomaze - labyrinth game.
 *
 *  Mokomaze is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mokomaze is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mokomaze.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <math.h>
#include "matrix.h"

// matrix inversion
// the result is put in Y
void MatrixInversion(float **A, int order, float **Y)
{
    // get the determinant of a
    float det = 1.0 / CalcDeterminant(A, order);

    // memory allocation
    float *temp = malloc((order - 1)*(order - 1) * sizeof(float));
    float **minor = malloc((order - 1) * sizeof(float*));
    for (int i = 0; i < order - 1; i++)
        minor[i] = temp + (i * (order - 1));

    for (int j = 0; j < order; j++)
    {
        for (int i = 0; i < order; i++)
        {
            // get the co-factor (matrix) of A(j,i)
            GetMinor(A, minor, j, i, order);
            Y[i][j] = det * CalcDeterminant(minor, order - 1);
            if ((i + j) % 2 == 1)
                Y[i][j] = -Y[i][j];
        }
    }

    // release memory
    free(temp);
    free(minor);
}

// calculate the cofactor of element (row,col)
void GetMinor(float **src, float **dest, int row, int col, int order)
{
    // indicate which col and row is being copied to dest
    int colCount = 0, rowCount = 0;

    for (int i = 0; i < order; i++)
    {
        if (i != row)
        {
            colCount = 0;
            for (int j = 0; j < order; j++)
            {
                // when j is not the element
                if (j != col)
                {
                    dest[rowCount][colCount] = src[i][j];
                    colCount++;
                }
            }
            rowCount++;
        }
    }
}

// Calculate the determinant recursively.
float CalcDeterminant(float **mat, int order)
{
    // order must be >= 0
    // stop the recursion when matrix is a single element
    if (order == 1)
        return mat[0][0];

    // the determinant value
    float det = 0;

    // allocate the cofactor matrix
    float **minor = malloc(sizeof(float*) * (order - 1));
    for (int i = 0; i < order - 1; i++)
        minor[i] = malloc(sizeof(float) * (order - 1));

    for (int i = 0; i < order; i++)
    {
        // get minor of element (0,i)
        GetMinor(mat, minor, 0, i, order);
        // the recursion is here!
        det += pow(-1.0, i) * mat[0][i] * CalcDeterminant(minor, order - 1);
    }

    // release memory
    for (int i = 0; i < order - 1; i++)
        free(minor[i]);
    free(minor);

    return det;
}
