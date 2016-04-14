#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "hdf5.h"
#include "functions.h"

#define tic clock_t start = clock();
#define tac clock_t end = clock(); long time = (end - start) / CLOCKS_PER_SEC; printf("Time elapsed: %d \n",time);
#define plop printf("plop\n");
#define newline printf("\n");



/*
 This main must be called with two arguments:
	- The path to the output directory
	- The required filename for the HDF5 file

Example: ./HDF5_maker /import/extend2/dorval/grid1.4/output test.h5
*/

main( int argc, char *argv[] )
{

// argc is the number of arguments received by the program + 1
if( argc != 3)
	{
	printf("Error: not enough or too many arguments for HDF5_maker.\n");
	printf("2 arguments needed: The path to the output directory and the required hdf5 filename.\nReturning.\n");
	exit(0);
	}
else
	{
	// We read the path argument 
	char source_path[100];
	strcpy(source_path,argv[1]);
	strcat(source_path,"/");
	
	// We read the hdf5 argument
	char hdf5_filename[50];
	strcpy(hdf5_filename,argv[2]);	
		
	// This filename is hardwired, you can change it here and recompile
	char logfilename[50]="logfile_hdf5";


	// Opening the todo-list, in which IDL will print the parameters of each new completed star 
	FILE* file = fopen("todolist","r");
	
	// Metallicity, mass and mixing length 
	char* z = malloc(9*sizeof(char));
	char* m = malloc(4*sizeof(char));
	char* l = malloc(4*sizeof(char));

	char ch,tmp[200];
	int star_count,count,i,j,error;

	// Current date and time, to be updated with ctime(&now) each time 
	time_t now;
	time(&now);

	// We read the number of stars that have already been processed to HDF5, 0 if the grid is starting, something else if we are resuming  
	star_count = read_star_count("star_count");
	
	if (star_count == 0 )
		{
		snprintf( tmp , 200 , "Starting the HDF5 processing on %s\n" , ctime(&now) );
		update_log(tmp,logfilename);
		}
	else
		{
		snprintf(tmp,200,"\n-------------------------------------------------------\nResuming the HDF5 processing on %s\n", ctime(&now));
		update_log(tmp,logfilename);
		}

	//This loop doesn't begin if star_count=0.
	for(i=0;i<star_count;i++)
		{
		// We go to the last star we processed in the todolist 
		ch = waitfor("todolist",file,'Z',EOF,'X', logfilename);
		// If there are less stars in the todolist than what the star_count states, we break out of the loop
		if (ch==EOF) break;
		}

	if(ch==EOF) //EOF means end of file
		{// If we had to break out of the loop because of a too high star_count, 
		 // there is a problem, but we can still proceed to process future stars
		update_log("Warning: There are less star in todolist than current starcount ! \n",logfilename);
		}


	// Entering the main loop. The program will keep waiting for stars to appear in todolist to process them until it sees 'END' appears
	for(;;)
		{

		// If STOP exists, that means we have to terminate the program
		if ( file_exists("STOP") ) break;
	
		// If todolist is not there anymore, we signal it and leave the loop.
		if ( file_doesnt_exist( "todolist" ) )
			{
			printf("todolist is not there anymore - shutting down.\n");
			break;
			}
		
		// We wait for a star or 'END' to appear in the todolist 
		ch=waitfor("todolist",file,'Z','E',EOF, logfilename);
		// As described in the declaration of waitfor in functions.c, this function reads the file in an infinite loop
		// until it reads one of the two specified characters

		if(ch=='Z')
			{// A star appeared, we read the parameters and print the time in the log file
			read_parameters(file, z, m, l);
			time(&now);
			update_log(ctime(&now),logfilename);
			snprintf(tmp,200,"Reading: Z:%s M:%s L:%s\n",z,m,l);
			update_log(tmp,logfilename);

			// We try to process it and print the result in the logfile 
			error = process_to_hdf5(source_path,hdf5_filename,z,m,l,logfilename);
			if(error)
				update_log("Error: HDF5 processing failed\n\n",logfilename);
			else	
				{
				update_log("Processed to HDF5\n\n",logfilename);
				delete_output_data(source_path,z,m,l);
				}

			// We tried a new star, incrementing star_count and updating the file
			star_count++;
			update_star_count("star_count",star_count);
			}


		if(ch=='E') 
			{ // We found a 'E' which can only mean "END" appeared. The last star was processed, we can exit the main loop 
			update_log("END found\n",logfilename);

			// We signal the deletion process we did all the stars by appending "END" at the end of the tobedeleted file
			FILE* deletion_file=fopen("tobedeleted","a");
			fprintf(deletion_file,"%s\n","END");
			fclose(deletion_file);//* /

			break;
			}
		}

	fclose(file);
	}

}
