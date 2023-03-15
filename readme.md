# Parallel code using pthreads to perform relaxation technique 

This project solves a system of equations in a matrix through the 'relaxation technique'. Each value is updated by taking the average of the 4 values to the north/east/south/west of the target value. This can be done in parallel. 

## Running the code 

Before running parallel or sequential program, first run createRandomFile.c
It will generate 'numbers.txt' which are randomly generated integers
It's currently set to allow runs up to n=10,000

gcc & ./ to run parallel 
gcc & ./ to run sequential 


## Approach 

See method.txt
