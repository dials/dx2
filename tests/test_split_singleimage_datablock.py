from __future__ import absolute_import, division, print_function

import os

import pytest

from dxtbx.datablock import DataBlockFactory

"""
Test deserializing a datablock that has single file indices while using check_format = True or False
"""


def get_indices(datablock):
    imageset = datablock.extract_imagesets()[0]
    return list(imageset.indices())


def test_split_single_image_datablock(dials_data, tmpdir):
    tmpdir.chdir()
    pytest.importorskip("h5py")
    sacla_file = os.path.join(
        dials_data("image_examples"),
        "SACLA-MPCCD-run266702-0-subset.h5",
    )
    db = DataBlockFactory.from_filenames([sacla_file])[0]
    assert db.num_images() == 4
    imageset = db.extract_imagesets()[0]
    subset = imageset[2:3]
    subblock = DataBlockFactory.from_imageset(subset)[0]
    assert subblock.num_images() == 1
    assert get_indices(subblock) == [2]
