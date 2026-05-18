#pragma once

#include <algorithm>
#include <math.h>

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

	constexpr Vector() {
		for (int i = 0; i < N; i++)
			data[i] = 0.0f;
	}

	template <typename... Args>
	constexpr Vector(Args... args) requires (sizeof...(args) == N)
		: data{ static_cast<float>(args)... } {
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

	constexpr Matrix() {
		set_identity();
	}

	void set_zero() {
		for (int i = 0; i < Rows * Cols; i++) {
			data[i] = 0.0f;
		}
	}

	void set_identity() {
		for (int i = 0; i < Rows; i++) {
			for (int j = 0; j < Cols; j++) {
				get(i, j) = (i == j) ? 1.0f : 0.0f;
			}
		}
	}

	float& get(int row, int col) { return data[col * Rows + row]; }
	const float& get(int row, int col) const { return data[col * Rows + row]; }
	float& operator()(int row, int col) { return get(row, col); }
	const float& operator()(int row, int col) const { return get(row, col); }

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

	Matrix<Cols, Rows> transposed() const {
		Matrix<Cols, Rows> result;
		for (int i = 0; i < Rows; i++) {
			for (int j = 0; j < Cols; j++) {
				result(j, i) = get(i, j);
			}
		}
		return result;
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
				result(i, j) = sum;
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

// should be passed in order of TL BL BR TR
inline Matrix3 compute_homography(const Vector3 corners[4]) {
	Matrix3 result;



	return result;
}