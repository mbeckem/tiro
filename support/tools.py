import os
import subprocess


THIS_DIR = os.path.dirname(__file__)


def run_clang_format(*filenames):
    subprocess.check_call(["clang-format", "-style=file", "-i", *filenames])


def run_cog(*filenames):
    subprocess.check_call(["cog", "-e", "-U", "-r", "-I", THIS_DIR, *filenames])
