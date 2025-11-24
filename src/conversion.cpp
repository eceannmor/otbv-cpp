#include "conversion.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <tuple>
#include <vector>

// enough to accommodate volumes @ 1M per dimension
static constexpr size_t RECURSION_MAX_DEPTH = 20;

namespace otbv {

// https://www.reddit.com/r/cpp_questions/comments/1h3sva9/
// not handling edge cases
size_t pow2_roof(size_t number) {
  if (!(number & (number - 1)))
    return number;
  size_t shift = 1;
  size_t val = 0;
  do {
    val = number;
    number |= (number >> shift);
    shift *= 2;
  } while (number > val);
  return val + 1;
}

size_t max_res_pow2_roof(const std::tuple<size_t, size_t, size_t> &resolution) {
  return max_res_pow2_roof(std::get<0>(resolution), std::get<1>(resolution),
                           std::get<2>(resolution));
}

size_t max_res_pow2_roof(const size_t &x_res, const size_t &y_res,
                         const size_t &z_res) {
  size_t max_res = std::max(x_res, y_res);
  max_res = std::max(max_res, z_res);
  return pow2_roof(max_res);
}

template <typename T> vector3<T> reshape_to_cubic(const std::vector<T> &data) {
  const size_t data_size = data.size();
  double edge_len = std::cbrt(data_size);
  if (std::floor(edge_len) != edge_len) {
    throw std::invalid_argument("Could not reshape data into a cubic tensor");
  } else {
    size_t new_edge_len = static_cast<size_t>(edge_len);
    vector3<T> out;
    out.resize(new_edge_len);
    for (auto &row : out) {
      row.resize(new_edge_len);
      for (auto &col : row) {
        col.resize(new_edge_len);
      }
    }
    size_t id = 0;
    for (size_t x = 0; x < edge_len; ++x) {
      for (size_t y = 0; y < edge_len; ++y) {
        for (size_t z = 0; z < edge_len; ++z) {
          out[x][y][z] = data[id++];
        }
      }
    }
    return out;
  }
}

vector3<bool> reshape(const std::vector<bool> &data,
                      const std::tuple<size_t, size_t, size_t> &resolution) {
  const size_t data_size = data.size(), //
      x_res = std::get<0>(resolution),  //
      y_res = std::get<1>(resolution),  //
      z_res = std::get<2>(resolution),  //
      resolution_size = x_res * y_res * z_res;

  if (data_size != resolution_size) {
    printf("%zu, %zu \n", data_size, resolution_size);
    throw std::invalid_argument(
        "Provided vector cannot be reshaped to a provided resolution");
  }

  vector3<bool> out;
  out.resize(x_res);
  for (auto &row : out) {
    row.resize(y_res);
    for (auto &col : row) {
      col.resize(z_res);
    }
  }
  size_t id = 0;
  for (size_t x = 0; x < x_res; ++x) {
    for (size_t y = 0; y < y_res; ++y) {
      for (size_t z = 0; z < z_res; ++z) {
        out[x][y][z] = data[id++];
      }
    }
  }
  return out;
}

template <typename T>
bool is_subvolume_homogeneous(const vector3<T> &data, const size_t xs,
                              const size_t xe, const size_t ys, const size_t ye,
                              const size_t zs, const size_t ze) {
  size_t subvolume_size = (xe - xs) * (ye - ys) * (ze - zs);
  if (subvolume_size < 2) {
    return true;
  }
  const T &first = data[xs][ys][zs];
  for (size_t x = xs; x < xe; ++x) {
    for (size_t y = ys; y < ye; ++y) {
      for (size_t z = zs; z < ze; ++z) {
        if (data[x][y][z] != first) {
          return false;
        }
      }
    }
  }
  return true;
}

template <typename T> size_t size(const vector3<T> &data) {
  size_t size = data.size();
  if (0 != size)
    size *= data[0].size();
  if (0 != size)
    size *= data[0][0].size();
  return size;
}

void encode_recursive(const vector3<bool> &data, std::vector<bool> &encoding,
                      const size_t xs, const size_t xe, const size_t ys,
                      const size_t ye, const size_t zs, const size_t ze,
                      size_t depth) {
  if (depth > RECURSION_MAX_DEPTH) {
    throw std::runtime_error("Reached maximum recursion depth while encoding. "
                             "The data is likely too large or malformed.");
  }
  // start index inclusive, end index exclusive
  size_t subvolume_size = (xe - xs) * (ye - ys) * (ze - zs);
  if (0 == subvolume_size) {
    printf("%zu, %zu, %zu, %zu, %zu, %zu\n", xs, xe, ys, ye, zs, ze);
    throw std::invalid_argument(
        "Encountered a subvolume with a size of 0 when encoding the volume. "
        "This should never happen. Please open a new issue and attach the data "
        "you are trying to encode. "
        "https://github.com/eceannmor/otbv-cpp/issues");
  }
  if (is_subvolume_homogeneous(data, xs, xe, ys, ye, zs, ze)) {
    // leaf
    encoding.push_back(0);
    encoding.push_back(data[xs][ys][zs]);
    return;
  }

  size_t x_split = (xe + xs) >> 1;
  size_t y_split = (ye + ys) >> 1;
  size_t z_split = (ze + zs) >> 1;

  encoding.push_back(1);

  for (char i = 0; i < 2; i++) {
    size_t xfirst = i ? x_split : xs;
    size_t xsecond = i ? xe : x_split;
    for (char j = 0; j < 2; j++) {
      size_t yfirst = j ? y_split : ys;
      size_t ysecond = j ? ye : y_split;
      for (char k = 0; k < 2; k++) {
        size_t zfirst = k ? z_split : zs;
        size_t zsecond = k ? ze : z_split;
        encode_recursive(data, encoding, xfirst, xsecond, yfirst, ysecond,
                         zfirst, zsecond, depth + 1);
      }
    }
  }
}

std::vector<bool> encode(const vector3<bool> &data) {
  std::vector<bool> out;
  size_t resolution = data.size();
  encode_recursive(data, out, 0, resolution, 0, resolution, 0, resolution, 0);
  return out;
}

void pad_to_cube(vector3<bool> &data) {
  if (0 == size(data)) {
    throw std::invalid_argument("Cannot pad data of size 0 to cube");
  }
  size_t max_res =
      max_res_pow2_roof(data.size(), data[0].size(), data[0][0].size());
  data.resize(max_res);
  for (auto &plane : data) {
    plane.resize(max_res);
    for (auto &col : plane) {
      col.resize(max_res);
    }
  }
}

vector3<bool> pad_to_cube(const vector3<bool> &data) {
  auto copy = deep_copy(data);
  pad_to_cube(copy);
  return copy;
}

template <typename T> vector3<T> deep_copy(const vector3<T> &vector) {
  vector3<T> copy;
  if (0 == size(vector)) {
    return copy;
  }
  size_t x_res = vector.size(), y_res = vector[0].size(),
         z_res = vector[0][0].size();
  copy.resize(x_res);
  for (auto &row : copy) {
    row.resize(y_res);
    for (auto &col : row) {
      col.resize(z_res);
    }
  }
  size_t id = 0;
  for (size_t x = 0; x < x_res; ++x) {
    for (size_t y = 0; y < y_res; ++y) {
      for (size_t z = 0; z < z_res; ++z) {
        copy[x][y][z] = vector[x][y][z];
      }
    }
  }
  return copy;
}

inline void set_range(vector3<bool> &data, bool value, const size_t xs,
                      const size_t xe, const size_t ys, const size_t ye,
                      const size_t zs, const size_t ze) {
  // start index inclusive, end index exclusive
  for (size_t x = xs; x < xe; x++) {
    for (size_t y = ys; y < ye; y++) {
      for (size_t z = zs; z < ze; z++) {
        data[x][y][z] = value;
      }
    }
  }
}

size_t decode_recursive(const std::vector<bool> &encoding, vector3<bool> &out,
                        size_t next_idx, const size_t xs, const size_t xe,
                        const size_t ys, const size_t ye, const size_t zs,
                        const size_t ze, size_t depth) {
  if (depth > RECURSION_MAX_DEPTH) {
    throw std::runtime_error("Reached maximum recursion depth while decoding. "
                             "The data is likely too large or malformed.");
  }
  if (next_idx >= encoding.size()) {
    throw std::out_of_range("Unexpected end of the encoding");
  }
  bool token = encoding[next_idx];
  next_idx++;
  if (!token) {
    // leaf
    if (next_idx >= encoding.size()) {
      throw std::out_of_range("Unexpected end of the encoding");
    }
    bool value = encoding[next_idx];
    set_range(out, value, xs, xe, ys, ye, zs, ze);
    return next_idx + 1;
  } else {
    std::array<size_t, 3> x_split = {xs, (xs + xe) / 2, xe};
    std::array<size_t, 3> y_split = {ys, (ys + ye) / 2, ye};
    std::array<size_t, 3> z_split = {zs, (zs + ze) / 2, ze};
    for (int x : {0, 1}) {
      for (int y : {0, 1}) {
        for (int z : {0, 1}) {
          next_idx = decode_recursive(
              encoding, out, next_idx, x_split[x], x_split[x + 1], y_split[y],
              y_split[y + 1], z_split[z], z_split[z + 1], depth + 1);
        }
      }
    }
    return next_idx;
  }
}

vector3<bool> decode(const std::vector<bool> &encoding,
                     const std::tuple<size_t, size_t, size_t> &resolution) {
  vector3<bool> out;
  size_t decoding_res = max_res_pow2_roof(resolution);
  cut_volume(out, decoding_res, decoding_res, decoding_res);
  size_t end_idx = decode_recursive(encoding, out, 0, 0, decoding_res, 0,
                                    decoding_res, 0, decoding_res, 0);
  assert(end_idx == encoding.size());
  cut_volume(out, resolution);
  return out;
}

void cut_volume(vector3<bool> &volume,
                const std::tuple<size_t, size_t, size_t> &resolution) {
  cut_volume(volume, std::get<0>(resolution), std::get<1>(resolution),
             std::get<2>(resolution));
}

void cut_volume(vector3<bool> &volume, const size_t &x_res, const size_t &y_res,
                const size_t &z_res) {
  volume.resize(x_res);
  for (auto &row : volume) {
    row.resize(y_res);
    for (auto &col : row) {
      col.resize(z_res);
    }
  }
}

} // namespace otbv
