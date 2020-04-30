import os
import subprocess


THIS_DIR = os.path.dirname(__file__)
CODEGEN_DIR = os.path.join(THIS_DIR, "modules")


def run_clang_format(filename):
    print("Formatting {}".format(filename))
    subprocess.check_call(["clang-format", "-style=file", "-i", filename])


def run_cog(filename):
    subprocess.check_call([
        "cog",
        "-e",
        "-U",
        "-r",
        "-I", CODEGEN_DIR,
        filename
    ])
