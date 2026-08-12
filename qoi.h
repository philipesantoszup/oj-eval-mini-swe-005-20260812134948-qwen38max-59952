#ifndef QOI_FORMAT_CODEC_QOI_H_
#define QOI_FORMAT_CODEC_QOI_H_

#include <string>

#include "utils.h"

constexpr uint8_t QOI_OP_INDEX_TAG = 0x00;
constexpr uint8_t QOI_OP_DIFF_TAG  = 0x40;
constexpr uint8_t QOI_OP_LUMA_TAG  = 0x80;
constexpr uint8_t QOI_OP_RUN_TAG   = 0xc0; 
constexpr uint8_t QOI_OP_RGB_TAG   = 0xfe;
constexpr uint8_t QOI_OP_RGBA_TAG  = 0xff;
constexpr uint8_t QOI_PADDING[8] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u};
constexpr uint8_t QOI_MASK_2 = 0xc0;

// upper bound of pixels this codec is willing to process in memory
constexpr uint64_t QOI_MAX_PIXELS = 1000000000ull;

/**
 * @brief compute (x - y) wrapped into the signed byte range [-128, 127],
 *        mimicking the two's complement byte arithmetic of the QOI spec.
 */
inline int WrapDiff(int x, int y) {
    int d = static_cast<uint8_t>(x - y);
    if (d > 127) d -= 256;
    return d;
}

/**
 * @brief encode the raw pixel data of an image to qoi format.
 *
 * @param[in] width image width in pixels
 * @param[in] height image height in pixels
 * @param[in] channels number of color channels, 3 = RGB, 4 = RGBA
 * @param[in] colorspace image color space, 0 = sRGB with linear alpha, 1 = all channels linear
 *
 * @return bool true if it is a valid qoi format image, false otherwise
 */
bool QoiEncode(uint32_t width, uint32_t height, uint8_t channels, uint8_t colorspace = 0);

/**
 * @brief decode the qoi format of an image to raw pixel data
 *
 * @param[out] width image width in pixels
 * @param[out] height image height in pixels
 * @param[out] channels number of color channels, 3 = RGB, 4 = RGBA
 * @param[out] colorspace image color space, 0 = sRGB with linear alpha, 1 = all channels linear
 *
 * @return bool true if it is a valid qoi format image, false otherwise
 */
bool QoiDecode(uint32_t &width, uint32_t &height, uint8_t &channels, uint8_t &colorspace);


bool QoiEncode(uint32_t width, uint32_t height, uint8_t channels, uint8_t colorspace) {

    // validate parameters before writing anything
    if ((channels != 3u && channels != 4u) || colorspace > 1u) return false;
    if (width == 0u || height == 0u) return false;
    const uint64_t px_num = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);
    if (px_num > QOI_MAX_PIXELS) return false;
    const uint64_t raw_size = px_num * static_cast<uint64_t>(channels);

    // read all raw pixel data from the input stream in one bulk operation
    std::string pixels(raw_size, '\0');
    std::cin.read(&pixels[0], static_cast<std::streamsize>(raw_size));
    if (std::cin.fail()) return false;

    // qoi-header part

    // write magic bytes "qoif"
    QoiWriteChar('q');
    QoiWriteChar('o');
    QoiWriteChar('i');
    QoiWriteChar('f');
    // write image width (big endian)
    QoiWriteU32(width);
    // write image height (big endian)
    QoiWriteU32(height);
    // write channel number
    QoiWriteU8(channels);
    // write color space specifier
    QoiWriteU8(colorspace);

    /* qoi-data part */
    // accumulate the encoded bytes and flush them in one bulk write at the
    // end; this keeps the runtime dominated by the actual encoding work
    // instead of per-byte stream operations
    std::string out;
    out.reserve(8u + static_cast<size_t>(px_num));
    auto emit = [&out](uint8_t byte) {
        out.push_back(static_cast<char>(byte));
    };

    int run = 0;

    // history table of recently used pixels for QOI_OP_INDEX
    uint8_t history[64][4];
    memset(history, 0, sizeof(history));

    uint8_t pre_r = 0u, pre_g = 0u, pre_b = 0u, pre_a = 255u;

    size_t pos = 0;
    for (uint64_t i = 0; i < px_num; ++i) {
        const uint8_t r = pixels[pos++];
        const uint8_t g = pixels[pos++];
        const uint8_t b = pixels[pos++];
        const uint8_t a = (channels == 4u) ? pixels[pos++] : 255u;

        // QOI_OP_RUN: the pixel is the same as the previous one
        if (r == pre_r && g == pre_g && b == pre_b && a == pre_a) {
            ++run;
            // a run can encode at most 62 pixels
            if (run == 62) {
                emit(QOI_OP_RUN_TAG | static_cast<uint8_t>(run - 1));
                run = 0;
            }
            continue;
        }

        // flush the pending run before encoding a different pixel
        if (run > 0) {
            emit(QOI_OP_RUN_TAG | static_cast<uint8_t>(run - 1));
            run = 0;
        }

        const int index = QoiColorHash(r, g, b, a);

        // QOI_OP_INDEX: the pixel is already in the history table
        if (history[index][0] == r && history[index][1] == g &&
            history[index][2] == b && history[index][3] == a) {
            emit(QOI_OP_INDEX_TAG | static_cast<uint8_t>(index));
        } else {
            // store the pixel in the history table
            history[index][0] = r;
            history[index][1] = g;
            history[index][2] = b;
            history[index][3] = a;

            // channel differences wrap around like signed bytes (mod 256)
            const int dr = WrapDiff(r, pre_r);
            const int dg = WrapDiff(g, pre_g);
            const int db = WrapDiff(b, pre_b);

            if (a == pre_a &&
                dr >= -2 && dr <= 1 && dg >= -2 && dg <= 1 && db >= -2 && db <= 1) {
                // QOI_OP_DIFF: small per-channel differences, alpha unchanged
                emit(QOI_OP_DIFF_TAG |
                     static_cast<uint8_t>(((dr + 2) << 4) | ((dg + 2) << 2) | (db + 2)));
            } else if (a == pre_a && dg >= -32 && dg <= 31 &&
                       WrapDiff(dr, dg) >= -8 && WrapDiff(dr, dg) <= 7 &&
                       WrapDiff(db, dg) >= -8 && WrapDiff(db, dg) <= 7) {
                // QOI_OP_LUMA: green diff in [-32, 31], red/blue diffs relative
                // to the green diff in [-8, 7], alpha unchanged
                emit(QOI_OP_LUMA_TAG | static_cast<uint8_t>(dg + 32));
                emit(static_cast<uint8_t>(((WrapDiff(dr, dg) + 8) << 4) |
                                          (WrapDiff(db, dg) + 8)));
            } else if (a == pre_a) {
                // QOI_OP_RGB: only the color channels changed
                emit(QOI_OP_RGB_TAG);
                emit(r);
                emit(g);
                emit(b);
            } else {
                // QOI_OP_RGBA: the alpha channel changed
                emit(QOI_OP_RGBA_TAG);
                emit(r);
                emit(g);
                emit(b);
                emit(a);
            }
        }

        pre_r = r;
        pre_g = g;
        pre_b = b;
        pre_a = a;
    }

    // flush the remaining run
    if (run > 0) {
        emit(QOI_OP_RUN_TAG | static_cast<uint8_t>(run - 1));
    }

    // flush the encoded data in one bulk write
    std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));

    // qoi-padding part
    for (size_t i = 0; i < sizeof(QOI_PADDING); ++i) {
        QoiWriteU8(QOI_PADDING[i]);
    }

    return true;
}

bool QoiDecode(uint32_t &width, uint32_t &height, uint8_t &channels, uint8_t &colorspace) {

    char c1 = QoiReadChar();
    char c2 = QoiReadChar();
    char c3 = QoiReadChar();
    char c4 = QoiReadChar();
    if (std::cin.fail() || c1 != 'q' || c2 != 'o' || c3 != 'i' || c4 != 'f') {
        return false;
    }

    // read image width
    width = QoiReadU32();
    // read image height
    height = QoiReadU32();
    // read channel number
    channels = QoiReadU8();
    // read color space specifier
    colorspace = QoiReadU8();
    if (std::cin.fail()) return false;

    // validate header fields
    if ((channels != 3u && channels != 4u) || colorspace > 1u) return false;
    if (width == 0u || height == 0u) return false;
    const uint64_t px_num = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);
    if (px_num > QOI_MAX_PIXELS) return false;

    // bulk-read the remainder of the stream (qoi-data followed by qoi-padding)
    std::string data;
    char bulk[1 << 20];
    while (std::cin.read(bulk, sizeof(bulk)) || std::cin.gcount() > 0) {
        data.append(bulk, static_cast<size_t>(std::cin.gcount()));
    }

    // the stream must end with the eight qoi-padding bytes
    if (data.size() < sizeof(QOI_PADDING)) return false;
    const size_t data_end = data.size() - sizeof(QOI_PADDING);
    for (size_t i = 0; i < sizeof(QOI_PADDING); ++i) {
        if (static_cast<uint8_t>(data[data_end + i]) != QOI_PADDING[i]) return false;
    }

    /* qoi-data part */
    // accumulate the decoded pixels and flush them in one bulk write at the end
    std::string out;
    out.resize(static_cast<size_t>(px_num) * channels);

    // history table mirroring the encoder's one
    uint8_t history[64][4];
    memset(history, 0, sizeof(history));

    uint8_t r = 0u, g = 0u, b = 0u, a = 255u;
    int run = 0;

    size_t pos = 0;   // read cursor into data
    size_t opos = 0;  // write cursor into out
    for (uint64_t i = 0; i < px_num; ++i) {

        if (run > 0) {
            // the pixel is the same as the previous one, keep (r,g,b,a)
            --run;
        } else {
            if (pos >= data_end) return false;  // not enough data chunks
            const uint8_t b1 = static_cast<uint8_t>(data[pos++]);

            if (b1 == QOI_OP_RGB_TAG) {
                // QOI_OP_RGB: new color channels, alpha unchanged
                if (pos + 3 > data_end) return false;
                r = static_cast<uint8_t>(data[pos++]);
                g = static_cast<uint8_t>(data[pos++]);
                b = static_cast<uint8_t>(data[pos++]);
            } else if (b1 == QOI_OP_RGBA_TAG) {
                // QOI_OP_RGBA: new color channels and new alpha
                if (pos + 4 > data_end) return false;
                r = static_cast<uint8_t>(data[pos++]);
                g = static_cast<uint8_t>(data[pos++]);
                b = static_cast<uint8_t>(data[pos++]);
                a = static_cast<uint8_t>(data[pos++]);
            } else {
                const uint8_t tag = b1 & QOI_MASK_2;
                if (tag == QOI_OP_INDEX_TAG) {
                    // QOI_OP_INDEX: fetch the pixel from the history table
                    const uint8_t index = b1 & 0x3fu;
                    r = history[index][0];
                    g = history[index][1];
                    b = history[index][2];
                    a = history[index][3];
                } else if (tag == QOI_OP_DIFF_TAG) {
                    // QOI_OP_DIFF: small per-channel differences, alpha unchanged
                    r = static_cast<uint8_t>(r + ((b1 >> 4) & 0x03u) - 2);
                    g = static_cast<uint8_t>(g + ((b1 >> 2) & 0x03u) - 2);
                    b = static_cast<uint8_t>(b + (b1 & 0x03u) - 2);
                } else if (tag == QOI_OP_LUMA_TAG) {
                    // QOI_OP_LUMA: green diff plus two relative diffs, alpha unchanged
                    if (pos >= data_end) return false;
                    const uint8_t b2 = static_cast<uint8_t>(data[pos++]);
                    const int dg = static_cast<int>(b1 & 0x3fu) - 32;
                    r = static_cast<uint8_t>(r + dg + static_cast<int>((b2 >> 4) & 0x0fu) - 8);
                    g = static_cast<uint8_t>(g + dg);
                    b = static_cast<uint8_t>(b + dg + static_cast<int>(b2 & 0x0fu) - 8);
                } else {
                    // QOI_OP_RUN: repeat the previous pixel
                    run = b1 & 0x3fu;
                }
            }

            // update the history table (for QOI_OP_INDEX this just rewrites the
            // pixel at the same hashed position it was fetched from)
            const int index = QoiColorHash(r, g, b, a);
            history[index][0] = r;
            history[index][1] = g;
            history[index][2] = b;
            history[index][3] = a;
        }

        out[opos++] = static_cast<char>(r);
        out[opos++] = static_cast<char>(g);
        out[opos++] = static_cast<char>(b);
        if (channels == 4) out[opos++] = static_cast<char>(a);
    }

    // the data chunks must have been consumed exactly up to the padding
    if (pos != data_end) return false;

    std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
    return true;
}

#endif // QOI_FORMAT_CODEC_QOI_H_
