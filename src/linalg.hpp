#pragma once

#include <math.h>
#include <stdio.h>

template <int N> struct Vector {
	static constexpr int Size = N;

	// I was NOT aboutta find a way to do some crazy code duplication
	// with partial specialization, so henceforth, I have declared all vectors
	// will have a minimum size of 4 floats, even if N < 4
	union {
		float data[N];
		struct {
			float x, y, z, w;
		};
	};

	// Instead of constructors we can use this to initialize a Vector<N>
	// This is so that we can aggregate initialize Vectors
	static Vector<N> zero() {
		Vector<N> vec;
		vec.set_zero();
		return vec;
	}

	void set_zero() {
		for(int i = 0; i < N; i++) {
			data[i] = 0.0f;
		}
	}

	void print(FILE* outFile = stdout, bool newLine = true) {
		if (1 == Size)
			fprintf(outFile, "[ %f ] ", data[0]);
		else {
			fprintf(outFile, "\xe2\x94\x8c %f \xe2\x94\x90\n", data[0]);       // ┌ ┐
			for (int i = 1; i < Size - 1; i++) {
				fprintf(outFile, "\xe2\x94\x82 %f \xe2\x94\x82\n", data[i]);   // │ │
			}
			fprintf(outFile, "\xe2\x94\x94 %f \xe2\x94\x98 ", data[Size - 1]); // └ ┘
		}
		if (newLine)
			fprintf(outFile, "\n");
	}

	float& operator[](int index) { return data[index]; }
	const float& operator[](int index) const { return data[index]; }

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
		if (0.0f == scalar)
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
		if (0.0f == scalar) {
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
		if (0.0f == len) {
			return Vector();
		}
		return *this / len;
	}

	constexpr static Vector<3> homogeneous(float x, float y) {
		return Vector<3>{x, y, 1.0f};
	}

	constexpr static float dot(const Vector& a, const Vector& b) {
		float sum = 0.0f;
		for (int i = 0; i < N; ++i)
			sum += a.data[i] * b.data[i];

		return sum;
	}

	constexpr static Vector cross(const Vector& a, const Vector& b) {
		static_assert(3 == N, "cross product only defined for Vector3");
		return Vector(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		);
	}

	static float distance(const Vector& a, const Vector& b) {
		return (b - a).length();
	}

	static Vector project(const Vector& vec, const Vector& onto) {
		const float denom = dot(onto, onto);
		if (0.0f == denom) {
			return Vector();
		}

		return onto * (dot(vec, onto) / denom);
	}
};

template <int N>
constexpr Vector<N> operator*(float scalar, const Vector<N>& vec) {
	return vec * scalar;
}

// should be stored in row-major order
template <int Rows, int Cols>
struct Matrix {
	static constexpr int rows = Rows;
	static constexpr int cols = Cols;
	float data[Rows * Cols];

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

	void set_zero() {
		for (int i = 0; i < Rows * Cols; i++)
			data[i] = 0.0f;
	}

	void set_identity() {
		set_zero();
		constexpr int MinDim = Rows < Cols ? Rows : Cols;

		for (int i = 0; i < MinDim; i++) {
			get(i, i) = 1.0f;
		}
	}

	void print(FILE* outFile = stdout, bool newLine = true) {
		if (1 == Rows && 1 == Cols) {
			fprintf(outFile, "[ %f ] ", get(0, 0));
		} else if (1 == Rows) {
			// row vector: [ f f f ]
			fprintf(outFile, "[ ");
			for (int c = 0; c < Cols; c++)
				fprintf(outFile, "%f ", get(0, c));
			fprintf(outFile, "]");
		} else if (1 == Cols) {
			// column vector (done the same way as Vector<>::print)
			for (int r = 0; r < Rows; r++) {
				if (r == 0)
					fprintf(outFile, "\xe2\x94\x8c %f \xe2\x94\x90\n", get(r, 0));
				else if (r == Rows - 1)
					fprintf(outFile, "\xe2\x94\x94 %f \xe2\x94\x98\n", get(r, 0));
				else
					fprintf(outFile, "\xe2\x94\x82 %f \xe2\x94\x82\n", get(r, 0));
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
					fprintf(outFile, "%f ", get(r, c));

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
//		printf("multiplying row %d by %f\n", row, scalar);

		for (int i = 0; i < Cols; i++)
			get(row, i) *= scalar;
	}

	void row_muladd(int targetRow, int sourceRow, float coefficient = 1.0f) {
//		printf("adding %f times row %d to row %d\n", coefficient, sourceRow, targetRow);

		for (int i = 0; i < Cols; i++)
			get(targetRow, i) += coefficient * get(sourceRow, i);
	}

	void row_swap(int row1, int row2) {
//		printf("swapping rows %d and %d\n", row1, row2);

		constexpr int ROW_SIZE = sizeof(float) * Cols;

		// place row1 in temp buffer
		float row[Cols];
		memcpy(row, data + (row1 * Cols), ROW_SIZE);

		// now overwrite row1 with row2
		memcpy(data + (row1 * Cols), data + (row1 * Cols), ROW_SIZE);

		// and overwrite row2 with the temp buffer containing the original row1
		memcpy(data + (row2 * Cols), row, ROW_SIZE);
	}

	//
	Matrix<Rows, Cols> rref() const {
		Matrix<Rows, Cols> reduced = *this;

		// we iterate by columns, but keep track of the pivot row and only increment it if the current column does not correspond to a free variable
		// (there is some nonzero entry either in the pivot entry spot or below it)
		int pivotRow = 0;

		for(int pivotCol = 0; pivotCol < Cols; pivotCol++) {
			// if the potential pivot entry is 0, then we MIGHT need a row swap
			if(reduced.get(pivotRow, pivotCol) == 0) {
				// to determine if we need a row swap or not, we need to know if there are any nonzero entries below the potential pivot entry
				// if there aren't any, then the current column corresponds to a free variable column and we need to go to the next column
				// but if there is one, then we can do a row swap with it
				bool foundNonzeroEntryBelowPivot = false;
				for(int i = pivotRow + 1; i < Rows; i++) {
					if(reduced.get(i, pivotCol) != 0) {
						foundNonzeroEntryBelowPivot = true;
						reduced.row_swap(pivotRow, i);
						break;
					}
				}

				// so in the case that the potential pivot entry and all entries below it are zero, then this column corresponds to a free variable
				// and we want to simply increment the current column, but not the current row
				if(!foundNonzeroEntryBelowPivot) {
					continue;
				}
			}

			// now that we've done a row swap, we want to ensure all entries above and below the pivot entry at (pivotRow, pivotCol) are 0
			// so first we normalize the pivot row (note that the above code should be a good enough guard against DivBy0)
			float pivotNormalizationFactor = 1.0f / reduced.get(pivotRow, pivotCol);
			if(1.0f != pivotNormalizationFactor) {
				reduced.row_multiply(pivotRow, pivotNormalizationFactor);
			}

			// next, we need to cancel out all nonzero entries above and below (pivotRow, pivotCol)
			for(int cancelRow = 0; cancelRow < Rows; cancelRow++) {
				if(cancelRow == pivotRow) continue;
				float cancelEntry = reduced.get(cancelRow, pivotCol);

				if(cancelEntry != 0.0f) {
					// so right now the pivot entry is 1, and we want to subtract cancelEntry * 1 from row that doesn't contain this pivot entry
					reduced.row_muladd(cancelRow, pivotRow, -cancelEntry);
				}
			}

			pivotRow++;
		}

		return reduced;
	}

	// invert a square matrix using the same Gauss-Jordan Elimination algorithm as above
	Matrix<Rows, Rows> inversed() const {
		static_assert(Rows == Cols, "inverse not defined for non-square matrices");
		constexpr int N = Rows;

		Matrix<Rows, Rows> current = *this;
		Matrix<Rows, Rows> inverse = Matrix<Rows, Rows>::identity();

		int pivotRow = 0;

		for(int pivotCol = 0; pivotCol < N; pivotCol++) {
			if(current.get(pivotRow, pivotCol) == 0) {
				bool foundNonzeroEntryBelowPivot = false;
				for(int i = pivotRow + 1; i < N; i++) {
					if(current.get(i, pivotCol) != 0) {
						foundNonzeroEntryBelowPivot = true;
						current.row_swap(pivotRow, i);
						inverse.row_swap(pivotRow, i);
						break;
					}
				}

				if(!foundNonzeroEntryBelowPivot) {
					continue;
				}
			}

			float pivotNormalizationFactor = 1.0f / current.get(pivotRow, pivotCol);
			if(1.0f != pivotNormalizationFactor) {
				current.row_multiply(pivotRow, pivotNormalizationFactor);
				inverse.row_multiply(pivotRow, pivotNormalizationFactor);
			}

			for(int cancelRow = 0; cancelRow < N; cancelRow++) {
				if(cancelRow == pivotRow) continue;
				float cancelEntry = current.get(cancelRow, pivotCol);

				if(cancelEntry != 0.0f) {
					current.row_muladd(cancelRow, pivotRow, -cancelEntry);
					inverse.row_muladd(cancelRow, pivotRow, -cancelEntry);
				}
			}

			pivotRow++;
		}

		return inverse;
	}

	constexpr Matrix operator+() const { return *this; }

	Matrix operator-() const {
		Matrix result;
		for (int i = 0; i < Rows * Cols; i++) {
			result.data[i] = -data[i];
		}
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
		Matrix result;
		if (0.0f == scalar)
			return result;

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
		if (0.0f == scalar) {
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

using Matrix3 = Matrix<3, 3>;
using Vector3 = Vector<3>;

//
// meat n potatoes
//

/* We want a matrix A that we can multiply square UVs by to get our mutated quad UVs.
 * We can do this by
 */

template <int Rows, int Cols>
struct SvdResult {
	Matrix<Rows, Rows> left;     // U
	Matrix<Rows, Cols> diagonal; // Sigma
	Matrix<Cols, Cols> right;    // V^T

	static SvdResult compute(Matrix<Rows, Cols> input) {
		SvdResult result;



		return result;
	}
};

// should be passed in order of TL BL BR TR
// In the fragment shader, we'll supply the matrix outputted by this function
// then the fragment shader we multiply the *regular* UVs by this matrix
Matrix3 compute_homography(const Vector3 mutated[4], const Vector3 square[4]) {
	// we can model this as Y=AX, hence if we take the right pseudo inverse of X to be Xp such that XXp=I
	// then we can multiply it on both sides to get YXp=A
	Matrix<3, 4> Y, X;
	for(int i = 0; i < 4; i++) {
		Y.set_column(mutated[i], i);
		X.set_column(square[i], i);
	}

	Matrix<4, 3> Xt = X.transposed();

	// so now compute pseudo inverse of X
	Matrix<4, 3> Xp = Xt * (X * Xt).inversed();

	return (Y * Xp); // since A = YXp
}