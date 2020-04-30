# Readme

## Virtual python environment

Creating and activating the virtual environment
$ python3 -m venv ~/tiro-venv
$ source ~/tiro-venv/bin/activate

After installing a new package
$ pip freeze > requirements.txt

## Regenerate code

Run from the project root (after activating your virtual environment):

$ ./utils/generate-all

## Format everything

Run from the project root:

$ ./utils/format-all
