Parallel Computing coursework 1 - Shared memory architecture 

Suppose you have the following 5x5 matrix:

a b c d e 
f g h i j 
k l m n o 
p q r s t 
u v w x y

g h i 
l m n
q r s 

are the values that we're going to change with the relaxation technique. 

My approach to parallelism for this problem was to split the matrix by columns. 
To do this, I split the 3x3 sub-matrix into columns that I passed to the start routine to evaluate. 

So one sub-matrix for 2 threads would be:
b
g
l
q
v

and the other would be:
c d
h i 
m n
r s
w x 
once you also include the top/bottom rows.

I then padded each of these sub-matrices with +1 columns either side to create:
a b c
f g h
k l m
p q r
u v w

and 
b c d e 
g h i j 
l m n o 
q r s t 
v w x y

These two sub-matrices were sent to the start routine, inside the struct matrix_rectangle. 

The start routine would then solve each of them, evaluate if it was under the given precision and return.
I used a join within a for loop to ensure that every thread had returned and I could continue with the main thread. 

I then made sure to update the padded columns that the sub-matrices share. 
For sub-matrix 1, the column starting with 'c' had to be updated to reflect the changes made in sub-matrix 2.
Once that was done, the threads could be called on the same sub-matrices until they were finished. 

There was no need to lock any shared resources as each thread had it's own copy of the data it needed to compute it's values. 
The idea being that removing the need for locking of shared resources would create a greater speed-up of the program.
The code did synchronize to ensure that each thread was finished before starting the next iteration. 