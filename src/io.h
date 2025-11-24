#pragma once

#include <cstddef>
#include <string>
#include <tuple>
#include <vector>

namespace otbv {
/**
 * @brief Formats \p data, \p resolution, and \p padded as a proper OTBV file,
 * and streams the resulting bytes to \p stream
 */
void stream_data_as_file_bytes(
    std::ostream &stream, const std::vector<bool> &data,
    const std::tuple<size_t, size_t, size_t> resolution, const bool padded);

/**
 * @brief Encodes \p data and writes it to \p filename
 */
void save(const std::string &filename,
          const std::vector<std::vector<std::vector<bool>>> &data);

/**
 * @brief Overload of \p save for flattened data.
 *
 * @param resolution Tuple describing the shape of the volume. \p data will be
 * automatically reshaped, if possible
 */
void save(const std::string &filename, const std::vector<bool> &data,
          const std::tuple<size_t, size_t, size_t> &resolution);

/**
 * @brief Reads and decodes the volume from \p filename
 */
std::vector<std::vector<std::vector<bool>>> load(const std::string &filename);

} // namespace otbv
