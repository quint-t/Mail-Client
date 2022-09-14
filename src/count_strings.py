import os
import string

printable = set(string.printable).difference(string.whitespace)
c = 0
for root, dirs, files in os.walk("."):  
    for filename in files:
    	if filename.endswith('.h') or filename.endswith('.cpp'):
    		with open(filename, 'r', encoding='utf-8') as fin:
    			for line in fin.readlines():
    				c += not bool(set(line).isdisjoint(printable))
print('Count: ' + str(c))
input()
