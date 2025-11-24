#include "io.h"
#include "conversion.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace otbv {

static constexpr char SIGNATURE[] = "OTBV\x96";
static constexpr size_t MAX_RESOLUTION = 100'000;
static constexpr size_t MAX_VOLUME =
    MAX_RESOLUTION * MAX_RESOLUTION * MAX_RESOLUTION;

void stream_data_as_file_bytes(
    std::ostream &stream, const std::vector<bool> &data,
    const std::tuple<size_t, size_t, size_t> resolution, const bool padded) {
  /*** metadata ***/
  char rem = data.size() % 8;
  char pad_len = rem == 0 ? 0 : 8 - rem;
  uint8_t meta_first = 0;
  meta_first |= (pad_len << 5);
  // flag for whether the volume was padded to cubic
  meta_first |= (padded << 4);

  uint32_t meta_res_x = std::get<0>(resolution), meta_res_y = 0, meta_res_z = 0;
  if (padded) {
    meta_res_y = std::get<1>(resolution);
    meta_res_z = std::get<2>(resolution);
  }

  uint32_t meta_data_len = (data.size() + pad_len) / 8;

  /*** data ***/
  std::vector<bool> data_out;
  data_out.reserve(pad_len + data.size());
  for (int i = 0; i < pad_len; ++i) {
    data_out.push_back(0);
  }
  data_out.insert(data_out.end(), data.begin(), data.end());

  // signature
  stream.write(SIGNATURE, 5);
  // metadata
  stream.write(reinterpret_cast<const char *>(&meta_first), sizeof(meta_first));
  stream.write(reinterpret_cast<const char *>(&meta_res_x), sizeof(meta_res_x));
  stream.write(reinterpret_cast<const char *>(&meta_res_y), sizeof(meta_res_y));
  stream.write(reinterpret_cast<const char *>(&meta_res_z), sizeof(meta_res_z));
  stream.write(reinterpret_cast<const char *>(&meta_data_len),
               sizeof(meta_data_len));
  // data
  for (std::size_t i = 0; i < data_out.size(); i += 8) {
    char c = 0;
    for (int j = 0; j < 8; j++) {
      c |= static_cast<char>(data_out[i + j]) << (7 - j);
    }
    stream.write(&c, 1);
  }
}

void save(const std::string &filename, const std::vector<bool> &data,
          const std::tuple<size_t, size_t, size_t> &resolution) {
  const vector3<bool> data_reshaped = reshape(data, resolution);
  save(filename, data_reshaped);
}

void save(const std::string &filename,
          const std::vector<std::vector<std::vector<bool>>> &data) {
  if (0 == size(data)) {
    printf("The provided volume size is 0. Nothing will be written");
    return;
  }
  const vector3<bool> padded_data = pad_to_cube(data);
  const std::vector<bool> encoded_data = encode(padded_data);
  const auto resolution =
      std::make_tuple(data.size(), data[0].size(), data[0][0].size());
  std::ofstream file_out(filename, std::ofstream::binary);
  stream_data_as_file_bytes(file_out, encoded_data, resolution,
                            size(padded_data) > size(data));
  int bytes_written = file_out.tellp();
  if (bytes_written > 0) {
    printf("Written %d bytes\n", bytes_written);
  }
  file_out.close();
}

uint32_t pack_chars(char *c) {
  uint32_t val = 0;
  val |= c[0];
  val |= c[1] << 8;
  val |= c[2] << 16;
  val |= c[3] << 24;
  return val;
}

std::vector<std::vector<std::vector<bool>>> load(const std::string &filename) {
  std::ifstream file_in(filename, std::ios::binary);
  if (!file_in) {
    throw std::runtime_error("Could not open file for reading.");
  }

  char sign_buffer[5], meta_buffer[17], padding_length;
  uint32_t x_res, y_res, z_res;

  // signature
  static_cast<void>(file_in.read(sign_buffer, 5));
  if (std::memcmp(SIGNATURE, sign_buffer, 5)) {
    throw std::runtime_error(
        "Signature validation failed. Could not confirm that the provided "
        "filename refers to a valid OTBV file.");
  }

  // metadata
  static_cast<void>(file_in.read(meta_buffer, 17));
  padding_length = meta_buffer[0] >> 5;
  bool is_padded = (meta_buffer[0] >> 4) & 1;
  x_res = pack_chars(meta_buffer + 1);
  if (!is_padded) {
    y_res = z_res = x_res;
  } else {
    y_res = pack_chars(meta_buffer + 5);
    z_res = pack_chars(meta_buffer + 9);
  }

  if (x_res > MAX_RESOLUTION || y_res > MAX_RESOLUTION ||
      z_res > MAX_RESOLUTION) {
    throw std::runtime_error("Provided volume lists resolution above allowed "
                             "maximum 10e5 per dimension.");
  }

  // data
  uint32_t data_length = pack_chars(meta_buffer + 13);
  char data_buffer[data_length];
  static_cast<void>(file_in.read(data_buffer, data_length));
  std::vector<bool> encoding;
  encoding.reserve(data_length * 8);
  for (size_t i = 0; i < data_length; i++) {
    for (char j = 7; j >= 0; j--) {
      encoding.push_back((data_buffer[i] >> j) & 1);
    }
  }
  encoding.erase(encoding.begin(), encoding.begin() + padding_length);
  auto out = decode(encoding, {x_res, y_res, z_res});
  return out;
}

} // namespace otbv
