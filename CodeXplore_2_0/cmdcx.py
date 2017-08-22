import sys

# Path for Python
py_path = "C:\\Python27"

# If Python is not in the default PATH variable, append it to the PATH.
if py_path not in sys.path:
    sys.path.append(py_path)

# Run from the codexplore module.
import codexplore
