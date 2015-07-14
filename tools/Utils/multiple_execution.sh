#! /bin/bash 

#echo "Usage:" 
#echo "first parameter : name of binary file to execute"
#echo "second parameter : times to execute it" 

 count="local variable"
 count=0

 while [  $count -lt $2 ]; do
          ./$1 
          sleep 0.01
          let count=count+1 
 done
 
 echo "Tests executed $count times"
 exit

# Script is useful for customers who use google test framework
# to run unit-tests
# It allows to execute test many times from separate processes
