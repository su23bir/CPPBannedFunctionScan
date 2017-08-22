import os, re, subprocess
import banfunc

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
    global _row
    xlist = exclusionlist()
    try:
        dircontents = os.listdir(dirpath)
    except:
        print ("Directory path not accessible. Please use a valid command!")
        os._exit(0)
    for filename in dircontents:
        filepath = os.path.normpath(os.path.join(dirpath, filename))
        if (os.path.isdir(filepath)):
            if filepath in xlist:
                pass
            else:
                fileSearch(filepath)
        elif (re.search(r'\b'+extn_c+r'\b', filename) or re.search(r'\b'+extn_h+r'\b', filename)):
            print(filepath)
            for i in range(len(badfuncs)):
                for key,value in badfuncs[i].items():
                    for keyword in key:
                        with open(filepath, 'r') as inFile:
                            block_comment_flag = 0
                            initial_flag = 0
                            line_count = 0
                            for line in inFile:
                                line_count += 1
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
                                        # Searching the banned function keyword from a legitimate code segment
                                    if re.search(r'\b'+keyword+r'\b', line):
                                        _row += 1
                                        _color = row_color[(_row+1)%2]
                                        html_file.write("<tr bgcolor="+_color+">"+"<td>"+"'"+keyword+"'"+"</td>"+"<td>"+filepath+': '+str(line_count)+'</td>'+'<td>'+value+'</td></tr>')
    return
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#

#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
badfuncs = banfunc.banfunc()    # Loading the list of the dictionaries that contain C++ band functions

# File types that'll be searched
extn_c = '\.cpp'
extn_h = '\.h'

if __name__ == "__main__":
    from Tkinter import *
    import tkFileDialog
    # Hiding the main Tk window
    Tk().withdraw()
    # Showing a small GUI to indicate that the script is running
    Label(Tk(), text="Searching for Banned C++ Functions ... ").pack()
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
_row = 0
row_color = ["\'#F2F3F4\'","\'#E5E7E9\'"]

# Designing the html page for generating the report
html_file = open('banned_functions.html', 'w+')
html_file.write('<table style=\"width:100%\">')
html_file.write('<style type="text/css">'
                +'.tg  {border-collapse:collapse;border-spacing:0;}'
                +'.tg td{font-family:Arial, sans-serif;font-size:14px;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;}'
                +'.tg th{font-family:Arial, sans-serif;font-size:14px;font-weight:normal;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;}'
                +'.tg .tg-yw4l{vertical-align:top}'
                +'</style>'
                +'<table class="tg">')
html_file.write('<p><font face="Arial"><h3>Banned Functions in C++ Code</h3></font></p>')
html_file.write('<tr class="tg-yw4l">' + '<th> <h3>Keyword</h3> </th>' + '<th> <h3>Found in<h3> </th>' + '<th> <h3>Suggestion</h3> </th>' + '</tr>')

print("Searching...")

fileSearch(origin)

# In case there's no banned function in the code ...             
if (_row == 0):
    html_file.write('<p><font face="Arial"><h3>Awesome! There\'s none in here :) </h3></font></p>')
html_file.close()
# If installed, Google Chrome would be used for displaying the results in HTML format. Otherwise, IE would be used.
if os.path.exists("C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe"):
    subprocess.call(["C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe", os.path.join(os.getcwd(),'banned_functions.html')],shell=True)
else:
    subprocess.call(["C:\\Program Files\\Internet Explorer\\iexplore.exe", os.path.join(os.getcwd(),'banned_functions.html')],shell=True)

print ("Done!!!")
os._exit(0) # Exit without a prompt
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
