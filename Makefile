USER=manikant
# Your CSE username

all : copy test source

copy :
		rsync -av `pwd` $(USER)@csecourses1.cse.iitk.ac.in:~/

test :
		ssh $(USER)@csecourses4.cse.iitk.ac.in 'cd cs330assignment3/nachos/code/bin/;make;cd ../test/;make'

source :
		ssh $(USER)@csecourses1.cse.iitk.ac.in 'cd cs330assignment3/nachos/code/userprog/; make depend;make'

run :
		ssh $(USER)@csecourses1.cse.iitk.ac.in 'cd cs330assignment3/nachos/code/userprog/; ./nachos -rs 0 -A 3 -P 100 -x ../test/semaphores'
