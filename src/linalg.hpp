#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <float.h>

// uses an epsilon from <float.h> for zero checking
// this is used in the matrix reduction and inversion algorithms
static inline bool float_is_zero(float val) {
	return fabsf(val) < FLT_EPSILON;
}

static inline float float_sign(float val) {
	if(float_is_zero(val)) return 0.0f;
	return val > 0.0f ? 1.0f : -1.0f;
}

// helps control the precision of any printed vectors or matrices
#define PRINT_FLOAT_STR "%+f"

template <int N> struct Vector {
	static constexpr int Size = N;

	float data[N];

	// Instead of constructors we can use this to initialize a Vector<N>
	// This is so that we can aggregate initialize Vectors
	static Vector<N> zero() {
		Vector<N> vec;
		vec.set_zero();
		return vec;
	}

	static Vector<N> one() {
		Vector<N> vec;
		vec.set_one();
		return vec;
	}

	void set_zero() {
		for(int i = 0; i < N; i++)
			data[i] = 0.0f;
	}

	void set_one() {
		for(int i = 0; i < N; i++)
			data[i] = 1.0f;
	}

	void fix_zero() {
		for (int i = 0; i < N; i++) {
			if(float_is_zero(data[i])) {
				data[i] = 0.0f;
			}
		}
	}

	void print(FILE* outFile = stdout, bool newLine = true) const {
		if (1 == Size)
			fprintf(outFile, "[ " PRINT_FLOAT_STR " ] ", data[0]);
		else {
			fprintf(outFile, "\xe2\x94\x8c " PRINT_FLOAT_STR " \xe2\x94\x90\n", data[0]);       // ┌ ┐
			for (int i = 1; i < Size - 1; i++) {
				fprintf(outFile, "\xe2\x94\x82 " PRINT_FLOAT_STR " \xe2\x94\x82\n", data[i]);   // │ │
			}
			fprintf(outFile, "\xe2\x94\x94 " PRINT_FLOAT_STR " \xe2\x94\x98 ", data[Size - 1]); // └ ┘
		}
		if (newLine)
			fprintf(outFile, "\n");
	}

	float& operator[](int index) { return data[index]; }
	const float& operator[](int index) const { return data[index]; }
	float& x() { static_assert(N >= 1); return data[0]; }
	float& y() { static_assert(N >= 2); return data[1]; }
	float& z() { static_assert(N >= 3); return data[2]; }
	float& w() { static_assert(N >= 4); return data[3]; }
	const float& x() const { static_assert(N >= 1); return data[0]; }
	const float& y() const { static_assert(N >= 2); return data[1]; }
	const float& z() const { static_assert(N >= 3); return data[2]; }
	const float& w() const { static_assert(N >= 4); return data[3]; }

	constexpr Vector operator+() const { return *this; }

	constexpr Vector operator-() const {
		Vector result;
		for (int i = 0; i < N; ++i)
			result.data[i] = -data[i];

		return result;
	}

	constexpr Vector operator+(const Vector& rhs) const {
		Vector result;
		for (int i = 0; i < N; ++i)
			result.data[i] = data[i] + rhs.data[i];

		return result;
	}

	constexpr Vector operator-(const Vector& rhs) const {
		Vector result;
		for (int i = 0; i < N; ++i)
			result.data[i] = data[i] - rhs.data[i];

		return result;
	}

	constexpr Vector operator*(float scalar) const {
		Vector result;
		for (int i = 0; i < N; ++i)
			result.data[i] = data[i] * scalar;

		return result;
	}

	Vector operator/(float scalar) const {
		Vector result;
		if (float_is_zero(scalar))
			return result;

		const float inverse = 1.0f / scalar;
		for (int i = 0; i < N; ++i)
			result.data[i] = data[i] * inverse;

		return result;
	}

	Vector& operator+=(const Vector& rhs) {
		for (int i = 0; i < N; ++i)
			data[i] += rhs.data[i];

		return *this;
	}

	Vector& operator-=(const Vector& rhs) {
		for (int i = 0; i < N; ++i)
			data[i] -= rhs.data[i];

		return *this;
	}

	Vector& operator*=(float scalar) {
		for (int i = 0; i < N; ++i)
			data[i] *= scalar;

		return *this;
	}

	Vector& operator/=(float scalar) {
		if (float_is_zero(scalar)) {
			for (int i = 0; i < N; ++i) {
				data[i] = 0.0f;
			}
			return *this;
		}

		const float inverse = 1.0f / scalar;
		for (int i = 0; i < N; ++i) {
			data[i] *= inverse;
		}
		return *this;
	}

	constexpr bool operator==(const Vector& rhs) const {
		for (int i = 0; i < N; ++i) {
			if (data[i] != rhs.data[i]) {
				return false;
			}
		}
		return true;
	}

	constexpr bool operator!=(const Vector& rhs) const {
		return !(*this == rhs);
	}

	constexpr float length_sq() const {
		float sum = 0.0f;
		for (int i = 0; i < N; ++i) {
			sum += data[i] * data[i];
		}
		return sum;
	}

	float length() const {
		return sqrtf(static_cast<float>(length_sq()));
	}

	Vector normalized() const {
		const float len = length();

		if (float_is_zero(len))
			return Vector::zero();

		return *this / len;
	}

	constexpr static float dot(const Vector& a, const Vector& b) {
		float sum = 0.0f;
		for (int i = 0; i < N; ++i)
			sum += a.data[i] * b.data[i];

		return sum;
	}

	static float distance(const Vector& a, const Vector& b) {
		return (b - a).length();
	}

	static Vector project(const Vector& vec, const Vector& onto) {
		const float denom = dot(onto, onto);
		if (float_is_zero(denom)) {
			return Vector();
		}

		return onto * (dot(vec, onto) / denom);
	}

	constexpr static Vector homogeneous(float x, float y) {
		static_assert(Size > 2, "2d homogeneous coordinates require an extra coordinate for perspective division... i think");
		Vector v;
		v.set_zero();
		v[0] = x;
		v[1] = y;
		v[N-1] = 1.0f;
		return v;
	}

	constexpr static Vector<3> cross(const Vector<3>& a, const Vector<3>& b) {
		return Vector<3>{
			a.y() * b.z() - a.z() * b.y(),
			a.z() * b.x() - a.x() * b.z(),
			a.x() * b.y() - a.y() * b.x()
		};
	}
};

template <int N>
constexpr Vector<N> operator*(float scalar, const Vector<N>& vec) {
	return vec * scalar;
}

// should be stored in row-major order
template <int Rows, int Cols>
struct Matrix {
	static constexpr int NumRows = Rows;
	static constexpr int NumCols = Cols;
	static constexpr int MinDim = Rows < Cols ? Rows : Cols;
	union {
		float data[Rows * Cols];
		float rows[Rows][Cols];
	};

	//
	// identity and zero matrices can be used for initialiation of any sized matrix
	//

	static Matrix identity() {
		Matrix result;
		result.set_identity();
		return result;
	}

	static Matrix zero() {
		Matrix result;
		result.set_zero();
		return result;
	}

	//
	// 4x4 transformation matrices, these can be used for vertex transforms
	//

	static Matrix<4,4> ortho(float left, float right, float bottom, float top, float near, float far) {
		Matrix<4,4> mat;
		mat.set_zero();
		mat.get(0, 0) =  2.0f / (right - left);
		mat.get(1, 1) =  2.0f / (top - bottom);
		mat.get(2, 2) = -2.0f / (far - near);
		mat.get(3, 3) = 1.0f;
		mat.get(0, 3) = -(right + left) / (right - left);
		mat.get(1, 3) = -(top + bottom) / (top - bottom);
		mat.get(2, 3) = -(far + near) / (far - near);
		return mat;
	}

	static Matrix<4,4> scale(float x, float y, float z) {
		Matrix<4,4> mat;
		mat.set_zero();
		mat.get(0, 0) = x;
		mat.get(1, 1) = y;
		mat.get(2, 2) = z;
		mat.get(3, 3) = 1.0f;
		return mat;
	}

	static Matrix<4,4> translate(float x, float y, float z) {
		Matrix<4,4> mat;
		mat.set_identity();
		mat.get(0, 3) = x;
		mat.get(1, 3) = y;
		mat.get(2, 3) = z;
		return mat;
	}

	void set_zero() {
		memset(data, 0, sizeof(float) * Rows * Cols);
	}

	void set_identity() {
		set_zero();
		for (int i = 0; i < MinDim; i++) {
			get(i, i) = 1.0f;
		}
	}

	void fix_zero() {
		for (int i = 0; i < Rows * Cols; i++) {
			if(float_is_zero(data[i])) {
				data[i] = 0.0f;
			}
		}
	}

	void print(FILE* outFile = stdout, bool newLine = true) const {
		if (1 == Rows && 1 == Cols) {
			fprintf(outFile, "[ " PRINT_FLOAT_STR " ] ", get(0, 0));
		} else if (1 == Rows) {
			// row vector: [ f f f ]
			fprintf(outFile, "[ ");
			for (int c = 0; c < Cols; c++)
				fprintf(outFile, "" PRINT_FLOAT_STR " ", get(0, c));
			fprintf(outFile, "]");
		} else if (1 == Cols) {
			// column vector (done the same way as Vector<>::print)
			for (int r = 0; r < Rows; r++) {
				if (r == 0)
					fprintf(outFile, "\xe2\x94\x8c " PRINT_FLOAT_STR " \xe2\x94\x90\n", get(r, 0));
				else if (r == Rows - 1)
					fprintf(outFile, "\xe2\x94\x94 " PRINT_FLOAT_STR " \xe2\x94\x98\n", get(r, 0));
				else
					fprintf(outFile, "\xe2\x94\x82 " PRINT_FLOAT_STR " \xe2\x94\x82\n", get(r, 0));
			}
		} else {
			// Full matrix
			for (int r = 0; r < Rows; r++) {
				if (r == 0)
					fprintf(outFile, "\xe2\x94\x8c ");  // ┌
				else if (r == Rows - 1)
					fprintf(outFile, "\xe2\x94\x94 ");  // └
				else
					fprintf(outFile, "\xe2\x94\x82 ");  // │

				for (int c = 0; c < Cols; c++)
					fprintf(outFile, "" PRINT_FLOAT_STR " ", get(r, c));

				if (r == 0)
					fprintf(outFile, "\xe2\x94\x90\n");  // ┐
				else if (r == Rows - 1)
					fprintf(outFile, "\xe2\x94\x98\n");  // ┘
				else
					fprintf(outFile, "\xe2\x94\x82\n");  // │
			}
		}

		if (newLine)
			fprintf(outFile, "\n");
	}

	Vector<Rows> get_column(int col) {
		Vector<Rows> column;
		for(int i = 0; i < Rows; i++)
			column[i] = get(i, col);
		return column;
	}

	void set_column(const Vector<Rows>& column, int col) {
		for(int i = 0; i < Rows; i++)
			get(i, col) = column[i];
	}

	Vector<Cols> get_row(int rowIdx) {
		Vector<Cols> row;
		for(int i = 0; i < Cols; i++)
			row[i] = get(rowIdx, i);
		return row;
	}

	void set_row(const Vector<Cols>& row, int rowIdx) {
		for(int i = 0; i < Cols; i++)
			get(rowIdx, i) = row[i];
	}

	float& get(int row, int col) { return data[(row * Cols) + col]; }
	const float& get(int row, int col) const { return data[(row * Cols) + col]; }

	Matrix<Cols, Rows> transposed() const {
		Matrix<Cols, Rows> result;
		for (int i = 0; i < Rows; i++) {
			for (int j = 0; j < Cols; j++) {
				result.get(j, i) = get(i, j);
			}
		}

		return result;
	}

	// elementary row operations (for Gaussian Elimination)
	void row_multiply(int row, float scalar) {
		for (int i = 0; i < Cols; i++)
			get(row, i) *= scalar;
	}

	void row_muladd(int targetRow, int sourceRow, float coefficient = 1.0f) {
		for (int i = 0; i < Cols; i++)
			get(targetRow, i) += coefficient * get(sourceRow, i);
	}

	void row_swap(int row1, int row2) {
		constexpr int ROW_SIZE = sizeof(float) * Cols;

		// place row1 in temp buffer
		float row[Cols];
		memcpy(row, data + (row1 * Cols), ROW_SIZE);

		// now overwrite row1 with row2
		memcpy(data + (row1 * Cols), data + (row2 * Cols), ROW_SIZE);

		// and overwrite row2 with the temp buffer containing the original row1
		memcpy(data + (row2 * Cols), row, ROW_SIZE);
	}

	// invert a square matrix using Gauss-Jordan Elimination
	Matrix<Rows, Rows> inversed() const {
		static_assert(Rows == Cols, "inverse not defined for non-square matrices");
		constexpr int N = Rows;

		Matrix<Rows, Rows> current = *this;
		Matrix<Rows, Rows> inverse = Matrix<Rows, Rows>::identity();

		// we iterate by columns, but keep track of the pivot row and only increment it if the current column does not correspond to a free variable
		// (there is some nonzero entry either in the pivot entry spot or below it)
		int pivotRow = 0;

		for(int pivotCol = 0; pivotCol < N && pivotRow < N; pivotCol++) {
			// if the potential pivot entry is 0, then we MIGHT need a row swap
			if(float_is_zero(current.get(pivotRow, pivotCol))) {
				// to determine if we need a row swap or not, we need to know if there are any nonzero entries below the potential pivot entry
				// if there aren't any, then the current column corresponds to a free variable column and we need to go to the next column
				// but if there is one, then we need to swap with the row that contains the largest absolute entry in the current column
				int bestRowWithNonzeroEntryBelowPivot = -1;
				float largestAbsoluteEntry = 0.0f;
				for(int i = pivotRow + 1; i < N; i++) {
					if(fabsf(current.get(i, pivotCol)) > largestAbsoluteEntry)
						bestRowWithNonzeroEntryBelowPivot = i;
				}

				if(bestRowWithNonzeroEntryBelowPivot >= 0) {
					current.row_swap(pivotRow, bestRowWithNonzeroEntryBelowPivot);
					inverse.row_swap(pivotRow, bestRowWithNonzeroEntryBelowPivot);
				} else continue;
				// so in the case that the potential pivot entry and all entries below it are zero, then this column corresponds to a free variable
				// and we want to simply increment the current column, but not the current row
			}

			// now that we've done a row swap, we want to ensure all entries above and below the pivot entry at (pivotRow, pivotCol) are 0
			// so first we normalize the pivot row (note that the above code should be a good enough guard against DivBy0)
			float pivotNormalizationFactor = 1.0f / current.get(pivotRow, pivotCol);
			current.row_multiply(pivotRow, pivotNormalizationFactor);
			inverse.row_multiply(pivotRow, pivotNormalizationFactor);

			// next, we need to cancel out all nonzero entries above and below (pivotRow, pivotCol)
			for(int cancelRow = 0; cancelRow < N; cancelRow++) {
				if(cancelRow == pivotRow) continue;
				float cancelEntry = current.get(cancelRow, pivotCol);

				if(!float_is_zero(cancelEntry)) {
					// so right now the pivot entry is 1, and we want to subtract cancelEntry * 1 from row that doesn't contain this pivot entry
					current.row_muladd(cancelRow, pivotRow, -cancelEntry);
					inverse.row_muladd(cancelRow, pivotRow, -cancelEntry);
				}
			}

			pivotRow++;
		}

		inverse.fix_zero();
		return inverse;
	}

	constexpr Matrix operator+() const { return *this; }

	Matrix operator-() const {
		Matrix result;
		for (int i = 0; i < Rows * Cols; i++)
			result.data[i] = -data[i];

		return result;
	}

	Matrix operator+(const Matrix& rhs) const {
		Matrix result;
		for (int i = 0; i < Rows * Cols; i++)
			result.data[i] = data[i] + rhs.data[i];

		return result;
	}

	Matrix operator-(const Matrix& rhs) const {
		Matrix result;
		for (int i = 0; i < Rows * Cols; i++)
			result.data[i] = data[i] - rhs.data[i];

		return result;
	}

	Matrix operator*(float scalar) const {
		Matrix result;
		for (int i = 0; i < Rows * Cols; i++)
			result.data[i] = data[i] * scalar;

		return result;
	}

	Matrix operator/(float scalar) const {
		if (float_is_zero(scalar))
			return Matrix::zero();

		Matrix result;
		const float inverse = 1.0f / scalar;
		for (int i = 0; i < Rows * Cols; i++)
			result.data[i] = data[i] * inverse;

		return result;
	}

	Matrix& operator+=(const Matrix& rhs) {
		for (int i = 0; i < Rows * Cols; i++)
			data[i] += rhs.data[i];

		return *this;
	}

	Matrix& operator-=(const Matrix& rhs) {
		for (int i = 0; i < Rows * Cols; i++) {
			data[i] -= rhs.data[i];

			return *this;
		}
	}

	Matrix& operator*=(float scalar) {
		for (int i = 0; i < Rows * Cols; i++) {
			data[i] *= scalar;

			return *this;
		}
	}

	Matrix& operator/=(float scalar) {
		if (float_is_zero(scalar)) {
			set_zero();
			return *this;
		}

		const float inverse = 1.0f / scalar;
		for (int i = 0; i < Rows * Cols; i++)
			data[i] *= inverse;

		return *this;
	}

	template <int OtherCols>
	Matrix<Rows, OtherCols> operator*(const Matrix<Cols, OtherCols>& rhs) const {
		Matrix<Rows, OtherCols> result;
		result.set_zero();
		for (int i = 0; i < Rows; i++) {
			for (int j = 0; j < OtherCols; j++) {
				float sum = 0.0f;
				for (int k = 0; k < Cols; k++) {
					sum += get(i, k) * rhs.get(k, j);
				}
				result.get(i, j) = sum;
			}
		}
		return result;
	}

	Vector<Rows> operator*(const Vector<Cols>& rhs) const {
		Vector<Rows> result;
		for (int i = 0; i < Rows; i++) {
			float sum = 0.0f;
			for (int j = 0; j < Cols; j++) {
				sum += get(i, j) * rhs[j];
			}
			result[i] = sum;
		}
		return result;
	}
};

template <int Rows, int Cols>
Matrix<Rows, Cols> operator*(float scalar, const Matrix<Rows, Cols>& mat) {
	return mat * scalar;
}

using Matrix4 = Matrix<4, 4>;
using Matrix3 = Matrix<3, 3>;
using Vector2 = Vector<2>;
using Vector3 = Vector<3>;
using Vector4 = Vector<4>;

Vector2 rotate(const Vector2& vec, float angle) {
	float vecAngle = atan2f(vec.y(), vec.x());
	vecAngle += angle;
	return { cosf(vecAngle), sinf(vecAngle) };
}

//
// meat n potatoes
//

// Notes from this page should be able to explain the goal: https://www.cs.cmu.edu/~16385/s17/Slides/10.2_2D_Alignment__DLT.pdf
//
// So to properly compute a homography using DLT, we need to also consider a per-pointwise-correspondence scaling factor alpha_i.
// See the following case for any pointwise correspondence (x, y) --> (x', y'), where xy is the square UVs and x'y' is the mutated quad UVs
//
// | x' |         | h00 h01 h02 |   | x |
// | y' | = alpha | h10 h11 h12 | * | y |
// | 1  |         | h20 h21 h22 |   | 1 |
//
// Or written as X' = alpha * A * X
//
// This is the equation for a single pair of points. We will first convert this matrix equation to a system of equations.
// x' = alpha(h00 * x + h01 * y + h02)
// y' = alpha(h10 * x + h11 * y + h12)
// 1  = alpha(h20 * x + h21 * y + h22)
//
// Swap the order of the last one to get alpha(h20 * x + h21 * y + h22) = 1 and multiply the first two equations by this to get:
// x' * alpha(h20 * x + h21 * y + h22) = alpha(h00 * x + h01 * y + h02)
// y' * alpha(h20 * x + h21 * y + h22) = alpha(h10 * x + h11 * y + h12)
//
// Doing this lets us divide alpha on both sides to get:
// x' * (h20 * x + h21 * y + h22) = h00 * x + h01 * y + h02
// y' * (h20 * x + h21 * y + h22) = h10 * x + h11 * y + h12
//
// This is the form we want since now we don't have to worry about the implicit scale factor anymore.
//
// Now move all of the terms on the right hand side to the left, to get a homogeneous system of 2 equations:
// (note that I'm also distributing the terms in the parentheses)
// h20xx' + h21yx' + h22x' - h00x - h01y - h02 = 0
// h20xy' + h21yy' + h22y' - h10x - h11y - h12 = 0
//
// Now, flatten the homography matrix (A) into a vector (H) containing the same entries (transposed here since it should be a column vector):
// Ht = | h00 h01 h02 h10 h11 h12 h20 h21 h22 |
//
// And rewrite the 2 equations above as a 2x9 matrix times H as follows
//
// | -x -y -1  0  0  0 xx' yx' x'|   | h00 |
// |  0  0  0 -x -y -1 xy' yy' y'| * | h01 | = 0
//                                   | h02 |
//                                   | h10 |
//                                   | h11 |
//                                   | h12 |
//                                   | h20 |
//                                   | h21 |
//                                   | h22 |
//
// And from here, we just need to stack 4 of these 2x9 matrices vertically (one for each correspondence) to get a single 8x9 matrix
// (Let's call it D, since it's our DLT matrix)
//
// In the end, the equation that we want to solve is D * H = 0, which we do by taking the SVD of D.
// Taking this SVD gives us D = U * Sigma * Vt.
//
// From here:
// - If m <  n ~ We can just take the nth row of Vt to be our solution H.
// - If m >= n ~ We find the column of Sigma with the smallest singular values along the diagonal. Denote this as column i.
//   Our solution H will then be the ith row of Vt, or the ith column of V (we'd have to transpose Vt).
//
// Explanation:
// H can take on any value in the Null space of D (since Null space is just all values x such that Dx = 0).
// Using the dimension theorem, recall that for D, which is an 8x9 matrix, rank(D) + nullity(D) = 9.
// Since there are 8 rows, that means that the maximum possible rank is 8, and the minimum possible nullity is 1.
// This means that we'll always have at least a single basis vector in the Null space, which we want to take as our solution H.
//
// Going to the use of SVD, since there are 8 rows and 9 columns in Sigma, there are only 8 singular values in Sigma,
// meaning that all remaining singular values (or more specifically, the 9th singular value) will automatically be 0.
// This 9th singular value corresponds to the basis vector H of the Null space that is always present
// (and we can obtain it by taking the 9th right singular vector from the SVD).
//
// We can always have more singular values that are 0, meaning more basis vectors in the Null space,
// but this isn't really all that important unless we increase the number of pointwise correspondences.
// In that scenario, our system will be overdetermined, and we'll have to instead select for the smallest singular value
// as it'll give us the "best" solution out of the bunch.
//
// Therefore... TODO:
// - probably rewrite this blurb/section to be a bit shorter and less repetetive
// - implement SVD, which means look up the Golub-Kahan-Reisch algorithm... should be fine for an 8x9 matrix

template<int Rows, int Cols>
struct SvdResult {
	// Note that if for we compute Sigma and Vt, but not U, we can simply get U by taking
	// u_i = Av_i / sigma_i
	// This comes from the fact that A * v_i = sigma_i * u_i
	Matrix<Rows, Rows> U;     // left singular vectors
	Matrix<Rows, Cols> Sigma; // contains the singular values
	Matrix<Cols, Cols> Vt;    // right singular vectors (in rows)

	// For general Householder matrices
	// H = I - 2vvT/vTv
	// To simplify, we separate 2/vTv into separate constant t = 2/vTv

	// HA = I - (t * v * vTA)
	//      A - tv * vTA
	template<int Rows, int Cols>
	static void left_householder_mul(const Vector<Rows>& v, Matrix<Rows, Cols>& A) {
		if(float_is_zero(v.length_sq())) return;

		Vector<Cols> rowVtA; // we don't have row vector * matrix implemented, we do it manually here
		rowVtA.set_zero();

		for(int i = 0; i < Rows; i++) {
			for(int j = 0; j < Cols; j++) {
				rowVtA[j] += v[i] * A.get(i, j);
			}
		}

		const float t = 2.0f / v.length_sq();
		for(int i = 0; i < Rows; i++) {
			const float tv = t * v[i];
			for(int j = 0; j < Cols; j++) {
				A.get(i, j) -= tv * rowVtA[j];
			}
		}
	}

	// AH = A(I - (t * v * vT))
	//      A - Av * tvT
	template<int Rows, int Cols>
	static void right_householder_mul(Matrix<Rows, Cols>& A, const Vector<Cols>& v) {
		if(float_is_zero(v.length_sq())) return;

		Vector<Rows> Av = A * v; // luckily we have matrix * vector implemented

		const float t = 2.0f / v.length_sq();
		for(int i = 0; i < Rows; i++) {
			const float tAv = t * Av[i];
			for(int j = 0; j < Cols; j++) {
				A.get(i, j) -= tAv * v[j];
			}
		}
	}

	template<int N>
	static Matrix<N, N> givens_rotation(int p, int q, float theta) {
		Matrix<N, N> matrix = Matrix<N, N>::identity();

		matrix.get(p, p) = cosf(theta);
		matrix.get(q, q) = matrix.get(p, p);
		matrix.get(p, q) = sinf(theta);
		matrix.get(q, p) = -matrix.get(p, q);

		return matrix;
	}

	// Bidiagonalizes the A matrix according to the GK (Golub-Kahan) paper on SVD
	// -> Performs multiple iterations of PA -> AQ -> ... -> B, where B is bidiagonal
	template<int Rows, int Cols>
	static void bidiagonalize(Matrix<Rows, Cols>& A) {
		for (int i = 0; i < A.MinDim; i++) {
			Vector<Rows> x = A.get_column(i);

			// zero out elements before i to not disturb any of the previous work we've done
			for(int j = 0; j < i; j++)
				x[j] = 0.0f;

			float xNorm = x.length(); // denoted as s_k in GK

			if(!float_is_zero(xNorm)) {
				const float aii = A.get(i, i);

				x[i] = sqrtf(0.5f * (1.0f + (fabsf(aii) / xNorm))); // x_i is set according to x_k^(k) in GK

				float c; // compute coefficient for elements i+1 to Rows-1
				if(!float_is_zero(aii))
					c = 1.0f / (2.0f * xNorm * x[i] * float_sign(aii));
				else
					// since aii = 0, we can pick whatever rotation we want to zero out the ith column
					// in real numbers we can use +1 or 1, and it doesn't really matter in the aii = 0 case
					c = 1.0f / (2.0f * xNorm * x[i]);

				for(int j = i + 1; j < Rows; j++)
					x[j] *= c;

				left_householder_mul<Rows, Cols>(x, A); // this corresponds to multiplying by P
			}

			Vector<Cols> y = A.get_row(i);

			// zero out elements before and including i to not disturb any of the previous work we've done
			for(int j = 0; j <= i; j++)
				y[j] = 0.0f;

			float yNorm = y.length(); // denoted as t_k in GK

			// since we want to zero out all elements to the right of the superdiagonal
			// we only perform the cancellation if we have a column to the right of our current column
			if(!float_is_zero(yNorm) && (i + 1 < A.MinDim)) {
				const float aii1 = A.get(i, i + 1); // y_i+1 is set according to y_k+1^(k) in GK
				y[i + 1] = sqrtf(0.5f * (1.0f + (fabsf(aii1) / yNorm)));

				float d; // compute coefficient for elements i+2 to Cols - 1
				if(!float_is_zero(aii1))
					d = 1.0f / (2.0f * yNorm * y[i + 1] * float_sign(aii1));
				else
					d = 1.0f / (2.0f * yNorm * y[i + 1]);

				for(int j = i + 2; j < Cols; j++)
					y[j] *= d;

				right_householder_mul<Rows, Cols>(A, y); // this corresponds to multiplying by Q
			}

			// NOTE: We don't handle the "else" case when checking if each norm is 0,
			// since in both cases it would lead to multiplying by an identity matrix
			// (which is the same as not performing a multiplication at all)
		}
	}

	// computes the singular value decomposition of a matrix using the Golub-Kahan-Reisch algorithm
	static SvdResult compute(const Matrix<Rows, Cols>& A) {
		SvdResult result;
		result.Sigma = A;
		bidiagonalize(result.Sigma);
		result.Sigma.print();

		//
		// Now we apply a QR algorithm to A
		//

		// TODO...

		return result;
	}
};

// Usage: mutated should be the quad UVs set by the program, and original should be a fresh set of square UVs
//        additionally, pass them in order of TL BL BR TR
Matrix4 compute_homography(const Vector2 mutated[4], const Vector2 original[4]) {
	Matrix<8, 9> dltMatrix;
	dltMatrix.set_zero();

	for(int i = 0; i < 4; i++) {
		// | -x -y -1  0  0  0 xx' yx' x' |
		// |  0  0  0 -x -y -1 xy' yy' y' |

		int row1 = (i * 2);
		int row2 = (i * 2) + 1;
		const float x = original[i].x();
		const float y = original[i].y();
		const float xp = mutated[i].x();
		const float yp = mutated[i].y();

		dltMatrix.get(row1, 0) = -x;
		dltMatrix.get(row1, 1) = -y;
		dltMatrix.get(row1, 2) = -1.0f;
		dltMatrix.get(row1, 6) = x * xp;
		dltMatrix.get(row1, 7) = y * xp;
		dltMatrix.get(row1, 8) = xp;

		dltMatrix.get(row2, 3) = -x;
		dltMatrix.get(row2, 4) = -y;
		dltMatrix.get(row2, 5) = -1.0f;
		dltMatrix.get(row2, 6) = x * yp;
		dltMatrix.get(row2, 7) = y * yp;
		dltMatrix.get(row2, 8) = yp;
	}

	// Explanation for why our homography is obtained from the 9th row of svd.Vt:
	//
	// Since the 8x9 matrix represents an underdetermined system, we only have 8 singular values in svd.Sigma.
	// This means that the 9th one is automatically zero by default.
	// Recall that the relationship between right and left singular vectors is given by A * v_i = sigma_i * u_i.
	// So if sigma_9 = 0, then this implies Av_9 = 0, and thus v_9 should be a nonzero vector that A maps to 0.
	// This means that v_9 solves the homogeneous equation Ax=0.
	//
	// Note that for over-determined systems (systems with more than 4 pointwise correspondences)
	// We would have to look through the 9 singular values for the smallest one, and pick that row from svd.Vt.
	//
	// Additionally, if all we're after is the right singular vectors of A, then it's the same thing as
	// diagonalizing AtA, since the eigenvalues give us squared singular values and the eigenvectors
	// are just the right singular vectors. We wouldn't do this on a computer though, since it results in precision loss.
	//

	auto svd = SvdResult<8, 9>::compute(dltMatrix);
	Vector<9> homographyVector = svd.Vt.get_row(8);

	// now we unroll the flattened 9-vector of the homography entries into a 3x3 homography matrix
	Matrix3 homography; // no need to initialize on declaration, we do that below
	homography.get(0, 0) = homographyVector[0];
	homography.get(0, 1) = homographyVector[1];
	homography.get(0, 2) = homographyVector[2];
	homography.get(1, 0) = homographyVector[3];
	homography.get(1, 1) = homographyVector[4];
	homography.get(1, 2) = homographyVector[5];
	homography.get(2, 0) = homographyVector[6];
	homography.get(2, 1) = homographyVector[7];
	homography.get(2, 2) = homographyVector[8];

	// this is purely a sokol_gfx workaround, since it sokol-shdc (shader compiler) doesn't allow for mat3
	// so we need to embed the 3x3 homography in a 4x4 matrix
	Matrix4 embeddedHomography = Matrix4::identity();

	// | a00 a01 a02 |     | a00 a01 0 a02 |
	// | a10 a11 a12 | --> | a10 a11 0 a12 |
	// | a20 a21 a22 |     |  0   0  1  0  |
	//                     | a20 a21 0 a22 |

	// and then this gets right-multiplied by a homogeneous coordinate of the form (x, y, 0, 1)
	// to give another coordinate of the form (x', y', 0, w)
	// we'll then divide x' and y' by w to get the final coordinate (x'/w, y'/w)

	embeddedHomography.get(0, 0) = homography.get(0, 0);
	embeddedHomography.get(1, 0) = homography.get(1, 0);
	embeddedHomography.get(0, 1) = homography.get(0, 1);
	embeddedHomography.get(1, 1) = homography.get(1, 1);
	embeddedHomography.get(3, 0) = homography.get(2, 0);
	embeddedHomography.get(3, 1) = homography.get(2, 1);
	embeddedHomography.get(0, 3) = homography.get(0, 2);
	embeddedHomography.get(1, 3) = homography.get(1, 2);
	embeddedHomography.get(3, 3) = homography.get(2, 2);

	return embeddedHomography;
}



// This was the old method that used the pseudo inverse to calculate
// a matrix that attempts to map each pointwise correspondence.
// It doesn't take into account an implicit scale factor for each pair,
// so the calculated transformation matrix ends up being affine instead of projective.

/*
Matrix3 compute_affine(const Vector2 mutated[4], const Vector2 square[4]) { // should be passed in order of TL BL BR TR
	// We want a matrix A that we can multiply square UVs (x,y) by to get our mutated quad UVs (x',y')
	// we can model this as Y=AX... if we take the right pseudo inverse of X to be Xp such that XXp=I
	// then we can multiply it on both sides to get YXp=A
	Matrix<3, 4> Y, X;
	for(int i = 0; i < 4; i++) {
		Y.set_column(Vector3::homogeneous(mutated[i].x(), mutated[i].y()), i);
		X.set_column(Vector3::homogeneous(square[i].x(), square[i].y()), i);
	}

	Matrix<4, 3> Xt = X.transposed();

	// so now compute pseudo inverse of X
	Matrix<4, 3> Xp = Xt * (X * Xt).inversed();

	return Y * Xp; // since A = YXp
}
*/