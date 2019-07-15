from os.path import exists, join, basename
from os import makedirs, remove
from six.moves import urllib
import tarfile
import os
from torchvision.transforms import Compose, CenterCrop, ToTensor, Resize

from dataset import DatasetFromFolder

TESTING_DIR = "dataset/testing"
TRAINING_DIR = "dataset/training"
TEST_URL = "http://www2.eecs.berkeley.edu/Research/Projects/CS/vision/bsds/BSDS300-images.tgz"


def download_bsd300(dest="dataset"):
    """
    WE SHOULD NOT BE USING THIS
    Part of the original demo boilerplate, it downloads training images from the BSDS300 repository
    """
    output_image_dir = join(dest, "BSDS300/images")

    if not exists(output_image_dir):
        makedirs(dest)
        url = "http://www2.eecs.berkeley.edu/Research/Projects/CS/vision/bsds/BSDS300-images.tgz"
        print("downloading url ", url)

        data = urllib.request.urlopen(url)

        file_path = join(dest, basename(url))
        with open(file_path, 'wb') as f:
            f.write(data.read())

        print("Extracting data")
        with tarfile.open(file_path) as tar:
            for item in tar:
                tar.extract(item, dest)

        remove(file_path)

    return output_image_dir


def calculate_valid_crop_size(crop_size, upscale_factor):
    return crop_size - (crop_size % upscale_factor)


def prediction_transform(crop_size, upscale_factor):
    return Compose([
        CenterCrop(crop_size),
        Resize(crop_size // upscale_factor),
        ToTensor(),
    ])


def target_transform(crop_size):
    return Compose([
        CenterCrop(crop_size),
        ToTensor(),
    ])


def contains_image_data(directory):
    files = os.listdir(directory)
    for file in files:
        if file.lower().endswith(('.png', '.jpg', '.jpeg')):
            return True
    return False


def get_training_set(upscale_factor):
    return get_data_set(upscale_factor, TRAINING_DIR)


def get_test_set(upscale_factor):
    return get_data_set(upscale_factor, TESTING_DIR)


def get_data_set(upscale_factor, directory):
    crop_size = calculate_valid_crop_size(256, upscale_factor)
    if not contains_image_data(directory):
        raise Exception("The \'%s\' directory does not contain any image files" % directory)
    else:
        return DatasetFromFolder(directory,
                                 prediction_transform=prediction_transform(crop_size, upscale_factor),
                                 target_transform=target_transform(crop_size))
