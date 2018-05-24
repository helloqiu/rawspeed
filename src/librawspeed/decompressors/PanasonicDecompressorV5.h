/*
    RawSpeed - RAW file decoder.

    Copyright (C) 2018 Roman Lebedev
    Copyright (C) 2018 Stefan Hoffmeister

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#pragma once

#include "common/Common.h"                                  // for uint32
#include "common/Point.h"                                   // for i
#include "common/RawImage.h"                                // for RawImage
#include "decompressors/AbstractParallelizedDecompressor.h" // for Abstract...
#include "io/ByteStream.h"                                  // for ByteStream

namespace rawspeed {

class RawImage;

class PanasonicDecompressorV5 final : public AbstractDecompressor {
public:
  struct CompressionDsc;

private:
  RawImage mRaw;

  // The RW2 raw image buffer consists of individual blocks,
  // each one BlockSize bytes in size.
  static constexpr uint32 BlockSize = 0x4000;

  // These blocks themselves comprise of two sections,
  // split and swapped at section_split_offset:
  //   bytes:  [0..sectionSplitOffset-1][sectionSplitOffset..BlockSize-1]
  //   pixels: [a..b][0..a-1]
  // When reading, these two sections need to be swapped to enable linear
  // processing..
  static constexpr uint32 sectionSplitOffset = 0x1FF8;

  // The blocks themselves consist of packets with fixed size of bytesPerPacket,
  // and each packet decodes to pixelsPerPacket pixels, which depends on bps.
  static constexpr uint32 bytesPerPacket = 16;
  static_assert(BlockSize % bytesPerPacket == 0, "");
  static constexpr uint32 PacketsPerBlock = BlockSize / bytesPerPacket;

  class ProxyStream;

  ByteStream input;

  const uint32 bps;

  struct Block {
    ByteStream bs;
    iPoint2D beginCoord;
    // The rectangle is an incorrect representation. All the rows
    // between the first and last one span the entire width of the image.
    iPoint2D endCoord;

    Block() = default;
    Block(ByteStream&& bs_, iPoint2D beginCoord_, iPoint2D endCoord_)
        : bs(std::move(bs_)), beginCoord(beginCoord_), endCoord(endCoord_) {}
  };

  // If really wanted, this vector could be avoided,
  // and each Block computed on-the-fly
  std::vector<Block> blocks;

  void chopInputIntoBlocks(const CompressionDsc& dsc);

  template <const CompressionDsc& dsc>
  void processPixelPacket(ushort16* dest, const uchar8* bytes) const;

  template <const CompressionDsc& dsc>
  void processBlock(const Block& block) const;

  template <const CompressionDsc& dsc> void decompressInternal() const;

public:
  PanasonicDecompressorV5(const RawImage& img, const ByteStream& input_,
                          uint32 bps_);

  void decompress() const;
};

} // namespace rawspeed
