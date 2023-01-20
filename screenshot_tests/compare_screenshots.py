#
# SPDX-License-Identifier: MPL-2.0
#
# This file is part of Ramses Composer
# (see https://github.com/bmwcarit/ramses-composer).
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

import sys
import os
from pathlib import Path

try:
    from PIL import Image
    from PIL import ImageChops
except ModuleNotFoundError as e:
    print("Didn't find module PIL. Please remove build/release*/bin/python* folder and rebuild.")
    raise

class ImageDiff:
    """Class for storing image differences."""
    color: Image
    alpha: Image

    def __init__(self, color, alpha):
        self.color = color
        self.alpha=alpha


def compareEqual(image1, image2, percentageOfWrongPixelsAllowed, percentageOfRGBDifferenceAllowedPerPixel, logs=False):
    if (image1.size[0] != image2.size[0]) or (image1.size[1] != image2.size[1]):
        if logs:
            print("images sizes do not match, cannot compare {}*{} vs {}*{}".format(image1.size[0], image1.size[1], image2.size[0], image2.size[1]))
        return False, None

    nrWrongPixels = 0
    nrDifferentPixels = 0

    # PIL image comparison is well optimized -> early out if images are identical
    diffs=None
    if image1 != image2:
        imageDiff = ImageChops.difference(image1.convert("RGBA"), image2.convert("RGBA"))
        imageData = imageDiff.getdata()
        percentageOfRGBDifferenceAllowedPerPixelScaled = int(percentageOfRGBDifferenceAllowedPerPixel * 255)

        black=(0,0,0)
        diffcolor = []
        diffalpha = []
        hasColorDiff=False
        hasAlphaDiff=False

        for i in range(0, image1.width * image1.height):
            chMax = max(imageData[i])
            color = imageData[i][0:3]
            alpha = imageData[i][3]
            if chMax > 0:
                if alpha > 0:
                    diffalpha.append((alpha, alpha, alpha))
                    diffcolor.append(black)
                    hasAlphaDiff=True
                else:
                    diffalpha.append(black)
                    diffcolor.append(color)
                    hasColorDiff=True
                nrDifferentPixels += 1
                if chMax > percentageOfRGBDifferenceAllowedPerPixelScaled:
                    nrWrongPixels += 1
            else:
                diffalpha.append((0,0,0))
                diffcolor.append((0,0,0))

        imageDiffColor = Image.new('RGB', (image1.width, image1.height))
        imageDiffAlpha = Image.new('RGB', (image1.width, image1.height))
        imageDiffColor.putdata(diffcolor)
        imageDiffAlpha.putdata(diffalpha)
        diffs = ImageDiff(
            imageDiffColor if hasColorDiff else None,
            imageDiffAlpha if hasAlphaDiff else None)


    totalNumberOfPixels = image1.width * image1.height
    if logs:
        print("Comparison stats: Percentage of wrong pixels: {0}%".format(float(nrWrongPixels) / totalNumberOfPixels * 100))
        print("Comparison stats: Percentage of different, but accepted pixels: {0}%".
                        format(float(nrDifferentPixels - nrWrongPixels) / totalNumberOfPixels * 100))

    if ((float(nrWrongPixels) / totalNumberOfPixels) > percentageOfWrongPixelsAllowed):
        if logs:
            print("compareEqual: Too many wrong pixels, aborting...")

        return False, diffs
    return True, diffs


class ImageDiffError(Exception):
    def __init__(self, test):
        self.test = test


def compareAndGenerateDiffs(expected_dir, actuals_dir, diffs_dir, test, logs):
    image_expected = Image.open(f'expected/{test}.png')
    image_actual = Image.open(f'actual/{test}.png')

    equal, diffs = compareEqual(image_expected, image_actual, 0.001, 4.0/256.0, logs=logs)
    if not equal:
        if diffs.color:
            color_file = f'diff/{test}_color.png'
            diffs.color.save(color_file)
            #print(f"Test {test} failed because of RGB differences! See image {str(color_file)}")
            return False
        if diffs.alpha:
            alpha_file = f'diff/{test}_alpha.png'
            diffs.alpha.save(alpha_file)
            #print(f"Test {test} failed because of alpha differences! See image {str(alpha_file)}")
            return False
        raise ImageDiffError(test)
    else:
        #print(f"Test {test} passed successfully!")
        return True


if __name__ == '__main__':
    test_name = sys.argv[1]
    success = compareAndGenerateDiffs('expected','actual', 'diff', test_name, True)
    sys.exit(0 if success else 1)