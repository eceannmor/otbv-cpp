#pragma once

#include <cstddef>
#include <tuple>
#include <vector>

template <typename T> using vector3 = std::vector<std::vector<std::vector<T>>>;

namespace otbv {

/**
 * @brief Returns the smallest power of 2 greater than or equal to \p number.
 */
size_t pow2_roof(size_t number);

/**
 * @brief Returns the smallest power of 2 that is greater or equal to the
 * maximum of dimensions in \p resolution.
 */
size_t max_res_pow2_roof(const std::tuple<size_t, size_t, size_t> &resolution);

/**
 * @brief Returns the smallest power of 2 that is greater or equal to the
 * maximum of \p x_res, \p y_res, and \p z_res.
 */
size_t max_res_pow2_roof(const size_t &x_res, const size_t &y_res,
                         const size_t &z_res);

/**
 * @brief Returns a copy of \p data reshaped into a cubic tensor, if possible
 *
 * @tparam T Any type
 * @param data A 1d vector, where <code>Ea s.th. a^3 == data.size()</code>
 * @return std::vector<std::vector<std::vector<T>>> Cubic tensor with side
 * length \p a (see \p data)
 */
template <typename T> vector3<T> reshape_to_cubic(const std::vector<T> &data);

/**
 * @brief Reshapes the vector
 *
 * @param data Flat vector
 * @param resolution New shape
 * @return vector3<bool> \p data, reshaped to \p resolution
 * @throws std::invalid_argument If \p data cannot be reshaped
 */
vector3<bool> reshape(const std::vector<bool> &data,
                      const std::tuple<size_t, size_t, size_t> &resolution);

/**
 * @brief Checks if a subvolume of \p data contains homogeneous values.
 * Parameters \p xs \p xe, and so on, define the subvolume start and end
 * coordinates. The range is [s, e)
 *
 * @param data the volume
 * @return true if the subvolume contains homogeneous values
 * @return false otherwise
 */
template <typename T>
bool is_subvolume_homogeneous(const vector3<T> &data, const size_t xs,
                              const size_t xe, const size_t ys, const size_t ye,
                              const size_t zs, const size_t ze);

/**
 * @brief Returns the total size of \p data
 * Assumes that \p data is a rectangular cuboid
 */
template <typename T> size_t size(const vector3<T> &data);

/**
 * @brief Helper function. Takes a volume slice and a reference to the encoding
 * vector, and encodes the slice
 */
void encode_recursive(const vector3<bool> &data, std::vector<bool> &encoding,
                      const size_t xs, const size_t xe, const size_t ys,
                      const size_t ye, const size_t zs, const size_t ze,
                      size_t depth);

/**
 * @brief Encodes the binary volume
 *
 * @return std::vector<bool>
 */
std::vector<bool> encode(const vector3<bool> &data);

/**
 * @brief Sets all data in \p data in range [xs ys zs, xe ye ze) to \p value
 */
inline void set_range(vector3<bool> &data, bool value, const size_t xs,
                      const size_t xe, const size_t ys, const size_t ye,
                      const size_t zs, const size_t ze);

/**
 * @brief Converts \p data to \p bool, if possible
 */
template <typename T> vector3<bool> convert_to_bool(const vector3<T> &data);

/**
 * @brief Modifies \p data in-place to pad it to a cube. The resulting cube is
 * the smallest cube with an edge length of a power of 2, that can fit \par
 * data.
 */
void pad_to_cube(vector3<bool> &data);

/**
 * @brief Overload of \ref pad_to_cube when the \p data is const. Returns a
 * copy of data padded to a cube.
 */
vector3<bool> pad_to_cube(const vector3<bool> &data);

/**
 * @brief Returns a deep copy of \p vector
 */
template <typename T> vector3<T> deep_copy(const vector3<T> &vector);

/**
 * @brief Helper function. Takes a volume slice and a reference to the encoding
 * vector, and decodes the value for the slice
 */
size_t decode_recursive(const std::vector<bool> &encoding, vector3<bool> &out,
                        size_t next_idx, const size_t xs, const size_t xe,
                        const size_t ys, const size_t ye, const size_t zs,
                        const size_t ze, size_t depth);

/**
 * @brief Decodes the encoded volume
 */
vector3<bool> decode(const std::vector<bool> &encoding,
                     const std::tuple<size_t, size_t, size_t> &resolution);

/**
 * @brief Cuts down the volume to the dimensions specified in \p resolution.
 * Makes the assumption that the dimensions are less or equal to the current
 * size of the volume. No checks are performed.
 */
void cut_volume(vector3<bool> &volume,
                const std::tuple<size_t, size_t, size_t> &resolution);
/**
 * @brief Cuts down the volume to the dimensions specified by \p x_res, \par
 * y_res, and \p z_res. Makes the assumption that the dimensions are less or
 * equal to the current size of the volume. No checks are performed.
 */
void cut_volume(vector3<bool> &volume, const size_t &x_res, const size_t &y_res,
                const size_t &z_res);
} // namespace otbv
