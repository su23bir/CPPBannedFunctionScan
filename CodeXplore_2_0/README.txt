This utility is to spot the usage of any C++ banned functions in a code repository. The identified C++ banned functions are displayed in a tabular form along with their locations in the code and their secure alternatives. An HTML page is displayed once the search is complete.

Usage (Command line):
	<FilePath>\cmdcx.py -f "Full Path for C++ Code Repo/Directory"
	or
	<FilePath>\cmdcx.py --filepath="Full Path for C++ Code Repo/Directory"

Usage (User Interface):
	- Double click on the "codexplore.pyw" file, and select the directory using the File Open Dialog
	- You can also open the "codexplore.pyw" file via IDLE editor, and then hit F5 key to run the script

Exclusion List (Optional): If you want to have a list of subdirectories that should be excluded from the code search for C++ banned functions, list the relative paths of the sub directories in the "excluded.txt" and include it to the <C++ Code Repo/Directory> location. A sample exclusion list is given as "_sample_excluded.txt" with this package.
