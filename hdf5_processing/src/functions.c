#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "hdf5.h"

#define tic clock_t start = clock();
#define tac clock_t end = clock();
#define plop printf("plop\n");
#define plop1 printf("plop1\n");
#define plop2 printf("plop2\n");
#define plop3 printf("plop3\n");
#define plop4 printf("plop4\n");
#define newline printf("\n");



// Returns 1 is the file exists 
int file_exists( char filename[] )
	{
	if ( access( filename, F_OK ) != -1)
		return 1;
	else
		return 0;
	}

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

// Returns 1 is the file doesn't exist. This is just for the sake of clarity in if statements. 
int file_doesnt_exist( char filename[] )
	{
	if ( access( filename, F_OK ) == -1)
		return 1;
	else
		return 0;
	}

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

//This function append new text to the hdf5 processing log file. It doesn't erase what is already in it.
void update_log( char new_content[] , char logfilename[] )
	{
	if( file_exists( logfilename ) )
		{
		FILE* file=fopen( logfilename , "a" ); // "a" is the append mode for fopen, the pointer goes straight to the end of the file.
		fprintf( file , new_content );
		fclose( file );
		}
	else
		{
		printf( "Log file: %s not found\n" , logfilename );
		exit(0);
		}
	}



/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

// This function returns a dynamically allocated 2D double array. Warning: a dynamically allocated array should always be freed after use.
double** make2Ddoublearray( int arraySizeX , int arraySizeY )
	{  
	double** theArray;  
	int i;
	theArray = (double**) malloc( arraySizeX*sizeof(double*) );  
    
	for (i = 0; i < arraySizeX; i++)  
		{
        	theArray[i] = (double*) malloc( arraySizeY*sizeof(double) );  
 		}     
 	return theArray;  
	}   

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */
// This function frees a dynamically allocated double 2D array
void free2Ddoublearray( double **array , int nrow )
	{
	int i;
	for( i=0 ; i<nrow ; i++ )
		{
		free(array[i]);
		}
	free(array);
	}

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */
// This function frees a dynamically allocated string array
void free_string_array( char **array , int nstring )
	{
	int i;
	for( i=0 ; i<nstring ; i++)
		{
		free(array[i]);
		}
	free(array);
	}


/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

/*
int getnames(char filename[],int skiplines, char ***outarray, int *nnames,char logfilename[])
    		
		This function takes
			- the name of the file containing the name, must be a typically formatted history file or profile from MESA.
			- an integer as the number of lines to skip to get to the names in the file
			- a pointer to a 2d array of char (which is actually a pointer to a pointer to a pointer of char
								and can be seen as a pointer to an array of string )
			- a pointer to an integer
			- the name of the logfile in which error messages will be printed


		The function will put the column names it finds in the string array (pointed by the char triple pointer) and the
		number of column names it found in the nlines integer. It returns 0 if the file isn't found, 1 if it is.
		Example of calling:
		
				char **array;
				int ncol;
				char filename[]="history.data";
				char logfilename[]="log_file";

				getnames(filename,&array,&ncol,logfile);


		/!\ /!\ VERY IMPORTANT /!\ /!\
		
		One should always remember to free the array returned by the function in the pointer. The dynamically allocated array issued by the function
		changes name by being passed to the pointer, but it still need to be freed before the end of the main. One should free the array 
		with the free_string_array function. */

int getnames( char filename[] , int skiplines , char ***outarray , int *nnames , char logfilename[] )
	{

	// Initialiazing variables 
	int i,ncol;
	char ch,tmp[200],buffer[4096];

	// Opening the file in which we'll read the names 
	FILE* file = fopen( filename , "r" );

	// If we couldn't open it, we signal it in the logfile and we return 0 
	if (file == NULL) 
		{
	       	snprintf( tmp , 200 , "Couldn't open %s\n" , filename );
	       	update_log(tmp,logfilename);
		return 0;
		}

	//We get to the line where the columns numbers are written
	for( i=0 ; i<skiplines-1 ; i++) 
		{
		fgets(buffer, 4096, file);
		}

	// We get to the end of this line
	do
		{
		ch = fgetc(file);
		}
	while( ch != '\n' );

	// We go 8 positions back and read what is the last column number, which is the total number of column
	fseek( file , -8 , SEEK_CUR);
	fscanf( file , "%d \n" , &ncol );

	// Back at the beginning of the file
	rewind(file);

	// This string array (char 2D array / pointer to a pointer of char) will contain the names of the columns  
	char **string_array = (char **)malloc( ncol * sizeof(char *) );
	if (string_array == NULL)
		{ // 	We signal a problem in the dynamical allocation, this could be due to a ressource leak: arrays being created
		  //	and not freed, again and again, filling up the memory. The program probably won't be able to work properly anymore.
		  //	We return -1 as a signal that the program should be stopped immediately 
		update_log("Problem creating the name array, returning\n",logfilename);
		return -1;
		}

	// We skip the header lines
	for(i=0 ; i<skiplines ; i++) 
		{
		fgets( buffer , 4096 , file );
		}

	// For each column, we create a string in the string array, check it is ok, then read the name of the column into it. 
	for( i=0 ; i<ncol ; i++)
		{
		string_array[i] = malloc(100*sizeof(char));
		if (string_array == NULL)
			{
			snprintf( tmp , 200 , "Problem creating the %dth name , returning\n" , i );
			update_log( tmp , logfilename );
			return -1;
			}
		fscanf( file , " %s " , &tmp );
		strcpy( string_array[i] , tmp );
		}

	fclose(file);

	// We get the result out through pointers 
	*outarray = string_array;
	*nnames = ncol;

	// We return 1 as we had no problems 
	return 1;
	}




/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

/*
int getdata(char filename[], int skiplines, int ncol, int *nlines, double ***dataarray, char logfilename[])
   		
		This function reads an array of numbers(double) written in exp format in a text file. The arguments are:
			- a filename string
			- an integer as the number of lines to skip in the file to get to the actual data
			- an integer as the number of columns in the data
			- an integer as the number of lines in the data
			- a pointer to a 2D array of doubles
			- the log file name
		
		The function will write the content of the file as a double 2D array in the triple pointer. Example:
		
				double **array;
				int skiplines=6; int ncol=10; int nlines=50;
				char filename[]="data.txt";

				getnames(filename,&array,&size,"logfile");

		/!\ /!\ VERY IMPORTANT /!\ /!\
		
		One should always remember to free the array returned by the function in the pointer. The dynamically allocated array issued by the function
		changes name by being passed to the pointer, but it still need to be freed before the end of the main. One should free the array 
		with the function free2Ddoublearray. */

int getdata( char filename[] , int skiplines , int ncol , int *nlines , double ***dataarray , char logfilename[] )
	{

	// Initialization 
	int i,j,nline=0;
	char ch,buffer[4096];
	double tmp;

	// Opening the file containing the data  
	FILE* file = fopen(filename, "r");

	// We count the number of lines in the file by reading each character and counting the \n 
	do 
		{
    		ch = fgetc(file);
    		if(ch == '\n')
			{
    			nline++;
			}
		} while (ch != EOF);

	// We substract the number of header lines and rewind the file 
	nline = nline - skiplines ;
	rewind(file);

	// This 2D double array (pointer to an array of pointer to doubles) will contain the data  
	double **array = make2Ddoublearray( ncol , nline );
	if (array == NULL)
		{
		update_log("Problem creating the data array, returning\n",logfilename);
		return -1;
		}

	// We skip the header lines 
	for ( i=0 ; i<skiplines ; i++)
		{
		fgets( buffer , sizeof(buffer) , file);
		}

	//  We read the data through fscanf one number at a time 
	for ( i=0 ; i<nline ; i++)
		{
		for ( j=0 ; j < ncol ; j++ )
			{
			fscanf( file , "%le" , &tmp );
			array[j][i] = tmp;
			}
		}

	// We print the data array and the number of lines out through the pointer 
	*dataarray = array;
	*nlines = nline;

	fclose(file);

	// We return 1 as we had no problems 
	return 1;
	}

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

// 	This function takes the path to the directory where MESA exports its files, Z, M and L and 
//	writes the specific path for the data from this star in the buffer string 
void get_file_path( char* buffer[] , char source_path[] , char z[] , char m[] , char L[] )
	{
	char tmp[300];
	
	strcpy(tmp,source_path);
	// Example of what this line is doing: "./output/" turns into "./output/0.0001000/M0.50_L1.80/"     		     
	strcat(tmp,z); strcat(tmp,"/M"); strcat(tmp,m); strcat(tmp,"_L"); strcat(tmp,L); strcat(tmp,"/");
	
	strcpy(*buffer,tmp);
	}


/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

/*	This function takes the file path from previous function and writes the complete path+name of each profile
	in the buffer. It takes the model number, z, m, l and a choice variable to get a profile or a fgong file:
		
	get_file_name(&buffer, 12, 0 ,"output/0.0001000/M0.50_1.80/", "0.0001000", "0.50" ,"1.80");
	==> buffer = "output/0.0001000/M0.50_1.80/m0050.z0.0001000.a180.o000.12"

	get_file_name(&buffer, 12, 1 ,"output/0.0001000/M0.50_1.80/", "0.0001000", "0.50" ,"1.80");
	==> buffer = "output/0.0001000/M0.50_1.80/fgong.m0050.z0.0001000.a180.o000.12"					

	/!\ /!\ WARNING /!\ /!\
	This function can produce fgong file paths, but get_data and get_names are not designed to handle fgong files.   */	

void get_file_name( char* buffer[] , int file_number , int type_choice , char file_path[] , char z[] , char m[] , char L[])
	{
	char tmp[100],filename[300],mass[10],length[10],nmodel[10];
	char *pos;

	// We write the file_path into our filename variable, and we are going to append various things to it 
	strcpy( filename , file_path );
	
	// If we want the fgong file 	
	if( type_choice == 1 )
		strcat( filename , "fgong." );

	// This block is probably overcomplicated, but it basically turns "0.50" into "050" 
	strcpy( tmp , m );
		tmp[1] = '\0';
		pos = &tmp[2];
		strcat( tmp , pos );

	//  tmp="050" -->  mass="0050"  /!\ This wouldn't be correct anymore if masses were to reach 10 solar masses and above 
	strcpy( mass , "0" );
		strcat( mass , tmp );

	// tmp="1.80" -->  length="180" 
	strcpy( tmp , L );
		tmp[1] = '\0';
		pos = &tmp[2];
		strcat( tmp , pos );
		strcpy( length , tmp );
	
	// We put all the pieces together 
	strcat( filename , "m" ); strcat( filename , mass ); 
	strcat( filename , ".z" ); strcat( filename , z ); 
	strcat( filename , ".a" ); strcat( filename, length );  // "a" is for mixing_length_Alpha
	strcat( filename , ".o000." );

	// We write the model number at the end 
	snprintf( nmodel , 10 , "%d" , file_number );
	strcat( filename , nmodel );

	// We write out the filename 
	strcpy( *buffer , filename );
	}


/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

// This function go fetch the last model number in the history file.	
int get_last_model( char filename[] )
	{
	char ch;
	int maxmodel,linemark,linenumber;

	FILE* file = fopen(filename, "r");
	linenumber = 1;
	linemark = 1;
	
	// We set the linemark to the last line (which is empty in history files) 
	do 
		{
		ch = fgetc(file);
		if(ch == '\n')
			{
	  		linemark++;
			}
		} while (ch != EOF);

	rewind(file);

	// We get to the second-to-last line (which contains the last model) by stopping one line before the linemark 
	do 
		{
		ch = fgetc(file);
    		if(ch == '\n')
			{
    			linenumber++;
			}
		} while (linenumber < linemark-1);
	
	// We read the number of the last model 
	fscanf( file , "%d" , &maxmodel );

	fclose(file);

	return maxmodel;
	}

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

// This function returns the number of zones (=data lines) in a profile 
int get_nzones(char filename[])
{
	int i,result;
	char ch;
	FILE* file = fopen(filename , "r");
	
	// We read the first two lines
 	for( i=0 ; i<2 ; i++ )
	{	do 
		{
		ch = fgetc(file);
		} while (ch != '\n');		}

	// We first read the version number, then the model number. The third time, we read the number of zones.
 	for( i=0 ; i<3 ; i++ )
	{
	fscanf(file,"%d",&result);		}	

	return result; // We return the number of zones

}

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

/*	This function writes the data from data_array within datasets named after columns_name_array at place_id
	(which is most probably a group name) into the HDF5 file currently open. The dimensions, ncol and nlines, are
	needed to allow enough space in the file.								*/	     

void writehdf5( hid_t place_id , char **columns_name_array , double **data_array , int ncol, int nline, int scalar_attribute, char attribute_name[] ) 
	{
	int attribute=scalar_attribute;
	// HDF5 type variables initialization 
	hid_t      dataset; 		// dataset identity
	hid_t   dataspace_id;		// dataspace identity
	hid_t  attribute_ds_id;		// attribute dataspace identity
	hid_t  attribute_id;		// attribute identity
	herr_t  ret; 			// return value for functions
	hsize_t fdim[] = {nline};	// HDF5 translation of the size of the data we are going to write

	int i;
	
	// If the attribute is non 0, we attach it to the current group. For history files, it is the number of models, and for profiles, the number of zones
	if (attribute != 0)
		{
		attribute_ds_id  = H5Screate(H5S_SCALAR);
		attribute_id = H5Acreate2(place_id, attribute_name, H5T_NATIVE_INT, attribute_ds_id, H5P_DEFAULT, H5P_DEFAULT);
		ret = H5Awrite(attribute_id, H5T_NATIVE_INT, &attribute);
		ret = H5Sclose(attribute_ds_id);
		ret = H5Aclose(attribute_id);
		}


	// This loop applies to columns. For each columns, we create a dataspace, a dataset, we write the data and close both the dataset and dataspace 
	for( i=0 ; i<ncol ; i++ )
		{
		dataspace_id = H5Screate(H5S_SIMPLE);			//Create the dataspace. H5S_SIMPLE: regular array of elements
		ret = H5Sset_extent_simple(dataspace_id, 1, fdim, NULL);	//Extend the dataspace to the needed size, here fdim (nline in HDF5 language)
	
		// We create a double-type dataset, named after the name array, on the previously created dataspace 
		dataset = H5Dcreate2(place_id, columns_name_array[i], H5T_NATIVE_DOUBLE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
		// We write the corresponding data in the dataset 
		ret = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL , H5S_ALL, H5P_DEFAULT, data_array[i]);

		// We close the dataspace and dataset
		ret = H5Sclose(dataspace_id);
		ret = H5Aclose(dataset);
		}



	}


/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */
/*
  	This function will return a non zero value if it detects a problem with data from the star it is told to inspect (z,m,l). If it 
	is the case, an error message will be written into the string passed as *error_message, as well as the number of files missing
	in *inc if that information is need. The problems which will cause the function to raise a red flag are:
		1 - No history file was found. This means either there is no data at all or the name of the star directory is not what it should be
		2 - There are more than 40 missing profiles compared to what is in the history file
		3 - There are more than 40 missing fgong files compared to what is in the history file
		4 - The last logdt in the history file is less than -7. This most likely means the star had a convergence problem and stopped.
		5 - The difference between the number of the last model and the number of lines in the history files is greater than 30.
		  This never happened to me but it could mean MESA has appended a new run to an existing history file.
	If no problem is detected, the function returns 0, else it returns the number of the error. This last feature is unused at this point
	but could be useful for further development of the code. The function was named to enhance clarity in a if statement:
	"if (star_is_invalid(..)) { stop } else { go on .... " */



int star_is_invalid( char source_path[] , char z[] , char m[] , char l[] , int *inc , char *error_message[] , char logfilename[] )
{

	char **columns_name_array_history;
	double **data_array_history,lastlogdt;

	// For some reason, I need to define these through dynamical allocation to pass them as pointers in functions. 
	char* filepath = malloc(300*sizeof(char));
	char* filename = malloc(300*sizeof(char));

	char groupname[40],new_groupname[200],modelnumber[10],history_filename[200], tmp[200];
	int i,maxmodel,col,comparison,incp,incf,ncol,nline,skiplines;

	// We get the path of the star we have to inspect 
	get_file_path( &filepath , source_path , z , m , l );

	// We get the full path of the history file 
	strcpy(history_filename,filepath);
	strcat(history_filename,"history.data");
		
	if(file_doesnt_exist( history_filename))
		{
		// If we didn't find the history file, we write the error message, put it in the pointer, free the arrays and return 1 
		snprintf(tmp,200,"No history file. The data probably isn't there at all.\nFile path for missing history: %s\n", history_filename);
		strcpy(*error_message,tmp);
		free(filepath);
		free(filename);
		return 1;
		}	
	
	// This block read the names of the columns in the history file and read its data  
	skiplines = 5;
	getnames( history_filename , skiplines , &columns_name_array_history , &ncol , logfilename);
	skiplines++;
	getdata( history_filename , skiplines , ncol , &nline , &data_array_history , logfilename);

	// We are now going to need the number of the last model 
	maxmodel = get_last_model(history_filename);

	incp = 0; // This will be incremented for profiles
	incf = 0; // This will be incremented for fgong files		

	// We go through all the models, checking the existence of the files (profiles and fgong files) 
	for( i=0 ; i<maxmodel ; i++)
		{
		get_file_name( &filename , i , 0 , filepath , z , m , l );
		if( file_doesnt_exist(filename) )	incp++;		
				
		get_file_name( &filename , i , 1 , filepath , z , m , l );
		if( file_doesnt_exist(filename) )	incf++;	
		}
	// If we found more than 40 missing profiles, we signal it in the error message and return 2 
	if (incp > 40)
			{
			*inc = incp;	
			snprintf( *error_message , 200 , "Error: There are %d missing profiles\n" , incp );
			free(filepath);
			free(filename);
			return 2;
			}
	// If we found more than 40 missing fgong files, we signal it in the error message and return 3 	
	if (incf > 40)
			{
			*inc=incf;
			snprintf( *error_message , 200 , "Error: There are %d missing fgong files\n" , incf);
			free(filepath);
			free(filename);
			return 3;
			}

	col = -1;
	comparison = -1;
	// As the order of columns in history files could change in the future, this loop looks for "log_dt" in the column name 
	while(comparison != 0)
		{
		col++;
		comparison = strncmp( "log_dt" , columns_name_array_history[col] , 6 );
		}

	// If the last log_dt is less than -7, it is likely MESA just stopped because of convergence problem 
	lastlogdt = data_array_history[col][nline-1];
	if(lastlogdt < -7)
		{
		strcpy( *error_message , "Error: Probable convergence problem\n" );
		free(filepath);
		free(filename);
		return 4;
		}

	// If the discrepancy number of lines / last model in the history file is too large, that can't be good.  
	if( abs(nline-maxmodel) > 40 ) 
		{
		strcpy( *error_message , "Error: Discrepancy between the number of history lines and the last model\n" );
		free(filepath);
		free(filename);
		return 5;
		}
	
	// We didn't encounter any red flag, the star is valid 
	free(filepath);
	free(filename);
	return 0;
}


/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

/*
	This is the "big" function, the most important. It basically just take a mass, a metallicity, a mixing_length and:
		- Check the data for any problem
		- Create/open the HDF5 file
		- Create/open the matching groups in the file
		- If it finds an already existing group for the star, it renames it as "old_[]" and create a new one
		- Read the history file and writes it into the HDF5
		- Read each profile and writes it into the HDF5
		- Close the groups and the HDF5 file

	If star_is_invalid detected a problem, the function update the logfile with the error message and returns 1. Else it
	proceeds to process the data into the HDF5 file and returns 0 once it finished.

	hid_t      dataset; 		// dataset identity
	hid_t   dataspace_id;		// dataspace identity
	herr_t  ret; 			// return value for functions
	hsize_t fdim[] = {nline};	// HDF5 translation of the size of the data we are going to write


*/


int process_to_hdf5( char source_path[] , char hdffilename[] , char z[] , char m[] , char l[] , char logfilename[] )
{
	// HDF5 type variables initialization 
	hid_t  file_id;							// file identifier 
	hid_t  zgroup , stargroup , modelgroup , historygroup ;  	// group 
	herr_t status , error , ret;					// return value	
	htri_t teststatus;						// boolean return value	

	// Buffer variables, will passed in functions as pointers 
	char* filepath = malloc(300*sizeof(char));
	char* filename = malloc(300*sizeof(char));
	char* error_message = malloc(200*sizeof(char));
	char **columns_name_array_profile, **columns_name_array_history;
	double **data_array_profile,**data_array_history;

	int p,skiplines, ncol,nline,inc,maxmodel,nzones;
	char groupname[40],new_groupname[200],modelnumber[10],history_filename[200],tmp[200];



	if ( star_is_invalid(source_path, z, m, l, &inc, &error_message ,logfilename)  )
		{
		// A problem with the star was detected, we update the log with the error message and return 1 
		// update_log(error_message,logfilename);
		return 1;
		}	
	else
		// There is no obvious problem with the data, proceeds to read it and write it into HDF5 format 
		{
		
		// We open or create the HDF5 file depending of whether it already exists. 
		if ( file_exists(hdffilename) )
			file_id = H5Fopen( hdffilename, H5F_ACC_RDWR, H5P_DEFAULT);
		else 
			file_id = H5Fcreate( hdffilename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

		// We check if the metallicity group for the current z already exists. 
		teststatus = H5Lexists( file_id, z, H5P_DEFAULT );
		snprintf( tmp , 200 , "Group %s/ : " , z );
		update_log(tmp,logfilename);
		
		if (teststatus) 
			zgroup = H5Gopen( file_id , z , H5P_DEFAULT );
		else	
			zgroup = H5Gcreate1( file_id, z, 0 );
		
		// We build the name of the star group, example: M0.50_L1.80 
		strcpy(groupname,"M");
		strcat(groupname,m); strcat(groupname,"_L"); strcat(groupname,l);
	
		// Is there already a group for the same star ? 
		teststatus = H5Lexists( zgroup, groupname, H5P_DEFAULT );
		if(teststatus)
			{ 
		 	// There is indeed already one. The program deals with existing groups by creating numbered
			// "old" groups: the first time, it renames the existing group: "old1_[name]". If the same 
			// group is again being created, the most recent one, [name] gets renamed into "old2_[name]"
			// And so on. This weird process is needed because HDF5 doesn't allow data deletion.  
			strcpy(new_groupname,groupname);
			inc=0;
			while(teststatus)
				{
				// This loop checks successively the existence of old*_name groups 
				strcpy(new_groupname,"old");
				snprintf( tmp , 10 , "%d" , inc );
				strcat(new_groupname,tmp);strcat(new_groupname,"_");strcat(new_groupname,groupname);
				teststatus = H5Lexists( zgroup, new_groupname, H5P_DEFAULT );
				inc++;
				}
			// We found an oldn_name group unused, which means, there are n-1 pre-existing groups: writing it in the logfile 
			snprintf( tmp , 200,"%d versions of %s already exists, creating a new one.\n" , inc , groupname );
			update_log( tmp , logfilename);
			
			//We rename the existing group with this new "old*_group" name
			status = H5Lmove( zgroup, groupname, zgroup, new_groupname, H5P_DEFAULT, H5P_DEFAULT);
			}

		update_log("Processing...\n",logfilename);

		// We begin the reading process by getting the path where we can find the files 
		get_file_path(&filepath,source_path,z,m,l);

		// We build the full path of the history file
		strcpy(history_filename,filepath);
		strcat(history_filename,"history.data");

		// We read the column names and the data of the history file 
		skiplines=5; //5 works for the current format of MESA files but should be changed if header lines are added or removed
		getnames( history_filename , skiplines , &columns_name_array_history , &ncol,logfilename );
		skiplines++;
		getdata( history_filename , skiplines , ncol , &nline , &data_array_history , logfilename );
	
		// We create the group for the current star. Should work since we rename any pre-existing group with the same name 
		stargroup = H5Gcreate1( zgroup , groupname , 0 ); // Note the zgroup at the location.

		//We create the sub-group containing the data from the history file 
		historygroup = H5Gcreate1(stargroup,"history",0);

		//We get the last model of the history filename to write it as an attribute in the HDF5 file 
		maxmodel = get_last_model(history_filename);
		
		//We write the history data into the HDF5 file
		writehdf5( historygroup , columns_name_array_history , data_array_history , ncol , nline , maxmodel, "maxmodel" );
		
		
		// When accessing the HDF5 file, the group structure acts like directories in the terminal. For example, to
		// read the log_dt in the HDF5 file, one should use the location "0.0001000/M0.50_L1.80/history/log_dt"
		
		
		// These arrays were not dynamically allocated at definition, but their content were "replaced" by arrays
		// dynamically allocated inside other functions. They need to be freed. 	
		free_string_array( columns_name_array_history , ncol );
		free2Ddoublearray( data_array_history , ncol );

		//We close the history group
		status=H5Gclose( historygroup );

		// For each model, read the profile and write it in HDF5 in its own subgroup 

		for( p=1 ; p<maxmodel ; p++ )
			{
			// Getting the filename 
			snprintf( modelnumber , 10 , "%d" , p ); //This is basically a conversion from int to char[]
			get_file_name ( &filename , p , 0 , filepath , z , m , l );
			// Reading the file 
			skiplines=5;
			getnames( filename , skiplines , &columns_name_array_profile , &ncol , logfilename );
			skiplines++;
			getdata( filename , skiplines , ncol , &nline , &data_array_profile , logfilename );

			// Getting the number of zones
			nzones = get_nzones( filename );

			// Writing it to HDF5, freeing the arrays and closing the group 
			modelgroup = H5Gcreate1 ( stargroup , modelnumber , 0 );
			writehdf5( modelgroup , columns_name_array_profile , data_array_profile , ncol , nline , nzones, "nzones" );
			free_string_array( columns_name_array_profile , ncol );
			free2Ddoublearray( data_array_profile , ncol );
			status=H5Gclose(modelgroup);
			}
	
		// After writing the history and profiles data, we can close the group for this star, close the metallicity group as well as the file 
		status = H5Gclose(stargroup);
		status = H5Gclose(zgroup);
		status = H5Fclose(file_id); 

		return 0;
		}		//End of the else concerning the validity of the star
}

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */

/*
	This function takes a file pointer as an argument. It takes the pointer at the place in the file where it currently is, 
	and search for the characters ch1 or ch2 in the rest of the file. If it doesnt find ch1 or ch2, when it reaches ch3, it goes
	back at the place where it was given the file and starts again. ch3 is EOF in the program
	for which this was designed, but can be anything. If the function finds ch1 or ch2, it returns it. Example:
		test.txt:
			"test1 test2 test3"
		In the code:
			FILE* file = fopen("test.txt","r");
			fscanf(file,"%s",&tmp1); // at this point, tmp1 contains "test1", and the file pointer is between test1 and test2.

			ch = waitfor( file,'4','y',EOF,1,logfilename ); 
		Calling this cause the function to read "test2 test3EOF" character after character, and to go back between test1 and
		test2 once it reaches EOF. It then sleeps 1 seconds and start again. The function will keep going forever if it doesn't
		find 4 or y. If I append " test4" to the file, the function stops and returns 4 in ch.

	The termination of the infinite loops in the programs was initially supposed to be handled only through the termination string "END"
	to the end of tobedeleted and todolist. However, some problems with that approach made me switch to another method. The programs now 
	stop when a file named "STOP" appears in hdf5_processing. It also stops if todolist doesn't exist anymore. This last feature avoids  
	old processes running in the background if Gridmaker is updated and directories are re-organized.

*/

char waitfor( char filename[], FILE* file , char ch1 , char ch2 , char ch3 , int waitperiod , char logfilename[] )
	{

	int count=0;
	char ch;
	// This is an infinite loop, only a break statement can "properly" get out of it 
	for(;;)
		{

		ch = fgetc(file);
		count++;

		// If the file we are reading actually isn't there anymore
		if ( file_doesnt_exist(filename) ) return '0';

		// If STOP exists, that means we have to terminate the program
		if ( file_exists("STOP") ) return '0';

		if(ch == ch1) break;
		if(ch == ch2) break;
		if(ch == ch3)
			{ // We were incrementing count for each character read, so we can go back at the exact place we started 
			fseek( file , -count + 1 , SEEK_CUR );
			count = 0;
			}
		}
	// We got out of the loop, so we found ch1 or ch2, returning it 
	return ch;
	}

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */


/* This function is designed to read the metallicity, mass and mixing length out of the todolist file in a very specific format:
	Z*.******* M*.** L*.**  Example: Z0.0001000 M0.50 L1.80
  It is called after the waitfor function, and the file pointer is just after the Z. It can then directly read the metallicity.
  The three values are stored in the three pointers provided as arguments. */

void read_parameters(FILE* file, char* z, char* m, char* l)
	{
	char ch;	
	fscanf(file,"%s",z);		// Read the metallicity
	do
		{ // Read everything until a M is reached (hopefully that should just be a white space) 
		ch = fgetc(file);
		}while( ch != 'M' );
	fscanf( file , "%s" , m );		// Read the mass

	do
		{// Read everything until a L is reached (hopefully that should just be a white space) 
		ch = fgetc(file);
		}while( ch != 'L' );
	fscanf( file , "%s" , l );		// Read the mixing length
	}  

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */
// This function simply return the int value it reads into the star_count file for which we supply the name 
int read_star_count( char star_count_filename[] )
	{
	int star_count;
	FILE* star_count_file = fopen( star_count_filename , "r" );
	fscanf( star_count_file , "%d" , &star_count );
	fclose(star_count_file);
	return star_count;
	}

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */
// This function opens the star_count file and prints the new value 
void update_star_count( char star_count_filename[] , int star_count )
	{
	FILE* star_count_file = fopen( star_count_filename , "w" );
	fprintf(star_count_file,"%d",star_count);
	fclose( star_count_file );
	}

/* --------------------------------------------------------------------------------------------------------------------------
   --------------------------------------------------------------------------------------------------------------------------
   -------------------------------------------------------------------------------------------------------------------------- */
/* This function prints out in the deletion file the name of the data files that can be deleted in the output directory. This
   name will be picked up by output_data_deletion, a background process that will perform the deletion. */
void delete_output_data(char source_path[] , char* z , char* m , char* l )
	{
	
	char star_path[100];
	strcpy(star_path,source_path);
	strcat(star_path,z);
	strcat(star_path,"/M");
	strcat(star_path,m);
	strcat(star_path,"_L");
	strcat(star_path,l);
	//strcat(star_path,"/m*"); //We only delete profiles, not fgong files.
		
	FILE* deletion_file = fopen( "tobedeleted" , "a" );
	fprintf(deletion_file,"%s\n",star_path);
	fclose(deletion_file);

	}


