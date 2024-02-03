/*
 * 矩阵类
 */
#ifndef MAT_H
#define MAT_H

#include "common.h"
#include "vec.h"

template<int n>
struct dt;

template<int nrows, int ncols>
class Mat
{
public:
    Vec<ncols> rows[nrows] = { {} };

    Vec<ncols>& operator[] (const int idx) { assert(idx >= 0 && idx < nrows); return rows[idx]; }
    const Vec<ncols>& operator[] (const int idx) const { assert(idx >= 0 && idx < nrows); return rows[idx]; }

    Vec<nrows> col(const int idx)
        const
    {
        assert(idx >= 0 && idx < ncols);
        Vec<nrows> ret;
        for (int i = nrows; i--; ret[i] = rows[i][idx]);
        return ret;
    }

    void set_col(const int idx, const Vec<nrows>& v)
    {
        assert(idx >= 0 && idx < ncols);
        for (int i = nrows; i--; rows[i][idx] = v[i]);
    }

    static Mat<nrows, ncols> identity()
    {
        Mat<nrows, ncols> ret;
        for (int i = nrows; i--; )
            for (int j = ncols; j--; ret[i][j] = (i == j));
        return ret;
    }

    double det()
        const
    {
        return dt<ncols>::det(*this);
    }

    Mat<nrows - 1, ncols - 1> get_minor(const int row, const int col)
        const
    {
        Mat<nrows - 1, ncols - 1> ret;
        for (int i = nrows - 1; i--; )
            for (int j = ncols - 1; j--; ret[i][j] = rows[i < row ? i : i + 1][j < col ? j : j + 1]);
        return ret;
    }

    double cofactor(const int row, const int col)
        const
    {
        return get_minor(row, col).det() * ((row + col) % 2 ? -1 : 1);
    }

    Mat<nrows, ncols> adjugate()
        const
    {
        Mat<nrows, ncols> ret;
        for (int i = nrows; i--; )
            for (int j = ncols; j--; ret[i][j] = cofactor(i, j));
        return ret;
    }

    Mat<nrows, ncols> invert_transpose()
        const
    {
        Mat<nrows, ncols> ret = adjugate();
        return ret / (ret[0] * rows[0]);
    }

    Mat<nrows, ncols> invert()
        const
    {
        return invert_transpose().transpose();
    }

    Mat<ncols, nrows> transpose()
        const
    {
        Mat<ncols, nrows> ret;
        for (int i = ncols; i--; ret[i] = this->col(i));
        return ret;
    }
};

template<int nrows, int ncols>
Vec<nrows> operator*(const Mat<nrows, ncols>& lhs, const Vec<ncols>& rhs)
{
    Vec<nrows> ret;
    for (int i = nrows; i--; ret[i] = dot(lhs[i], rhs));
    return ret;
}

template<int R1, int C1, int C2>
Mat<R1, C2> operator*(const Mat<R1, C1>& lhs, const Mat<C1, C2>& rhs)
{
    Mat<R1, C2> result;
    for (int i = R1; i--; )
        for (int j = C2; j--; result[i][j] = dot(lhs[i], rhs.col(j)));
    return result;
}

template<int nrows, int ncols>
Mat<nrows, ncols> operator*(const Mat<nrows, ncols>& lhs, const double& val)
{
    Mat<nrows, ncols> result;
    for (int i = nrows; i--; result[i] = lhs[i] * val);
    return result;
}

template<int nrows, int ncols>
Mat<nrows, ncols> operator/(const Mat<nrows, ncols>& lhs, const double& val)
{
    Mat<nrows, ncols> result;
    for (int i = nrows; i--; result[i] = lhs[i] / val);
    return result;
}

template<int nrows, int ncols>
Mat<nrows, ncols> operator+(const Mat<nrows, ncols>& lhs, const Mat<nrows, ncols>& rhs)
{
    Mat<nrows, ncols> result;
    for (int i = nrows; i--; )
        for (int j = ncols; j--; result[i][j] = lhs[i][j] + rhs[i][j]);
    return result;
}

template<int nrows, int ncols>
Mat<nrows, ncols> operator-(const Mat<nrows, ncols>& lhs, const Mat<nrows, ncols>& rhs)
{
    Mat<nrows, ncols> result;
    for (int i = nrows; i--; )
        for (int j = ncols; j--; result[i][j] = lhs[i][j] - rhs[i][j]);
    return result;
}

template<int nrows, int ncols>
std::ostream& operator<<(std::ostream& out, const Mat<nrows, ncols>& m)
{
    for (int i = 0; i < nrows; i++) out << m[i] << std::endl;
    return out;
}

template<int n>
struct dt
{
    static double det(const Mat<n, n>& src)
    {
        double ret = 0;
        for (int i = n; i--; ret += src[0][i] * src.cofactor(0, i));
        return ret;
    }
};

template<>
struct dt<1>
{
    static double det(const Mat<1, 1>& src)
    {
        return src[0][0];
    }
};

#endif // !MAT_H