'''
This package will help spotting the occurrences of any C++ banned functions in a code repository.
The identified C++ banned functions are displayed in a tabular form along with their locations in the code and their secure alternatives.
An HTML page is displayed once the search is complete.

Usage (Command line):
	<FilePath>\cmdcx.py -f "Full Path for C++ Code Repo/Directory"
	or
	<FilePath>\cmdcx.py --filepath="Full Path for C++ Code Repo/Directory"

Usage (User Interface):
	- Double click on the "codexplore.pyw" file, and select the directory using the File Open Dialog
	- You can also open the "codexplore.pyw" file via IDLE editor, and then hit F5 key to run the script

Exclusion List (Optional): If you want to have a list of subdirectories that should be excluded from the code search for C++ banned functions,
list the relative paths of the sub directories in the "excluded.txt" and include it to the <C++ Code Repo/Directory> location.
A sample exclusion list is given as "_sample_excluded.txt" with this package.
'''

import os, re, subprocess

search_main = ["main", "wmain", "_tmain", "WinMain", "wWinMain", "_tWinMain",]
search_initSecureDllLoading = ["CoreTech::InitSecureDllLoading",]
search_LoadLibrary = ["LoadLibrary",]

all_keywords = search_main + search_initSecureDllLoading + search_LoadLibrary

#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
def exclusionlist():
    xlist = []
    xpath = os.path.join(origin, "excluded.txt")
    if not os.path.exists(xpath):
        pass
    else:
        with open(os.path.join(origin, "excluded.txt"), 'r') as excldfile:
            for x_item in excldfile:
                x_item = os.path.normpath(os.path.join(origin, x_item))
                xlist.append(x_item.rstrip())
    return xlist

# This below function is used for extracting the repeating comment segments from a line of C++ code 
def stripmulticomments(syntx):
    if((syntx.find("/*")>=0) and (syntx.find("*/")>=0) and (syntx.find("*/")>syntx.find("/*"))):
        syntx = syntx[:syntx.find("/*")] + syntx[syntx.find("*/")+2:]
        return stripmulticomments(syntx)
    else:
        return syntx  

def fileSearch(dirpath): 
    global table_row_count
    
    xlist = exclusionlist()
    try:
        dircontents = os.listdir(dirpath)
    except:
        print ("Directory path not accessible. Please use a valid command!")
        os._exit(0)
    for filename in dircontents:
        filepath = os.path.normpath(os.path.join(dirpath, filename))
        if (os.path.isdir(filepath)):
            if ((filepath in xlist) or (filepath.find(".svn")>=0)):
                pass
            else:
                fileSearch(filepath)
        elif (re.search(r'\b'+extn_c+r'\b', filename) or re.search(r'\b'+extn_h+r'\b', filename)):
            print(filepath)            
            #for i in range(len(search_terms)):
            main_flag = False
            initSecureDllLoading_flag = False
            LoadLibrary_flag = False
            keywords_count = 0
            block_comment_flag = 0
            initial_flag = 0
            

            for keyword_ in all_keywords:
                with open(filepath, 'r') as inFile:
                    for line in inFile:
                        
                        #(BEGIN) Extracting the valid code segments from the code written along with the comments
                        if (line.lstrip().startswith("/*") and (initial_flag == 0)):
                            block_comment_flag = 1
                            initial_flag = 1
                        if ((line.find("*/") >= 0) and (block_comment_flag == 1) and (initial_flag == 1)):
                            block_comment_flag = 0
                            initial_flag = 0
                            line = line[line.find("*/")+2:]    
                        if ((block_comment_flag == 1) and (initial_flag == 1)):
                            pass
                        elif (not line.lstrip().startswith("//")):
                            if ((line.find("/*")>0) and (line.find("*/")<0)):
                                block_comment_flag = 1
                                initial_flag = 1
                                line = line[:line.find("/*")+1]
                            elif ((line.find("/*")>=0) and (line.find("*/")>=0) and (line.find("*/")>line.find("/*"))):
                                line = line[:line.find("/*")] + line[line.find("*/")+2:]
                                line = stripmulticomments(line)
                            if (line.find("//")):
                                line = line[:line.find("//")]
                        #(END) Extracting the valid code segments from the code written along with the comments
                                
                            # Searching the function keyword from a legitimate code segment
                            if re.search(r'\b'+keyword_+r'\b', line):
                                if (keyword_ in search_main):
                                    main_flag = True
                                elif (keyword_ in search_initSecureDllLoading):
                                    initSecureDllLoading_flag = True
                                elif (keyword_ in search_LoadLibrary):
                                    LoadLibrary_flag = True
                    if (not initSecureDllLoading_flag):
                        if (LoadLibrary_flag):
                            _color = hig_color
                        else:
                            _color = med_color
                    keywords_count += 1
                    if ((keywords_count == len(all_keywords)) and (main_flag == True) and (initSecureDllLoading_flag == False)):
                        html_file.write("<tr bgcolor="+_color+">"+"<td>"+"'"+filepath+"'"+"</td>"+"<td>"+str(initSecureDllLoading_flag)+'</td>'+'<td>'+str(LoadLibrary_flag)+'</td></tr>')
                        table_row_count += 1

    return
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#

#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#


# File types that'll be searched
extn_c = '\.cpp'
extn_h = '\.h'

if __name__ == "__main__":
    from Tkinter import *
    import tkFileDialog
    # Hiding the main Tk window
    Tk().withdraw()
    # Showing a small GUI to indicate that the script is running
    Label(Tk(), text="Searching for Keywords ... ").pack()
    # Show an "Open" dialog box and return the path to the selected file
    origin = tkFileDialog.askdirectory()
    # In case if a user closes or cancels the Tk window
    if (origin == '') or not os.path.exists(origin):
        print("Directory path not found. The program is exiting!")
        os._exit(0)
else:
    import sys
    from optparse import OptionParser

    parser = OptionParser()
    parser.add_option("-f", "--filename", dest="dirpath", help="C++ code directory location", metavar="FILE")
    (options, args) = parser.parse_args()
    
    if (len(sys.argv) > 1) and (sys.argv[1]=="-f" or sys.argv[1].startswith("--filename")):
        origin = options.dirpath
    else:
        print ("Invalid parameter(s). Use cmdcx.py -h for help.")
        sys.exit(0)

print(origin)

# Initializing the colors for the table rows

med_color = "\'#FFC100\'"
hig_color = "\'#FF5700\'"

# Designing the html page for generating the report
html_file = open('InsecureLoading.html', 'w+')
html_file.write('<table style=\"width:100%\">')
html_file.write('<style type="text/css">'
                +'.tg  {border-collapse:collapse;border-spacing:0;}'
                +'.tg td{font-family:Arial, sans-serif;font-size:12px;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;}'
                +'.tg th{font-family:Arial, sans-serif;font-size:12px;font-weight:normal;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;}'
                +'.tg .tg-yw4l{vertical-align:top}'
                +'</style>'
                +'<table class="tg">')
html_file.write('<p><font face="Arial"><h3>Insecure Loading of MS DLLs</h3></font></p>')
html_file.write('<tr class="tg-yw4l">' + '<th> <h3>File With Main func.</h3> </th>' + '<th> <h3>CoreTech::InitSecureDllLoading<h3> </th>' + '<th> <h3>LoadLibrary used</h3> </th>' + '</tr>')

print("Searching...")
table_row_count = 0

fileSearch(origin)

# In case there's no banned function in the code ...
if (table_row_count == 0):
    html_file.write('<p><font face="Arial"><h3>Hurray! No Insecure Loading Here!</h3></font></p>')

html_file.close()

# If installed, Google Chrome would be used for displaying the results in HTML format. Otherwise, IE would be used.
if os.path.exists("C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe"):
    subprocess.call(["C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe", os.path.join(os.getcwd(),'InsecureLoading.html')],shell=True)
else:
    subprocess.call(["C:\\Program Files\\Internet Explorer\\iexplore.exe", os.path.join(os.getcwd(),'InsecureLoading.html')],shell=True)

print ("Done!!!")
os._exit(0) # Exit without a prompt
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
