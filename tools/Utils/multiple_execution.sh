#! /bin/bash 

#echo "Usage:" 
#echo "first parameter : name of binary file to execute"
#echo "second parameter : times to execute it" 
#echo "third  parameter : time between retries in milliseconds" 


 count=0
 let iter_sleep=($3/1000)

 while [  $count -lt $2 ]; do
          ./$1 
          sleep ${iter_sleep}
          let count=count+1 
 done
 
 echo "Tests executed $count times"
 exit

# Script is useful for customers who use google test framework
# to run unit-tests
# It allows to execute test many times from separate processes
