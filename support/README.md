# Readme

## Virtual python environment

Creating and activating the virtual environment

    $ python3 -m venv ~/.venv/tiro-venv
    $ source ~/.venv/tiro-venv/bin/activate
    $ pip install -r ./support/requirements.txt

After installing a new package

    $ pip freeze > requirements.txt

## Regenerate code

Run from the project root (after activating your virtual environment):

    $ ./support/generate-all

## Format everything

Run from the project root:

    $ ./support/format-all
