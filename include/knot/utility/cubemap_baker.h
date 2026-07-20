// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

namespace knot {
unsigned int bakeHDRMapToCubemap(unsigned int hdrTexture2D, int size);
unsigned int bakeCubemapToIrradianceMap(unsigned int envCubemapID, int size);
}