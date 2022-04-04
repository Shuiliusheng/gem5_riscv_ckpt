#!/bin/bash
if [[ $# < 1 ]]; then
    echo "parameter is not enought!"
    exit
fi

textinfo=`grep -r textRange $1 | awk -F '{' '{print "{"$2}' `
echo $textinfo
echo $textinfo >temp.log
cat $1 | awk -F '{' '{print "{"$2}' >> temp.log
mv temp.log $1
